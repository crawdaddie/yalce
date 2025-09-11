#ifndef _LANG_TYPE_INFERENCE_TYPE_EXPRS_H
#define _LANG_TYPE_INFERENCE_TYPE_EXPRS_H
#include "./inference.h"
#include "parse.h"
Scheme *compute_typescheme(Ast *expr, TICtx *ctx);

Type *type_declaration(Ast *ast, TICtx *ctx);
#endif
