#ifndef _LANG_TYPE_UNIFICATION_H
#define _LANG_TYPE_UNIFICATION_H
#include "types/inference.h"

// Enhanced substitution structure to hold constraints
typedef struct Constraint {
  Type *var;  // Variable name (e.g., "t0")
  Type *type; // Required type (e.g., Int or Double)
  struct Constraint *next;
} Constraint;

typedef struct {
  Subst *subst;
  Constraint *constraints;
  InferenceTree *inf;
} UnifyResult;

int unify(Type *t1, Type *t2, UnifyResult *unify_res);

void print_constraints(Constraint *constraints);

Subst *solve_constraints(Constraint *constraints);

#endif
