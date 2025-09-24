#ifndef _LANG_TYPE_UNIFICATION_H
#define _LANG_TYPE_UNIFICATION_H
#include "types/inference.h"
#include "types/type.h"
int unify(Type *a, Type *b, TICtx *ctx);
#endif
