#ifndef _LANG_EVAL_H
#define _LANG_EVAL_H
#include "parse.h"
#include "value.h"

Value *eval(Ast *ast, Value *val);

void print_value(Value *val);
#endif
