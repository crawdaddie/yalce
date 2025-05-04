#ifndef _LANG_TYPE_UNIFICATION_H
#define _LANG_TYPE_UNIFICATION_H
#include "./type.h"
Type *unify_in_ctx(Type *t1, Type *t2, TICtx *ctx, Ast *node);

Type *apply_substitution(Substitution *subst, Type *t);
#endif
