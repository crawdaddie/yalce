#include "types/inference.h"
#include "serde.h"
#include "types/type.h"
#include "types/util.h"
#include <stdlib.h>
#include <string.h>

// forward decl
Type *infer(TypeEnv **env, Ast *ast);

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

static bool is_placeholder(Ast *p) {
  return p->tag == AST_IDENTIFIER &&
         strcmp(p->data.AST_IDENTIFIER.value, "_") == 0;
}

static Type *infer_void_arg_lambda(TypeEnv **env, Ast *ast) {

  // Create the function type
  Type *ret_var = next_tvar();
  Type *fn_type = create_type_fn(&t_void, ret_var);

  TypeEnv *new_env = *env;

  // If the lambda has a name, add it to the environment for recursion
  if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
    new_env = env_extend(new_env, ast->data.AST_LAMBDA.fn_name.chars, fn_type);
  }

  // Infer the type of the body
  Type *body_type = infer(&new_env, ast->data.AST_LAMBDA.body);

  // Unify the return type with the body type
  unify(ret_var, body_type);

  return fn_type;
}

static Type *type_of_var(Ast *ast) {
  switch (ast->tag) {
  case AST_IDENTIFIER: {
    return next_tvar();
  }
  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;
    Type **tuple_mems = malloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      tuple_mems[i] = type_of_var(ast->data.AST_LIST.items + i);
    }
    return tcons("Tuple", tuple_mems, len);
  }
  default: {
    print_ast(ast);
    printf("\n");
    fprintf(stderr, "Typecheck err: lambda arg type %d unsupported\n",
            ast->tag);
    return NULL;
  }
  }
}
static TypeEnv *add_var_to_env(TypeEnv *env, Type *param_type, Ast *param_ast) {
  switch (param_ast->tag) {
  case AST_IDENTIFIER: {
    return env_extend(env, param_ast->data.AST_IDENTIFIER.value, param_type);
  }

  case AST_TUPLE: {
    int len = param_ast->data.AST_LIST.len;
    Ast *arg_asts = param_ast->data.AST_LIST.items;
    TypeEnv *new_env = env;
    for (int i = 0; i < len; i++) {
      Type *arg_type_var = param_type->data.T_CONS.args[i];
      new_env = add_var_to_env(new_env, arg_type_var, arg_asts + i);
    }
    return new_env;
  }

  default: {
    return env;
  }
  }
}

// Main type inference function
//
Type *infer(TypeEnv **env, Ast *ast) {

  if (!ast)
    return NULL;

  Type *type = NULL;

  switch (ast->tag) {
  case AST_BODY: {
    TypeEnv **current_env = env;
    for (size_t i = 0; i < ast->data.AST_BODY.len; i++) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      type = infer(current_env, stmt);
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

        fprintf(stderr, "Not implemented warning: type coercion for "
                        "non-arithmetic (eg T_VAR) "
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

        fprintf(stderr, "Not implemented warning: type coercion for non-ord "
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

  case AST_VOID: {
    type = &t_void;
    break;
  }

  case AST_IDENTIFIER: {
    if (is_placeholder(ast)) {
      type = next_tvar();
      break;
    }

    type = env_lookup(*env, ast->data.AST_IDENTIFIER.value);

    if (!type) {
      fprintf(stderr, "Typecheck Error: unbound variable %s\n",
              ast->data.AST_IDENTIFIER.value);
      return NULL;
    }
    break;
  }
  case AST_LET: {

    Type *expr_type = infer(env, ast->data.AST_LET.expr);
    *env = add_var_to_env(*env, expr_type, ast->data.AST_LET.binding);

    if (ast->data.AST_LET.in_expr) {
      type = infer(env, ast->data.AST_LET.in_expr);
    } else {
      type = expr_type;
    }
    break;
  }

  case AST_EXTERN_FN: {
    int param_count = ast->data.AST_EXTERN_FN.len - 1;

    Type **param_types;
    if (param_count == 0) {
      param_types = malloc(sizeof(Type *));
      *param_types = &t_void;
    } else {
      param_types = malloc(param_count * sizeof(Type *));
      for (int i = 0; i < param_count; i++) {
        param_types[i] =
            builtin_type(ast->data.AST_EXTERN_FN.signature_types + i);
      }
    }
    Type *ex_t = create_type_multi_param_fn(
        param_count || 1, param_types,
        builtin_type(ast->data.AST_EXTERN_FN.signature_types + param_count));
    type = ex_t;
    break;
  }

  case AST_LAMBDA: {
    int args_len = ast->data.AST_LAMBDA.len;
    if (args_len == 0) {
      type = infer_void_arg_lambda(env, ast);
      break;
    }
    TypeEnv *fn_scope_env = *env;

    // Create the function type
    Type *fn_type =
        next_tvar(); // Start with a type variable for the whole function
    Type *current = fn_type;

    for (size_t i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      Ast *param_ast = ast->data.AST_LAMBDA.params + i;
      Type *param_type = type_of_var(param_ast);
      // Add parameter to the environment
      fn_scope_env = add_var_to_env(fn_scope_env, param_type, param_ast);
      Type *next = (i == ast->data.AST_LAMBDA.len - 1)
                       ? next_tvar()
                       : create_type_fn(NULL, NULL);
      current->kind = T_FN;
      current->data.T_FN.from = param_type;
      current->data.T_FN.to = next;
      current = next;
    }

    // If the lambda has a name, add it to the environment for recursion
    if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
      fn_scope_env =
          env_extend(fn_scope_env, ast->data.AST_LAMBDA.fn_name.chars, fn_type);
    }

    // Infer the type of the body
    Type *body_type = infer(&fn_scope_env, ast->data.AST_LAMBDA.body);

    // Unify the return type with the body type
    unify(current, body_type);

    type = fn_type;

    break;
  }
  case AST_APPLICATION: {
    Type *fn_type =
        deep_copy_type(infer(env, ast->data.AST_APPLICATION.function));
    if (!fn_type) {
      return NULL;
    }

    Type *result_type = next_tvar();
    Type *expected_fn_type = result_type;

    Type *fn_return = fn_type;

    for (int i = ast->data.AST_APPLICATION.len - 1; i >= 0; i--) {
      expected_fn_type = create_type_fn(
          infer(env, ast->data.AST_APPLICATION.args + i), expected_fn_type);

      fn_return = fn_return->data.T_FN.to;
    }

    unify(result_type, fn_return);
    unify(fn_type, expected_fn_type);

    Type *res = fn_type;

    for (int i = ast->data.AST_APPLICATION.len - 1; i >= 0; i--) {
      res = res->data.T_FN.to;
    }
    type = deep_copy_type(res);
    free_type(fn_type);
    break;
  }
  case AST_MATCH: {
    Ast *branches = ast->data.AST_MATCH.branches;
    Ast *expr = ast->data.AST_MATCH.expr;
    Type *expr_type = infer(env, expr);

    Type *test_type;
    Type *body_type = NULL;

    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      Ast *test = branches;
      if (!ast_is_placeholder_id(test)) {
        test_type = infer(env, test);
        unify(expr_type, test_type);
      }

      Ast *body = branches + 1;
      Type *btype = infer(env, body);
      if (body_type != NULL) {
        unify(btype, body_type);
      }

      body_type = btype;

      branches += 2;
    }
    type = body_type;
    break;
  }

  case AST_TUPLE: {
    int arity = ast->data.AST_LIST.len;

    Type **cons_args = malloc(sizeof(Type) * arity);
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      cons_args[i] = infer(env, member);
    }
    type = tcons("Tuple", cons_args, arity);
    break;
  }

  case AST_LIST: {
    Type *list_type = infer(env, ast->data.AST_LIST.items);

    int len = ast->data.AST_LIST.len;
    Type **cons_args = malloc(sizeof(Type));
    for (int i = 1; i < len; i++) {
      Ast *list_member = ast->data.AST_LIST.items + i;
      Type *member_type = infer(env, list_member);
      unify(member_type, list_type);
    }
    cons_args[0] = list_type;
    type = tcons("List", cons_args, 1);
    break;
  }
  }

  ast->md = type;
  return type;
}

Type *infer_ast(TypeEnv **env, Ast *ast) {
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
