#include "types/inference.h"
#include "common.h"
#include "format_utils.h"
#include "serde.h"
#include "types/fn_application.h"
#include "types/type.h"
#include "types/type_declaration.h"
#include "types/util.h"
#include <stdlib.h>
#include <string.h>

// forward decl
Type *infer(TypeEnv **env, Ast *ast);

// Global variables
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }
const char *fresh_tvar_name() {
  char *new_name = malloc(5 * sizeof(char));
  if (new_name == NULL) {
    return NULL;
  }
  sprintf(new_name, "t%d", type_var_counter);
  type_var_counter++;
  return new_name;
}

Type *next_tvar() { return create_type_var(fresh_tvar_name()); }
static bool is_ord(Type *t) { return (t->kind >= T_INT) && (t->kind <= T_NUM); }

static Type *max_numeric_type(Type *lt, Type *rt) {
  if (lt->kind >= rt->kind) {
    return lt;
  }
  return rt;
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

  case AST_LIST: {
    int len = param_ast->data.AST_LIST.len;
    Ast *arg_asts = param_ast->data.AST_LIST.items;
    TypeEnv *new_env = env;
    for (int i = 0; i < len; i++) {
      Type *arg_type_var = param_type->data.T_CONS.args[i];
      new_env = add_var_to_env(new_env, arg_type_var, arg_asts + i);
    }
    return new_env;
  }

  case AST_BINOP: {
    if (param_ast->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {
      TypeEnv *new_env = env;
      new_env = add_var_to_env(new_env, param_type->data.T_CONS.args[0],
                               param_ast->data.AST_BINOP.left);

      new_env =
          add_var_to_env(new_env, param_type, param_ast->data.AST_BINOP.right);
      return new_env;
    }
  }

  default: {
    return env;
  }
  }
}

static TypeEnv *create_implicit_var_bindings(TypeEnv *env, Ast *expr) {
  if (ast_is_placeholder_id(expr)) {
    return env;
  }

  if (expr->tag == AST_IDENTIFIER) {
    const char *id = expr->data.AST_IDENTIFIER.value;
    if (!env_lookup(env, id)) {
      Type *t = next_tvar();
      expr->md = t;
      return env_extend(env, id, t);
    }
    return env;
  }

  if (expr->tag == AST_TUPLE) {
    for (int i = 0; i < expr->data.AST_LIST.len; i++) {
      env = create_implicit_var_bindings(env, expr->data.AST_LIST.items + i);
    }
    return env;
  }

  if (expr->tag == AST_LIST) {
    for (int i = 0; i < expr->data.AST_LIST.len; i++) {
      env = create_implicit_var_bindings(env, expr->data.AST_LIST.items + i);
    }
    return env;
  }
  if (expr->tag == AST_BINOP && expr->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {
    env = create_implicit_var_bindings(env, expr->data.AST_BINOP.left);
    env = create_implicit_var_bindings(env, expr->data.AST_BINOP.right);
    return env;
  }

  return env;
}

static Type *infer_match_expr(TypeEnv **env, Ast *ast) {

  Ast *expr = ast->data.AST_MATCH.expr;
  Type *expr_type = infer(env, expr);

  Type *test_type;
  Type *final_type = NULL;
  int len = ast->data.AST_MATCH.len;

  Ast *branches = ast->data.AST_MATCH.branches;
  for (int i = 0; i < len; i++) {
    Ast *test_expr = branches + (2 * i);
    Ast *result_expr = branches + (2 * i + 1);

    *env = create_implicit_var_bindings(*env, test_expr);
    if (i == len - 1) {
      if (!ast_is_placeholder_id(test_expr)) {
        test_type = infer(env, test_expr);
        unify(expr_type, test_type);
      }
    } else {
      test_type = infer(env, test_expr);

      unify(expr_type, test_type);
    }
    result_expr->md = test_type;
    Type *res_type = infer(env, result_expr);

    if (final_type != NULL) {
      unify(res_type, final_type);
    }

    unify(expr_type, test_type);

    final_type = res_type;
  }
  return final_type;
}

Type *cast_char_list(Type *t) {
  if (is_list_type(t) && t->data.T_CONS.args[0]->kind == T_CHAR) {
    return &t_string;
  }
  return t;
}

// Function to determine the resulting type of an arithmetic operation
Type *arithmetic_result_type(Type *lt, Type *rt) {
  if (is_arithmetic(lt) && is_arithmetic(rt)) {
    Type *type = max_numeric_type(lt, rt);
    return type;
  } else {
    unify(lt, rt);

    if (implements_typeclass(lt, &TCNum) && implements_typeclass(rt, &TCNum)) {
      // TODO: this is not quite correct logic for upcasting
      if ((lt->kind != T_NUM) && (lt->kind != T_INT)) {
        return lt;
      }

      if ((rt->kind != T_NUM) && (rt->kind != T_INT)) {
        return rt;
      }
    }

    return lt;
  }

  return NULL;
}

// Main type inference function
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
    if (ast->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {

      Type *list_el_type = next_tvar();
      Type *list_type = create_list_type(list_el_type);
      unify(rt, list_type);
      unify(lt, list_el_type);
      type = list_type;
      break;
    }
    if ((ast->data.AST_BINOP.op >= TOKEN_PLUS) &&
        (ast->data.AST_BINOP.op <= TOKEN_MODULO)) {
      type = arithmetic_result_type(lt, rt);
      break;
    }
    if ((ast->data.AST_BINOP.op >= TOKEN_LT) &&
        (ast->data.AST_BINOP.op <= TOKEN_NOT_EQUAL)) {

      if (is_arithmetic(lt) && is_arithmetic(rt)) {
        type = &t_bool;
        break;
      } else {
        unify(lt, rt);
        type = lt;

        fprintf(stderr,
                STYLE_DIM "Not implemented warning: type coercion for non-ord "
                          "type to Ord typeclass\n" STYLE_RESET_ALL);
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
  case AST_DOUBLE:
    type = &t_num;
    break;
  case AST_STRING:
    type = &t_string;
    break;
  case AST_CHAR:
    type = &t_char;
    break;
  case AST_BOOL:
    type = &t_bool;
    break;

  case AST_VOID: {
    type = &t_void;
    break;
  }

  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(ast)) {
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

    Ast *binding = ast->data.AST_LET.binding;
    *env = add_var_to_env(*env, expr_type, binding);
    infer(env, binding);

    if (ast->data.AST_LET.in_expr) {
      type = infer(env, ast->data.AST_LET.in_expr);
    } else {
      type = expr_type;
    }
    break;
  }

  case AST_EXTERN_FN: {
    int param_count = ast->data.AST_EXTERN_FN.len - 1;
    int real_param_count = param_count || 1;
    // printf("param count %d\n", real_param_count);

    Type **param_types = malloc(sizeof(Type *) * real_param_count);
    if (param_count == 0) {
      param_types[0] = &t_void;
    } else {
      for (int i = 0; i < real_param_count; i++) {
        Ast *param_ast = ast->data.AST_EXTERN_FN.signature_types + i;

        Type *param_type = get_type(*env, param_ast);

        if (!param_type) {
          fprintf(stderr, "Error declaring extern function: type %s not found",
                  param_ast->data.AST_IDENTIFIER.value);
        }
        param_types[i] = param_type;
      }
    }

    Ast *ret_type_ast = ast->data.AST_EXTERN_FN.signature_types + param_count;
    Type *ret_type = get_type(*env, ret_type_ast);

    // Type *ex_t = create_type_fn(param_types[param_count - 1], ret_type);
    //
    // for (int i = param_count - 2; i >= 0; i--) {
    //   ex_t = create_type_fn(param_types[i], ex_t);
    // }
    // ex_t->data.T_FN.from = param_types[0];

    // type = ex_t;
    type = create_type_multi_param_fn(real_param_count, param_types, ret_type);
    printf("extern fn type: ");
    print_ast(ast);
    print_type(type);
    printf("\n");
    break;
  }

  case AST_EXTERN_VARIANTS: {

    Ast *extern_variant_ast = ast->data.AST_LIST.items;
    infer(env, extern_variant_ast);

    Type *generic_fn = deep_copy_type(extern_variant_ast->md);
    generic_fn->data.T_FN.from = next_tvar();

    size_t fn_len = extern_variant_ast->data.AST_EXTERN_FN.len;

    Ast ret_type =
        extern_variant_ast->data.AST_EXTERN_FN.signature_types[fn_len - 1];

    for (int i = 1; i < ast->data.AST_LIST.len; i++) {
      extern_variant_ast = ast->data.AST_LIST.items + i;
      infer(env, extern_variant_ast);
      size_t this_fn_len = extern_variant_ast->data.AST_EXTERN_FN.len;

      Ast this_ret_type = extern_variant_ast->data.AST_EXTERN_FN
                              .signature_types[this_fn_len - 1];

      if (this_fn_len != fn_len) {
        fprintf(stderr, "Error: function variants must all have the same size");
        return NULL;
      }

      if ((this_ret_type.data.AST_IDENTIFIER.length !=
           ret_type.data.AST_IDENTIFIER.length) ||
          (strcmp(this_ret_type.data.AST_IDENTIFIER.value,
                  ret_type.data.AST_IDENTIFIER.value) != 0)) {
        fprintf(stderr,
                "Error: function variants must all return the same type");
        return NULL;
      }
    }

    type = generic_fn;
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

    // Type *t = fn_type;
    // while (t->kind == T_FN) {
    //   print_type_w_tc(t->data.T_FN.from);
    //   printf("->");
    //
    //   t = t->data.T_FN.to;
    // }

    break;
  }
  case AST_APPLICATION: {
    type = infer_fn_application(env, ast);
    break;
  }
  case AST_MATCH: {
    type = infer_match_expr(env, ast);
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

  case AST_FMT_STRING: {
    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      infer(env, member);
    }
    type = &t_string;
    break;
  }

  case AST_LIST: {
    Type *list_type = infer(env, ast->data.AST_LIST.items);

    int len = ast->data.AST_LIST.len;
    Type **cons_args = malloc(sizeof(Type));
    if (len == 0) {
      cons_args[0] = next_tvar();
      type = tcons("List", cons_args, 1);
      break;
    }
    for (int i = 1; i < len; i++) {
      Ast *list_member = ast->data.AST_LIST.items + i;
      Type *member_type = infer(env, list_member);
      unify(member_type, list_type);
    }
    cons_args[0] = list_type;
    type = tcons("List", cons_args, 1);
    break;
  }
  case AST_IMPORT: {
    type = next_tvar();
    break;
  }
  case AST_RECORD_ACCESS: {
    Ast *record = ast->data.AST_RECORD_ACCESS.record;
    infer(env, record);
    Ast *member = ast->data.AST_RECORD_ACCESS.member;
    if (member->tag != AST_IDENTIFIER) {
      return NULL;
    }
    member->md = env_lookup(((Type *)record->md)->data.T_MODULE,
                            member->data.AST_IDENTIFIER.value);
    type = member->md;
    break;
  }
  case AST_TYPE_DECL: {
    type_declaration(ast, env);
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
