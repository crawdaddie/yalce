#ifndef _LANG_VALUE_H
#define _LANG_VALUE_H
#include "common.h"
#include "parse.h"
#include <stdbool.h>

typedef void *(*extern_handle_t)(void *, ...);
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
  extern_handle_t handle;
  value_type *input_types;
  int len;

} ExternFn;

typedef struct {
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
  } value;
} Value;
#endif
