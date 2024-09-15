#ifndef _LANG_TYPES_UNIFICATION_H
#define _LANG_TYPES_UNIFICATION_H
#include "types/type.h"

bool occurs_check(Type *var, Type *type);
Type *unify(Type *t1, Type *t2, TypeEnv **env);

void unify_recursive_defs_mut(Type *fn, Type *rec_ref, TypeEnv **env);

#endif
