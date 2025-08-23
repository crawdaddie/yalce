#ifndef _LANG_TYPE_INFER_BINDING_H
#define _LANG_TYPE_INFER_BINDING_H
#include "./type.h"
#include "./inference.h"
Type *infer_pattern_binding(Ast *binding, Ast *val, Ast *body, TICtx *ctx) ;
#endif
