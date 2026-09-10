#ifndef _LANG_TYPES_INFER_APPLICATION_H
#define _LANG_TYPES_INFER_APPLICATION_H

#include "types/inference.h"
#include "types/type.h"
Type *infer_application(Ast *ast, TICtx *ctx);

Type *callable_view(Type *type);
#endif
