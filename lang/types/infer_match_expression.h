#ifndef _LANG_TYPE_INFER_MATCH_EXPR_H
#define _LANG_TYPE_INFER_MATCH_EXPR_H

#include "./inference.h"
Type *infer_match_expression(Ast *ast, TICtx *ctx);
#endif
