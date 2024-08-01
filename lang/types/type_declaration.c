#include "types/type_declaration.h"
#include "serde.h"
#include "types/util.h"
#include <stdlib.h>

static Type *compute_type_expression(Ast *expr, TypeEnv *env) {
  switch (expr->tag) {

  case AST_LIST: {
    int len = expr->data.AST_LIST.len;
    if (len == 1) {
      return compute_type_expression(expr->data.AST_LIST.items, env);
    }

    Type *un = malloc(sizeof(Type));
    un->kind = T_UNION;
    un->data.T_UNION.num_args = len;
    un->data.T_UNION.args = malloc(sizeof(Type *));

    for (int i = 0; i < len; i++) {
      Type *t = compute_type_expression(expr->data.AST_LIST.items + i, env);
      un->data.T_UNION.args[i] = t;
    }
    return un;
  }

  case AST_APPLICATION: {
    Type *tc_type =
        compute_type_expression(expr->data.AST_APPLICATION.function, env);

    if (!tc_type || !(tc_type->kind == T_TYPECLASS)) {
      return NULL;
    }

    Ast *type_ast = expr->data.AST_APPLICATION.args;
    Type *type = compute_type_expression(type_ast, env);
    add_typeclass_impl(type, tc_type->data.T_TYPECLASS);
    print_type_w_tc(type);
    return type;
  }
  case AST_IDENTIFIER: {
    Type *type = get_type(env, expr);
    return type;
  }
  }
  return NULL;
}

void type_declaration(Ast *ast, TypeEnv **env) {
  Ast *binding = ast->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;

  Ast *type_expr_ast = ast->data.AST_LET.expr;
  Type *type = compute_type_expression(type_expr_ast, *env);
  type->alias = name;

  *env = env_extend(*env, name, type);
}
