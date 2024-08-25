#include "types/type.h"
#include "backend_llvm/codegen_types.h"
#include "types/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DBG_UNIFY
// clang-format off
Type t_num_method_signature = {T_FN, {.T_FN = {
  &(Type){T_VAR, {.T_VAR = "a : Num"}},
    &(Type){T_FN, {.T_FN = {
      &(Type){T_VAR, {.T_VAR = "b : Num"}},
      &(Type){T_VAR, {.T_VAR = "c : Num"}},
}}}

}}};
static Method tcnum_methods[] = {
  {.name = "+", &t_num_method_signature},
  {.name = "-", &t_num_method_signature},
  {.name = "*", &t_num_method_signature},
  {.name = "/", &t_num_method_signature},
  {.name = "%", &t_num_method_signature},
};
// clang-format on
TypeClass TCNum = {"Num", tcnum_methods, .method_size = sizeof(Method),
                   .num_methods = 5};

Type t_ord_method_signature = {
    T_FN,
    {.T_FN = {&(Type){T_VAR, {.T_VAR = "a : Ord"}},
              &(Type){T_FN,
                      {.T_FN =
                           {
                               &(Type){T_VAR, {.T_VAR = "b : Ord"}},
                               &(Type){T_BOOL},
                           }}}

     }}};

// clang-format off
static Method tcord_methods[] = {
    {.name = "<",   &t_ord_method_signature},
    {.name = "<=",  &t_ord_method_signature},
    {.name = ">",   &t_ord_method_signature},
    {.name = ">=",  &t_ord_method_signature},
};

TypeClass TCOrd = {"Ord", tcord_methods, .method_size = sizeof(Method),
                   .num_methods = 4};

Type t_int =    {T_INT, .implements = (TypeClass *[]){&TCNum},
                        .num_implements = 1};

Type t_uint64 = {T_UINT64,
                .constructor = uint64_constructor,
                .constructor_size = sizeof(ConsMethod),
                .implements = (TypeClass *[]){&TCNum},
                .num_implements = 1};
Type t_num =    {T_NUM,
                .constructor = double_constructor,
                .constructor_size = sizeof(ConsMethod),
                .implements = (TypeClass *[]){&TCNum},
                .num_implements = 1};

Type t_char =   {T_CHAR};
Type t_string = {T_CONS, {.T_CONS = {TYPE_NAME_LIST, (Type *[]){&t_char}, 1}}};
Type t_bool =   {T_BOOL};
Type t_void =   {T_VOID};
Type t_ptr =    {T_CONS,
                {.T_CONS = {TYPE_NAME_PTR, (Type *[]){&t_char}, 1}},
                .constructor = ptr_constructor,
                .constructor_size = sizeof(ConsMethod),
                };

// clang-format on

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type) {
  TypeEnv *new_env = malloc(sizeof(TypeEnv));
  new_env->name = name;
  new_env->type = type;
  new_env->next = env;
  return new_env;
}

bool variant_contains(Type *variant, const char *name) {
  for (int i = 0; i < variant->data.T_VARIANT.num_args; i++) {
    Type *variant_member = variant->data.T_VARIANT.args[i];
    const char *mem_name;
    if (variant_member->kind == T_CONS) {
      mem_name = variant_member->data.T_CONS.name;
    } else if (variant_member->kind == T_VAR) {
      mem_name = variant_member->data.T_VAR;
    } else {
      continue;
    }

    if (strcmp(mem_name, name) == 0) {
      return true;
    }
  }
  return false;
}

bool find_variant_index(Type *variant, const char *name, int *index) {
  for (int i = 0; i < variant->data.T_VARIANT.num_args; i++) {
    Type *variant_member = variant->data.T_VARIANT.args[i];
    const char *mem_name;
    if (variant_member->kind == T_CONS) {
      mem_name = variant_member->data.T_CONS.name;
    } else if (variant_member->kind == T_VAR) {
      mem_name = variant_member->data.T_VAR;
    } else {
      continue;
    }

    if (strcmp(mem_name, name) == 0) {
      *index = i;
      return true;
    }
  }
  return false;
}

Type *env_lookup(TypeEnv *env, const char *name) {
  while (env) {
    if (strcmp(env->name, name) == 0) {
      return env->type;
    }
    if (env->type->kind == T_VARIANT) {
      Type *variant = env->type;
      for (int i = 0; i < variant->data.T_VARIANT.num_args; i++) {
        Type *variant_member = variant->data.T_VARIANT.args[i];
        const char *mem_name;
        if (variant_member->kind == T_CONS) {
          mem_name = variant_member->data.T_CONS.name;
        } else if (variant_member->kind == T_VAR) {
          mem_name = variant_member->data.T_VAR;
        } else {
          continue;
        }

        if (strcmp(mem_name, name) == 0) {
          return variant_member;
        }
      }
    }
    env = env->next;
  }
  return NULL;
}

Type *find_variant_type(TypeEnv *env, const char *name) {
  while (env) {

    if (env->type->kind == T_VARIANT) {
      Type *variant = env->type;
      if (variant_contains(variant, name)) {
        return variant;
      }
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

// Type create_type_var(const char *name) {
//   return (Type){
//       .kind = T_VAR,
//       .data.T_VAR = name,
//   };
// }

// Type tvar(const char *name) { return create_type_var(name); }

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
  tuple_type->data.T_CONS.name = TYPE_NAME_TUPLE;
  tuple_type->data.T_CONS.args = element_types;
  tuple_type->data.T_CONS.num_args = num_elements;
  return tuple_type;
}

Type *create_list_type(Type *element_type) {
  Type *tuple_type = malloc(sizeof(Type));
  tuple_type->kind = T_CONS;
  tuple_type->data.T_CONS.name = TYPE_NAME_LIST;
  tuple_type->data.T_CONS.args = malloc(sizeof(Type *));
  tuple_type->data.T_CONS.args[0] = element_type;
  tuple_type->data.T_CONS.num_args = 1;
  return tuple_type;
}

// Type *create_type_fn(Type *from, Type *to) {
//   Type *type = malloc(sizeof(Type));
//   *type = (Type){T_FN, .data = {.T_FN = {.from = from, .to = to}}};
//   return type;
//   // Create a new function type
// }
//
// Type *create_type_multi_param_fn(int param_count, Type **param_types,
//                                  Type *return_type) {
//
//   Type *func_type = return_type;
//   for (int i = param_count - 1; i >= 0; i--) {
//     func_type = create_type_fn(param_types[i], func_type);
//   }
//   return func_type;
// }

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
  if (t2->implements != NULL) {
    for (int i = 0; i < t2->num_implements; i++) {
      TypeClass *tc = t2->implements[i];
      if (!implements_typeclass(t1, tc)) {
        add_typeclass_impl(t1, tc);
      }
    }
  }

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
      // exit(1);
    }

    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      _unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i], env);
    }

    return;
  }

  // variants
  if (t2->kind == T_VARIANT) {
    if (t1->kind == T_VAR && variant_contains(t2, t1->data.T_VAR)) {
      return;
    }
    if (t1->kind == T_CONS && variant_contains(t2, t1->data.T_CONS.name)) {
      return;
    }
  }

  if (t1->alias) {
    printf("[%s]", t1->alias);
  } else {
    print_type(t1);
  }
  printf(" != ");

  if (t2->alias) {
    printf("[%s]", t2->alias);
  } else {
    print_type(t2);
  }
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

// Helper function to add a type class implementation to a type
void add_typeclass_impl(Type *t, TypeClass *class) {
  if (!(t->kind == T_VAR || t->kind == T_CONS)) {
    return;
  }
  t->num_implements++;
  t->implements =
      t->implements != NULL
          ? realloc(t->implements, t->num_implements * sizeof(TypeClass *))
          : malloc(sizeof(TypeClass *));

  t->implements[t->num_implements - 1] = class;
}

// Helper function to check if a type implements a specific type class
bool implements_typeclass(Type *t, TypeClass *class) {
  for (int i = 0; i < t->num_implements; i++) {
    if (strcmp(t->implements[i]->name, class->name) == 0) {
      return true;
    }
  }
  return false;
}

// Helper function to get a specific type class implementation from a type
TypeClass *typeclass_impl(Type *t, TypeClass *class) {
  for (int i = 0; i < t->num_implements; i++) {
    if (strcmp(t->implements[i]->name, class->name) == 0) {
      return t->implements[i];
    }
  }
  return NULL;
}

#define TYPE_FROM_TYPECLASS(tc)                                                \
  (Type) {                                                                     \
    T_TYPECLASS, { .T_TYPECLASS = tc }                                         \
  }

TypeClass *typeclass_instance(TypeClass *tc) {
  TypeClass *new_tc = malloc(sizeof(TypeClass));
  new_tc->name = tc->name;
  new_tc->num_methods = tc->num_methods;
  new_tc->methods = malloc(sizeof(Method) * tc->num_methods);
  return new_tc;
}

TypeEnv *initialize_type_env(TypeEnv *env) {
  Type *t = malloc(sizeof(Type));
  *t = TYPE_FROM_TYPECLASS(&TCNum);
  env = env_extend(env, "Num", t);

  Type *tt = malloc(sizeof(Type));
  *tt = TYPE_FROM_TYPECLASS(&TCOrd);
  env = env_extend(env, "Ord", tt);
  return env;
}

// Generic function to get a method from a TypeClass
void *get_typeclass_method(TypeClass *tc, int index) {
  if (index < 0 || index >= tc->num_methods) {
    return NULL;
  }
  return (void *)tc->methods + (index * tc->method_size);
}

bool is_arithmetic(Type *t) { return (t->kind >= T_INT) && (t->kind <= T_NUM); }
