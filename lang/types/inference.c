#include "types/inference.h"
#include "common.h"
#include "format_utils.h"
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

typedef struct TypeMap {
  Type *key;
  Type *type;
  struct TypeMap *next;
} TypeMap;

TypeMap *map_extend(TypeMap *env, Type *key, Type *type) {
  TypeMap *new_env = malloc(sizeof(TypeMap));
  new_env->key = key;
  new_env->type = type;
  new_env->next = env;
  return new_env;
}

Type *map_lookup(TypeMap *env, Type *key) {
  while (env) {
    if (types_equal(env->key, key)) {
      return env->type;
    }
    env = env->next;
  }
  return NULL;
}

Type *resolve_in_map(TypeMap *constraint_map, Type *key) {
  if (key->kind == T_CONS) {
    return key;
  }
  if (key->kind == T_FN) {
    Type *t = key;
    while (t->kind == T_FN) {
      Type *from = t->data.T_FN.from;
      t = t->data.T_FN.to;

      if (from->kind == T_VAR) {
        Type *resolved_from = resolve_in_map(constraint_map, from);
        from->kind = resolved_from->kind;
        from->data = resolved_from->data;
      }

      if (t->kind == T_VAR) {
        Type *resolved_from = resolve_in_map(constraint_map, t);

        t->kind = resolved_from->kind;
        t->data = resolved_from->data;
      }
    }
    return key;
  }

  while (constraint_map) {
    if (types_equal(constraint_map->key, key)) {
      Type *t = constraint_map->type;
      if (is_generic(t)) {
        return resolve_in_map(constraint_map->next, t);
      } else {
        return t;
      }
    }
    constraint_map = constraint_map->next;
  }
}

void map_print(TypeMap *env) {
  while (env) {
    print_type(env->key);
    printf(" : ");
    print_type(env->type);
    printf("\n");
    env = env->next;
  }
}

void free_type_map(TypeMap *env) {
  if (env->next) {
    free_type_map(env->next);
    free(env);
  }
}

static TypeEnv *create_implicit_var_bindings(TypeEnv *env, Ast *expr) {
  if (is_placeholder(expr)) {
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
      if (!is_placeholder(test_expr)) {
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

TypeMap *add_constraints(TypeMap *constraints_map, Type *arg_type, Type *t) {

  if (arg_type->kind == T_CONS && t->kind == T_CONS &&
      (strcmp(arg_type->data.T_CONS.name, t->data.T_CONS.name) == 0) &&
      (arg_type->data.T_CONS.num_args == t->data.T_CONS.num_args)) {

    for (int i = 0; i < arg_type->data.T_CONS.num_args; i++) {
      constraints_map = add_constraints(constraints_map, t->data.T_CONS.args[i],
                                        arg_type->data.T_CONS.args[i]);
    }
    return constraints_map;

  } else if (arg_type->kind == T_FN && t->kind == T_FN) {
    Type *a = arg_type;
    Type *_t = t;

    constraints_map =
        map_extend(constraints_map, a->data.T_FN.from, _t->data.T_FN.from);
    return add_constraints(constraints_map, a->data.T_FN.to, _t->data.T_FN.to);
  } else {
    constraints_map = map_extend(constraints_map, t, arg_type);
    return constraints_map;
  }
}

static Type *infer_fn_application(TypeEnv **env, Ast *ast) {

  Type *fn_type = infer(env, ast->data.AST_APPLICATION.function);

  if (!fn_type) {
    return NULL;
  }

  if (fn_type->kind == T_VAR) {
    Type *result_type = next_tvar();
    Type *expected_fn_type = result_type;
    Type *fn_return;

    for (int i = ast->data.AST_APPLICATION.len - 1; i >= 0; i--) {

      expected_fn_type = create_type_fn(
          infer(env, ast->data.AST_APPLICATION.args + i), expected_fn_type);
      result_type = expected_fn_type->data.T_FN.to;
    }

    fn_return = result_type->data.T_FN.to;

    TypeEnv *_env = NULL;

    _unify(fn_type, expected_fn_type, &_env);

    Type *res = fn_type;

    for (int i = ast->data.AST_APPLICATION.len - 1; i >= 0; i--) {
      res = res->data.T_FN.to;
    }

    return resolve_in_env(res, _env);
  }

  Type *_fn_type = fn_type;
  if (fn_type->kind == T_FN) {

    fn_type = deep_copy_type(fn_type);
  };

  Type *result_type = next_tvar();

  Type *fn_return = fn_type;

  Type *arg_types[ast->data.AST_APPLICATION.len];

  TypeMap *constraints_map = NULL;

  int len = ast->data.AST_APPLICATION.len;
  bool contains_generic_fn_arg = false;
  for (int i = 0; i < len; i++) {
    Type *arg_type = infer(env, ast->data.AST_APPLICATION.args + i);
    arg_types[i] = arg_type;

    if (arg_type->kind == T_FN && is_generic(arg_type)) {
      contains_generic_fn_arg = true;
    }

    Type *t = fn_return->data.T_FN.from;
    constraints_map = add_constraints(constraints_map, arg_type, t);

    if (fn_return->data.T_FN.to == NULL) {
      fprintf(stderr, "Error: too many parameters to function\n");
      return NULL;
    }

    Type *next = (i == len - 1) ? result_type : create_type_fn(NULL, NULL);

    fn_return = fn_return->data.T_FN.to;
  }
  Type *ret_type;
  Type *exp = create_type_multi_param_fn(len, arg_types, result_type);
  if (contains_generic_fn_arg) {

    Type *res = fn_type;
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      Type *resolved = resolve_in_map(constraints_map, res->data.T_FN.from);
      res->data.T_FN.from = resolved;
      ast->data.AST_APPLICATION.args[i].md = resolved;
      res = res->data.T_FN.to;
    }

    *res = *resolve_in_map(constraints_map, res);
    ret_type = res;
    ast->data.AST_APPLICATION.function->md = fn_type;
  } else {
    TypeEnv *_env = NULL;
    _unify(result_type, fn_return, &_env);
    _unify(fn_type, exp, &_env);

    Type *res = fn_type;
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      res = res->data.T_FN.to;
    }
    ret_type = resolve_in_env(res, _env);
    free_type_env(_env);
  }

  free_type_map(constraints_map);
  return ret_type;
}

Type *cast_char_list(Type *t) {
  if (is_list_type(t) && t->data.T_CONS.args[0]->kind == T_CHAR) {
    return &t_string;
  }
  return t;
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
      if (is_arithmetic(lt) && is_arithmetic(rt)) {
        type = max_numeric_type(lt, rt);
        break;
      } else {
        unify(lt, rt);
        type = lt;

        fprintf(stderr,
                STYLE_DIM "Not implemented warning: type coercion for "
                          "non-arithmetic (eg T_VAR) "
                          "type to Arithmetic typeclass\n" STYLE_RESET_ALL);
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
  case AST_NUMBER:
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
