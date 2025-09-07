#ifndef _LANG_TYPE_UNIFICATION_H
#define _LANG_TYPE_UNIFICATION_H
#include "types/inference.h"
#include "types/type.h"

int unify(Type *t1, Type *t2, TICtx *unify_res);

void print_constraints(Constraint *constraints);
Constraint *merge_constraints(Constraint *list1, Constraint *list2);

Subst *solve_constraints(Constraint *constraints);

Constraint *constraints_extend(Constraint *constraints, Type *var, Type *type);

#endif
