#ifndef _LANG_TYPES_UNIFICATION_H
#define _LANG_TYPES_UNIFICATION_H
#include "types/type.h"

Type *unify(Type *t1, Type *t2, TypeEnv **env);

#endif
