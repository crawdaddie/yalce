#include "infer_match_expr.h"
#include "serde.h"
#include "types/unification.h"
#include <stdlib.h>

#define TRY(expr)                                                              \
  ({                                                                           \
    typeof(expr) _result = (expr);                                             \
    if (!_result) {                                                            \
      fprintf(stderr, "Error: expected something @%s:%d\n", __FILE__,          \
              __LINE__);                                                       \
      return NULL;                                                             \
    }                                                                          \
    _result;                                                                   \
  })

Type *next_tvar();

Type *create_type(enum TypeKind kind) {
  Type *type = empty_type();
  type->kind = kind;
  return type;
}

Type *create_type_var() { return next_tvar(); }

Type *infer(Ast *ast, TypeEnv **env);

TypeEnv *extend_env_with_bindings(Ast *test_expr, TypeEnv *env) {
  Type *expr_type = test_expr->md;
  switch (test_expr->tag) {
  case AST_IDENTIFIER:
    return env_extend(env, test_expr->data.AST_IDENTIFIER.value, expr_type);
  case AST_TUPLE: {
    TypeEnv *new_env = env;
    for (int i = 0; i < test_expr->data.AST_LIST.len; i++) {
      Type *elem_type = expr_type->data.T_CONS.args[i];

      new_env =
          extend_env_with_bindings(test_expr->data.AST_LIST.items + i, new_env);
      if (new_env == NULL)
        return NULL;
    }
    return new_env;
  }
  case AST_APPLICATION: {
    TypeEnv *new_env = env;
    for (int i = 0; i < test_expr->data.AST_APPLICATION.len; i++) {
      Type *arg_type = expr_type->data.T_CONS.args[i];
      new_env = extend_env_with_bindings(
          test_expr->data.AST_APPLICATION.args + i, new_env);
      if (new_env == NULL)
        return NULL;
    }
    return new_env;
  }
  default:
    // No bindings for literal values
    return env;
  }
}

Type *infer_test_expr(Ast *test_expr, TypeEnv **env) {
  Type *t;
  switch (test_expr->tag) {
  case AST_APPLICATION: {

    Type **arg_types =
        talloc(sizeof(Type *) * test_expr->data.AST_APPLICATION.len);

    for (int i = 0; i < test_expr->data.AST_APPLICATION.len; i++) {
      arg_types[i] = TRY(infer(test_expr->data.AST_APPLICATION.args + i, env));
    }

    const char *cons_name =
        test_expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;

    t = create_cons_type(cons_name, test_expr->data.AST_APPLICATION.len,
                         arg_types);
    test_expr->md = t;
    return t;
  }

  default: {

    t = infer(test_expr, env);
    return t;
  }
  }
}

Type *infer_match(Ast *ast, TypeEnv **env) {
  Ast *expr = ast->data.AST_MATCH.expr;
  Type *expr_type = TRY(infer(expr, env));

  Type *res_type = NULL;

  for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
    Ast *test_expr = ast->data.AST_MATCH.branches + (2 * i);
    Ast *result_expr = ast->data.AST_MATCH.branches + (2 * i) + 1;
    Type *test_type = TRY(infer_test_expr(test_expr, env));
    Type *unified_type = TRY(unify(expr_type, test_type, env));
    *expr_type = *unified_type;
    TypeEnv *extended_env = TRY(extend_env_with_bindings(test_expr, *env));
    Type *branch_res_type = TRY(infer(result_expr, &extended_env));

    if (res_type == NULL) {
      res_type = branch_res_type;
    } else {
      Type *unified_res_type = TRY(unify(res_type, branch_res_type, env));
      res_type = unified_res_type;
    }
  }

  return res_type;
}
