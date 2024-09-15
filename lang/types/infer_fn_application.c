#include "types/infer_fn_application.h"

#include "inference.h"
#include "serde.h"
#include "types/unification.h"

typedef struct TypeMap {
  Type *key;
  Type *val;
  struct TypeMap *next;
} TypeMap;

TypeMap *constraints_map_extend(TypeMap *map, Type *key, Type *val) {
  switch (key->kind) {
  case T_VAR: {
    TypeMap *new_map = talloc(sizeof(TypeMap));
    new_map->key = key;
    new_map->val = val;
    new_map->next = map;
    return new_map;
  }
  }
  return map;
}
void print_constraints_map(TypeMap *map) {
  if (map) {
    print_type(map->key);
    printf(" : ");
    print_type(map->val);
    print_constraints_map(map->next);
  }
}

Type *constraints_map_lookup(TypeMap *map, Type *key) {
  while (map) {
    if (types_equal(map->key, key)) {
      return map->val;
    }

    if (occurs_check(map->key, key)) {
      return replace_in(key, map->key, map->val);
    }

    map = map->next;
  }
  return NULL;
}

Type *__infer_fn_application(Ast *ast, Type **arg_types, int len,
                             TypeEnv **env) {

  Type *fn_type = ast->data.AST_APPLICATION.function->md;

  fn_type = copy_type(fn_type);

  if (fn_type->kind != T_FN) {
    fprintf(stderr, "Error: Attempting to apply a non-function type ");
    print_type_err(fn_type);
    return NULL;
  }

  Type *a = fn_type;
  TypeMap *map = NULL;
  // TypeEnv *map = NULL;

  Type **app_args = talloc(sizeof(Type *) * len);

  // print_type(fn_type);
  // printf("application args: \n");
  for (int i = 0; i < len; i++) {
    Type *app_arg_type = arg_types[i];
    app_args[i] = app_arg_type;

    Type *fn_arg_type = a->data.T_FN.from;

    // printf("%d: ", i);
    // print_type(app_arg_type);
    // print_type(fn_arg_type);

    Type *unif = unify(app_arg_type, fn_arg_type, env);

    map = constraints_map_extend(map, fn_arg_type, app_arg_type);

    if (a->kind != T_FN) {
      fprintf(stderr, "Error too may args (%d) passed to fn\n", len);
      print_type_err(fn_type);
      return NULL;
    }

    a = a->data.T_FN.to;
  }

  Type *app_result_type = a;

  if (app_result_type->kind == T_FN) {
    TypeMap *_map = map;
    while (_map) {
      app_result_type = replace_in(app_result_type, _map->key, _map->val);
      _map = _map->next;
    }
    return app_result_type;
  }

  if (is_generic(app_result_type)) {

    Type *lookup = constraints_map_lookup(map, app_result_type);

    // lookup = resolve_generic_variant(lookup, *env);

    if (lookup == NULL) {
      Type *res = resolve_generic_type(app_result_type, *env);
      // fprintf(stderr, "Error: constraint not found in constraint map\n");
      // print_type(app_result_type);
      // printf("\n");
      Type *specific_fn = create_type_multi_param_fn(len, app_args, res);
      // print_type(specific_fn);
      // print_type(res);
      // printf("specific_fn: ");
      // print_type(specific_fn);
      // printf("\n");
      // *fn_type = *specific_fn;
      ast->data.AST_APPLICATION.function->md = specific_fn;
      return res;
    }

    Type *specific_fn = create_type_multi_param_fn(len, app_args, lookup);
    ast->data.AST_APPLICATION.function->md = specific_fn;
    return lookup;
  }

  return app_result_type;
}

// forward decl
Type *infer(Ast *ast, TypeEnv **env);
Type *infer_fn_application(Ast *ast, TypeEnv **env) {

  int len = ast->data.AST_APPLICATION.len;
  Type *app_arg_types[len];
  for (int i = 0; i < len; i++) {
    app_arg_types[i] = TRY_MSG(infer(ast->data.AST_APPLICATION.args + i, env),
                               "could not infer application argument");
  }

  Type *_fn_type = ast->data.AST_APPLICATION.function->md;

  // Type *fn_type = is_generic(_fn_type) ? _fn_type : copy_type(_fn_type);

  Type *fn_type = copy_type(_fn_type);
  // Type *fn_type = _fn_type;

  TypeEnv *replacement_env = NULL;

  Type *result_fn = fn_type;

  for (int i = 0; i < len; i++) {
    Type *unif =
        unify(result_fn->data.T_FN.from, app_arg_types[i], &replacement_env);

    if (!unif) {
      return NULL;
    }

    result_fn = result_fn->data.T_FN.to;
  }

  result_fn = resolve_generic_type(result_fn, replacement_env);

  ast->data.AST_APPLICATION.function->md =
      resolve_generic_type(fn_type, replacement_env);

  return result_fn;
}

Type *infer_cons(Ast *ast, TypeEnv **env) {
  Type *cons = copy_type(ast->data.AST_APPLICATION.function->md);

  TypeEnv *replacement_env = NULL;
  int len = ast->data.AST_APPLICATION.len;

  for (int i = 0; i < len; i++) {
    Type *arg_type =
        TRY_MSG(infer(ast->data.AST_APPLICATION.args + i, &replacement_env),
                "could not infer cons argument");
    unify(cons->data.T_CONS.args[i], arg_type, &replacement_env);
  }

  return cons;
}

Type *infer_unknown_fn_signature(Ast *ast, TypeEnv **env) {

  Type *res_type = next_tvar();

  Type *fn_type = res_type;
  int len = ast->data.AST_APPLICATION.len;

  for (int i = len - 1; i >= 0; i--) {
    Type *arg_type = infer(ast->data.AST_APPLICATION.args + i, env);
    fn_type = type_fn(arg_type, fn_type);
  }
  fn_type->is_recursive_fn_ref = true;

  Type *fn_var = ast->data.AST_APPLICATION.function->md;
  *fn_var = *fn_type;

  // printf("infer unknown %s\n",
  //        ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value);
  // print_ast(ast);
  //
  // print_type(fn_var);
  // *env = env_extend(
  //     *env, ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
  //     fn_var);

  // print_type_env(*env);

  return res_type;
}
