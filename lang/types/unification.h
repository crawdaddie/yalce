#ifndef _LANG_TYPE_UNIFICATION_H
#define _LANG_TYPE_UNIFICATION_H
#include "./inference.h"
#include "./type.h"
int unify(Type *a, Type *b, TICtx *ctx);
#endif
