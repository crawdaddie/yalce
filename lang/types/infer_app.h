#ifndef _LANG_TYPES_INFER_APP_H
#define _LANG_TYPES_INFER_APP_H

#include "types/inference.h"
#include "types/type.h"

Type *infer_app(Ast *ast, TICtx *ctx);
#endif
