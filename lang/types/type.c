#include "types/type.h"
#include "types/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DBG_UNIFY
// clang-format off
TypeClass TClassOrd = {"Ord"};
TypeClass TClassNum = {"Num"};
Type t_int =    {T_INT};
// Type t_int =    {T_INT,
//   .type_class = &(InstTypeClass){
//     .class = &TClassNum
//   }
// };

Type t_num =    {T_NUM};
// Type t_num =    {T_NUM,
//   .type_class = &(InstTypeClass){
//     .class = &TClassNum
//   }
// };
Type t_char =   {T_CHAR};
Type t_string = {T_CONS, {.T_CONS = {"List", (Type*[]){&t_char}, 1}}};
Type t_bool =   {T_BOOL};
Type t_void =   {T_VOID};
// clang-format on
//

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type) {
  TypeEnv *new_env = malloc(sizeof(TypeEnv));
  new_env->name = name;
  new_env->type = type;
  new_env->next = env;
  return new_env;
}

Type *env_lookup(TypeEnv *env, const char *name) {
  while (env) {
    if (strcmp(env->name, name) == 0) {
      return env->type;
    }
    env = env->next;
  }
  return NULL;
}

//// Helper function to check if a type variable occurs in a type
bool occurs(const char *var, Type *type) {
  switch (type->kind) {
  case T_VAR:
    return strcmp(type->data.T_VAR, var) == 0;
  case T_FN:
    return occurs(var, type->data.T_FN.from) || occurs(var, type->data.T_FN.to);
  case T_CONS:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (occurs(var, type->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  default:
    return false;
  }
}

// Helper function to substitute a type variable with another type
void substitute(Type *type, const char *var, Type *replacement) {
  switch (type->kind) {
  case T_VAR:
    if (strcmp(type->data.T_VAR, var) == 0) {
      *type = *replacement; // Replace the entire type
    }
    break;
  case T_FN:
    substitute(type->data.T_FN.from, var, replacement);
    substitute(type->data.T_FN.to, var, replacement);
    break;
  case T_CONS:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      substitute(type->data.T_CONS.args[i], var, replacement);
    }
    break;
  default:
    break;
  }
}

Type *create_type_var(const char *name) {
  Type *type = malloc(sizeof(Type));
  type->kind = T_VAR;
  type->data.T_VAR = name;
  return type;
}

Type *tvar(const char *name) { return create_type_var(name); }

Type *tcons(const char *name, Type **_args, int num_args) {
  Type *type = malloc(sizeof(Type));
  type->kind = T_CONS;
  type->data.T_CONS.name = name;
  type->data.T_CONS.args = _args;
  type->data.T_CONS.num_args = num_args;
  return type;
}

Type *create_tuple_type(Type **element_types, int num_elements) {
  Type *tuple_type = malloc(sizeof(Type));
  tuple_type->kind = T_CONS;
  tuple_type->data.T_CONS.name = "Tuple";
  tuple_type->data.T_CONS.args = element_types;
  tuple_type->data.T_CONS.num_args = num_elements;
  tuple_type->type_class = NULL; // Or set appropriately if needed
  return tuple_type;
}

Type *create_list_type(Type *element_type) {
  Type *tuple_type = malloc(sizeof(Type));
  tuple_type->kind = T_CONS;
  tuple_type->data.T_CONS.name = "List";
  tuple_type->data.T_CONS.args = malloc(sizeof(Type *));
  tuple_type->data.T_CONS.args[0] = element_type;
  tuple_type->data.T_CONS.num_args = 1;
  tuple_type->type_class = NULL; // Or set appropriately if needed
  return tuple_type;
}

Type *create_type_fn(Type *from, Type *to) {
  Type *type = malloc(sizeof(Type));
  *type = (Type){T_FN, .data = {.T_FN = {.from = from, .to = to}}};
  return type;
  // Create a new function type
}

Type *create_type_multi_param_fn(int param_count, Type **param_types,
                                 Type *return_type) {

  Type *func_type = return_type;
  for (int i = param_count - 1; i >= 0; i--) {
    func_type = create_type_fn(param_types[i], func_type);
  }
  return func_type;
}

Type *fresh(Type *type) {
  // Create a fresh instance of a type, replacing type variables with new ones
}

// Main unification function
//
// unify(Meta(α), t) =
//
// if α ∈ domain(σm)
// then unify(σm(α),t)
//
// else if t ≡ App(TyFun . . .)
//
//    then unify(Meta(α), expand TyFun type as usual)
//
// else if t ≡ Meta(γ ) and γ ∈ domain(σm )
//
//    then unify(Meta(α), σm (γ ))
//
// else if t ≡ Meta(α)
//     then OK
//
// else if Meta(α) occurs in t
//      then error
//
// else σm ← σm + {α → t}; # extend σm with {α → t}
// OK
//
// unify(t, Meta(α)) = where t is not a Meta
//      unify(Meta(α), t)
//
//
//
void _unify(Type *t1, Type *t2, TypeEnv **env) {
#ifdef DBG_UNIFY
  printf("unify l: ");
  print_type(t1);

  printf(" r: ");
  print_type(t2);
  printf("\n");
#endif

  if (types_equal(t1, t2)) {
    return;
  }

  if (t1->kind == T_VAR && t2->kind == T_VAR &&
      strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0) {
    return;
  }

  if (t1->kind == T_VAR) {

    if (occurs(t1->data.T_VAR, t2)) {
      fprintf(stderr, "Error: Recursive unification\n");
    }

    substitute(t2, t1->data.T_VAR, t1);

    *env = env_extend(*env, t1->data.T_VAR, t2);
    *t1 = *t2; // Update t1 to point to t2

    return;
  }

  if (t2->kind == T_VAR) {
    _unify(t2, t1, env);
    return;
  }

  if (t1->kind == T_FN && t2->kind == T_FN) {
    if (t1->data.T_FN.from->kind == T_VAR) {
      substitute(t1, t1->data.T_FN.from->data.T_VAR, t2->data.T_FN.from);
    }

    _unify(t1->data.T_FN.from, t2->data.T_FN.from, env);
    _unify(t1->data.T_FN.to, t2->data.T_FN.to, env);

    for (Type *lookedup;
         ((t1->data.T_FN.to->kind == T_VAR) &&
          (lookedup = env_lookup(*env, t1->data.T_FN.to->data.T_VAR)));) {
      substitute(t1->data.T_FN.to, t1->data.T_FN.to->data.T_VAR, lookedup);
    }

    return;
  }

  if (t1->kind == T_CONS && t2->kind == T_CONS) {
    if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0 ||
        t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
      fprintf(stderr, "Error: Type mismatch between %s and %s\n",
              t1->data.T_CONS.name, t2->data.T_CONS.name);
      exit(1);
    }

    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      _unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i], env);
    }

    return;
  }
  printf("---\n");
  print_type(t1);
  print_type(t2);
  printf("\n");
  fprintf(stderr, "Error: Types are not unifiable\n");
}

void unify(Type *t1, Type *t2) {
  TypeEnv *env = NULL;
  _unify(t1, t2, &env);
}

void free_type_env(TypeEnv *env) {
  if (env->next) {
    free_type_env(env->next);
    free(env);
  }
}

static Type *resolve_single_type(Type *t, TypeEnv *env) {
  if (t == NULL)
    return NULL;

  switch (t->kind) {
  case T_VAR: {
    Type *resolved = env_lookup(env, t->data.T_VAR);
    if (resolved != NULL) {
      // Recursively resolve the found type
      return resolve_single_type(resolved, env);
    }
    return t; // If not found in env, return the original type
  }
  case T_CONS: {
    Type *new_type = deep_copy_type(t);

    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      new_type->data.T_CONS.args[i] =
          resolve_single_type(t->data.T_CONS.args[i], env);
    }
    return new_type;
  }
  case T_FN: {
    Type *new_type = malloc(sizeof(Type));
    new_type->kind = T_FN;
    new_type->data.T_FN.from = resolve_single_type(t->data.T_FN.from, env);
    new_type->data.T_FN.to = resolve_single_type(t->data.T_FN.to, env);
    return new_type;
  }
  default:
    return t;
  }
}

Type *resolve_in_env(Type *t, TypeEnv *env) {
  if (t == NULL)
    return NULL;

  return resolve_single_type(t, env);
}
