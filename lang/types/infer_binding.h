#ifndef _LANG_TYPE_INFER_BINDING_H
#define _LANG_TYPE_INFER_BINDING_H
#include "./inference.h"
#include "./type.h"

Type *bind_pattern_recursive(Ast *pattern, Type *pattern_type, binding_md md,
                             TICtx *ctx);
Type *infer_let_pattern_binding(Ast *binding, Ast *val, Ast *body, TICtx *ctx);
#endif
