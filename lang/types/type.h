#ifndef _LANG_TYPES_TYPE_H
#define _LANG_TYPES_TYPE_H
#include "typeclass.h"
#include <stdbool.h>
#include <stdlib.h>

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

enum TypeKind {
  // Type Operator
  T_INT,
  T_UINT64,
  T_NUM,
  // T_STRING,
  T_BOOL,
  T_VOID,
  T_CHAR,

  T_CONS,
  T_FN,
  T_VAR,
  T_VARIANT,
  /*
  T_TUPLE,
  T_LIST,

  // Type Variable
  T_MODULE,
  T_TYPECLASS,
  */
};

typedef struct Type {
  enum TypeKind kind;
  union {
    const char *T_VAR;

    struct {
      struct Type *from;
      struct Type *to;
    } T_FN;

    struct {
      const char *name;
      struct Type *args;
      int num_args;
    } T_CONS;

  } data;
  int num_implements;
  TypeClass *implements;
  void *constructor;
  size_t constructor_size;
} Type;

typedef struct TypeEnv {
  const char *name;
  Type *type;
  struct TypeEnv *next;
} TypeEnv;

bool types_equal(Type a, Type b);

extern Type t_int;
extern Type t_uint64;
extern Type t_num;
extern Type t_string;
extern Type t_bool;
extern Type t_void;
extern Type t_char;
extern Type t_string;
extern Type t_ptr;

char *type_to_string(Type t, char *buffer);
void print_type(Type t);
void print_full_type(Type t);

Type *fn_ret_type(Type *fn_type);

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type);
Type *env_lookup(TypeEnv *env, const char *name);
void free_type_env(TypeEnv *env);
void print_type_env(TypeEnv *env);

Type *get_type(TypeEnv *env, const char *name);

void set_tuple_type(Type *type, Type *cons_args, int arity);
void set_list_type(Type *type, Type *el_type);
void set_fn_type(Type *type, int num_args);

bool is_generic(Type *t);

#endif
