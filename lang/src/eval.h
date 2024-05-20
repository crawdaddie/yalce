#ifndef _LANG_EVAL_H
#define _LANG_EVAL_H

#include "parse.h"

typedef struct {
  enum type {
    VALUE_INT,
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_BOOL,
    VALUE_VOID,
    VALUE_OBJ,
  } type;

  union {
    int vint;
    double vnum;
    char *vstr;
    bool vbool;
    void *vobj;
  } value;
} Value;

Value *eval(Ast *ast, Value *val);

void print_value(Value *val);
#endif
