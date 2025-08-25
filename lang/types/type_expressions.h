#ifndef _LANG_TYPE_INFERENCE_TYPE_EXPRS_H
#define _LANG_TYPE_INFERENCE_TYPE_EXPRS_H
#include "./inference.h"
#include "parse.h"
Scheme *compute_type_expression(Ast *expr, TICtx *ctx);
#endif
