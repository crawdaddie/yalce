#ifndef _LANG_TYPE_TYPE_H
#define _LANG_TYPE_TYPE_H

#include <stdbool.h>
#include <stdlib.h>
typedef struct TypeEnv TypeEnv;
typedef struct Type Type;

typedef struct Method {
  const char *name;
  Type *type;
} Method;

typedef struct TypeClass {
  const char *name;
  void *methods;
  size_t method_size;
  size_t num_methods;
} TypeClass;

void type_arena_init();

// clang-format off
#define TYPE_NAME_LIST    "List"
#define TYPE_NAME_TUPLE   "Tuple"
#define TYPE_NAME_PTR     "Ptr"
#define TYPE_NAME_CHAR    "Char"
#define TYPE_NAME_STRING  "String"
#define TYPE_NAME_BOOL    "Bool"
#define TYPE_NAME_INT     "Int"
#define TYPE_NAME_DOUBLE  "Double"
#define TYPE_NAME_UINT64  "Uint64"
// clang-format on

enum TypeKind {
  /* Type Operator */
  T_INT,
  T_UINT64,
  T_NUM,
  T_STRING,
  T_CHAR,
  T_BOOL,
  T_VOID,
  T_FN,
  T_PAIR,
  T_TUPLE,
  T_LIST,
  T_CONS,
  /* Type Variable  */
  T_VAR,
  T_VARIANT,
  // T_VARIANT_MEMBER,
  T_MODULE,
  T_TYPECLASS,
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

    TypeEnv *T_MODULE;

    struct {
      struct Type **args;
      int num_args;
    } T_VARIANT;

    TypeClass *T_TYPECLASS;
  } data;

  TypeClass **implements; // Array of type classes this type implements
  int num_implements;
  const char *alias;
  void *constructor;
  size_t constructor_size;
} Type;

extern TypeClass TCNum;
extern TypeClass TCOrd;

extern Type t_int;
extern Type t_uint64;
extern Type t_num;
extern Type t_string;
extern Type t_bool;
extern Type t_void;
extern Type t_char;
extern Type t_ptr;

// TypeEnv represents a mapping from variable names to their types
typedef struct TypeEnv {
  const char *name;
  Type *type;
  struct TypeEnv *next;
} TypeEnv;

void unify(Type *t1, Type *t2);

void _unify(Type *t1, Type *t2, TypeEnv **env);

Type create_type_var(const char *name);
Type tvar(const char *name);

Type *create_type_cons(const char *name, Type *args, int num_args);
Type *tcons(const char *name, Type **args, int num_args);
// Type create_type_fn(Type *from, Type *to);
// Type create_type_multi_param_fn(int num_params, Type **params,
//                                 Type *return_type);

Type *create_tuple_type(Type **element_types, int num_elements);
Type *create_list_type(Type *element_type);

Type *fresh(Type *type);

Type *env_lookup(TypeEnv *env, const char *name);
Type *find_variant_type(TypeEnv *env, const char *name);
bool find_variant_index(Type *variant, const char *name, int *index);

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type);
void free_type_env(TypeEnv *env);

Type *resolve_in_env(Type *t, TypeEnv *env);

void add_typeclass_impl(Type *t, TypeClass *class);
bool implements_typeclass(Type *t, TypeClass *class);
TypeEnv *initialize_type_env(TypeEnv *env);
TypeClass *typeclass_instance(TypeClass *tc);
TypeClass *typeclass_impl(Type *t, TypeClass *class);
void *get_typeclass_method(TypeClass *tc, int index);

bool is_arithmetic(Type *t);

Type *deep_copy_type(const Type *t);
#endif
