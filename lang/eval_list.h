#ifndef _LANG_EVAL_LIST_H
#define _LANG_EVAL_LIST_H
#include "ht.h"
#include "parse.h"
#include "value.h"

Value eval_list(Ast *list, LangCtx *ctx);
#endif
