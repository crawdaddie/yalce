#ifndef _LANG_EVAL_LIST_H
#define _LANG_EVAL_LIST_H
#include "ht.h"
#include "parse.h"
#include "value.h"

Value eval_list(Ast *list, ht *stack, int stack_ptr, val_bind_fn_t val_bind);
#endif
