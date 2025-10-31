#ifndef _LANG_TYPE_INFER_APPLICATION__H
#define _LANG_TYPE_INFER_APPLICATION__H
#include "types/inference.h"
#include "types/type.h"
Type *infer_application(Ast *ast, TICtx *ctx);

const char *find_constructor_method(Type *cons_mod, int len, Type **inputs,
                                    int *index, Type **method);
#endif
