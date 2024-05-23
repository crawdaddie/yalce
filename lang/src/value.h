#ifndef _LANG_VALUE_H
#define _LANG_VALUE_H
#include "common.h"
#include "parse.h"
#include <stdbool.h>

typedef union {
  double d;
  void *vp;
  char *cp;
  int i;
  uintptr_t up;
} UniversalType;

typedef struct Value Value;

// typedef UniversalType (*extern_handle_t)(UniversalType, ...);
typedef double (*extern_double)(void *, ...);
typedef int (*extern_int)(void *, ...);
typedef void *(*extern_void_ptr)(void *, ...);
typedef void (*extern_void)(void *, ...);
typedef char *(*extern_str)(void *, ...);
typedef Value (*native_fn_t)(int, Value *);

typedef enum value_type {
  VALUE_INT,
  VALUE_NUMBER,
  VALUE_STRING,
  VALUE_BOOL,
  VALUE_VOID,
  VALUE_OBJ,
  VALUE_FN,
  VALUE_EXTERN_FN,
  VALUE_RECURSIVE_REF,
  VALUE_CURRIED_FN,
  VALUE_TYPE,
  VALUE_NATIVE_FN,
} value_type;

typedef struct {
  size_t len;
  ObjString *params;
  const char *fn_name;
  void *env_ptr;
  // bool is_recursive;
  Ast *body;
  int scope_ptr;
} Function;

typedef struct {
  void *handle;
  value_type *input_types;
  int len;
  value_type return_type;
} ExternFn;

typedef struct {
  native_fn_t handle;
  value_type *input_types;
  int len;
  value_type return_type;
} NativeFn;

struct Value {
  value_type type;
  union {
    int vint;
    double vnum;
    ObjString vstr;
    bool vbool;
    void *vobj;
    Function function;
    Function recursive_ref;
    ExternFn extern_fn;
    NativeFn native_fn;
    value_type type;
  } value;
};

#define INT(i)                                                                 \
  (Value) {                                                                    \
    VALUE_INT, { .vint = i }                                                   \
  }

#define NUM(i)                                                                 \
  (Value) {                                                                    \
    VALUE_NUMBER, { .vnum = i }                                                \
  }

#define STRING(i)                                                              \
  (Value) {                                                                    \
    VALUE_STRING, { .vstr = i }                                                \
  }

#define BOOL(i)                                                                \
  (Value) {                                                                    \
    VALUE_BOOL, { .vbool = i }                                                 \
  }

#define NATIVE_FN(_handle, _len)                                               \
  &(Value) {                                                                   \
    .type = VALUE_NATIVE_FN, .value = {                                        \
      .native_fn = {.handle = &_handle, .len = _len}                           \
    }                                                                          \
  }

#define TYPE_MAP(_id, _type)                                                   \
  (native_symbol_map) {                                                        \
    .id = _id, &(Value) {                                                      \
      .type = VALUE_TYPE, .value = {.type = _type }                            \
    }                                                                          \
  }
typedef struct {
  const char *id;
  Value *type;
} native_symbol_map;

#endif
