#ifndef _LANG_EVAL_H
#define _LANG_EVAL_H
#include "env.h"
#include "parse.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
  Table envs[STACK_MAX];
  int stack_ptr;
} EnvStack;

Value *eval(Ast *ast, Value *val, EnvStack *stack);

void print_value(Value *val);
#endif
