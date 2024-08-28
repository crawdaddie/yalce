#include "type_declaration.h"
#include "serde.h"
#include <string.h>

Type *next_tvar();
Type *compute_type_expression(Ast *expr, TypeEnv *env) {
  switch (expr->tag) {
  case AST_LIST: {
    int len = expr->data.AST_LIST.len;
    if (len == 1) {
      return compute_type_expression(expr->data.AST_LIST.items, env);
    }

    Type *variant = empty_type();
    Type **variant_members = talloc(sizeof(Type *) * len);

    for (int i = 0; i < len; i++) {
      Ast *item = expr->data.AST_LIST.items + i;
      variant_members[i] = compute_type_expression(item, env);
    }
    *variant = (Type){T_CONS, {.T_CONS = {"Variant", variant_members, len}}};
    return variant;
  }

  case AST_IDENTIFIER: {
    Type *type = find_type_in_env(env, expr->data.AST_IDENTIFIER.value);
    if (!type) {
      return tvar(expr->data.AST_IDENTIFIER.value);
    }
    return type;
  }

  case AST_LAMBDA: {
    TypeEnv *_env = env;
    int len = expr->data.AST_LAMBDA.len;

    Ast *param_ast = expr->data.AST_LAMBDA.params;
    const char *param_name = strdup(param_ast->data.AST_IDENTIFIER.value);
    Type *param_type = tvar(param_name);
    _env = env_extend(_env, param_name, param_type);
    Type *fn = param_type;

    for (int i = 1; i < len; i++) {
      Ast *param_ast = expr->data.AST_LAMBDA.params + i;
      const char *param_name = strdup(param_ast->data.AST_IDENTIFIER.value);
      Type *param_type = tvar(param_name);
      _env = env_extend(_env, param_name, param_type);
      fn = type_fn(fn, param_type);
    }

    fn = type_fn(fn, compute_type_expression(expr->data.AST_LAMBDA.body, _env));
    return fn;
  }

  case AST_BINOP: {
    if (expr->data.AST_BINOP.op == TOKEN_OF) {
      Type *contained_type =
          compute_type_expression(expr->data.AST_BINOP.right, env);
      Type *cons = empty_type();
      *cons = (Type){
          T_CONS,
          {.T_CONS = {.name = strdup(
                          expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value),
                      .args = &contained_type,
                      .num_args = 1}}};

      return cons;
    }
  }
  }
  return NULL;
}

Type *type_declaration(Ast *ast, TypeEnv **env) {

  Ast *binding = ast->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;

  Ast *type_expr_ast = ast->data.AST_LET.expr;
  Type *type = compute_type_expression(type_expr_ast, *env);
  if (!type) {
    fprintf(stderr, "Error computing type declaration");
  }
  type->alias = name;

  *env = env_extend(*env, name, type);
  return type;
}
