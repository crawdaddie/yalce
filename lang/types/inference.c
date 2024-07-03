#include "types/inference.h"
#include "types/type.h"
#include "types/util.h"
#include <stdlib.h>

// Global variables
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }
const char *fresh_tvar_name() {
  char prefix = 't';
  char *new_name = malloc(5 * sizeof(char));
  if (new_name == NULL) {
    return NULL;
  }
  sprintf(new_name, "t%d", type_var_counter);
  type_var_counter++;
  return new_name;
}

static Type *next_tvar() { return create_type_var(fresh_tvar_name()); }
static bool is_ord(Type *t) { return (t->kind >= T_INT) && (t->kind <= T_NUM); }
static bool is_arithmetic(Type *t) {
  return (t->kind >= T_INT) && (t->kind <= T_NUM);
}
static Type *max_numeric_type(Type *lt, Type *rt) {
  if (lt->kind >= rt->kind) {
    return lt;
  }
  return rt;
}

static Type *copy_type(Type *t) {
  Type *copy = malloc(sizeof(Type));
  *copy = *t;
  if (copy->kind == T_VAR) {
    char *new_name = malloc(5 * sizeof(char));
    sprintf(new_name, "%s'", t->data.T_VAR);
    copy->data.T_VAR = new_name;
  }
  return copy;
}

static Type *copy_fn_type(Type *fn) {
  if (fn->kind != T_FN) {
    return copy_type(fn);
  }

  Type *copy = create_type_fn(copy_fn_type(fn->data.T_FN.from),
                              copy_fn_type(fn->data.T_FN.to));
  return copy;
}
// Main type inference function
//
Type *infer(TypeEnv *env, Ast *ast) {
  if (!ast)
    return NULL;

  Type *type = NULL;

  switch (ast->tag) {
  case AST_BODY: {
    TypeEnv *current_env = env;
    for (size_t i = 0; i < ast->data.AST_BODY.len; i++) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      type = infer(current_env, stmt);
      if (stmt->tag == AST_LET) {
        current_env =
            env_extend(current_env, stmt->data.AST_LET.name.chars, type);
      }
    }
    break;
  }
  case AST_BINOP: {
    Type *lt = (infer(env, ast->data.AST_BINOP.left));
    Type *rt = (infer(env, ast->data.AST_BINOP.right));
    if (lt == NULL || rt == NULL) {
      return NULL;
    }

    if ((ast->data.AST_BINOP.op >= TOKEN_PLUS) &&
        (ast->data.AST_BINOP.op <= TOKEN_MODULO)) {
      if (is_arithmetic(lt) && is_arithmetic(rt)) {
        type = max_numeric_type(lt, rt);
        break;
      } else {
        unify(lt, rt);
        type = lt;

        fprintf(
            stderr,
            "Not implemented err: type coercion for non-arithmetic (eg T_VAR) "
            "type to Arithmetic typeclass\n");
        break;
      }
      // arithmetic binop

    } else if ((ast->data.AST_BINOP.op >= TOKEN_LT) &&
               (ast->data.AST_BINOP.op <= TOKEN_NOT_EQUAL)) {

      if (is_arithmetic(lt) && is_arithmetic(rt)) {
        type = &t_bool;
        break;
      } else {
        unify(lt, rt);
        type = lt;

        fprintf(stderr, "Not implemented err: type coercion for non-ord "
                        "type to Ord typeclass\n");
        break;
      }
      break;
    }
    type = lt;
    break;
  }
  case AST_INT:
    type = &t_int;
    break;
  case AST_NUMBER:
    type = &t_num;
    break;
  case AST_STRING:
    type = &t_string;
    break;
  case AST_BOOL:
    type = &t_bool;
    break;

  case AST_IDENTIFIER: {
    type = env_lookup(env, ast->data.AST_IDENTIFIER.value);
    // printf("infer id %s: ", ast->data.AST_IDENTIFIER.value);
    // print_type(type);
    // printf("\n");

    if (!type) {
      fprintf(stderr, "Typecheck Error: unbound variable %s\n",
              ast->data.AST_IDENTIFIER.value);
      return NULL;
    }
    break;
  }
  case AST_LET: {
    Type *expr_type = infer(env, ast->data.AST_LET.expr);
    TypeEnv *new_env = env_extend(env, ast->data.AST_LET.name.chars, expr_type);
    if (ast->data.AST_LET.in_expr) {
      type = infer(new_env, ast->data.AST_LET.in_expr);
    } else {
      type = expr_type;
    }
    break;
  }
    /*
  case AST_LAMBDA: {
    Type *param_types[ast->data.AST_LAMBDA.len];
    for (size_t i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      param_types[i] = next_tvar();
    }

    TypeEnv *new_env = env;
    for (size_t i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      new_env = env_extend(new_env, ast->data.AST_LAMBDA.params[i].chars,
                           param_types[i]);
    }

    Type *body_type = infer(new_env, ast->data.AST_LAMBDA.body);
    type = body_type;

    for (int i = ast->data.AST_LAMBDA.len - 1; i >= 0; i--) {

      type = create_type_fn(param_types[i], type);
    }
    printf("inferred fn type: ");
    print_type(type);
    printf("\n");

    break;
  }
*/
  case AST_LAMBDA: {
    Type *param_types[ast->data.AST_LAMBDA.len];
    for (size_t i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      param_types[i] = next_tvar();
    }

    // Create the function type
    Type *fn_type =
        next_tvar(); // Start with a type variable for the whole function
    Type *current = fn_type;
    for (int i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      Type *next = (i == ast->data.AST_LAMBDA.len - 1)
                       ? next_tvar()
                       : create_type_fn(NULL, NULL);
      current->kind = T_FN;
      current->data.T_FN.from = param_types[i];
      current->data.T_FN.to = next;
      current = next;
    }

    TypeEnv *new_env = env;

    // If the lambda has a name, add it to the environment for recursion
    if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
      new_env =
          env_extend(new_env, ast->data.AST_LAMBDA.fn_name.chars, fn_type);
    }

    // Add parameters to the environment
    for (size_t i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      new_env = env_extend(new_env, ast->data.AST_LAMBDA.params[i].chars,
                           param_types[i]);
    }

    // Infer the type of the body
    Type *body_type = infer(new_env, ast->data.AST_LAMBDA.body);

    // Unify the return type with the body type
    unify(current, body_type);

    type = fn_type;

    break;
  }
  case AST_APPLICATION: {
    Type *fn_type = infer(env, ast->data.AST_APPLICATION.function);
    if (!fn_type) {
      return NULL;
    }
    Type *arg_types[ast->data.AST_APPLICATION.len];
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      arg_types[i] = infer(env, ast->data.AST_APPLICATION.args + i);
    }

    Type *result_type = next_tvar();
    Type *expected_fn_type = result_type;

    for (int i = ast->data.AST_APPLICATION.len - 1; i >= 0; i--) {
      expected_fn_type = create_type_fn(arg_types[i], expected_fn_type);
    }
    printf("apply: ");
    print_type(expected_fn_type);
    printf(" -- ");
    print_type(fn_type);
    printf("\n");

    unify(fn_type, expected_fn_type);
    type = result_type;
    break;
  }
  case AST_MATCH: {
    Ast *branches = ast->data.AST_MATCH.branches;
    Ast *expr = ast->data.AST_MATCH.expr;
    Type *expr_type = infer(env, expr);

    Type *test_type;
    Type *body_type;

    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      Ast *test = ++branches;
      test_type = infer(env, test);
      unify(expr_type, test_type);

      Ast *body = ++branches;
      body_type = infer(env, body);

      branches += 2;
    }
    type = body_type;
    break;
  }
  }

  ast->md = type;
  return type;
}

Type *infer_ast(TypeEnv *env, Ast *ast) {
  // TypeTypeEnv env = NULL;
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
