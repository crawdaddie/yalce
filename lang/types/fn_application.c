#include "fn_application.h"
#include "types/util.h"
#include <string.h>

Type *next_tvar();
Type *infer(TypeEnv **env, Ast *ast);
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
static TypeMap *add_constraints(TypeMap *constraints_map, Type *arg_type,
                                Type *t) {

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

Type *infer_fn_application(TypeEnv **env, Ast *ast) {
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
