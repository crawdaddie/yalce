#ifndef _LANG_TYPES_UNIFICATION_H
#define _LANG_TYPES_UNIFICATION_H
#include "types/type.h"

bool occurs_check(Type *var, Type *type);
Type *unify(Type *t1, Type *t2, TypeEnv **env);

#endif
