#include "./inference.h"
#include "../ht.h"
#include "../modules.h"
#include "../parse.h"
#include "../serde.h"
#include "./builtins.h"
#include "./closures.h"
#include "./freshen_map.h"
#include "./subst_table.h"
#include "./type.h"
#include "./type_ser.h"
#include "infer_application.h"
#include "infer_lambda.h"
#include "infer_let.h"
#include "type_expressions.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Forward declarations for static helpers implemented in this file
// ============================================================================
static Subst *extend_subst(Subst *subst, int var_id, Type *type);
static Type *find_root_var(Subst *subst, Type *t);
static int unify_types(Type *t1, Type *t2, Subst *subst, Subst **out);
int bind_pattern(Ast *pattern, Type *value_type, TICtx *ctx);
static void finalize_env_generalization(TypeEnv *env, Subst *subst);
static void finalize_ast_types(Ast *ast, Subst *subst);
static TypeList *typelist_apply_subst(Subst *subst, TypeList *params);
static void constrain_argument_for_parameter(TICtx *ctx, Type *arg_type,
                                             Type *param_type, Ast *arg_ast);
static Predicate *predicate_filter_generic(Predicate *preds);
static bool is_empty_subst(Subst *subst);
static Subst *clone_subst(Subst *subst);
static bool is_recursive_self_reference(Ast *ast, TICtx *ctx);
static Type *infer_import_expr(Ast *ast, TICtx *ctx);
static TypeEnv *copy_typeenv_entry(TypeEnv *src);
static TypeEnv *copy_typeenv_chain(TypeEnv *src, int *size_out);
static void open_module_env_into_scope(TypeEnv *mod_env, TICtx *ctx);
static void import_module_binops_into_ctx(custom_binops_t *binops, TICtx *ctx);
static Type *create_module_type_from_env(TypeEnv *mod_env, int mod_size);

static FILE *err_stream;

static bool is_recursive_self_reference(Ast *ast, TICtx *ctx) {
  if (!ast || !ctx || !ctx->current_fn_ast) {
    return false;
  }

  const char *fn_name = ctx->current_fn_ast->data.AST_LAMBDA.fn_name.chars;
  if (!fn_name) {
    return false;
  }

  if (ast->tag == AST_IDENTIFIER) {
    return strcmp(ast->data.AST_IDENTIFIER.value, fn_name) == 0;
  }

  if (ast->tag == AST_APPLICATION &&
      ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
    return strcmp(ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
                  fn_name) == 0;
  }

  return false;
}

static TypeEnv *copy_typeenv_entry(TypeEnv *src) {
  if (!src) {
    return NULL;
  }

  TypeEnv *dst = t_alloc(sizeof(TypeEnv));
  *dst = *src;
  dst->next = NULL;
  return dst;
}

static TypeEnv *copy_typeenv_chain(TypeEnv *src, int *size_out) {
  TypeEnv *head = NULL;
  TypeEnv *tail = NULL;
  int size = 0;

  for (TypeEnv *cur = src; cur; cur = cur->next) {
    TypeEnv *node = copy_typeenv_entry(cur);
    if (!head) {
      head = node;
    } else {
      tail->next = node;
    }
    tail = node;
    size++;
  }

  if (size_out) {
    *size_out = size;
  }
  return head;
}

static void open_module_env_into_scope(TypeEnv *mod_env, TICtx *ctx) {
  if (!ctx) {
    return;
  }

  for (TypeEnv *entry = mod_env; entry; entry = entry->next) {
    TypeEnv *opened = copy_typeenv_entry(entry);
    opened->is_opened_var = true;
    opened->next = ctx->env;
    ctx->env = opened;
  }
}

static void import_module_binops_into_ctx(custom_binops_t *binops, TICtx *ctx) {
  if (!ctx) {
    return;
  }

  for (custom_binops_t *b = binops; b; b = b->next) {
    custom_binops_t *copy = t_alloc(sizeof(custom_binops_t));
    *copy = *b;
    copy->next = ctx->custom_binops;
    ctx->custom_binops = copy;
  }
}

static Type *create_module_type_from_env(TypeEnv *mod_env, int mod_size) {
  Type *mod = t_alloc(sizeof(Type));
  *mod = (Type){.kind = T_MODULE,
                .data = {.T_MODULE = {.env = mod_env, .size = mod_size}}};
  return mod;
}

static Type *infer_import_expr(Ast *ast, TICtx *ctx) {
  const char *key = ast->data.AST_IMPORT.fully_qualified_name;
  const char *identifier = ast->data.AST_IMPORT.identifier;

  if (!key) {
    TypeEnv *mod_ref = lookup_type_ref(ctx->env, identifier);
    if (!mod_ref || !mod_ref->type || mod_ref->type->kind != T_MODULE) {
      fprintf(stderr, "Error: module %s not found in scope\n", identifier);
      return NULL;
    }

    Type *module_type = mod_ref->type;
    if (ast->data.AST_IMPORT.import_all) {
      open_module_env_into_scope(module_type->data.T_MODULE.env, ctx);
    } else {
      ctx->env = env_extend(ctx->env, identifier, module_type);
    }
    return module_type;
  }

  YLCModule *mod = get_module(key);
  if (!mod) {
    fprintf(stderr, "Error: module %s not found\n", key);
    return NULL;
  }

  if (!mod->env) {
    mod = init_import(mod);
  }
  if (!mod || !mod->env) {
    fprintf(stderr, "Error: failed to initialize module %s\n", key);
    return NULL;
  }

  int mod_size = 0;
  TypeEnv *mod_env_copy = copy_typeenv_chain(mod->env, &mod_size);
  Type *module_type = create_module_type_from_env(mod_env_copy, mod_size);

  if (ast->data.AST_IMPORT.import_all) {
    open_module_env_into_scope(mod_env_copy, ctx);
    import_module_binops_into_ctx(mod->custom_binops, ctx);
  } else {
    ctx->env = env_extend(ctx->env, identifier, module_type);
  }

  return module_type;
}

void set_env_slice_scope(TypeEnv *slice_head, TypeEnv *boundary, int scope) {
  for (TypeEnv *e = slice_head; e != boundary; e = e->next) {
    if (e->md.type == BT_VAR) {
      e->md.data.VAR.scope = scope;
    }
  }
}

void set_env_slice_yield_boundary(TypeEnv *slice_head, TypeEnv *boundary,
                                  int yield_boundary_scope) {
  for (TypeEnv *e = slice_head; e != boundary; e = e->next) {
    if (e->md.type == BT_VAR || e->md.type == BT_FN_PARAM) {
      e->md.data.VAR.yield_boundary_scope = yield_boundary_scope;
    }
  }
}

Type *infer_match_expression(Ast *ast, TICtx *ctx) {
  Type *scrutinee_type = infer_expr(ast->data.AST_MATCH.expr, ctx);
  if (!scrutinee_type) {
    return NULL;
  }

  Type *result_type = next_tvar();
  for (size_t i = 0; i < ast->data.AST_MATCH.len; i++) {
    Ast *pattern = ast->data.AST_MATCH.branches + (i * 2);
    Ast *body = ast->data.AST_MATCH.branches + (i * 2) + 1;
    TypeEnv *saved_env = ctx->env;

    if (bind_pattern(pattern, scrutinee_type, ctx) != 0) {
      ctx->env = saved_env;
      return type_error(pattern, "Unsupported match pattern");
    }

    Type *body_type = infer_expr(body, ctx);
    ctx->env = saved_env;
    if (!body_type) {
      return NULL;
    }
    add_constraint(ctx, body_type, result_type);
  }

  return result_type;
}

Type *infer_inline_module(Ast *ast, TICtx *ctx) {
  TypeEnv *saved_env = ctx->env;
  int len;

  AstList *params = ast->data.AST_LAMBDA.params;

  for (AstList *p = params; p; p = p->next) {
    printf("module type arg\n");
    print_ast(p->ast);
  }

  AstList *module_body;
  if (ast->data.AST_LAMBDA.body->tag != AST_BODY) {
    module_body = alloca(sizeof(AstList));
    *module_body = (AstList){.ast = ast->data.AST_LAMBDA.body, .next = NULL};
    len = 1;
  } else {
    module_body = ast->data.AST_LAMBDA.body->data.AST_BODY.stmts;
    len = ast->data.AST_LAMBDA.body->data.AST_BODY.len;
  }

  for (AstList *tll = module_body; tll != NULL; tll = tll->next) {
    Ast *tl = tll->ast;
    Type *t = infer_expr(tl, ctx);
    if (!t) {
      ctx->env = saved_env;
      return NULL;
    }
  }

  Solution sol = {0};
  if (infer_solve(ctx, &sol) != 0) {
    ctx->env = saved_env;
    return NULL;
  }

  Subst *step_subst = sol.subst;
  if (ctx->predicates) {
    Predicate *resolved = predicate_apply_subst(step_subst, ctx->predicates);
    if (resolve_predicates(&step_subst, resolved) != 0) {
      ctx->env = saved_env;
      return NULL;
    }
    ctx->predicates = resolved;
  }

  ctx->subst = compose_subst(step_subst, ctx->subst);
  apply_subst_env(ctx->subst, ctx->env);
  ctx->predicates = predicate_apply_subst(ctx->subst, ctx->predicates);

  finalize_env_slice(ctx->env, saved_env, NULL);
  if (ctx->subst) {
    finalize_ast_types(ast->data.AST_LAMBDA.body, ctx->subst);
  }

  // Module inference performs its own local solve/finalize pass. The module
  // value we return is already closed over that state, so the surrounding
  // expression should not re-solve the module's internal constraints or trait
  // obligations.
  ctx->constraints = NULL;
  ctx->predicates = NULL;

  int mlen = 0;
  for (TypeEnv *e = ctx->env; e != saved_env; e = e->next) {
    mlen++;
  }

  TypeEnv **entries = mlen ? t_alloc(sizeof(TypeEnv *) * mlen) : NULL;
  int j = mlen - 1;
  for (TypeEnv *e = ctx->env; e != saved_env; e = e->next, j--) {
    entries[j] = e;
  }

  TypeEnv *mod_env = NULL;
  TypeEnv *tail = NULL;
  for (int i = 0; i < mlen; i++) {
    TypeEnv *src = entries[i];
    TypeEnv *dst = t_alloc(sizeof(TypeEnv));
    *dst = *src;
    dst->next = NULL;

    if (!mod_env) {
      mod_env = dst;
    } else {
      tail->next = dst;
    }
    tail = dst;
  }

  ctx->env = saved_env;
  Type *mod = t_alloc(sizeof(Type));
  *mod = (Type){.kind = T_MODULE,
                .data = {.T_MODULE = {.env = mod_env, .size = mlen}}};
  return mod;
}

// ============================================================================
// Forward declarations for static helpers in this file
// ============================================================================
static Subst *extend_subst(Subst *subst, int var_id, Type *type);
static Type *find_root_var(Subst *subst, Type *t);
static int unify_types(Type *t1, Type *t2, Subst *subst, Subst **out);

// ============================================================================
// Top-level inference pipeline
// ============================================================================

// infer_solve: solve accumulated constraints, return substitution.
// Empty constraint set is trivially satisfiable.
int infer_solve(TICtx *ctx, Solution *sol) {
  if (!ctx->constraints) {
    sol->subst = NULL;
    return 0;
  }
  sol->subst = solve_constraints(ctx->constraints);
  return sol->subst ? 0 : 1;
}

void infer_final(Ast *ast, const Solution *solved, TICtx *ctx) {
  if (!ctx) {
    return;
  }
  Subst *subst = solved ? solved->subst : NULL;
  finalize_env_generalization(ctx->env, subst);

  if (subst) {
    finalize_ast_types(ast, subst);
  }
}

Type *apply_solution(Type *raw, Solution *solved) {
  if (!solved || !solved->subst) {
    return raw;
  }
  return apply_subst_to_type(solved->subst, raw);
}

// infer: the public entry point.
// 1. Infer expression types and generate constraints + predicates
// 2. Solve equality constraints once
// 3. Resolve trait predicates using the substitution (may extend subst)
// 4. Apply substitution to the result type
// 5. Finalize AST annotations
Type *infer(Ast *ast, TICtx *ctx) {
  err_stream = stderr;
  Type *raw = infer_expr(ast, ctx);
  if (!raw) {
    return type_error(ast, "failed to infer type");
  }
  // aux = ast->data.AST_BODY.stmts->ast->data.AST_LAMBDA.body->data.AST_BODY
  //           .stmts->next->ast;

  // printf("[Constraints]\n");
  // print_constraints(ctx->constraints);
  //
  // printf("[Predicates]\n");
  // print_predicates(ctx->predicates);
  //
  Solution sol = {0};
  if (infer_solve(ctx, &sol)) {
    return type_error(ast, "failed to solve constraints");
  }

  // printf("[Solution]\n");
  // print_subst(ctx->subst);

  // Resolve accumulated trait predicates using the solved substitution
  Subst *step_subst = sol.subst;
  if (ctx->predicates) {
    Predicate *resolved = predicate_apply_subst(step_subst, ctx->predicates);
    if (resolve_predicates(&step_subst, resolved) != 0) {
      return type_error(ast, "failed to resolve predicates");
    }
    ctx->predicates = resolved;
  }

  ctx->subst = compose_subst(step_subst, ctx->subst);
  apply_subst_env(ctx->subst, ctx->env);

  Solution final_sol = {.subst = ctx->subst};
  Type *final = apply_solution(raw, &final_sol);

  infer_final(ast, &final_sol, ctx);

  return final;
}

// ============================================================================
// Literal inference helpers
// ============================================================================

Type *infer_list_literal(Ast *ast, TICtx *ctx) {
  Type *el_type = next_tvar();
  int len = ast->data.AST_LIST.len;

  for (int i = 0; i < len; i++) {
    Ast *el = ast->data.AST_LIST.items + i;
    Type *item_type = infer_expr(el, ctx);
    if (!item_type) {
      return NULL;
    }
    add_constraint(ctx, item_type, el_type);
  }

  Type *type = t_alloc(sizeof(Type));
  Type **contained = t_alloc(sizeof(Type *));
  contained[0] = el_type;

  *type = (Type){
      T_CONS,
      {.T_CONS = {ast->tag == AST_LIST ? TYPE_NAME_LIST : TYPE_NAME_ARRAY,
                  contained, 1}}};
  return type;
}

// ============================================================================
// Environment helpers
// ============================================================================

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type) {
  return env_extend_with_preds(env, name, type, NULL);
}

TypeEnv *env_extend_with_preds(TypeEnv *env, const char *name, Type *type,
                               Predicate *preds) {
  TypeEnv *new_env = t_alloc(sizeof(TypeEnv));
  new_env->name = name;
  new_env->type = type;
  new_env->md = (binding_md){
      .type = BT_VAR, .data = {.VAR = {.scope = 0, .yield_boundary_scope = 0}}};
  new_env->ref_count = 0;
  new_env->scheme_vars = NULL;
  new_env->predicates = preds;
  new_env->next = env;
  new_env->generalize_boundary = NULL;
  new_env->can_generalize = false;
  new_env->needs_generalization = false;
  new_env->is_opened_var = false;
  return new_env;
}

TypeEnv *lookup_type_ref(TypeEnv *env, const char *name) {
  for (TypeEnv *e = env; e != NULL; e = e->next) {
    if (strcmp(e->name, name) == 0) {
      return e;
    }
  }
  return NULL;
}

Type *env_lookup(TypeEnv *env, const char *name) {
  TypeEnv *e = lookup_type_ref(env, name);
  if (!e)
    return NULL;
  return e->type;
}

static void finalize_env_generalization(TypeEnv *env, Subst *subst) {
  if (!env) {
    return;
  }

  int len = 0;
  for (TypeEnv *e = env; e; e = e->next) {
    len++;
  }

  TypeEnv **entries = len ? t_alloc(sizeof(TypeEnv *) * len) : NULL;
  int i = len - 1;
  for (TypeEnv *e = env; e; e = e->next, i--) {
    entries[i] = e;
  }

  for (int j = 0; j < len; j++) {
    TypeEnv *e = entries[j];
    e->type = apply_subst_to_type(subst, e->type);
    if (e->needs_generalization) {
      e->scheme_vars = NULL;
      if (e->can_generalize) {
        generalize_env(e, e->generalize_boundary);
      }
      e->needs_generalization = false;
    }
  }
}

// ============================================================================
// Free variable helpers
// ============================================================================

static bool type_list_contains_var_id(TypeList *l, int var_id) {
  for (TypeList *c = l; c; c = c->next) {
    if (c->type && c->type->kind == T_VAR && c->type->data.T_VAR.id == var_id) {
      return true;
    }
  }
  return false;
}

static TypeList *type_list_append_var(TypeList *acc, Type *tvar) {
  TypeList *node = t_alloc(sizeof(TypeList));
  node->type = tvar;
  node->next = NULL;
  if (!acc) {
    return node;
  }
  TypeList *tail = acc;
  while (tail->next) {
    tail = tail->next;
  }
  tail->next = node;
  return acc;
}

TypeList *free_vars_type(TypeList *acc, Type *t) {
  if (!t)
    return acc;
  switch (t->kind) {
  case T_VAR:
    if (!type_list_contains_var_id(acc, t->data.T_VAR.id)) {
      acc = type_list_append_var(acc, t);
    }
    return acc;
  case T_RECURSIVE_REF:
    return acc;
  case T_FN:
    acc = free_vars_type(acc, t->data.T_FN.from);
    acc = free_vars_type(acc, t->data.T_FN.to);
    return acc;
  case T_CONS:
  case T_SUM:
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      acc = free_vars_type(acc, t->data.T_CONS.args[i]);
    }
    return acc;
  default:
    return acc;
  }
}

TypeList *free_vars_env(TypeList *acc, TypeEnv *env) {
  for (TypeEnv *e = env; e; e = e->next) {
    acc = free_vars_type(acc, e->type);
  }
  return acc;
}

// Collect the free type variables appearing in a list of (deferred)
// predicates. A type variable that is constrained by an unresolved
// trait/comparable obligation must not be generalized: generalizing it
// would freeze it as a polymorphic scheme variable, severing the link
// between the binding's stored type and the freshened copies later use
// sites resolve. This keeps the binding monomorphic in exactly those
// constrained variables so a subsequent checkpoint (with more context)
// can push a concrete witness back into the original variable.
TypeList *free_vars_predicate(TypeList *acc, Predicate *preds) {
  for (Predicate *p = preds; p; p = p->next) {
    if (p->kind == PRED_TRAIT) {
      acc = free_vars_type(acc, p->data.TRAIT.type);
      for (TypeList *tl = p->data.TRAIT.params; tl; tl = tl->next) {
        acc = free_vars_type(acc, tl->type);
      }
    } else if (p->kind == PRED_COMPARABLE) {
      acc = free_vars_type(acc, p->data.COMPARABLE.witness);
      for (int i = 0; p->data.COMPARABLE.args && p->data.COMPARABLE.args[i];
           i++) {
        acc = free_vars_type(acc, p->data.COMPARABLE.args[i]);
      }
    }
  }
  return acc;
}

static TypeList *set_diff(TypeList *a, TypeList *b) {
  TypeList *result = NULL;
  for (TypeList *la = a; la; la = la->next) {
    if (la->type && la->type->kind == T_VAR) {
      if (!type_list_contains_var_id(b, la->type->data.T_VAR.id)) {
        result = type_list_append_var(result, la->type);
      }
    }
  }
  return result;
}

// ============================================================================
// Generalize / Instantiate
// Operate on TypeEnv entries, not on Type nodes directly.
// ============================================================================

void generalize_env(TypeEnv *entry, TypeEnv *env) {
  TypeList *fv_type = free_vars_type(NULL, entry->type);
  TypeList *fv_env = free_vars_env(NULL, env);
  TypeList *scheme_vars = set_diff(fv_type, fv_env);
  // A non-function value binding (e.g. `let x2 = x / 10`) whose type is a
  // bare generic variable still constrained by a deferred trait predicate
  // must not be generalized. Its value is codegen'd once at the binding
  // site and needs a concrete type; generalizing the constrained variable
  // would freeze it as a scheme var, so the freshened copies later use
  // sites resolve never reach the binding's own type. Function bindings
  // are codegen'd lazily per instantiation, so their constrained params
  // generalize as usual.
  if (entry->predicates && !(entry->type && entry->type->kind == T_FN)) {
    TypeList *fv_preds = free_vars_predicate(NULL, entry->predicates);
    scheme_vars = set_diff(scheme_vars, fv_preds);
  }
  entry->scheme_vars = scheme_vars;
}

// instantiate: replace scheme_vars with fresh type variables, and copy
// predicates into the inference context with freshened types.
Type *instantiate_env(TypeEnv *entry, TICtx *ctx) {

  // No scheme vars: monomorphic. Still copy predicates if present.
  if (!entry->scheme_vars) {
    for (Predicate *p = entry->predicates; p; p = p->next) {
      if (p->kind == PRED_TRAIT) {
        ctx->predicates =
            predicate_append_applied(ctx->predicates, p->trait,
                                     p->data.TRAIT.type, p->data.TRAIT.params);
      } else if (p->kind == PRED_COMPARABLE) {
        // Monomorphic but has comparability obligations — copy as-is
        int n = 0;
        while (p->data.COMPARABLE.args[n])
          n++;
        Type **args = t_alloc(sizeof(Type *) * (n + 1));
        for (int i = 0; i < n; i++)
          args[i] = p->data.COMPARABLE.args[i];
        args[n] = NULL;
        ctx->predicates = predicate_append_comparable(
            ctx->predicates, p->trait, p->data.COMPARABLE.witness, args);
      }
    }
    return entry->type;
  }

  // Build freshening substitution from scheme vars
  FreshenMap base = {0};
  for (TypeList *v = entry->scheme_vars; v; v = v->next) {
    if (v->type && v->type->kind == T_VAR) {
      Type *fresh = next_tvar();
      fresh->implements = v->type->implements;
      freshen_map_extend(&base, v->type->data.T_VAR.id, fresh);
    }
  }

  // Copy predicates with freshened types / result / args
  for (Predicate *p = entry->predicates; p; p = p->next) {
    if (p->kind == PRED_TRAIT) {
      Type *fresh_type =
          base.len ? freshen_map_apply_to_type(&base, p->data.TRAIT.type)
                   : p->data.TRAIT.type;
      TypeList *fresh_params =
          base.len ? freshen_map_apply_to_typelist(&base, p->data.TRAIT.params)
                   : p->data.TRAIT.params;
      ctx->predicates = predicate_append_applied(ctx->predicates, p->trait,
                                                 fresh_type, fresh_params);
    } else if (p->kind == PRED_COMPARABLE) {
      Type *fresh_witness =
          base.len
              ? freshen_map_apply_to_type(&base, p->data.COMPARABLE.witness)
              : p->data.COMPARABLE.witness;
      int n = 0;
      while (p->data.COMPARABLE.args[n])
        n++;
      Type **args = t_alloc(sizeof(Type *) * (n + 1));
      for (int i = 0; i < n; i++) {
        args[i] =
            base.len
                ? freshen_map_apply_to_type(&base, p->data.COMPARABLE.args[i])
                : p->data.COMPARABLE.args[i];
      }
      args[n] = NULL;
      ctx->predicates = predicate_append_comparable(ctx->predicates, p->trait,
                                                    fresh_witness, args);
    }
  }

  if (!base.len) {
    return entry->type;
  }
  return freshen_map_apply_to_type(&base, entry->type);
}

// Short-circuit instantiation for entries that are truly monomorphic and have
// no predicates to copy.  Keeps predicate-copying centralized in
// instantiate_env when it is needed.
static Type *instantiate_ref(TypeEnv *ref, TICtx *ctx) {
  if (!ref->scheme_vars && !ref->predicates) {
    return ref->type;
  }
  return instantiate_env(ref, ctx);
}

Type *instantiate_type_in_env(Type *sch, TypeEnv *env) { return sch; }

// ============================================================================
// Expression inference - HM core dispatcher
// ============================================================================

static Type *infer_identifier(Ast *ast, TICtx *ctx) {
  const char *name = ast->data.AST_IDENTIFIER.value;
  TypeEnv *ref = lookup_type_ref(ctx->env, name);

  if (ref) {
    if (ref->md.type == BT_TYPE_DECL && ref->type &&
        ref->type->kind == T_CONS && !is_sum_type(ref->type)) {
      return create_type_multi_param_fn(ref->type->data.T_CONS.num_args,
                                        ref->type->data.T_CONS.args, ref->type);
    }
    Type *inst = instantiate_ref(ref, ctx);
    ast->type = inst;

    if (ref->md.type == BT_VAR || ref->md.type == BT_FN_PARAM) {
      handle_closed_over_value(ref->md, ast, ctx);
    }

    return inst;
  }

  // New: builtins stored as TypeEnv entries with predicates
  TypeEnv *builtin = lookup_builtin_env(name);
  if (builtin) {
    return instantiate_env(builtin, ctx);
  }

  return next_tvar();
}

int bind_pattern(Ast *pattern, Type *value_type, TICtx *ctx) {
  if (!pattern || !value_type) {
    return 1;
  }

  if (pattern->tag == AST_LET) {
    pattern = pattern->data.AST_LET.binding;
  }

  switch (pattern->tag) {
  case AST_PLACEHOLDER_ID:
    return 0;
  case AST_INT:
    add_constraint(ctx, value_type, &t_int);
    return 0;
  case AST_DOUBLE:
    add_constraint(ctx, value_type, &t_num);
    return 0;
  case AST_STRING:
    add_constraint(ctx, value_type, &t_string);
    return 0;
  case AST_CHAR:
    add_constraint(ctx, value_type, &t_char);
    return 0;
  case AST_BOOL:
    add_constraint(ctx, value_type, &t_bool);
    return 0;
  case AST_VOID:
    add_constraint(ctx, value_type, &t_void);
    return 0;
  case AST_IDENTIFIER: {
    const char *name = pattern->data.AST_IDENTIFIER.value;
    if (strcmp(name, "_") == 0) {
      pattern->type = value_type;
      return 0;
    }
    TypeEnv *ref = lookup_type_ref(ctx->env, name);
    if (ref && ref->md.type == BT_TYPE_CONSTRUCTOR && ref->type &&
        ref->type->kind != T_FN) {
      add_constraint(ctx, value_type, instantiate_env(ref, ctx));
      return 0;
    }
    TypeEnv *builtin = lookup_builtin_env(name);
    if (builtin && builtin->type && builtin->type->kind != T_FN) {
      add_constraint(ctx, value_type, instantiate_env(builtin, ctx));
      return 0;
    }
    pattern->type = value_type;
    ctx->env = env_extend(ctx->env, name, value_type);
    return 0;
  }
  case AST_MATCH_GUARD_CLAUSE: {
    if (bind_pattern(pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr, value_type,
                     ctx) != 0) {
      return 1;
    }
    Type *guard_type =
        infer_expr(pattern->data.AST_MATCH_GUARD_CLAUSE.guard_expr, ctx);
    if (!guard_type) {
      return 1;
    }
    add_constraint(ctx, guard_type, &t_bool);
    return 0;
  }
  case AST_TUPLE: {
    int len = pattern->data.AST_LIST.len;
    Type **items = t_alloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      items[i] = next_tvar();
    }
    add_constraint(ctx, value_type, create_tuple_type(len, items));
    for (int i = 0; i < len; i++) {
      if (bind_pattern(pattern->data.AST_LIST.items + i, items[i], ctx) != 0) {
        return 1;
      }
    }
    return 0;
  }
  case AST_LIST: {
    if (pattern->data.AST_LIST.len == 0) {
      Type *item_type = next_tvar();
      add_constraint(ctx, value_type, create_list_type_of_type(item_type));
      return 0;
    }
    break;
  }
  case AST_EMPTY_CONTAINER: {
    Type *item_type = next_tvar();
    add_constraint(ctx, value_type, create_list_type_of_type(item_type));
    return 0;
  }
  case AST_APPLICATION: {
    if (pattern->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
        strcmp(
            pattern->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
            TYPE_NAME_OP_LIST_PREPEND) == 0 &&
        pattern->data.AST_APPLICATION.len == 2) {
      Type *item_type = next_tvar();
      Type *rest_type = create_list_type_of_type(item_type);
      add_constraint(ctx, value_type, rest_type);

      if (bind_pattern(pattern->data.AST_APPLICATION.args, item_type, ctx) !=
          0) {
        return 1;
      }
      return bind_pattern(pattern->data.AST_APPLICATION.args + 1, rest_type,
                          ctx);
    }

    if (pattern->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
      const char *ctor_name =
          pattern->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;
      TypeEnv *ctor = lookup_type_ref(ctx->env, ctor_name);

      if (ctor && ctor->md.type != BT_TYPE_CONSTRUCTOR) {
        ctor = NULL;
      }

      if (!ctor) {
        ctor = lookup_builtin_env(ctor_name);
      }

      if (ctor) {
        Type *current = instantiate_env(ctor, ctx);
        for (size_t i = 0; i < pattern->data.AST_APPLICATION.len; i++) {
          if (!current || current->kind != T_FN) {

            return 1;
          }
          if (bind_pattern(pattern->data.AST_APPLICATION.args + i,
                           current->data.T_FN.from, ctx) != 0) {
            return 1;
          }
          current = current->data.T_FN.to;
        }
        add_constraint(ctx, value_type, current);
        return 0;
      }
    }
    break;
  }
  default:
    break;
  }

  return 1;
}

Type *infer_expr(Ast *ast, TICtx *ctx) {
  Type *type = NULL;

  switch (ast->tag) {
  case AST_BODY: {
    AST_LIST_ITER(ast->data.AST_BODY.stmts, ({
                    Ast *stmt = l->ast;
                    Type *res = infer_expr(stmt, ctx);
                    if (res == NULL) {
                      return type_error(stmt, "Error: typecheck failed at ");
                    }
                    type = res;
                  }));
    break;
  }
  case AST_INT:
    type = &t_int;
    break;
  case AST_DOUBLE:
    type = &t_num;
    break;
  case AST_STRING:
    type = &t_string;
    break;
  case AST_CHAR:
    type = &t_char;
    break;
  case AST_BOOL:
    type = &t_bool;
    break;
  case AST_VOID:
    type = &t_void;
    break;

  case AST_ARRAY:
  case AST_LIST:
    type = infer_list_literal(ast, ctx);
    break;

  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;
    Type **args = t_alloc(sizeof(Type *) * len);
    const char **names = NULL;
    if (ast->data.AST_LIST.items[0].tag == AST_LET) {
      names = t_alloc(sizeof(char *) * len);
    }
    for (int i = 0; i < len; i++) {
      if (names) {
        names[i] = ast->data.AST_LIST.items[i]
                       .data.AST_LET.binding->data.AST_IDENTIFIER.value;
        args[i] =
            infer_expr(ast->data.AST_LIST.items[i].data.AST_LET.expr, ctx);
      } else {
        args[i] = infer_expr(&ast->data.AST_LIST.items[i], ctx);
      }
      if (!args[i])
        return NULL;
    }
    type = create_tuple_type(len, args);
    if (names)
      type->data.T_CONS.names = names;
    break;
  }

  case AST_LET:
    type = infer_let_expr(ast, ctx);
    break;
  case AST_IDENTIFIER:
    type = infer_identifier(ast, ctx);
    break;

  case AST_APPLICATION:
    type = infer_application(ast, ctx);
    break;
  case AST_LAMBDA:
    type = infer_lambda(ast, ctx);
    break;
  case AST_MATCH:
    type = infer_match_expression(ast, ctx);
    break;

  case AST_TYPE_DECL:
    type = infer_type_declaration(ast, ctx);
    break;
  case AST_IMPORT:
    type = infer_import_expr(ast, ctx);
    break;
  case AST_FMT_STRING:
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      Ast *item = ast->data.AST_LIST.items + i;
      if (infer_expr(item, ctx) == NULL) {
        return NULL;
      }
    }
    type = &t_string;
    break;
  case AST_EXTERN_FN: {
    Ast *sig = ast->data.AST_EXTERN_FN.signature_types;
    type = compute_type_expression(sig, ctx);
    break;
  }

  case AST_MODULE: {
    type = infer_inline_module(ast, ctx);
    break;
  }
  case AST_RECORD_ACCESS: {

    Type *rec_type = infer_expr(ast->data.AST_RECORD_ACCESS.record, ctx);
    if (!rec_type) {
      return NULL;
    }

    const char *member_name =
        ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;

    // if (rec_type->kind == T_FN && !is_generic(rec_type)) {
    //   // TODO: this is dodgy - fix
    //   rec_type = fn_return_type(rec_type);
    //   ast->data.AST_RECORD_ACCESS.record->type = rec_type;
    // }

    if (rec_type->kind == T_MODULE) {
      int i = 0;
      for (TypeEnv *te = rec_type->data.T_MODULE.env; te; te = te->next, i++) {
        if (CHARS_EQ(te->name, member_name)) {
          type = instantiate_env(te, ctx);
          ast->data.AST_RECORD_ACCESS.index = i;
          break;
        }
      }
      if (!type) {
        fprintf(stderr, "Error: module member %s not found\n", member_name);
        return NULL;
      }
      break;
    }

    if (rec_type->kind != T_CONS) {
      fprintf(stderr, "Error: record type not cons\n");
      return NULL;
    }

    if (rec_type->kind == T_CONS && rec_type->data.T_CONS.names == NULL) {
      fprintf(stderr, "Error: record type does not have names\n");
      return NULL;
    }

    for (int i = 0; i < rec_type->data.T_CONS.num_args; i++) {
      if (CHARS_EQ(rec_type->data.T_CONS.names[i], member_name)) {
        type = rec_type->data.T_CONS.args[i];
        // printf("found type @ %d??\n", i);
        // print_type(type);
        ast->data.AST_RECORD_ACCESS.index = i;
        break;
      }
    }

    break;
  }

  case AST_LOOP: {
    Ast let = *ast;
    let.tag = AST_LET;
    type = infer_expr(&let, ctx);
    ast->type = let.type;
    break;
  }
  case AST_RANGE_EXPRESSION: {
    Type *from = infer_expr(ast->data.AST_RANGE_EXPRESSION.from, ctx);
    Type *to = infer_expr(ast->data.AST_RANGE_EXPRESSION.to, ctx);

    unify(from, &t_int, ctx);
    unify(to, &t_int, ctx);

    type = &t_int;
    break;
  }
  case AST_YIELD: {
    if (ctx->current_fn_ast) {
      ctx->current_fn_ast->data.AST_LAMBDA.is_coroutine = true;
      ctx->current_fn_ast->data.AST_LAMBDA.num_yields++;
    }

    Type *yield = infer_expr(ast->data.AST_YIELD.expr, ctx);
    if (yield && ctx->yielded_type == NULL) {
      ctx->yielded_type = next_tvar();
    }
    if (yield && ctx->yielded_type &&
        is_recursive_self_reference(ast->data.AST_YIELD.expr, ctx)) {
      add_constraint(ctx, yield,
                     create_coroutine_instance_type(ctx->yielded_type));
    } else if (yield && ctx->yielded_type && is_coroutine_type(yield)) {
      add_constraint(ctx, yield->data.T_CONS.args[0], ctx->yielded_type);
    } else if (yield && ctx->yielded_type) {
      add_constraint(ctx, yield, ctx->yielded_type);
    }
    type = ctx->yielded_type ? ctx->yielded_type : yield;
    break;
  }

  case AST_TRAIT_IMPL: {
    Ast *impl = ast->data.AST_TRAIT_IMPL.impl;
    Type *impl_type = infer_expr(impl, ctx);
    if (!impl_type) {
      return NULL;
    }

    const char *trait_name = ast->data.AST_TRAIT_IMPL.trait_name.chars;
    const char *type_name = ast->data.AST_TRAIT_IMPL.type.chars;

    Type *target = env_lookup(ctx->env, type_name);

    if (!target) {
      target = lookup_builtin_type(type_name);
    }

    if (!target) {
      type_error(ast, "cannot register trait %s: type %s not in scope",
                 trait_name, type_name);
      return NULL;
    }

    TypeClass *tc = t_alloc(sizeof(TypeClass));
    *tc = (TypeClass){.name = trait_name, .module = impl_type};
    typeclasses_extend(target, tc);

    if (strcmp(trait_name, TYPE_NAME_TYPECLASS_FROM) == 0) {
      if (impl_type->kind == T_MODULE) {
        for (TypeEnv *te = impl_type->data.T_MODULE.env; te; te = te->next) {
          const char *mname = te->name;
          if (strncmp(mname, "from_", 5) != 0) {
            continue;
          }
          Type *mtype = te->type;
          if (!mtype || mtype->kind != T_FN) {
            continue;
          }
          Type *src = mtype->data.T_FN.from;
          if (!src || is_generic(src)) {
            continue;
          }
          TypeList *src_params = t_alloc(sizeof(TypeList));
          src_params->type = src;
          src_params->next = NULL;
          TypeClass *src_tc = t_alloc(sizeof(TypeClass));
          *src_tc = (TypeClass){
              .name = trait_name, .module = impl_type, .params = src_params};
          typeclasses_extend(target, src_tc);
        }
      }

      Type *a = tvar("a");
      Type *ctor_fn = type_fn(a, target);
      TypeList *scheme_vars = t_alloc(sizeof(TypeList));
      scheme_vars->type = a;
      scheme_vars->next = NULL;
      TypeList *from_params = t_alloc(sizeof(TypeList));
      from_params->type = a;
      from_params->next = NULL;
      Predicate *preds =
          predicate_append_applied(NULL, GenericFrom, target, from_params);
      ctx->env = env_extend_with_preds(ctx->env, type_name, ctor_fn, preds);
      ctx->env->scheme_vars = scheme_vars;
      ctx->env->can_generalize = true;
      ctx->env->needs_generalization = true;
    }

    ast->type = impl_type;
    type = impl_type;
    break;
  }

  default:
    break;
  }

  ast->type = type;
  if (type == NULL) {
    fprintf(stderr, "Error: could not infer type at");
    print_location(ast);
    // print_ast_err(ast);
  }
  return type;
}

// ============================================================================
// Predicate helpers
// ============================================================================

Predicate *predicate_append(Predicate *list, TypeClass *trait, Type *type) {
  return predicate_append_applied(list, trait, type, NULL);
}

Predicate *predicate_append_applied(Predicate *list, TypeClass *trait,
                                    Type *type, TypeList *params) {
  Predicate *p = t_alloc(sizeof(Predicate));
  *p = (Predicate){.kind = PRED_TRAIT,
                   .trait = trait,
                   .data = {.TRAIT = {.type = type, .params = params}},
                   .next = list};
  return p;
}

Predicate *predicate_append_comparable(Predicate *list, TypeClass *trait,
                                       Type *witness, Type **args) {
  Predicate *p = t_alloc(sizeof(Predicate));
  *p = (Predicate){.kind = PRED_COMPARABLE,
                   .trait = trait,
                   .data = {.COMPARABLE = {.witness = witness, .args = args}},
                   .next = list};
  return p;
}

Predicate *predicate_apply_subst(Subst *subst, Predicate *preds) {
  Predicate *result = NULL;
  for (Predicate *p = preds; p; p = p->next) {
    if (p->kind == PRED_TRAIT) {
      Type *resolved = apply_subst_to_type(subst, p->data.TRAIT.type);
      TypeList *resolved_params =
          typelist_apply_subst(subst, p->data.TRAIT.params);
      result =
          predicate_append_applied(result, p->trait, resolved, resolved_params);
    } else if (p->kind == PRED_COMPARABLE) {
      Type *resolved_witness =
          apply_subst_to_type(subst, p->data.COMPARABLE.witness);
      int n = 0;
      while (p->data.COMPARABLE.args[n])
        n++;
      Type **resolved_args = t_alloc(sizeof(Type *) * (n + 1));
      for (int i = 0; i < n; i++) {
        resolved_args[i] =
            apply_subst_to_type(subst, p->data.COMPARABLE.args[i]);
      }
      resolved_args[n] = NULL;
      result = predicate_append_comparable(result, p->trait, resolved_witness,
                                           resolved_args);
    }
  }
  return result;
}

Predicate *predicate_duplicate(Predicate *preds) {
  Predicate *result = NULL;
  for (Predicate *p = preds; p; p = p->next) {
    if (p->kind == PRED_TRAIT) {
      result = predicate_append_applied(result, p->trait, p->data.TRAIT.type,
                                        p->data.TRAIT.params);
    } else if (p->kind == PRED_COMPARABLE) {
      int n = 0;
      while (p->data.COMPARABLE.args[n])
        n++;
      Type **args = t_alloc(sizeof(Type *) * (n + 1));
      for (int i = 0; i < n; i++)
        args[i] = p->data.COMPARABLE.args[i];
      args[n] = NULL;
      result = predicate_append_comparable(result, p->trait,
                                           p->data.COMPARABLE.witness, args);
    }
  }
  return result;
}

void print_predicate(Predicate *p) {
  switch (p->kind) {
  case PRED_TRAIT: {
    printf("Trait( ");
    if (p->data.TRAIT.type) {
      print_type_to_stream(p->data.TRAIT.type, stdout);
    } else {
      printf("(null)");
    }
    printf(" : %s", p->trait ? p->trait->name : "(null)");
    if (p->data.TRAIT.params) {
      printf("<");
      for (TypeList *tl = p->data.TRAIT.params; tl; tl = tl->next) {
        print_type_to_stream(tl->type, stdout);
        if (tl->next) {
          printf(", ");
        }
      }
      printf(">");
    }
    printf(" )");
    printf("\n");
    break;
  }

  case PRED_COMPARABLE: {

    printf("Comparable( ");
    if (p->data.COMPARABLE.witness) {
      print_type_to_stream(p->data.COMPARABLE.witness, stdout);
    } else {
      printf("(null)");
    }
    printf(" = resolve(%s,", p->trait ? p->trait->name : "(null)");
    for (int i = 0; p->data.COMPARABLE.args && p->data.COMPARABLE.args[i];
         i++) {

      if (i > 0) {
        printf(", ");
      }

      print_type_to_stream(p->data.COMPARABLE.args[i], stdout);
    }
    printf(") )");
    printf("\n");
    break;
  }
  default: {
  }
  }
}

void print_predicates(Predicate *predicates) {
  for (Predicate *p = predicates; p; p = p->next) {
    print_predicate(p);
  }
}

int resolve_predicates(Subst **subst_ptr, Predicate *preds) {
  Subst *subst = subst_ptr ? *subst_ptr : NULL;
  bool changed = true;
  while (changed) {
    changed = false;
    for (Predicate *p = preds; p; p = p->next) {
      if (p->kind == PRED_TRAIT) {
        Type *t = apply_subst_to_type(subst, p->data.TRAIT.type);
        TypeList *params = typelist_apply_subst(subst, p->data.TRAIT.params);

        // Still generic after substitution — skip (defer)
        if (is_generic(t)) {
          continue;
        }
        bool generic_params = false;
        for (TypeList *tl = params; tl; tl = tl->next) {
          if (is_generic(tl->type)) {
            generic_params = true;
            break;
          }
        }
        if (generic_params) {
          continue;
        }

        // Check the trait
        if ((strcmp(p->trait->name, "Eq") != 0) &&
            !get_typeclass_instance(t, p->trait->name, params)) {
          if (err_stream) {
            fprintf(err_stream, "Type Error: ");
            print_type_to_stream(t, err_stream);
            fprintf(err_stream, " does not implement %s\n", p->trait->name);
            fflush(err_stream);
          }
          return 1;
        }
      } else if (p->kind == PRED_COMPARABLE) {
        // Resolve a common witness type for the operands. If all operands are
        // concrete, use the highest-ranked implementation for this trait. If
        // only some operands are concrete, use that concrete operand as the
        // witness and push it back into the generic operands.
        Type *witness = NULL;
        double max_rank = -1.;
        bool all_concrete = true;
        int i = 0;
        for (; p->data.COMPARABLE.args[i]; i++) {
          Type *arg = apply_subst_to_type(subst, p->data.COMPARABLE.args[i]);
          if (is_generic(arg)) {
            all_concrete = false;
            continue;
          }
          if (!witness) {
            witness = arg;
          }
          double rank = get_typeclass_rank(arg, p->trait->name);
          if (rank > max_rank) {
            max_rank = rank;
            witness = arg;
          }
        }

        Type *result = find_root_var(subst, p->data.COMPARABLE.witness);
        Type *resolved_result = apply_subst_to_type(subst, result);

        if (!witness && !is_generic(resolved_result)) {
          witness = resolved_result;
        }

        // When the result is already concrete (forced by an outer context, e.g.
        // a function argument type or a match-branch unification), prefer it as
        // the witness over an operand-derived one, unless a concrete operand
        // has strictly higher rank.  This prevents a partially-resolved
        // expression like `w - sample_rate * dt` (result Double, operands Int
        // and a still-generic nested result) from collapsing to the lower-rank
        // concrete operand Int before the generic operand resolves.
        if (!is_generic(resolved_result)) {
          double result_rank =
              get_typeclass_rank(resolved_result, p->trait->name);
          if (!witness || result_rank >= max_rank) {
            witness = resolved_result;
            max_rank = result_rank;
          }
        }

        if (!witness) {
          continue;
        }

        // Arithmetic needs all operands concrete before choosing a witness
        // from the operands themselves, otherwise nested expressions like `1
        // + (2.0 * 8)` can collapse to the first concrete operand too early.
        // A concrete result witness, however, is safe to push back into
        // generic operands.
        if (p->trait == GenericArithmetic && !all_concrete &&
            is_generic(resolved_result)) {
          continue;
        }

        Subst *next_subst = NULL;
        if (unify_types(result, witness, subst, &next_subst) != 0) {
          if (err_stream) {
            fprintf(err_stream, "Type Error: failed to resolve comparable ");
            print_type_to_stream(result, err_stream);
            fprintf(err_stream, " to ");
            print_type_to_stream(witness, err_stream);
            fprintf(err_stream, " for %s\n",
                    p->trait ? p->trait->name : "(null)");
            fflush(err_stream);
          }
          return 1;
        }
        if (next_subst) {
          subst = next_subst;
          changed = true;
        }

        if (p->trait == GenericArithmetic && !all_concrete) {
          for (int j = 0; p->data.COMPARABLE.args[j]; j++) {
            Type *arg = apply_subst_to_type(subst, p->data.COMPARABLE.args[j]);
            if (!is_generic(arg)) {
              continue;
            }
            next_subst = NULL;
            if (unify_types(p->data.COMPARABLE.args[j], witness, subst,
                            &next_subst) != 0) {
              if (err_stream) {
                fprintf(err_stream, "Type Error: failed to make arithmetic "
                                    "operand comparable at ");
                print_type_to_stream(witness, err_stream);
                fprintf(err_stream, "\n");
                fflush(err_stream);
              }
              return 1;
            }
            if (next_subst) {
              subst = next_subst;
              changed = true;
            }
          }
        }

        // Eq/Ord comparability should push the resolved witness back only
        // into operands that are still generic. This forces `None` in `Some 1
        // == None` to become `Option Int` without rejecting already concrete
        // mixed comparisons like `1 == 2.0`.
        if (p->trait == GenericEq || p->trait == GenericOrd) {
          for (int j = 0; p->data.COMPARABLE.args[j]; j++) {
            Type *arg = apply_subst_to_type(subst, p->data.COMPARABLE.args[j]);
            if (!is_generic(arg)) {
              continue;
            }
            next_subst = NULL;
            if (unify_types(p->data.COMPARABLE.args[j], witness, subst,
                            &next_subst) != 0) {
              if (err_stream) {
                fprintf(err_stream,
                        "Type Error: failed to make operand comparable at ");
                print_type_to_stream(witness, err_stream);
                fprintf(err_stream, " for %s\n",
                        p->trait ? p->trait->name : "(null)");
                fflush(err_stream);
              }
              return 1;
            }
            if (next_subst) {
              subst = next_subst;
              changed = true;
            }
          }
        }
      }
    }
  }
  if (subst_ptr) {
    *subst_ptr = subst;
  }
  return 0;
}

static TypeList *typelist_apply_subst(Subst *subst, TypeList *params) {
  if (!params) {
    return NULL;
  }
  TypeList *head = NULL;
  TypeList *tail = NULL;
  for (TypeList *tl = params; tl; tl = tl->next) {
    TypeList *node = t_alloc(sizeof(TypeList));
    node->type = apply_subst_to_type(subst, tl->type);
    node->next = NULL;
    if (!head) {
      head = node;
    } else {
      tail->next = node;
    }
    tail = node;
  }
  return head;
}

bool predicate_is_generic(Predicate *p) {
  if (!p) {
    return false;
  }
  if (p->kind == PRED_TRAIT) {
    if (is_generic(p->data.TRAIT.type)) {
      return true;
    }
    for (TypeList *tl = p->data.TRAIT.params; tl; tl = tl->next) {
      if (is_generic(tl->type)) {
        return true;
      }
    }
    return false;
  }
  if (p->kind == PRED_COMPARABLE) {
    if (is_generic(p->data.COMPARABLE.witness)) {
      return true;
    }
    for (int i = 0; p->data.COMPARABLE.args && p->data.COMPARABLE.args[i];
         i++) {
      if (is_generic(p->data.COMPARABLE.args[i])) {
        return true;
      }
    }
    return false;
  }
  return false;
}

// ============================================================================
// Constraint infrastructure
// ============================================================================

void add_constraint(TICtx *result, Type *var, Type *type) {
  if (!var || !type) {
    return;
  }
  for (Constraint *c = result->constraints; c; c = c->next) {
    if (c->kind == CONSTRAINT_EQUALITY &&
        types_equal(c->data.EQUALITY.left, var) &&
        types_equal(c->data.EQUALITY.right, type)) {
      return;
    }
  }
  Constraint *constraint = t_alloc(sizeof(Constraint));
  *constraint = (Constraint){.kind = CONSTRAINT_EQUALITY,
                             .data = {.EQUALITY = {.left = var, .right = type}},
                             .next = result->constraints};
  result->constraints = constraint;
}

Constraint *merge_constraints(Constraint *list1, Constraint *list2) {
  // Simple concat for now
  if (!list1)
    return list2;
  if (!list2)
    return list1;
  Constraint *tail = list1;
  while (tail->next)
    tail = tail->next;
  tail->next = list2;
  return list1;
}

// ============================================================================
// Unification (structural only - no feature-specific branching)
// ============================================================================

static bool occurs_in(int var_id, Type *type) {
  if (!type)
    return false;

  switch (type->kind) {
  case T_VAR:
    return type->data.T_VAR.id == var_id;
  case T_RECURSIVE_REF:
    return false;
  case T_FN:
    return occurs_in(var_id, type->data.T_FN.from) ||
           occurs_in(var_id, type->data.T_FN.to);
  case T_CONS:
  case T_SUM:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (occurs_in(var_id, type->data.T_CONS.args[i]))
        return true;
    }
    return false;
  default:
    return false;
  }
}

static Subst *clone_subst(Subst *subst) { return subst_table_clone(subst); }

static Subst *extend_subst(Subst *subst, int var_id, Type *type) {
  if (is_empty_subst(subst)) {
    subst = subst_table_create(type_var_counter);
  }
  return subst_table_extend(subst, var_id, type);
}

Type *lookup_subst(Subst *subst, int var_id) {
  return subst_table_lookup(subst, var_id);
}

static Type *find_root_var(Subst *subst, Type *t) {
  if (!t || t->kind != T_VAR)
    return apply_subst_to_type(subst, t);

  Type *next = lookup_subst(subst, t->data.T_VAR.id);
  if (!next)
    return t;
  if (next->kind != T_VAR || next->data.T_VAR.id == t->data.T_VAR.id)
    return apply_subst_to_type(subst, next);
  return find_root_var(subst, next);
}

static int unify_types(Type *t1, Type *t2, Subst *subst, Subst **out);

static int unify_types(Type *t1, Type *t2, Subst *subst, Subst **out) {
  t1 = apply_subst_to_type(subst, t1);
  t2 = apply_subst_to_type(subst, t2);

  if (types_equal(t1, t2))
    return 0;

  if (t1->kind == T_VAR) {
    if (occurs_in(t1->data.T_VAR.id, t2))
      return 1;
    *out = extend_subst(subst, t1->data.T_VAR.id, t2);
    return 0;
  }

  if (t2->kind == T_VAR) {
    if (occurs_in(t2->data.T_VAR.id, t1))
      return 1;
    *out = extend_subst(subst, t2->data.T_VAR.id, t1);
    return 0;
  }

  if (t1->kind == T_CONS && t2->kind == T_FN) {
    Type *view = callable_view(t1);
    if (view != t1) {
      return unify_types(view, t2, subst, out);
    }
  }

  if (t2->kind == T_CONS && t1->kind == T_FN) {
    Type *view = callable_view(t2);
    if (view != t2) {
      return unify_types(t1, view, subst, out);
    }
  }

  if (t2->kind == T_RECURSIVE_REF &&
      (t1->kind == T_CONS || t1->kind == T_SUM)) {
    TypeEnv *decl = t2->data.T_RECURSIVE_REF.decl;
    if (decl && decl->type && types_equal(decl->type, t1)) {
      return 0;
    }
    return 1;
  }

  if (t1->kind == T_RECURSIVE_REF &&
      (t2->kind == T_CONS || t2->kind == T_SUM)) {
    TypeEnv *decl = t1->data.T_RECURSIVE_REF.decl;
    if (decl && decl->type && types_equal(decl->type, t2)) {
      return 0;
    }
    return 1;
  }

  if (t1->kind == T_RECURSIVE_REF || t2->kind == T_RECURSIVE_REF) {
    return 1;
  }

  if (t1->kind == T_FN && t2->kind == T_FN) {
    Subst *s1 = NULL;
    if (unify_types(t1->data.T_FN.from, t2->data.T_FN.from, subst, &s1))
      return 1;
    Subst *s2 = NULL;
    Subst *use_subst = (s1 != NULL) ? s1 : subst;
    if (unify_types(t1->data.T_FN.to, t2->data.T_FN.to, use_subst, &s2))
      return 1;
    *out = (s2 != NULL) ? s2 : use_subst;
    return 0;
  }

  if ((t1->kind == T_CONS || t1->kind == T_SUM) &&
      (t2->kind == T_CONS || t2->kind == T_SUM)) {
    if (t1->data.T_CONS.num_args != t2->data.T_CONS.num_args)
      return 1;
    Subst *s = subst;
    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      Subst *next = NULL;
      if (unify_types(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i], s,
                      &next))
        return 1;
      s = next ? next : s;
    }
    *out = s;
    return 0;
  }

  return 1;
}

Subst *solve_constraints(Constraint *constraints) {
  Subst *subst = subst_table_create(type_var_counter);

  for (Constraint *c = constraints; c != NULL; c = c->next) {
    if (c->kind != CONSTRAINT_EQUALITY) {
      return NULL;
    }
    Subst *new_subst = NULL;
    if (unify_types(c->data.EQUALITY.left, c->data.EQUALITY.right, subst,
                    &new_subst) != 0) {
      return NULL;
    }
    if (new_subst) {
      subst = new_subst;
    }
  }

  return is_empty_subst(subst) ? subst_table_empty() : subst;
}

Subst *compose_subst(Subst *s1, Subst *s2) {
  if (is_empty_subst(s1)) {
    s1 = NULL;
  }
  if (is_empty_subst(s2)) {
    s2 = NULL;
  }
  if (!s1 && !s2) {
    return subst_table_empty();
  }
  if (!s1) {
    return s2;
  }
  if (!s2) {
    return s1;
  }

  Subst *result = clone_subst(s2);
  int binding_count = subst_table_binding_count(s1);
  for (int i = 0; i < binding_count; i++) {
    int var_id = subst_table_bound_var_id(s1, i);
    if (var_id < 0) {
      continue;
    }
    Type *binding = lookup_subst(s1, var_id);
    if (!binding) {
      continue;
    }
    Type *applied = apply_subst_to_type(s2, binding);
    result = extend_subst(result, var_id, applied);
  }
  return result ? result : subst_table_empty();
}

Type *apply_substitution(Subst *subst, Type *t) {
  return apply_subst_to_type(subst, t);
}

static bool is_empty_subst(Subst *subst) { return subst_table_is_empty(subst); }

Type *apply_subst_to_type(Subst *subst, Type *t) {
  if (!t) {
    return NULL;
  }

  switch (t->kind) {
  case T_VAR: {
    Type *found = lookup_subst(subst, t->data.T_VAR.id);
    if (!found || types_equal(found, t))
      return t;
    return apply_subst_to_type(subst, found);
  }
  case T_RECURSIVE_REF:
    return t;
  case T_FN: {
    Type *from = apply_subst_to_type(subst, t->data.T_FN.from);
    Type *to = apply_subst_to_type(subst, t->data.T_FN.to);
    if (from == t->data.T_FN.from && to == t->data.T_FN.to) {
      return t;
    }
    Type *result = t_alloc(sizeof(Type));
    *result = (Type){T_FN, {.T_FN = {from, to}}};
    result->data.T_FN.attributes = t->data.T_FN.attributes;
    result->closure_meta = apply_subst_to_type(subst, t->closure_meta);
    return result;
  }
  case T_CONS:
  case T_SUM: {
    Type **new_args = NULL;
    bool changed = false;
    if (t->data.T_CONS.num_args > 0) {
      new_args = t_alloc(sizeof(Type *) * t->data.T_CONS.num_args);
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        new_args[i] = apply_subst_to_type(subst, t->data.T_CONS.args[i]);
        if (new_args[i] != t->data.T_CONS.args[i])
          changed = true;
      }
    }
    if (!changed)
      return t;
    if (is_coroutine_type(t)) {
      return create_coroutine_instance_type(new_args[0]);
    }
    Type *result = t_alloc(sizeof(Type));
    *result = *t;
    result->data.T_CONS.args = new_args;
    return result;
  }
  case T_MODULE: {
    if (t->data.T_MODULE.env) {
      apply_subst_env(subst, t->data.T_MODULE.env);
    }
    return t;
  }
  default:
    return t;
  }
}

// ============================================================================
// Stubs for remaining infrastructure
// ============================================================================

void *type_error(Ast *ast, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "Type Error: ");
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, " ");
  print_location(ast);
  return NULL;
}

TypeEnv *apply_subst_env(Subst *subst, TypeEnv *env) {
  for (TypeEnv *e = env; e; e = e->next) {
    e->type = apply_subst_to_type(subst, e->type);
    if (e->predicates) {
      e->predicates = predicate_apply_subst(subst, e->predicates);
    }
  }
  return env;
}

int unify(Type *t1, Type *t2, TICtx *unify_res) {
  // Legacy unify returning 0 always; application inference will move
  // to explicit constraint generation instead.
  add_constraint(unify_res, t1, t2);
  return 0;
}

void print_constraints(Constraint *constraints) {
  for (Constraint *c = constraints; c; c = c->next) {
    if (c->kind != CONSTRAINT_EQUALITY) {
      continue;
    }
    print_type_to_stream(c->data.EQUALITY.left, stdout);
    printf("\t:: ");
    print_type_to_stream(c->data.EQUALITY.right, stdout);
    printf("\n");
  }
}

void print_subst(Subst *subst) {}

int bind_type_in_ctx(Ast *binding, Type *type, binding_md binding_type,
                     TICtx *ctx) {
  return 0;
}

bool is_list_cons_operator(Ast *ast) {
  if (!ast || ast->tag != AST_APPLICATION) {
    return false;
  }

  Ast *fn = ast->data.AST_APPLICATION.function;
  while (fn && fn->tag == AST_APPLICATION) {
    fn = fn->data.AST_APPLICATION.function;
  }

  return fn && fn->tag == AST_IDENTIFIER &&
         CHARS_EQ(fn->data.AST_IDENTIFIER.value, TYPE_NAME_OP_LIST_PREPEND);
}

void apply_substitution_to_lambda_body(Ast *ast, Subst *subst) {
  finalize_ast_types(ast, subst);
}

Type *resolve_type_in_env(Type *r, TypeEnv *env) {
  if (!r) {
    return NULL;
  }

  if (r->closure_meta) {
    r->closure_meta = resolve_type_in_env(r->closure_meta, env);
  }

  switch (r->kind) {
  case T_VAR: {
    Type *saved_closure_meta = r->closure_meta;

    if (r->is_recursive_type_ref) {
      return r;
    }

    Type *resolved = env_lookup(env, r->data.T_VAR.name);
    if (!resolved) {
      return r;
    }

    if (resolved->kind == T_VAR && types_equal(resolved, r)) {
      return r;
    }

    Type *copy = deep_copy_type(resolved);
    copy = resolve_type_in_env(copy, env);
    if (saved_closure_meta && !copy->closure_meta) {
      copy->closure_meta = deep_copy_type(saved_closure_meta);
    }
    return copy;
  }

  case T_TYPECLASS_RESOLVE: {
    return resolve_tc_rank_in_env(r, env);
  }

  case T_CONS:
  case T_SUM: {
    for (int i = 0; i < r->data.T_CONS.num_args; i++) {
      r->data.T_CONS.args[i] = resolve_type_in_env(r->data.T_CONS.args[i], env);
    }
    return r;
  }

  case T_FN: {
    r->data.T_FN.from = resolve_type_in_env(r->data.T_FN.from, env);
    r->data.T_FN.to = resolve_type_in_env(r->data.T_FN.to, env);
    return r;
  }

  default: {
    return r;
  }
  }
}

Type *find_in_subst(Subst *subst, int var_id) {
  return lookup_subst(subst, var_id);
}

Type *extract_member_from_sum_type(Type *cons, Ast *id) {
  if (!cons || cons->kind != T_SUM || !id) {
    return NULL;
  }

  while (id->tag == AST_RECORD_ACCESS) {
    id = id->data.AST_RECORD_ACCESS.member;
  }

  if (id->tag != AST_IDENTIFIER) {
    return NULL;
  }

  for (int i = 0; i < cons->data.T_CONS.num_args; i++) {
    Type *mem = cons->data.T_CONS.args[i];
    if (mem && (mem->kind == T_CONS || mem->kind == T_SUM) &&
        CHARS_EQ(id->data.AST_IDENTIFIER.value, mem->data.T_CONS.name)) {
      return mem;
    }
  }
  return NULL;
}

Type *extract_member_from_sum_type_idx(Type *cons, Ast *id, int *idx) {
  if (idx) {
    *idx = -1;
  }

  if (!cons || cons->kind != T_SUM || !id) {
    return NULL;
  }

  while (id->tag == AST_RECORD_ACCESS) {
    id = id->data.AST_RECORD_ACCESS.member;
  }

  if (id->tag != AST_IDENTIFIER) {
    return NULL;
  }

  for (int i = 0; i < cons->data.T_CONS.num_args; i++) {
    Type *mem = cons->data.T_CONS.args[i];
    if (mem && (mem->kind == T_CONS || mem->kind == T_SUM) &&
        CHARS_EQ(id->data.AST_IDENTIFIER.value, mem->data.T_CONS.name)) {
      if (idx) {
        *idx = i;
      }
      return mem;
    }
  }
  return NULL;
}

bool is_constant_expr(Ast *expr, TICtx *ctx) {
  if (!expr) {
    return false;
  }

  switch (expr->tag) {
  case AST_INT:
  case AST_FLOAT:
  case AST_DOUBLE:
  case AST_CHAR:
  case AST_BOOL:
    return true;

  case AST_APPLICATION: {
    for (int i = 0; i < expr->data.AST_APPLICATION.len; i++) {
      if (!is_constant_expr(expr->data.AST_APPLICATION.args + i, ctx)) {
        return false;
      }
    }
    return true;
  }

  default:
    return false;
  }
}

Type *empty_type() {
  Type *t = t_alloc(sizeof(Type));
  memset(t, 0, sizeof(Type));
  return t;
}

// ============================================================================
// Type variable generation
// ============================================================================

int type_var_counter = 0;

void reset_type_var_counter() { type_var_counter = 0; }

Type *next_tvar() {
  Type *tvar = t_alloc(sizeof(Type));
  char *tname = t_alloc(sizeof(char) * 5);
  sprintf(tname, "`%d", type_var_counter);
  *tvar = (Type){T_VAR, {.T_VAR = {.name = tname, .id = type_var_counter}}};
  type_var_counter++;
  return tvar;
}

// ============================================================================
// AST finalization: apply substitution to every node's type annotation
// ============================================================================

static void finalize_ast_types(Ast *ast, Subst *subst) {
  if (!ast)
    return;

  if (ast->type) {

    ast->type = apply_subst_to_type(subst, ast->type);
  }

  switch (ast->tag) {
  case AST_BODY: {
    AST_LIST_ITER(ast->data.AST_BODY.stmts,
                  ({ finalize_ast_types(l->ast, subst); }));
    break;
  }
  case AST_LET:
  case AST_LOOP: {
    finalize_ast_types(ast->data.AST_LET.binding, subst);
    finalize_ast_types(ast->data.AST_LET.expr, subst);
    finalize_ast_types(ast->data.AST_LET.in_expr, subst);
    break;
  }
  case AST_APPLICATION: {
    finalize_ast_types(ast->data.AST_APPLICATION.function, subst);
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      finalize_ast_types(ast->data.AST_APPLICATION.args + i, subst);
    }
    break;
  }
  case AST_LAMBDA: {
    finalize_ast_types(ast->data.AST_LAMBDA.body, subst);
    break;
  }
  case AST_MATCH: {
    finalize_ast_types(ast->data.AST_MATCH.expr, subst);
    for (int i = 0; i < ast->data.AST_MATCH.len * 2; i++) {
      finalize_ast_types(ast->data.AST_MATCH.branches + i, subst);
    }
    break;
  }
  case AST_MATCH_GUARD_CLAUSE: {
    finalize_ast_types(ast->data.AST_MATCH_GUARD_CLAUSE.test_expr, subst);
    finalize_ast_types(ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr, subst);
    break;
  }
  case AST_TUPLE:
  case AST_ARRAY:
  case AST_LIST: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      finalize_ast_types(ast->data.AST_LIST.items + i, subst);
    }
    break;
  }
  case AST_FMT_STRING: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      finalize_ast_types(ast->data.AST_LIST.items + i, subst);
    }
    break;
  }
  case AST_RECORD_ACCESS: {
    finalize_ast_types(ast->data.AST_RECORD_ACCESS.record, subst);
    finalize_ast_types(ast->data.AST_RECORD_ACCESS.member, subst);
    break;
  }
  case AST_YIELD: {
    finalize_ast_types(ast->data.AST_YIELD.expr, subst);
    break;
  }
  case AST_UNOP: {
    finalize_ast_types(ast->data.AST_UNOP.expr, subst);
    break;
  }
  case AST_RANGE_EXPRESSION: {
    finalize_ast_types(ast->data.AST_RANGE_EXPRESSION.from, subst);
    finalize_ast_types(ast->data.AST_RANGE_EXPRESSION.to, subst);
    break;
  }
  default:
    break;
  }
}

// ============================================================================
// Binding helpers (legacy, will be replaced by env-based binding)
// ============================================================================

void register_binding(Ast *b, Type *bt, TICtx *ctx) {
  switch (b->tag) {
  case AST_IDENTIFIER: {
    ctx->env = env_extend(ctx->env, b->data.AST_IDENTIFIER.value, bt);
    break;
  }
  case AST_TUPLE: {
    int len = b->data.AST_LIST.len;
    for (int i = 0; i < len; i++) {
      register_binding(b->data.AST_LIST.items + i, bt->data.T_CONS.args[i],
                       ctx);
    }
    break;
  }
  default:
    break;
  }
}
