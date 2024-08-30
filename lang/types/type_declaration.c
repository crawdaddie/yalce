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

    Type **variant_members = talloc(sizeof(Type *) * len);
    Type *variant = empty_type();
    variant->kind = T_CONS;
    variant->data.T_CONS.name = "Variant";
    variant->data.T_CONS.args = variant_members;
    variant->data.T_CONS.num_args = len;

    for (int i = 0; i < len; i++) {
      Ast *item = expr->data.AST_LIST.items + i;
      variant->data.T_CONS.args[i] = compute_type_expression(item, env);
    }
    return variant;
  }

  case AST_IDENTIFIER: {
    Type *type = find_type_in_env(env, expr->data.AST_IDENTIFIER.value);
    if (!type) {
      const char *id_chars = expr->data.AST_IDENTIFIER.value;
      type = talloc(sizeof(Type));
      type->kind = T_VAR;
      type->data.T_VAR = id_chars;

      return type;
    }
    return type;
  }

  case AST_LAMBDA: {
    Type *t = compute_type_expression(expr->data.AST_LAMBDA.body, env);
    return t;
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
    return NULL;
  }

  type->alias = name;

  *env = env_extend(*env, name, type);
  return type;
}
