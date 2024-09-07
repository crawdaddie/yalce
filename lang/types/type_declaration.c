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
    variant->kind = T_CONS;
    variant->data.T_CONS.name = TYPE_NAME_VARIANT;
    variant->data.T_CONS.args = talloc(sizeof(Type *) * len);
    variant->data.T_CONS.num_args = len;

    for (int i = 0; i < len; i++) {
      Ast *item = expr->data.AST_LIST.items + i;

      Type *member;
      if (item->tag == AST_IDENTIFIER) {
        member = empty_type();
        member->kind = T_CONS;
        member->data.T_CONS.args = NULL;
        member->data.T_CONS.num_args = 0;
        member->data.T_CONS.name = item->data.AST_IDENTIFIER.value;
      } else {
        member = compute_type_expression(item, env);
      }
      variant->data.T_CONS.args[i] = member;
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
    TypeEnv *_env = env;
    int len = expr->data.AST_LAMBDA.len;
    Type **param_types = talloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      Ast *param = expr->data.AST_LAMBDA.params + i;
      Type *param_type = compute_type_expression(param, env);
      param_types[i] = param_type;
      _env = env_extend(_env, param_type->data.T_VAR, param_type);
    }

    Type *t = compute_type_expression(expr->data.AST_LAMBDA.body, _env);

    expr->md = create_type_multi_param_fn(len, param_types, t);
    return t;
  }

  case AST_BINOP: {
    if (expr->data.AST_BINOP.op == TOKEN_OF) {
      Type *contained_type =
          compute_type_expression(expr->data.AST_BINOP.right, env);

      Type *cons = empty_type();
      cons->kind = T_CONS;
      cons->data.T_CONS.name =
          strdup(expr->data.AST_BINOP.left->data.AST_IDENTIFIER.value);
      cons->data.T_CONS.num_args = 1;
      cons->data.T_CONS.args = talloc(sizeof(Type *));
      cons->data.T_CONS.args[0] = contained_type;
      expr->data.AST_BINOP.right->md = contained_type;
      expr->data.AST_BINOP.right->md = contained_type;

      expr->data.AST_BINOP.left->md = type_fn(contained_type, cons);
      expr->md = cons;
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
