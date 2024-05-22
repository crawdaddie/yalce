#ifndef _LANG_EXTERN_H
#define _LANG_EXTERN_H
#include "parse.h"
#include "value.h"
Value *register_external_symbol(Ast *ast);

Value *call_external_function(Value *extern_fn, Value *vals, int len);
#endif
