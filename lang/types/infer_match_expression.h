#ifndef _LANG_TYPE_INFER_MATCH_H
#define _LANG_TYPE_INFER_MATCH_H

#include "parse.h"
#include "types/inference.h"
#include "types/type.h"
Type *infer_match_expression(Ast *ast, TICtx *ctx);
#endif
