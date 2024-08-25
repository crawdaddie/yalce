#include "type_declaration.h"
int compute_type_expression(Ast *expr, TypeEnv *env) { return 0; }

int type_declaration(Ast *ast, TypeEnv **env) {
  Ast *binding = ast->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;

  Ast *type_expr_ast = ast->data.AST_LET.expr;

  if (compute_type_expression(type_expr_ast, *env)) {
    return 1;
  };
  // type->alias = name;

  *env = env_extend(*env, name, &type_expr_ast->md);
  return 0;
}
