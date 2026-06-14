#include "./inference.h"
#include "../parse.h"
#include "./builtins.h"
#include "types/type.h"
#include "types/type_ser.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *infer_expr(Ast *ast, TICtx *ctx);

int infer_solve(TICtx *ctx, Solution *sol) {
  sol->subst = solve_constraints(ctx->constraints);
  return sol->subst ? 0 : 1;
}

void infer_final(Ast *ast, const Solution *solved, TICtx *ctx) {}

Type *apply_solution(Type *raw, Solution *solved) {
  if (!solved || !solved->subst)
    return raw;

  return apply_substitution(solved->subst, raw);
}

Type *infer(Ast *ast, TICtx *ctx) {
  Type *raw = infer_expr(ast, ctx);
  if (!raw) {
    return NULL;
  }
  print_constraints(ctx->constraints);

  // Solution solution;
  // if (infer_solve(ctx, &solution)) {
  //   return NULL;
  // }
  //
  // infer_final(ast, &solution, ctx);
  // return apply_solution(raw, &solution);
  return raw;
}

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
  }
}

Type *infer_let_expr(Ast *ast, TICtx *ctx) {

  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  Ast *body = ast->data.AST_LET.in_expr;
  TICtx body_ctx;

  if (body) {
    body_ctx = *ctx;
    body_ctx.scope++;
  }

  Type *expr_type = infer_expr(expr, body ? &body_ctx : ctx);

  if (!expr_type) {
    return NULL;
  }

  switch (binding->tag) {
  case AST_IDENTIFIER: {
    Type *binding_type;
    if (is_generic(expr_type)) {
      binding_type = next_tvar();
      add_constraint(ctx, expr_type, binding_type);
    } else {
      binding_type = expr_type;
    }
    register_binding(binding, binding_type, ctx);
    break;
  }

  case AST_TUPLE: {
    int len = binding->data.AST_LIST.len;

    Type **vars = t_alloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      Ast *b = binding->data.AST_LIST.items + i;
      Type *bt = next_tvar();
      vars[i] = b->type;

      b->type = bt;
      ctx->env = env_extend(ctx->env, binding->data.AST_IDENTIFIER.value, bt);
    };
    Type *lhs_tuple = create_tuple_type(len, vars);
    add_constraint(ctx, expr_type, lhs_tuple);
    register_binding(binding, lhs_tuple, ctx);
  }
  default: {
    type_error(ast, "Unsupported let binding shape");
    return NULL;
  }
  }

  if (body) {
    return infer_expr(body, &body_ctx);
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
  case AST_ARRAY:
  case AST_LIST: {
    type = infer_list_literal(ast, ctx);
    break;
  }
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
    }

    type = create_tuple_type(len, args);

    if (names) {
      type->data.T_CONS.names = names;
    }

    break;
  }
  case AST_LET: {
    type = infer_let_expr(ast, ctx);
    break;
  }
  case AST_TYPE_DECL: {
    break;
  }
  case AST_FMT_STRING: {
    break;
  }
  case AST_IDENTIFIER: {
    break;
  }
  case AST_APPLICATION: {
    break;
  }
  case AST_LAMBDA: {
    break;
  }
  case AST_MATCH: {
    break;
  }
  case AST_EXTERN_FN: {
    break;
  }
  case AST_YIELD: {
    break;
  }
  default: {
  }
  }

  ast->type = type;
  return type;
}

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type) {
  TypeEnv *new_env = t_alloc(sizeof(TypeEnv));
  new_env->name = name;
  new_env->type = type;
  new_env->next = env;
  return new_env;
}

Type *env_lookup(TypeEnv *env, const char *name) {
  for (TypeEnv *e = env; e != NULL; e = e->next) {
    if (strcmp(e->name, name) == 0) {
      return e->type;
    }
  }
  return NULL;
}

void *type_error(Ast *ast, const char *fmt, ...) { return NULL; }

Type *generalize(Type *t, TICtx *ctx) { return t; }

Type *instantiate(Type *sch, TICtx *ctx) { return sch; }

Type *instantiate_type_in_env(Type *sch, TypeEnv *env) { return sch; }

Type *extract_member_from_sum_type(Type *cons, Ast *id) { return NULL; }

Type *extract_member_from_sum_type_idx(Type *cons, Ast *id, int *idx) {
  return NULL;
}

Constraint *merge_constraints(Constraint *list1, Constraint *list2) {
  return list1;
}

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
    if (unify_types(t1->data.T_FN.to, t2->data.T_FN.to, s1 ? s1 : subst, &s2))
      return 1;
    *out = s2 ? s2 : subst;
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

TypeEnv *apply_subst_env(Subst *subst, TypeEnv *env) { return env; }

int unify(Type *t1, Type *t2, TICtx *unify_res) { return 0; }

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

TypeEnv *lookup_type_ref(TypeEnv *env, const char *name) { return NULL; }

bool is_list_cons_operator(Ast *ast) { return false; }

void apply_substitution_to_lambda_body(Ast *ast, Subst *subst) {}

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

Type *resolve_type_in_env(Type *r, TypeEnv *env) { return r; }

Type *find_in_subst(Subst *subst, const char *name) { return NULL; }

bool is_constant_expr(Ast *expr, TICtx *ctx) { return false; }

void reset_type_var_counter() {}

static int type_var_counter = 0;

Type *next_tvar() {
  Type *tvar = t_alloc(sizeof(Type));
  char *tname = t_alloc(sizeof(char) * 5);
  sprintf(tname, "`%d", type_var_counter);
  *tvar = (Type){T_VAR, {.T_VAR = tname}};
  type_var_counter++;
  return tvar;
}

Type *empty_type() {
  Type *t = t_alloc(sizeof(Type));
  memset(t, 0, sizeof(Type));
  return t;
}
