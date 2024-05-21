#ifndef _LANG_EVAL_H
#define _LANG_EVAL_H
#include "env.h"
#include "parse.h"
#include "value.h"

#define STACK_MAX 256

Value *eval(Ast *ast, Value *val, Table *stack, int stack_ptr, Table *fn_args);

void print_value(Value *val);
#endif
