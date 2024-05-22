#ifndef _LANG_EVAL_H
#define _LANG_EVAL_H
#include "ht.h"
#include "parse.h"
#include "value.h"

#define STACK_MAX 256

Value *eval(Ast *ast, Value *val, ht *stack, int stack_ptr);

#endif
