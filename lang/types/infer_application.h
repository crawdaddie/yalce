#ifndef _LANG_TYPE_INFER_APPLICATION__H
#define _LANG_TYPE_INFER_APPLICATION__H
#include "types/inference.h"
#include "types/type.h"
Type *infer_application(Ast *ast, TICtx *ctx);
#endif
