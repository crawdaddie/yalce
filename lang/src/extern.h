#ifndef _LANG_EXTERN_H
#define _LANG_EXTERN_H
#include "ht.h"
#include "parse.h"
#include "value.h"
Value *register_external_symbol(Ast *ast, ht *, int);

Value *call_external_function(Value *extern_fn, Value *input_vals, int len,
                              Value *return_val);
#endif
