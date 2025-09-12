#ifndef _LANG_TYPE_INFERENCE_TYPE_EXPRS_H
#define _LANG_TYPE_INFERENCE_TYPE_EXPRS_H
#include "./inference.h"
#include "parse.h"

Type *compute_type_expression(Ast *expr, TICtx *ctx);
Scheme *compute_typescheme(Ast *expr, TICtx *ctx);

Type *type_declaration(Ast *ast, TICtx *ctx);
#endif
