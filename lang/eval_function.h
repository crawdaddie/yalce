#ifndef _LANG_FUNCTION_H
#define _LANG_FUNCTION_H
#include "ht.h"
#include "parse.h"
#include "value.h"

Value eval_application(Ast *ast, LangCtx *ctx);
Value eval_lambda_declaration(Ast *ast, LangCtx *ctx);

Value fn_call(Function fn, Value *input_vals, LangCtx *ctx);
#endif
