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

Type *unify_variable(Type *var, Type *t, TypeEnv **env) {

  if (var->kind == T_VAR && t->kind == T_VAR &&
      strcmp(var->data.T_VAR, t->data.T_VAR) == 0) {
    // TODO: merge typeclasses?
    return t;
  }
  if (occurs_check(var, t)) {

    // Occurs check failed, infinite type error
    return NULL;
  }

  // Check if the variable is already bound in the environment
  Type *bound = env_lookup(*env, var->data.T_VAR);
  if (bound) {
    // If it's bound, unify the bound type with t
    return unify(bound, t, env);
  }

  // If not bound, bind it in the environment
  *env = env_extend(*env, var->data.T_VAR, t);
  *var = *t;
  return t;
}
Type *create_cons_type(const char *name, int len, Type **unified_args) {
  Type *cons = empty_type();
  cons->kind = T_CONS;
  cons->data.T_CONS.name = name;
  cons->data.T_CONS.num_args = len;
  cons->data.T_CONS.args = unified_args;
  return cons;
}

// Type *unify_function(Type *t1, Type *t2, TypeEnv **env) { return NULL; }
// Type *unify_cons(Type *t1, Type *t2, TypeEnv **env) { return NULL; }
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

  if (strcmp(t1->data.T_CONS.name, TYPE_NAME_VARIANT) == 0) {
    return t1;
  }

  if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0 ||
      t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
    return NULL;
  }

  if (t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
    return NULL;
  }
  int len = t1->data.T_CONS.num_args;

  Type **unified_args = talloc(sizeof(Type *) * len);
  for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
    unified_args[i] =
        unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i], env);
    if (!unified_args[i])
      return NULL;
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

  if (t1->kind == T_VAR) {
    return unify_variable(t1, t2, env);
  }
  if (t2->kind == T_VAR) {
    return unify_variable(t2, t1, env);
  }

  if (t1->kind != t2->kind) {
    return NULL;
  }

  switch (t1->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_STRING:
    // These are atomic types, so they unify if they're the same kind
    return t1;

  case T_FN:
    // Unify function types
    return unify_function(t1, t2, env);

  case T_CONS:
    // Unify constructed types
    return unify_cons(t1, t2, env);

  default:
    // Unsupported type error
    return NULL;
  }

  return NULL;
}
