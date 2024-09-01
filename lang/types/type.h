#ifndef _LANG_TYPE_TYPE_H
#define _LANG_TYPE_TYPE_H
#include "typeclass.h"
#include <stdbool.h>

typedef struct TypeEnv TypeEnv;
typedef struct Type Type;

extern Type t_int;
extern Type t_uint64;
extern Type t_num;
extern Type t_string;
extern Type t_bool;
extern Type t_void;
extern Type t_char;
extern Type t_ptr;

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
#define TYPE_NAME_VOID    "()"
// clang-format on
//

enum TypeKind {
  /* Type Operator */
  T_INT,
  T_UINT64,
  T_NUM,
  T_CHAR,
  T_BOOL,
  T_VOID,
  T_STRING,
  T_FN,
  T_TUPLE,
  T_CONS,
  T_VARIANT,
  /* Type Variable  */
  T_VAR,
  // T_VARIANT_MEMBER,
  // T_MODULE,
  // T_TYPECLASS,
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

  const char *alias;
  TypeClass **implements; // Array of type classes this type implements
  int num_implements;
  void *constructor;
  size_t constructor_size;
} Type;

// TypeEnv represents a mapping from variable names to their types
typedef struct TypeEnv {
  const char *name;
  Type *type;
  struct TypeEnv *next;
} TypeEnv;

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type);
Type *env_lookup(TypeEnv *env, const char *name);
Type *variant_lookup(TypeEnv *env, Type *member);
void free_type_env(TypeEnv *env);
void print_type_env(TypeEnv *env);
Type *find_type_in_env(TypeEnv *env, const char *name);

char *type_to_string(Type *t, char *buffer);

void print_type(Type *t);
bool types_equal(Type *l, Type *r);

Type *fn_return_type(Type *);

void *talloc(size_t size);
void tfree(void *mem);
Type *empty_type();
Type *tvar(const char *name);
bool is_generic(Type *t);

Type *type_fn(Type *from, Type *to);
#endif
