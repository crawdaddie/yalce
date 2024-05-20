#ifndef _LANG_VALUE_H
#define _LANG_VALUE_H
#include "parse.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int length;
  char *chars;
  uint32_t hash;
} ObjString;

typedef struct {
  size_t len;
  const char **params;
  const char *fn_name;
  void *env_ptr;
  // bool is_recursive;
  Ast *body;
} Function;

typedef struct {
  enum type {
    VALUE_INT,
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_BOOL,
    VALUE_VOID,
    VALUE_OBJ,
    VALUE_FN,
  } type;

  union {
    int vint;
    double vnum;
    ObjString vstr;
    bool vbool;
    void *vobj;
    Function function;
  } value;
} Value;
#endif
