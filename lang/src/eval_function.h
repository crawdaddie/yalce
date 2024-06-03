#ifndef _LANG_FUNCTION_H
#define _LANG_FUNCTION_H
#include "ht.h"
#include "parse.h"
#include "value.h"

Value eval_application(Ast *ast, ht *stack, int stack_ptr,
                       val_bind_fn_t val_bind);
Value eval_lambda_declaration(Ast *ast, ht *stack, int stack_ptr);

Value fn_call(Function fn, Value *input_vals, ht *stack,
              val_bind_fn_t val_bind);
#endif
