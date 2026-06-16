#include "./inference.h"
#include "../ht.h"
#include "../parse.h"
#include "./builtins.h"
#include "types/type.h"
#include "types/type_ser.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *infer_match_expression(Ast *ast, TICtx *ctx) {
  // Stub: will contain pattern matching type inference
  return next_tvar();
}

Type *infer_application(Ast *ast, TICtx *ctx) {
  // Stub: will contain pattern matching type inference
  return next_tvar();
}

Type *infer_lambda(Ast *ast, TICtx *ctx) {
  // Stub: will contain pattern matching type inference
  return next_tvar();
}

// ============================================================================
// Forward declarations for static helpers in this file
// ============================================================================
Type *infer_expr(Ast *ast, TICtx *ctx);
static Type *apply_subst_to_type(Subst *subst, Type *t);
static Subst *extend_subst(Subst *subst, const char *var, Type *type);

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

// infer_final: apply final substitution to all AST node types.
// This is the ONE place where AST type annotations are mutated post-solve.
static void finalize_ast_types(Ast *ast, Subst *subst);

void infer_final(Ast *ast, const Solution *solved, TICtx *ctx) {
  if (!solved || !solved->subst) {
    return;
  }
  finalize_ast_types(ast, solved->subst);
}

Type *apply_solution(Type *raw, Solution *solved) {
  if (!solved || !solved->subst) {
    return raw;
  }
  return apply_subst_to_type(solved->subst, raw);
}

// infer: the public entry point.
// 1. Infer expression types and generate constraints
// 2. Solve constraints once
// 3. Apply substitution to the result type
// 4. Finalize AST annotations
Type *infer(Ast *ast, TICtx *ctx) {
  Type *raw = infer_expr(ast, ctx);
  if (!raw) {
    return NULL;
  }

  Solution sol = {0};
  if (infer_solve(ctx, &sol)) {
    return NULL;
  }

  Type *final = apply_solution(raw, &sol);
  infer_final(ast, &sol, ctx);
  return final;
}

// ============================================================================
// Literal inference helpers
// ============================================================================

Type *infer_list_literal(Ast *ast, TICtx *ctx) {
  if (ast->data.AST_LIST.len == 0) {
    Type *t = t_alloc(sizeof(Type));
    Type **contained = t_alloc(sizeof(Type *));
    contained[0] = next_tvar();
    *t = (Type){
        T_CONS,
        {.T_CONS = {ast->tag == AST_LIST ? TYPE_NAME_LIST : TYPE_NAME_ARRAY,
                    contained, 1}}};
    return t;
  }

  int len = ast->data.AST_LIST.len;
  Type *el_type = infer_expr(ast->data.AST_LIST.items, ctx);

  for (int i = 1; i < len; i++) {
    Ast *el = ast->data.AST_LIST.items + i;
    Type *_el_type = infer_expr(el, ctx);
    el_type = _el_type;
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
  TypeEnv *new_env = t_alloc(sizeof(TypeEnv));
  new_env->name = name;
  new_env->type = type;
  new_env->scheme_vars = NULL;
  new_env->next = env;
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

// ============================================================================
// Free variable helpers
// ============================================================================

static bool type_list_contains_str(TypeList *l, const char *name) {
  for (TypeList *c = l; c; c = c->next) {
    if (c->type && c->type->kind == T_VAR &&
        strcmp(c->type->data.T_VAR, name) == 0) {
      return true;
    }
  }
  return false;
}

static TypeList *type_list_append_var(TypeList *acc, Type *tvar) {
  TypeList *node = t_alloc(sizeof(TypeList));
  node->type = tvar;
  node->next = NULL;
  if (!acc)
    return node;
  TypeList *tail = acc;
  while (tail->next)
    tail = tail->next;
  tail->next = node;
  return acc;
}

TypeList *free_vars_type(TypeList *acc, Type *t) {
  if (!t)
    return acc;
  switch (t->kind) {
  case T_VAR:
    // Skip recursive placeholders
    if (t->is_recursive_type_ref)
      return acc;
    if (!type_list_contains_str(acc, t->data.T_VAR)) {
      acc = type_list_append_var(acc, t);
    }
    return acc;
  case T_FN:
    acc = free_vars_type(acc, t->data.T_FN.from);
    acc = free_vars_type(acc, t->data.T_FN.to);
    return acc;
  case T_CONS:
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      acc = free_vars_type(acc, t->data.T_CONS.args[i]);
    }
    return acc;
  case T_SCHEME:
    // During transition: some places still use T_SCHEME
    return free_vars_type(acc, t->data.T_SCHEME.type);
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

static TypeList *set_diff(TypeList *a, TypeList *b) {
  TypeList *result = NULL;
  for (TypeList *la = a; la; la = la->next) {
    if (la->type && la->type->kind == T_VAR) {
      if (!type_list_contains_str(b, la->type->data.T_VAR)) {
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
  entry->scheme_vars = set_diff(fv_type, fv_env);
}

// instantiate: replace scheme_vars with fresh type variables.
Type *instantiate_env(TypeEnv *entry, TICtx *ctx) {
  if (!entry->scheme_vars) {
    return entry->type;
  }

  Subst *base = NULL;
  for (TypeList *v = entry->scheme_vars; v; v = v->next) {
    if (v->type && v->type->kind == T_VAR) {
      Type *fresh = next_tvar();
      fresh->implements = v->type->implements;
      base = extend_subst(base, v->type->data.T_VAR, fresh);
    }
  }
  if (!base)
    return entry->type;
  return apply_subst_to_type(base, entry->type);
}

Type *instantiate_type_in_env(Type *sch, TypeEnv *env) {
  // Stub for external callers still using T_SCHEME
  return sch;
}

// ============================================================================
// Backward-compatible wrappers (transitionary - external callers use T_SCHEME)
// ============================================================================

// generalize: create a T_SCHEME wrapper from a type.
Type *generalize(Type *t, TICtx *ctx) {
  (void)ctx;
  if (!is_generic(t))
    return t;

  Type *scheme = t_alloc(sizeof(Type));
  TypeList *vars = free_vars_type(NULL, t);
  int n = 0;
  for (TypeList *vl = vars; vl; vl = vl->next)
    n++;

  *scheme =
      (Type){T_SCHEME, {.T_SCHEME = {.vars = vars, .num_vars = n, .type = t}}};
  return scheme;
}

// instantiate: unwrap T_SCHEME and freshen its vars.
Type *instantiate(Type *t, TICtx *ctx) {
  if (!t || t->kind != T_SCHEME)
    return t;

  TypeEnv stub = {.name = "",
                  .type = t->data.T_SCHEME.type,
                  .scheme_vars = t->data.T_SCHEME.vars};
  return instantiate_env(&stub, ctx);
}

// ============================================================================
// Expression inference - HM core dispatcher
// ============================================================================

static Type *infer_identifier(Ast *ast, TICtx *ctx) {
  const char *name = ast->data.AST_IDENTIFIER.value;
  TypeEnv *ref = lookup_type_ref(ctx->env, name);

  if (ref) {
    return instantiate_env(ref, ctx);
  }

  Type *builtin = lookup_builtin_type(name);
  if (builtin) {
    if (builtin->kind == T_SCHEME) {
      TypeEnv stub = {.name = name,
                      .type = builtin->data.T_SCHEME.type,
                      .scheme_vars = builtin->data.T_SCHEME.vars};
      return instantiate_env(&stub, ctx);
    }
    return builtin;
  }

  return next_tvar();
}

static Type *infer_let_expr(Ast *ast, TICtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  Ast *body = ast->data.AST_LET.in_expr;

  Type *expr_type = infer_expr(expr, ctx);
  if (!expr_type) {
    return NULL;
  }

  switch (binding->tag) {
  case AST_IDENTIFIER: {
    ctx->env =
        env_extend(ctx->env, binding->data.AST_IDENTIFIER.value, expr_type);
    generalize_env(ctx->env, ctx->env->next);
    break;
  }
  case AST_TUPLE: {
    int len = binding->data.AST_LIST.len;
    Type **vars = t_alloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      Ast *b = binding->data.AST_LIST.items + i;
      Type *bt = next_tvar();
      vars[i] = bt;
      Ast *id = b;
      if (b->tag == AST_LET)
        id = b->data.AST_LET.binding;
      if (id->tag == AST_IDENTIFIER) {
        ctx->env = env_extend(ctx->env, id->data.AST_IDENTIFIER.value, bt);
      }
    }
    Type *lhs_tuple = create_tuple_type(len, vars);
    add_constraint(ctx, expr_type, lhs_tuple);
    break;
  }
  default:
    type_error(ast, "Unsupported let binding shape");
    return NULL;
  }

  if (body) {
    return infer_expr(body, ctx);
  }

  return expr_type;
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

  // Feature overlays
  case AST_APPLICATION:
    type = infer_application(ast, ctx);
    break;
  case AST_LAMBDA:
    type = infer_lambda(ast, ctx);
    break;
  case AST_MATCH:
    type = infer_match_expression(ast, ctx);
    break;

  // Stubs for constructs handled by future overlays
  case AST_TYPE_DECL:
    break;
  case AST_FMT_STRING:
    break;
  case AST_EXTERN_FN:
    break;
  case AST_YIELD:
    break;

  default:
    break;
  }

  ast->type = type;
  return type;
}

// ============================================================================
// Constraint infrastructure
// ============================================================================

void add_constraint(TICtx *result, Type *var, Type *type) {
  if (!var || !type) {
    return;
  }
  for (Constraint *c = result->constraints; c; c = c->next) {
    if (types_equal(c->var, var) && types_equal(c->type, type)) {
      return;
    }
  }
  Constraint *constraint = t_alloc(sizeof(Constraint));
  *constraint =
      (Constraint){.var = var, .type = type, .next = result->constraints};
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

static bool occurs_in(const char *var, Type *type) {
  if (!type)
    return false;

  switch (type->kind) {
  case T_VAR:
    return strcmp(type->data.T_VAR, var) == 0;
  case T_FN:
    return occurs_in(var, type->data.T_FN.from) ||
           occurs_in(var, type->data.T_FN.to);
  case T_CONS:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (occurs_in(var, type->data.T_CONS.args[i]))
        return true;
    }
    return false;
  default:
    return false;
  }
}

static Subst *extend_subst(Subst *subst, const char *var, Type *type) {
  Subst *s = t_alloc(sizeof(Subst));
  s->var = var;
  s->type = type;
  s->next = subst;
  return s;
}

static Type *lookup_subst(Subst *subst, const char *var) {
  for (Subst *s = subst; s != NULL; s = s->next) {
    if (strcmp(s->var, var) == 0) {
      return s->type;
    }
  }
  return NULL;
}

static Type *apply_subst_to_type(Subst *subst, Type *t) {
  if (!t)
    return NULL;

  switch (t->kind) {
  case T_VAR: {
    Type *found = lookup_subst(subst, t->data.T_VAR);
    return found ? found : t;
  }
  case T_FN: {
    Type *from = apply_subst_to_type(subst, t->data.T_FN.from);
    Type *to = apply_subst_to_type(subst, t->data.T_FN.to);
    if (from == t->data.T_FN.from && to == t->data.T_FN.to) {
      return t;
    }
    Type *result = t_alloc(sizeof(Type));
    *result = (Type){T_FN, {.T_FN = {from, to}}};
    return result;
  }
  case T_CONS: {
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
    Type *result = t_alloc(sizeof(Type));
    *result = *t;
    result->data.T_CONS.args = new_args;
    return result;
  }
  default:
    return t;
  }
}

static int unify_types(Type *t1, Type *t2, Subst *subst, Subst **out) {
  t1 = apply_subst_to_type(subst, t1);
  t2 = apply_subst_to_type(subst, t2);

  if (types_equal(t1, t2))
    return 0;

  if (t1->kind == T_VAR) {
    if (occurs_in(t1->data.T_VAR, t2))
      return 1;
    *out = extend_subst(subst, t1->data.T_VAR, t2);
    return 0;
  }

  if (t2->kind == T_VAR) {
    if (occurs_in(t2->data.T_VAR, t1))
      return 1;
    *out = extend_subst(subst, t2->data.T_VAR, t1);
    return 0;
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

  if (t1->kind == T_CONS && t2->kind == T_CONS) {
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
  Subst *subst = NULL;

  for (Constraint *c = constraints; c != NULL; c = c->next) {
    Subst *new_subst = NULL;
    if (unify_types(c->var, c->type, subst, &new_subst) != 0) {
      return NULL;
    }
    if (new_subst) {
      subst = new_subst;
    }
  }

  return subst;
}

Subst *compose_subst(Subst *s1, Subst *s2) {
  Subst *result = s2;
  for (Subst *s = s1; s != NULL; s = s->next) {
    Type *applied = apply_subst_to_type(s2, s->type);
    Subst *new_s = t_alloc(sizeof(Subst));
    new_s->var = s->var;
    new_s->type = applied;
    new_s->next = result;
    result = new_s;
  }
  return result;
}

Type *apply_substitution(Subst *subst, Type *t) {
  return apply_subst_to_type(subst, t);
}

// ============================================================================
// Backward-compatible wrappers (transitionary - external callers use T_SCHEME)
// ============================================================================

// generalize_type: create a T_SCHEME wrapper from a type.
// Used during transition by type_expressions.c and modules.c.
Type *generalize_type(Type *t, TICtx *ctx) {
  if (!is_generic(t))
    return t;

  Type *scheme = t_alloc(sizeof(Type));
  TypeList *vars = free_vars_type(NULL, t);
  int n = 0;
  for (TypeList *vl = vars; vl; vl = vl->next)
    n++;

  *scheme =
      (Type){T_SCHEME, {.T_SCHEME = {.vars = vars, .num_vars = n, .type = t}}};
  return scheme;
}

// instantiate_type: unwrap T_SCHEME and freshen its vars.
// Used during transition by type_expressions.c and modules.c.
Type *instantiate_type(Type *t, TICtx *ctx) {
  if (t->kind != T_SCHEME)
    return t;

  TypeEnv stub = {.name = "",
                  .type = t->data.T_SCHEME.type,
                  .scheme_vars = t->data.T_SCHEME.vars};
  return instantiate_env(&stub, ctx);
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

TypeEnv *apply_subst_env(Subst *subst, TypeEnv *env) { return env; }

int unify(Type *t1, Type *t2, TICtx *unify_res) {
  // Legacy unify returning 0 always; application inference will move
  // to explicit constraint generation instead.
  add_constraint(unify_res, t1, t2);
  return 0;
}

void print_constraints(Constraint *constraints) {
  for (Constraint *c = constraints; c; c = c->next) {
    print_type(c->var);
    printf("::");
    print_type(c->type);
  }
}

void print_subst(Subst *subst) {}

int bind_type_in_ctx(Ast *binding, Type *type, binding_md binding_type,
                     TICtx *ctx) {
  return 0;
}

bool is_list_cons_operator(Ast *ast) { return false; }

void apply_substitution_to_lambda_body(Ast *ast, Subst *subst) {}

Type *resolve_type_in_env(Type *r, TypeEnv *env) { return r; }

Type *find_in_subst(Subst *subst, const char *name) { return NULL; }

bool is_constant_expr(Ast *expr, TICtx *ctx) { return false; }

Type *empty_type() {
  Type *t = t_alloc(sizeof(Type));
  memset(t, 0, sizeof(Type));
  return t;
}

// ============================================================================
// Type variable generation
// ============================================================================

static int type_var_counter = 0;

void reset_type_var_counter() { type_var_counter = 0; }

Type *next_tvar() {
  Type *tvar = t_alloc(sizeof(Type));
  char *tname = t_alloc(sizeof(char) * 5);
  sprintf(tname, "`%d", type_var_counter);
  *tvar = (Type){T_VAR, {.T_VAR = tname}};
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
  case AST_LET: {
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
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      finalize_ast_types(ast->data.AST_MATCH.branches + i, subst);
    }
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
  case AST_BINOP: {
    finalize_ast_types(ast->data.AST_BINOP.left, subst);
    finalize_ast_types(ast->data.AST_BINOP.right, subst);
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
