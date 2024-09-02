#include "types/unification.h"
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

  case T_CONS:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (occurs_check_helper(var_name, type->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;

  case T_VARIANT:

    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (occurs_check_helper(var_name, type->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;

  default:
    // For atomic types (T_INT, T_BOOL, etc.), no occurrence is possible
    return false;
  }
}

Type *unify_variable(Type *var, Type *t, TypeEnv **env) {
  // printf("unify tvars: ");
  // print_type(var);
  // print_type(t);
  // printf("\n");
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
  return t;
}

Type *unify_function(Type *t1, Type *t2, TypeEnv **env) { return NULL; }
Type *unify_cons(Type *t1, Type *t2, TypeEnv **env) { return NULL; }
Type *unify_variant(Type *t1, Type *t2, TypeEnv **env) { return NULL; }

// void unify(Type *l, Type *r) {
//
//   if (types_equal(l, r)) {
//     return;
//   }
//
//   if (l->kind == T_VAR && r->kind != T_VAR) {
//     *l = *r;
//   }
//
//   if (l->kind == T_VAR && r->num_implements > 0) {
//     // l->implements = r->implements;
//     // l->num_implements = r->num_implements;
//     *l = *r;
//   }
//
//   if (l->kind == T_VAR && r->kind == T_VAR) {
//     // l->implements = r->implements;
//     // l->num_implements = r->num_implements;
//     *l = *r;
//   }
//
//   if (l->kind == T_CONS && r->kind == T_CONS) {
//     if (strcmp(l->data.T_CONS.name, r->data.T_CONS.name) != 0 ||
//         l->data.T_CONS.num_args != r->data.T_CONS.num_args) {
//       fprintf(stderr, "Error: Type mismatch between %s and %s\n",
//               l->data.T_CONS.name, r->data.T_CONS.name);
//     }
//
//     for (int i = 0; i < l->data.T_CONS.num_args; i++) {
//       unify(l->data.T_CONS.args[i], r->data.T_CONS.args[i]);
//     }
//
//     return;
//   }
// }
//
Type *unify(Type *t1, Type *t2, TypeEnv **env) {
  if (t1->kind == T_VAR) {
    return unify_variable(t1, t2, env);
  }
  if (t2->kind == T_VAR) {
    return unify_variable(t2, t1, env);
  }

  if (t1->kind != t2->kind) {
    // Type mismatch error
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

  case T_VARIANT:
    // Unify variant types
    return unify_variant(t1, t2, env);

  default:
    // Unsupported type error
    return NULL;
  }

  return NULL;
}
