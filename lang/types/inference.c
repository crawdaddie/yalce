#include "./inference.h"
#include "./builtins.h"
#include "./common.h"
#include "./infer_binding.h"
#include "./infer_lambda.h"
#include "./type.h"
#include "./unification.h"
#include "serde.h"
#include "types/infer_app.h"
#include "types/infer_match_expression.h"
#include "types/type_expressions.h"
#include <stdarg.h>
#include <string.h>

void *type_error(TICtx *ctx, Ast *node, const char *fmt, ...) {

  FILE *err_stream = ctx->err_stream ? ctx->err_stream : stderr;
  va_list args;
  va_start(args, fmt);

  vfprintf(err_stream, fmt, args);
  if (node && node->loc_info) {
    _print_location(node, err_stream);
  } else if (node) {
    print_ast_err(node);
  }
  va_end(args);
  return NULL;
}

Type *apply_substitution(Subst *subst, Type *t);
void print_subst(Subst *subst);

TypeEnv *env_extend(TypeEnv *env, const char *name, VarList *names,
                    Type *type) {
  TypeEnv *new_env = talloc(sizeof(TypeEnv));
  *new_env = (TypeEnv){
      .name = name,
      .scheme =
          {
              .vars = names,
              .type = type,
          },
      .next = env,
  };
  return new_env;
}

void print_typescheme(Scheme scheme) {
  for (VarList *v = scheme.vars; v; v = v->next) {
    const char *n = v->var;
    printf("%s, ", n);
  }
  if (scheme.vars) {
    printf(" : ");
  }
  print_type(scheme.type);
}

void print_type_env(TypeEnv *env) {
  if (!env) {
    return;
  }
  printf("%s : ", env->name);
  print_typescheme(env->scheme);
  if (env->next) {
    print_type_env(env->next);
  }
}

Scheme *lookup_scheme(TypeEnv *env, const char *name) {
  for (TypeEnv *e = env; e; e = e->next) {
    if (CHARS_EQ(name, e->name)) {
      return &e->scheme;
    }
  }
  return lookup_builtin_scheme(name);
}

TypeEnv *lookup_scheme_ref(TypeEnv *env, const char *name) {
  for (TypeEnv *e = env; e; e = e->next) {
    if (CHARS_EQ(name, e->name)) {
      return e;
    }
  }
  return NULL;
}

Type *find_in_subst(Subst *subst, const char *name) {
  for (Subst *s = subst; s; s = s->next) {
    if (CHARS_EQ(name, s->var)) {
      return s->type;
    }
  }
  return NULL;
}

bool subst_contains(Subst *subst, const char *k, Type *t) {
  Type *found = find_in_subst(subst, k);
  return (found != NULL) && (types_equal(t, found));
}

Subst *subst_extend(Subst *s, const char *key, Type *type) {
  if (subst_contains(s, key, type)) {
    return s;
  }
  Subst *n = talloc(sizeof(Subst));
  *n = (Subst){.var = key, .type = type, .next = s};
  return n;
}

Subst *compose_subst(Subst *s1, Subst *s2) {
  for (Subst *kv = s2; kv; kv = kv->next) {
    const char *k = kv->var;

    Type *type = kv->type;
    type = apply_substitution(s1, type);
    if (!subst_contains(s1, k, type)) {
      s1 = subst_extend(s1, k, type);
    }
  }
  return s1;
}

bool varlist_contains(VarList *vs, const char *name) {
  for (VarList *vl = vs; vl; vl = vl->next) {
    if (CHARS_EQ(name, vl->var)) {
      return true;
    }
  }
  return false;
}

VarList *varlist_extend(VarList *vars, const char *new) {
  VarList *n = talloc(sizeof(VarList));
  *n = (VarList){.var = new, .next = vars};
  return n;
}

VarList *varlist_add(VarList *vars, const char *v) {
  if (!varlist_contains(vars, v)) {
    vars = varlist_extend(vars, v);
  }
  return vars;
}
int scheme_vars_count(VarList *l) {
  int n = 0;
  while (l) {
    l = l->next;
    n++;
  }
  return n;
}

bool schemes_match(Scheme *s1, Scheme *s2) {
  if (!s1 && !s2)
    return true;
  if (!s1 || !s2)
    return false;

  int n = scheme_vars_count(s1->vars);
  // Check same number of quantified variables
  if (n != scheme_vars_count(s2->vars)) {
    return false;
  }

  // If no variables, just compare types directly
  if (!s1->vars && !s2->vars) {
    return types_equal(s1->type, s2->type);
  }

  // Stack allocate arrays for the known number of variables
  char shared_names[n][16];   // Names for shared variables
  Type shared_vars[n];        // Shared type variables
  Subst substitutions[n * 2]; // Substitution entries (2 per variable pair)

  // Initialize shared variables
  for (int i = 0; i < n; i++) {
    snprintf(shared_names[i], 16, "shared%d", i);
    shared_vars[i] = (Type){.kind = T_VAR, .data.T_VAR = shared_names[i]};
  }

  // Build substitution list using stack-allocated entries
  VarList *v1 = s1->vars;
  VarList *v2 = s2->vars;

  for (int i = 0; i < n; i++) {
    // First substitution: s1's var -> shared var
    substitutions[i * 2] =
        (Subst){.var = v1->var,
                .type = &shared_vars[i],
                .next = (i == 0) ? NULL : &substitutions[i * 2 - 1]};

    // Second substitution: s2's var -> shared var
    substitutions[i * 2 + 1] = (Subst){
        .var = v2->var, .type = &shared_vars[i], .next = &substitutions[i * 2]};

    v1 = v1->next;
    v2 = v2->next;
  }

  // Head of substitution list (last entry created)
  Subst *shared_subst = &substitutions[n * 2 - 1];

  // Apply the substitution to both types
  Type *inst1 = apply_substitution(shared_subst, s1->type);
  Type *inst2 = apply_substitution(shared_subst, s2->type);

  // Compare the results
  return types_equal(inst1, inst2);
}

Scheme apply_subst_scheme(Subst *subst, Scheme scheme) {
  Subst *filtered_subst = NULL;
  for (Subst *s = subst; s; s = s->next) {
    if (!varlist_contains(scheme.vars, s->var)) {
      filtered_subst = subst_extend(filtered_subst, s->var, s->type);
    }
  }
  return (Scheme){.vars = scheme.vars,
                  .type = apply_substitution(filtered_subst, scheme.type)};
}

TypeEnv *apply_subst_env(Subst *subst, TypeEnv *env) {
  for (TypeEnv *e = env; e; e = e->next) {
    e->scheme = apply_subst_scheme(subst, e->scheme);
  }
  return env;
}

double tc_rank(const char *tc_name, Type *t) {
  TypeClass *tc = t->implements;
  for (TypeClass *tc = t->implements; tc; tc = tc->next) {
    if CHARS_EQ (tc->name, tc_name) {
      return tc->rank;
    }
  }
  return 0.;
}
Type *tc_resolve(Type *tcr) {
  const char *tc_name = tcr->data.T_CONS.name;
  Type *a = tcr->data.T_CONS.args[0];
  Type *b = tcr->data.T_CONS.args[1];

  if (b->kind == T_TYPECLASS_RESOLVE) {
    b = tc_resolve(b);
  }

  if (tc_rank(tc_name, a) >= tc_rank(tc_name, b)) {
    return a;
  }
  return b;
}

Type *apply_substitution(Subst *subst, Type *t) {
  if (!subst) {
    return t;
  }
  if (!t) {
    return NULL;
  }
  switch (t->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_STRING: {
    break;
  }

  case T_VAR: {
    Type *x = find_in_subst(subst, t->data.T_VAR);

    if (x) {
      return x;
    }
    return t;
  }
  case T_FN: {
    t->data.T_FN.from = apply_substitution(subst, t->data.T_FN.from);
    t->data.T_FN.to = apply_substitution(subst, t->data.T_FN.to);
    return t;
  }

  case T_TYPECLASS_RESOLVE: {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      Type *s = apply_substitution(subst, t->data.T_CONS.args[i]);
      // if (get_typeclass_by_name(s, t->data.T_CONS.name) == 0) {
      //   fprintf(stderr, "Error: cannot apply substitution because ");
      //   print_type_err(s);
      //   fprintf(stderr, "does not implement %s\n", t->data.T_CONS.name);
      //   return NULL;
      // }
      //
      t->data.T_CONS.args[i] = s;
    }

    if (!is_generic(t)) {
      return tc_resolve(t);
    }

    return t;
  }
  case T_CONS: {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      t->data.T_CONS.args[i] =
          apply_substitution(subst, t->data.T_CONS.args[i]);
    }
    return t;
  }
  }
  return t;
}

VarList *free_vars_type(Type *t) {
  if (!t) {
    return NULL;
  }
  switch (t->kind) {
  case T_VAR: {
    VarList *v = talloc(sizeof(VarList));
    *v = (VarList){.var = t->data.T_VAR, .next = NULL};
    return v;
  }
  case T_FN: {
    VarList *from_vars = free_vars_type(t->data.T_FN.from);
    VarList *to_vars = free_vars_type(t->data.T_FN.to);
    // Concatenate lists (simplified - should remove duplicates)
    if (!from_vars)
      return to_vars;
    VarList *curr = from_vars;
    while (curr->next)
      curr = curr->next;
    curr->next = to_vars;
    return from_vars;
  }

  case T_TYPECLASS_RESOLVE:
  case T_CONS: {
    VarList *vars = NULL;
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      VarList *arg_vars = free_vars_type(t->data.T_CONS.args[i]);
      if (!vars) {
        vars = arg_vars;
      } else {
        VarList *curr = vars;
        while (curr->next)
          curr = curr->next;
        curr->next = arg_vars;
      }
    }
    return vars;
  }
  default:
    return NULL;
  }
}

VarList *free_vars_env(TypeEnv *env) {
  VarList *vars = NULL;
  for (TypeEnv *e = env; e; e = e->next) {
    VarList *scheme_vars = free_vars_type(e->scheme.type);
    VarList *filtered = NULL;
    for (VarList *sv = scheme_vars; sv; sv = sv->next) {
      if (!varlist_contains(e->scheme.vars, sv->var)) {
        VarList *new_var = talloc(sizeof(VarList));
        *new_var = (VarList){.var = sv->var, .next = filtered};
        filtered = new_var;
      }
    }
    if (!vars) {
      vars = filtered;
    } else {
      VarList *curr = vars;
      while (curr->next)
        curr = curr->next;
      curr->next = filtered;
    }
  }
  return vars;
}

Scheme generalize(Type *type, TypeEnv *env) {
  VarList *type_vars = free_vars_type(type);
  VarList *env_vars = free_vars_env(env);

  VarList *free_vars = NULL;
  for (VarList *tv = type_vars; tv; tv = tv->next) {
    if (!varlist_contains(env_vars, tv->var)) {
      VarList *new_var = talloc(sizeof(VarList));
      *new_var = (VarList){.var = tv->var, .next = free_vars};
      free_vars = new_var;
    }
  }

  return (Scheme){.vars = free_vars, .type = type};
}

void print_subst(Subst *subst) {
  printf("substitutions:\n");
  for (Subst *s = subst; s; s = s->next) {
    printf("  %s : ", s->var);
    print_type(s->type);
  }
  printf("\n");
}

// Helper function to find a variable by name in a type
Type *find_var_in_type(Type *type, const char *var_name) {
  if (!type)
    return NULL;

  switch (type->kind) {
  case T_VAR:
    return CHARS_EQ(type->data.T_VAR, var_name) ? type : NULL;
  case T_FN: {
    Type *found = find_var_in_type(type->data.T_FN.from, var_name);
    return found ? found : find_var_in_type(type->data.T_FN.to, var_name);
  }
  case T_CONS:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      Type *found = find_var_in_type(type->data.T_CONS.args[i], var_name);
      if (found)
        return found;
    }
    return NULL;
  default:
    return NULL;
  }
}

Type *instantiate(Scheme *scheme, TICtx *ctx) {
  if (!scheme) {
    return NULL;
  }

  if (!scheme->vars) {
    return scheme->type;
  }

  Subst *inst_subst = NULL;
  for (VarList *v = scheme->vars; v; v = v->next) {
    Type *fresh_type = next_tvar();
    inst_subst = subst_extend(inst_subst, v->var, fresh_type);
  }

  Type *stype = deep_copy_type(scheme->type);

  Type *s = apply_substitution(inst_subst, stype);
  return s;
}

Type *instantiate_with_args(Scheme *scheme, Ast *args, TICtx *ctx) {
  if (!scheme) {
    return NULL;
  }

  if (!scheme->vars) {
    return scheme->type;
  }

  Subst *inst_subst = NULL;
  for (VarList *v = scheme->vars; v; v = v->next, args++) {

    Type *t = infer(args, ctx);
    inst_subst = subst_extend(inst_subst, v->var, t);
  }

  Type *stype = deep_copy_type(scheme->type);

  Type *s = apply_substitution(inst_subst, stype);
  return s;
}
void handle_yield_boundary_crossing(binding_md binding_info, Ast *ast,
                                    TICtx *ctx) {
  if (!ctx->current_fn_ast) {
    return;
  }
  int binding_scope = binding_info.data.VAR.scope;
  if (binding_scope < ctx->current_fn_base_scope) {
    // not defined within current function, ignore
    return;
  }

  int yield_boundary = binding_info.data.VAR.yield_boundary_scope;
  int crosses_yield_boundary =
      ctx->current_fn_ast &&
      ctx->current_fn_ast->data.AST_LAMBDA.num_yields >
          yield_boundary; // there is a yield between the creation of this
                          // binding and its use
  if (!crosses_yield_boundary) {
    // print_ast(ast);
    // printf("defined in scope %d [boundary %d] - current scope %d current "
    //        "boundary %d\n",
    //        binding_info.data.VAR.scope,
    //        binding_info.data.VAR.yield_boundary_scope, ctx->scope,
    //        ctx->current_fn_ast->data.AST_LAMBDA.num_yields);

    return;
  }

  // scan boundary xer list
  for (AstList *l =
           ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers;
       l; l = l->next) {
    Ast *a = l->ast;
    if (CHARS_EQ(a->data.AST_IDENTIFIER.value,
                 ast->data.AST_IDENTIFIER.value)) {
      // already registered
      // printf("already registered\n");
      return;
    }
  }

  // printf("extend boundary crossers of ");
  // print_ast(ctx->current_fn_ast);
  // printf(" with ");
  // print_ast(ast);
  ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers =
      ast_list_extend_left(
          ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers, ast);

  return;
}

Type *infer_identifier(Ast *ast, TICtx *ctx) {

  TypeEnv *scheme_ref =
      lookup_scheme_ref(ctx->env, ast->data.AST_IDENTIFIER.value);

  if (scheme_ref) {
    if (scheme_ref->md.type == BT_RECURSIVE_REF) {
      // printf("looked up recursive ref\n");
      // print_type(scheme_ref->scheme.type);
      return scheme_ref->scheme.type;
    }

    if (scheme_ref->md.type == BT_VAR) {
      handle_yield_boundary_crossing(scheme_ref->md, ast, ctx);
    }

    Scheme s = scheme_ref->scheme;
    Type *inst = instantiate(&s, ctx);
    return inst;
  }

  Scheme *builtin_scheme =
      lookup_builtin_scheme(ast->data.AST_IDENTIFIER.value);

  if (!builtin_scheme) {
    return next_tvar();
  }
  return instantiate(builtin_scheme, ctx);
}

Type *create_list_type(Ast *ast, const char *cons_name, TICtx *ctx) {

  if (ast->data.AST_LIST.len == 0) {
    Type *t = talloc(sizeof(Type));
    Type **el = talloc(sizeof(Type *));
    el[0] = next_tvar();
    *t = (Type){T_CONS, {.T_CONS = {cons_name, el, 1}}};
    return t;
  }

  int len = ast->data.AST_LIST.len;
  Type *el_type = infer(ast->data.AST_LIST.items, ctx);

  for (int i = 1; i < len; i++) {
    Ast *el = ast->data.AST_LIST.items + i;
    Type *_el_type = infer(el, ctx);

    if (_el_type->kind == T_VAR) {
      unify(_el_type, el_type, ctx);

    } else if (!types_equal(el_type, _el_type)) {
      print_type_err(el_type);
      print_type_err(_el_type);
      return type_error(
          ctx, ast,
          "Typecheck Error: typechecking list literal - all elements must "
          "be of the same type");
    }
    el_type = _el_type;
  }
  Type *type = talloc(sizeof(Type));
  Type **contained = talloc(sizeof(Type *));
  contained[0] = el_type;
  *type = (Type){T_CONS, {.T_CONS = {cons_name, contained, 1}}};
  return type;
}

Type *infer_yield_expr(Ast *ast, TICtx *ctx) {
  Ast *expr = ast->data.AST_YIELD.expr;

  Type *expr_type = infer(expr, ctx);

  if (ctx->yielded_type != NULL) {
    if (unify(expr_type, ctx->yielded_type, ctx)) {
      fprintf(stderr, "Error: could not unify yield expressions in function - "
                      "all yields must be of the same type\n");
      return NULL;
    }
  }
  ctx->yielded_type = expr_type;
  ctx->current_fn_ast->data.AST_LAMBDA.num_yields++;
  return expr_type;
}

Type *infer(Ast *ast, TICtx *ctx) {
  Type *type = NULL;
  switch (ast->tag) {

  case AST_INT: {
    type = &t_int;
    break;
  }
  case AST_DOUBLE: {
    type = &t_num;
    break;
  }
  case AST_STRING: {
    type = &t_string;
    break;
  }
  case AST_CHAR: {
    type = &t_char;
    break;
  }

  case AST_BOOL: {
    type = &t_bool;
    break;
  }
  case AST_VOID: {
    type = &t_void;
    break;
  }

  case AST_ARRAY: {
    type = create_list_type(ast, TYPE_NAME_ARRAY, ctx);
    if (!type) {
      return NULL;
    }
    break;
  }

  case AST_LIST: {
    type = create_list_type(ast, TYPE_NAME_LIST, ctx);
    if (!type) {
      return NULL;
    }
    break;
  }

  case AST_TUPLE: {

    int arity = ast->data.AST_LIST.len;
    Type **args = talloc(sizeof(Type *) * arity);
    if (ast->data.AST_LIST.items[0].tag == AST_LET) {
      const char **names = talloc(sizeof(char *) * arity);

      // named tuple
      for (int i = 0; i < arity; i++) {
        if (ast->data.AST_LIST.items[i].data.AST_LET.binding->tag !=
            AST_IDENTIFIER) {
          return NULL;
        }
        names[i] = ast->data.AST_LIST.items[i]
                       .data.AST_LET.binding->data.AST_IDENTIFIER.value;
        TICtx _ctx = *ctx;
        _ctx.scope++;
        args[i] = infer(ast->data.AST_LIST.items[i].data.AST_LET.expr, &_ctx);
      }
      type = create_tuple_type(arity, args);
      type->data.T_CONS.names = names;
      break;
    }

    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;

      Type *member_type;
      member_type = infer(member, ctx);

      args[i] = member_type;
      if (!member_type) {
        return NULL;
      }
    }

    type = create_tuple_type(arity, args);
    break;
  }

  case AST_TYPE_DECL: {
    type = type_declaration(ast, ctx);
    break;
  }

  case AST_FMT_STRING: {
    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      Type *member_type = infer(member, ctx);
      if (!member_type) {
        return NULL;
      }
    }
    type = &t_string;
    break;
  }

  case AST_BODY: {
    Ast *stmt;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      stmt = ast->data.AST_BODY.stmts[i];
      Type *t = infer(stmt, ctx);
      if (!t) {
        // print_ast_err(stmt);
        return NULL;
      }
      type = t;
    }
    break;
  }

  case AST_IDENTIFIER: {
    type = infer_identifier(ast, ctx);
    break;
  }

  case AST_APPLICATION: {
    type = infer_app(ast, ctx);
    break;
  }
  case AST_LET: {
    Ast *binding = ast->data.AST_LET.binding;
    Ast *val = ast->data.AST_LET.expr;
    Ast *body = ast->data.AST_LET.in_expr;
    type = infer_let_pattern_binding(binding, val, body, ctx);
    break;
  }

  case AST_LAMBDA: {
    type = infer_lambda(ast, ctx);
    break;
  }

  case AST_MATCH: {
    type = infer_match_expression(ast, ctx);
    break;
  }

  case AST_EXTERN_FN: {

    Ast *sig = ast->data.AST_EXTERN_FN.signature_types;

    if (sig->tag == AST_FN_SIGNATURE) {
      Scheme *s = compute_type_expression(sig, ctx);
      type = s->type;
    }
    break;
  }

  case AST_YIELD: {
    type = infer_yield_expr(ast, ctx);
    // TODO: Implement yield inference
    break;
  }

  case AST_MODULE: {
    // TODO: Implement module inference
    break;
  }

  case AST_TRAIT_IMPL: {
    // TODO: Implement trait implementation inference
    break;
  }

  case AST_IMPORT: {
    // TODO: Implement import inference
    break;
  }

  case AST_RECORD_ACCESS: {
    // TODO: Implement record access inference
    break;
  }

  case AST_RANGE_EXPRESSION: {
    // TODO: Implement range expression inference
    break;
  }

  case AST_LOOP: {
    // TODO: Implement loop inference
    break;
  }

  case AST_BINOP: {
    // TODO: Implement binary operation inference
    break;
  }

  default: {
    return type_error(ctx, ast, "Unknown AST node type: %d\n", ast->tag);
  }
  }

  // Store the inferred type in the AST node's metadata
  ast->md = type;
  // Pure constraint collection unification - NO SOLVING
  return type;
}
