#include "types/inference.h"
#include "types/util.h"

// Global variables
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }

Type *infer(TypeEnv env, Ast *ast) {
  switch (ast->tag) {

  case AST_BODY: {
    Type *body_type = NULL;
    TypeEnv current_env = env;
    for (size_t i = 0; i < ast->data.AST_BODY.len; i++) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      body_type = infer(current_env, stmt);
    }
    ast->md = body_type;
    return body_type;
  }
  case AST_INT: {
    ast->md = &t_int;
    return &t_int;
  }

  case AST_NUMBER: {
    ast->md = &t_num;
    return &t_num;
  }

  case AST_STRING: {
    ast->md = &t_string;
    return &t_string;
  }

  case AST_BOOL: {
    ast->md = &t_bool;
    return &t_bool;
  }

  case AST_VOID: {
    ast->md = &t_void;
    return &t_void;
  }

  case AST_BINOP: {
    Type *lt = infer(env, ast->data.AST_BINOP.left);
    Type *rt = infer(env, ast->data.AST_BINOP.right);
    if (lt == NULL || rt == NULL) {
      return NULL;
    }
    Type *res;
    if (is_numeric_type(lt) && is_numeric_type(rt)) {
      token_type op = ast->data.AST_BINOP.op;
      if (op >= TOKEN_PLUS && op <= TOKEN_MODULO) {
        res = lt->kind >= rt->kind ? lt : rt;
      } else if (op >= TOKEN_LT && op <= TOKEN_NOT_EQUAL) {
        res = &t_bool;
      } else {
        return NULL;
      }
    } else if (is_type_variable(lt) && is_numeric_type(rt)) {
      res = lt;
    } else if (is_type_variable(rt) && is_numeric_type(lt)) {
      res = rt;
    } else {
      res = lt;
    }

    ast->md = res;
    return res;
  }
  }
  return NULL;
}

Type *infer_ast(TypeEnv env, Ast *ast) {
  // TypeEnv env = NULL;
  // Add initial environment entries (e.g., built-in functions)
  // env = extend_env(env, "+", create_type_scheme(...));
  // env = extend_env(env, "-", create_type_scheme(...));
  // ...

  Type *result = infer(env, ast);

  if (result == NULL) {
    fprintf(stderr, "Type inference failed\n");
  }

  return result;
}
