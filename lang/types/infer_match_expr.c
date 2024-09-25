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

TypeEnv *extend_env_with_bindings(Ast *test_expr, Type *bind_val_type,
                                  TypeEnv *env) {
  // printf("extend env with bindings\n");
  // print_ast(test_expr);
  // print_type(bind_val_type);
  switch (test_expr->tag) {
  case AST_IDENTIFIER:
    return env_extend(env, test_expr->data.AST_IDENTIFIER.value, bind_val_type);

  case AST_TUPLE:
  case AST_LIST: {

    TypeEnv *_env = env;
    for (int i = 0; i < test_expr->data.AST_LIST.len; i++) {
      _env =
          extend_env_with_bindings(test_expr->data.AST_LIST.items + i,
                                   test_expr->data.AST_LIST.items[i].md, _env);
    }
    return _env;
  }

  case AST_APPLICATION: {
    TypeEnv *_env = env;
    for (int i = 0; i < test_expr->data.AST_APPLICATION.len; i++) {
      _env = extend_env_with_bindings(
          test_expr->data.AST_APPLICATION.args + i,
          test_expr->data.AST_APPLICATION.args[i].md, _env);
    }
    return _env;
  }

  case AST_MATCH_GUARD_CLAUSE: {
    return extend_env_with_bindings(
        test_expr->data.AST_MATCH_GUARD_CLAUSE.test_expr,
        test_expr->data.AST_MATCH_GUARD_CLAUSE.test_expr->md, env);
  }
  default:
    // No bindings for literal values
    return env;
  }
}

Type *infer_match(Ast *ast, TypeEnv **env) {
  Ast *expr = ast->data.AST_MATCH.expr;
  Type *expr_type = TRY(infer(expr, env));

  // printf("match expr type: ");
  // print_type(expr_type);

  Type *res_type = NULL;
  TypeEnv *extended_env = *env;

  for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
    Ast *test_expr = ast->data.AST_MATCH.branches + (2 * i);
    Ast *guard = NULL;

    if (test_expr->tag == AST_MATCH_GUARD_CLAUSE) {
      guard = test_expr->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
      test_expr = test_expr->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    }

    Ast *result_expr = ast->data.AST_MATCH.branches + (2 * i) + 1;

    Type *test_type = TRY(infer(test_expr, &extended_env));

    Type *unified_type = TRY(unify(expr_type, test_type, &extended_env));

    *expr_type = *unified_type;

    extended_env = extend_env_with_bindings(test_expr, expr_type, extended_env);

    if (guard != NULL) {
      TRY_MSG(infer(guard, &extended_env),
              "could not infer type of guard clause\n");
    }

    Type *branch_res_type = TRY(infer(result_expr, &extended_env));
    // printf("BRANCH %d:\n", i);
    // print_ast(result_expr->data.AST_BODY.stmts[2]->data.AST_APPLICATION.args
    // +
    //           1);
    //
    // print_type(
    //     (result_expr->data.AST_BODY.stmts[2]->data.AST_APPLICATION.args + 1)
    //         ->md);
    // print_type(branch_res_type);

    if (res_type == NULL) {
      res_type = branch_res_type;
    } else {

      Type *unified_res_type =
          TRY(unify(res_type, branch_res_type, &extended_env));

      res_type = unified_res_type;
    }
  }

  *expr_type = *resolve_generic_type(expr_type, extended_env);

  while (extended_env) {
    extended_env = extended_env->next;
  }

  return res_type;
}
