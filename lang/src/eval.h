#ifndef _LANG_EVAL_H
#define _LANG_EVAL_H
#include "env.h"
#include "ht.h"
#include "parse.h"
#include "value.h"

#define STACK_MAX 256

Value *eval(Ast *ast, Value *val, ht *stack, int stack_ptr, ht *fn_args);

#endif
