#ifndef _LANG_EVAL_H
#define _LANG_EVAL_H
#include "ht.h"
#include "parse.h"
#include "value.h"

#define STACK_MAX 256

Value eval(Ast *ast, ht *stack, int stack_ptr, val_bind_fn_t val_bind);

#endif
