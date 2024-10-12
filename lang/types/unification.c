#include "types/unification.h"
#include "types/type.h"
#include <stdbool.h>
#include <string.h>

// Forward declaration
bool occurs_check_helper(const char *var, Type *type);

bool occurs_check(Type *var, Type *type) {
  if (var->kind != T_VAR) {
    return false; // Not a type variable, so no occurrence possible
  }
  return occurs_check_helper(var->data.T_VAR, type);
}

bool occurs_check_helper(const char *var_name, Type *type) {
  switch (type->kind) {
  case T_VAR:
    return strcmp(var_name, type->data.T_VAR) == 0;

  case T_FN:
    return occurs_check_helper(var_name, type->data.T_FN.from) ||
           occurs_check_helper(var_name, type->data.T_FN.to);

  case T_CONS: {
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (occurs_check_helper(var_name, type->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  }
  case T_TYPECLASS_RESOLVE: {
    return occurs_check_helper(
               var_name, type->data.T_TYPECLASS_RESOLVE.dependencies[0]) ||
           occurs_check_helper(var_name,
                               type->data.T_TYPECLASS_RESOLVE.dependencies[1]);
  }

  default:
    // For atomic types (T_INT, T_BOOL, etc.), no occurrence is possible
    return false;
  }
}

Type *unify_variable(Type *t1, Type *t2, TypeEnv **env) {

  if (t1->kind == T_VAR && t2->kind == T_VAR &&
      strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0) {
    // TODO: merge typeclasses?
    return t2;
  }

  if (t1->kind == T_VAR && t2->kind == T_VOID) {
    *env = env_extend(*env, t1, &t_void);
    *t1 = t_void;
    return t1;
  }

  if (t1->kind == T_VAR && t2->kind == T_VAR && t1->num_implements > 0 &&
      t2->num_implements == 0) {
    t2->implements = t1->implements;
    t2->num_implements = t1->num_implements;
  }

  if (occurs_check(t1, t2)) {
    if (t2->kind == T_TYPECLASS_RESOLVE &&
        types_equal(t2->data.T_TYPECLASS_RESOLVE.dependencies[0], t1) &&
        (t2->data.T_TYPECLASS_RESOLVE.dependencies[1]->kind == T_VAR)) {

      *env = env_extend(
          *env, t2->data.T_TYPECLASS_RESOLVE.dependencies[1]->data.T_VAR, t1);

      return t1;
    }

    if (t2->kind == T_TYPECLASS_RESOLVE &&
        types_equal(t2->data.T_TYPECLASS_RESOLVE.dependencies[1], t1) &&
        (t2->data.T_TYPECLASS_RESOLVE.dependencies[0]->kind == T_VAR)) {

      *env = env_extend(
          *env, t2->data.T_TYPECLASS_RESOLVE.dependencies[0]->data.T_VAR, t1);
      return t1;
    }

    // Occurs check failed, infinite type error
    return NULL;
  }
  // if (t2->kind == T_TYPECLASS_RESOLVE) {
  //   if (t2->data.T_TYPECLASS_RESOLVE.dependencies[0]->kind == T_VAR) {
  //     *env = env_extend(*env, t1->data.T_VAR,
  //                       t2->data.T_TYPECLASS_RESOLVE.dependencies[1]);
  //     *t1 = *t2->data.T_TYPECLASS_RESOLVE.dependencies[1];
  //   }
  //
  //   if (t2->data.T_TYPECLASS_RESOLVE.dependencies[1]->kind == T_VAR) {
  //     *env = env_extend(*env, t1->data.T_VAR,
  //                       t2->data.T_TYPECLASS_RESOLVE.dependencies[0]);
  //     *t1 = *t2->data.T_TYPECLASS_RESOLVE.dependencies[0];
  //   }
  //   print_type_env(*env);
  //   return t1;
  // }

  // Check if the variable is already bound in the environment
  Type *bound = env_lookup(*env, t1->data.T_VAR);

  if (bound) {
    // If it's bound, unify the bound type with t
    return unify(bound, t2, env);
  }

  // If not bound, bind it in the environment
  *env = env_extend(*env, t1->data.T_VAR, t2);
  *t1 = *t2;
  return t2;
}

Type *unify_function(Type *t1, Type *t2, TypeEnv **env) {

  Type *from = unify(t1->data.T_FN.from, t2->data.T_FN.from, env);
  if (!from)
    return NULL;

  Type *to = unify(t1->data.T_FN.to, t2->data.T_FN.to, env);
  if (!to)
    return NULL;

  return type_fn(from, to);
}

Type *unify_cons(Type *t1, Type *t2, TypeEnv **env) {

  if (is_pointer_type(t1) && !(types_equal(t1, t2))) {
    t1->data.T_CONS.args[0] = t2;
    return t1;
  }

  if (is_array_type(t1) && is_array_type(t2)) {
    Type *unif = unify(t1->data.T_CONS.args[0], t2->data.T_CONS.args[0], env);
    t1->data.T_CONS.args[0] = unif;
    return t1;
  }

  // if (is_array_type(t1) && is_string_type(t2)) {
  //   // Type *unif = unify(t1->data.T_CONS.args[0], t2->data.T_CONS.args[0],
  //   // env); t1->data.T_CONS.args[0] = unif;
  //   *t1->data.T_CONS.args[0] = t_char;
  //   return t1;
  // }

  if (is_tuple_type(t1) && is_tuple_type(t2) &&
      t1->data.T_CONS.num_args == t2->data.T_CONS.num_args) {
    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      Type *unif = unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i], env);

      if (unif) {
        t1->data.T_CONS.args[i] = unif;
      }
    }
    return t1;
  }

  int vidx1;
  Type *v1 = is_variant_type(t1) ? t1 : variant_lookup(*env, t1, &vidx1);

  int vidx2;
  Type *v2 = types_equal(v1, t2) ? t2 : variant_lookup(*env, t2, &vidx2);
  if (v1 && v2 && types_equal(v1, v2)) {
    if (!is_generic(v1)) {
      *t1 = *v1;
      return t1;
    }
    v1 = copy_type(v1);

    TypeEnv *_env = NULL;

    Type *ret = v1;
    Type *type = t1;

    for (int i = 0; i < ret->data.T_CONS.num_args; i++) {
      Type *gen_mem = ret->data.T_CONS.args[i];

      if (strcmp(gen_mem->data.T_CONS.name, type->data.T_CONS.name) == 0) {
        for (int j = 0; j < gen_mem->data.T_CONS.num_args; j++) {
          Type *t = gen_mem->data.T_CONS.args[i];
          Type *v = type->data.T_CONS.args[i];
          if (t->kind == T_VAR) {
            _env = env_extend(_env, t->data.T_VAR, v);
          }
        }
      }
    }
    v1 = resolve_generic_type(v1, _env);

    type = t2;

    for (int i = 0; i < ret->data.T_CONS.num_args; i++) {
      Type *gen_mem = ret->data.T_CONS.args[i];

      if (strcmp(gen_mem->data.T_CONS.name, type->data.T_CONS.name) == 0) {
        for (int j = 0; j < gen_mem->data.T_CONS.num_args; j++) {
          Type *t = gen_mem->data.T_CONS.args[i];
          Type *v = type->data.T_CONS.args[i];
          if (t->kind == T_VAR) {
            _env = env_extend(_env, t->data.T_VAR, v);
          }
        }
      }
    }
    v1 = resolve_generic_type(v1, _env);

    return v1;
  }

  if (t1->kind == T_CONS && t2->kind != T_CONS) {
    return t2;
  }

  if (is_variant_type(t1)) {
    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      if (strcmp(t1->data.T_CONS.args[i]->data.T_CONS.name,
                 t2->data.T_CONS.name) == 0) {
        Type *t = t1->data.T_CONS.args[i];

        return unify(t, t2, env);

        // for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        //   t->data.T_CONS.args[i] =
        //       unify(t->data.T_CONS.args[i], t2->data.T_CONS.args[i], env);
        // }
        // *t1 = *unify(t1->data.T_CONS.args[i], t2, env);
        break;
      }
    }
  }

  if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0 ||
      t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
    print_type_err(t1);
    print_type_err(t2);

    printf("return NULL %s:%d\n", __FILE__, __LINE__);
    return NULL;
  }

  if (t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {

    printf("retur NULL %s:%d\n", __FILE__, __LINE__);
    return NULL;
  }

  int len = t1->data.T_CONS.num_args;

  Type **unified_args = talloc(sizeof(Type *) * len);
  for (int i = 0; i < t1->data.T_CONS.num_args; i++) {

    Type *unif = unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i], env);

    if (!unif) {

      printf("retur NULL %s:%d\n", __FILE__, __LINE__);
      return NULL;
    }
    unified_args[i] = unif;
  }

  return create_cons_type(t1->data.T_CONS.name, t1->data.T_CONS.num_args,
                          unified_args);
}

Type *unify_typeclass_resolve(Type *t1, Type *t2, TypeEnv **env) {
  Type *dep1 = unify(t1->data.T_TYPECLASS_RESOLVE.dependencies[0],
                     t2->data.T_TYPECLASS_RESOLVE.dependencies[0], env);
  if (!dep1)
    return NULL;

  Type *dep2 = unify(t1->data.T_TYPECLASS_RESOLVE.dependencies[1],
                     t2->data.T_TYPECLASS_RESOLVE.dependencies[1], env);
  if (!dep2)
    return NULL;

  return create_typeclass_resolve_type(
      t1->data.T_TYPECLASS_RESOLVE.comparison_tc, dep1, dep2);
}

Type *unify(Type *t1, Type *t2, TypeEnv **env) {
  // printf("unify: ");
  // print_type(t1);
  // print_type(t2);

  if (t1 == NULL) {
    printf(stderr, "Error unifying type - lhs is NULL\n");
    return NULL;
  }
  if (t1->kind == T_VAR) {
    return unify_variable(t1, t2, env);
  }
  if (t2->kind == T_VAR) {
    return unify_variable(t2, t1, env);
  }
  if (t1->kind == T_VOID) {
    return t1;
  }

  if (t1->kind != t2->kind) {
    if (t2->kind == T_TYPECLASS_RESOLVE &&
        types_equal(t2->data.T_TYPECLASS_RESOLVE.dependencies[0],
                    t2->data.T_TYPECLASS_RESOLVE.dependencies[1])) {
      *t2 = *t2->data.T_TYPECLASS_RESOLVE.dependencies[0];
      return unify(t2, t1, env);
    }

    if (t1->kind == T_TYPECLASS_RESOLVE &&
        types_equal(t1->data.T_TYPECLASS_RESOLVE.dependencies[0],
                    t1->data.T_TYPECLASS_RESOLVE.dependencies[1])) {
      *t1 = *t1->data.T_TYPECLASS_RESOLVE.dependencies[0];
      return unify(t1, t2, env);
    }

    if (t1->kind == T_TYPECLASS_RESOLVE &&
        types_equal(t2, t1->data.T_TYPECLASS_RESOLVE.dependencies[0])) {

      return unify(t1->data.T_TYPECLASS_RESOLVE.dependencies[1], t2, env);
    }

    if (t1->kind == T_TYPECLASS_RESOLVE &&
        types_equal(t2, t1->data.T_TYPECLASS_RESOLVE.dependencies[1])) {

      return unify(t1->data.T_TYPECLASS_RESOLVE.dependencies[0], t2, env);
    }

    if (t2->kind == T_TYPECLASS_RESOLVE &&
        types_equal(t1, t2->data.T_TYPECLASS_RESOLVE.dependencies[0])) {

      return unify(t2->data.T_TYPECLASS_RESOLVE.dependencies[1], t1, env);
    }

    if (t2->kind == T_TYPECLASS_RESOLVE &&
        types_equal(t1, t2->data.T_TYPECLASS_RESOLVE.dependencies[1])) {

      return unify(t2->data.T_TYPECLASS_RESOLVE.dependencies[0], t1, env);
    }

    return NULL;
  }

  int vidx;
  if (is_variant_type(t1) && variant_contains_type(t1, t2, &vidx)) {
    return t1;
  }

  switch (t1->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  // case T_VOID:
  case T_STRING:
    // These are atomic types, so they unify if they're the same kind
    return t1;

  case T_FN:
    // Unify function types
    return unify_function(t1, t2, env);

  case T_CONS:

    // Unify constructed types
    if (types_equal(t1, t2)) {
      return t1;
    }
    return unify_cons(t1, t2, env);

  case T_TYPECLASS_RESOLVE: {
    if (types_equal(t1->data.T_TYPECLASS_RESOLVE.dependencies[0], t2) ||
        types_equal(t1->data.T_TYPECLASS_RESOLVE.dependencies[1], t2)) {
      unify(t1->data.T_TYPECLASS_RESOLVE.dependencies[0], t2, env);
      unify(t1->data.T_TYPECLASS_RESOLVE.dependencies[1], t2, env);
      *t1 = *resolve_tc_rank(t1);

      return t1;
    }

    return unify_typeclass_resolve(t1, t2, env);
  }

  default:

    // Unsupported type error
    return NULL;
  }

  return NULL;
}

Type *variable_tc_resolve(Type *a, Type *b, TypeEnv **env) {
  Type *replacement;
  if (types_equal(a, b->data.T_TYPECLASS_RESOLVE.dependencies[0])) {
    replacement = b->data.T_TYPECLASS_RESOLVE.dependencies[1];
  } else {
    replacement = b->data.T_TYPECLASS_RESOLVE.dependencies[0];
  }
  *env = env_extend(*env, a->data.T_VAR, replacement);
  *a = *replacement;
  return a;
}

void unify_rec_fn_mem(Type *a, Type *b, TypeEnv **env) {
  if (a->kind == T_VOID && b->kind == T_VOID) {
    return;
  }

  if (a->kind == T_VAR && b->kind == T_TYPECLASS_RESOLVE &&
      occurs_check(a, b)) {
    Type *replacement;
    if (types_equal(a, b->data.T_TYPECLASS_RESOLVE.dependencies[0])) {
      replacement = b->data.T_TYPECLASS_RESOLVE.dependencies[1];
    } else {
      replacement = b->data.T_TYPECLASS_RESOLVE.dependencies[0];
    }
    *env = env_extend(*env, a->data.T_VAR, replacement);
    *a = *replacement;
  } else {
    Type *replacement = env_lookup(*env, a->data.T_VAR);
    if (replacement) {
      *a = *replacement;
    }
  }

  // unify(a, b, env);

  // printf("recursive resolve: ");
  // print_type(a);
  // print_type(b);
}

void unify_recursive_defs_mut(Type *fn, Type *rec_ref, TypeEnv **env) {
  Type *a;
  Type *b;
  if (fn->kind == T_FN) {
    a = fn->data.T_FN.from;
    b = rec_ref->data.T_FN.from;
    unify_rec_fn_mem(a, b, env);

    if (fn->data.T_FN.to) {
      unify_recursive_defs_mut(fn->data.T_FN.to, rec_ref->data.T_FN.to, env);
    }
  } else {
    a = fn;
    b = rec_ref;
    unify_rec_fn_mem(a, b, env);
  }
}
