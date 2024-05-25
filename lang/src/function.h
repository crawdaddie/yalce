#ifndef _LANG_FUNCTION_H
#define _LANG_FUNCTION_H
#include "ht.h"
#include "parse.h"
#include "value.h"

Value eval_application(Ast *ast, ht *stack, int stack_ptr);
#endif
