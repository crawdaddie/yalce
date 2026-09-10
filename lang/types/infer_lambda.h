#ifndef _LANG_TYPES_INFER_LAMBDA_H
#define _LANG_TYPES_INFER_LAMBDA_H

#include "types/inference.h"
#include "types/type.h"
Type *infer_lambda(Ast *ast, TICtx *ctx);
#endif
