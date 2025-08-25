#ifndef _LANG_TYPE_INFER_LAMBDA_H
#define _LANG_TYPE_INFER_LAMBDA_H
#include "parse.h"
#include "types/inference.h"
#include "types/type.h"
Type *infer_lambda(Ast *ast, TICtx *ctx);
#endif
