#ifndef _LANG_TYPE_TYPE_H
#define _LANG_TYPE_TYPE_H

#include <stdbool.h>

typedef struct TypeClass {
  const char *name;
} TypeClass;

typedef struct InstTypeClass {
  TypeClass *class;
  struct InstTypeClass *next;
} InstTypeClass;

enum TypeKind {
  /* Type Operator */
  T_INT,
  T_NUM,
  T_STRING,
  T_BOOL,
  T_VOID,
  T_FN,
  T_PAIR,
  T_TUPLE,
  T_LIST,
  T_CONS,
  /* Type Variable  */
  T_VAR,
};

typedef struct Type {
  enum TypeKind kind;
  union {
    // Type Variables (T_VAR):
    // They represent unknown types that can be unified with other types during
    // inference.
    const char *T_VAR;
    struct {
      const char *name;
      struct Type **args;
      int num_args;
    } T_CONS;
    struct {
      struct Type *from;
      struct Type *to;
    } T_FN;
  } data;
  InstTypeClass *type_class;

} Type;

extern Type t_int;
extern Type t_num;
extern Type t_string;
extern Type t_bool;
extern Type t_void;

extern TypeClass TClassOrd;

// A TypeScheme represents a polymorphic type in Hindley-Milner type systems.
// It consists of two parts:
// A set of type variables that are universally quantified.
// A type expression that may contain these quantified
// variables.
typedef struct TypeScheme {
  Type *variables; // A set of type variables that are universally quantified.
  int num_variables;
  Type *type; // A type expression that may contain these quantified variables
} TypeScheme;

// TypeEnv represents a mapping from variable names to their types
typedef struct TypeEnv {
  const char *name;
  Type *type;
  struct TypeEnv *next;
} TypeEnv;

TypeScheme *generalize(TypeEnv env, Type *type);
Type *instantiate(TypeScheme *scheme);
void unify(Type *t1, Type *t2);

Type *env_lookup(TypeEnv *env, const char *name);
Type *create_type_var(const char *name);
Type *tvar(const char *name);

Type *create_type_cons(const char *name, Type *args, int num_args);
Type *tcons(const char *name, Type **args, int num_args);
Type *create_type_fn(Type *from, Type *to);
Type *create_type_multi_param_fn(int num_params, Type **params,
                                 Type *return_type);

Type *fresh(Type *type);
TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type);

#endif
