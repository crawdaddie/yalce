#ifndef _LANG_TYPE_INFER_APPLICATION_H
#define _LANG_TYPE_INFER_APPLICATION_H
#include "./type.h"
#include "parse.h"
Type *infer_fn_application(Ast *ast, TICtx *ctx);
Type *infer_application(Ast *ast, TICtx *ctx);
Type *infer_cons_application(Ast *ast, TICtx *ctx);

#endif
