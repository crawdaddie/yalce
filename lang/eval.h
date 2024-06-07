#ifndef _LANG_EVAL_H
#define _LANG_EVAL_H
#include "ht.h"
#include "parse.h"
#include "value.h"

Value eval(Ast *ast, LangCtx *ctx);

#endif
