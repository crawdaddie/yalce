#include "./unification.h"
#include "types/common.h"
#include <stdlib.h>
#include <string.h>

// Subst *subst_extend(Subst *s, const char *key, Type *type);
// Subst *compose_subst(Subst *s1, Subst *s2);
// Type *apply_substitution(Subst *subst, Type *t);
// Type *find_in_subst(Subst *subst, const char *name);

void print_subst(Subst *subst);

bool occurs_check(const char *var, Type *ty) {
  switch (ty->kind) {
  case T_VAR: {
    return CHARS_EQ(ty->data.T_VAR, var);
  }
  case T_FN: {
    return occurs_check(var, ty->data.T_FN.from) ||
           occurs_check(var, ty->data.T_FN.to);
  }
  case T_CONS: {
    for (int i = 0; i < ty->data.T_CONS.num_args; i++) {
      if (occurs_check(var, ty->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  }
  default: {
    return false;
  }
  }
}
// Add a constraint to the result
void add_constraint(TICtx *result, Type *var, Type *type) {
  for (Constraint *c = result->constraints; c; c = c->next) {
    if (types_equal(c->var, var) && types_equal(c->type, type)) {
      return;
    }
  }

  Constraint *constraint = talloc(sizeof(Constraint));
  *constraint =
      (Constraint){.var = var, .type = type, .next = result->constraints};
  result->constraints = constraint;
}

Constraint *constraints_extend(Constraint *constraints, Type *var, Type *type) {
  Constraint *constraint = talloc(sizeof(Constraint));
  *constraint = (Constraint){.var = var, .type = type, .next = constraints};
  return constraint;
}

// Simple constraint list merging
Constraint *merge_constraints(Constraint *list1, Constraint *list2) {
  if (!list1)
    return list2;
  if (!list2)
    return list1;

  // Find end of list1 and append list2
  Constraint *current = list1;
  while (current->next) {
    current = current->next;
  }
  current->next = list2;

  return list1;
}

// Print all collected constraints
void print_constraints(Constraint *constraints) {
  printf("Collected constraints:\n");
  if (!constraints) {
    printf("  (none)\n");
    return;
  }

  for (Constraint *c = constraints; c; c = c->next) {
    printf("  %s := ", c->var->data.T_VAR);
    print_type(c->type);
  }
}

int unify(Type *t1, Type *t2, TICtx *unify_res) {
  // printf("unify ");
  // print_type(t1);
  // printf(" ~ \n");
  // print_type(t2);

  if (types_equal(t1, t2)) {
    // No constraints needed for identical types
    return 0;
  }

  // Case 1: First type is a variable - COLLECT CONSTRAINT
  if (t1->kind == T_VAR) {
    if (occurs_check(t1->data.T_VAR, t2)) {
      return 1; // Occurs check failure
    }

    add_constraint(unify_res, t1, t2);
    return 0;
  }

  // Case 2: Second type is a variable - COLLECT CONSTRAINT
  if (t2->kind == T_VAR) {
    if (occurs_check(t2->data.T_VAR, t1)) {
      return 1; // Occurs check failure
    }

    add_constraint(unify_res, t2, t1);
    return 0;
  }

  // Case 3: Function types - recurse and merge constraints
  if (t1->kind == T_FN && t2->kind == T_FN) {
    // Unify parameter types
    TICtx ur1 = {};
    if (unify(t1->data.T_FN.from, t2->data.T_FN.from, &ur1) != 0) {
      return 1;
    }

    // Unify return types
    TICtx ur2 = {};
    if (unify(t1->data.T_FN.to, t2->data.T_FN.to, &ur2) != 0) {
      return 1;
    }

    // Merge all constraints (don't solve them)
    unify_res->constraints =
        merge_constraints(unify_res->constraints, ur1.constraints);
    unify_res->constraints =
        merge_constraints(unify_res->constraints, ur2.constraints);

    return 0;
  }

  // Case 4: Constructor types - recurse and merge constraints
  if (t1->kind == T_CONS && t2->kind == T_CONS) {
    if (!CHARS_EQ(t1->data.T_CONS.name, t2->data.T_CONS.name) ||
        t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
      return 1;
    }

    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      TICtx ur = {};
      if (unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i], &ur) != 0) {
        return 1;
      }

      // Merge constraints from this argument
      unify_res->constraints =
          merge_constraints(unify_res->constraints, ur.constraints);
    }

    return 0;
  }

  // Case 5: Two concrete types - this will be handled by constraint solver
  // later
  if (t1->kind != T_VAR && t2->kind != T_VAR) {
    // printf("Two concrete types - cannot unify directly: ");
    // print_type(t1);
    // printf(" vs ");
    // print_type(t2);
    // printf("\n");
    return 1;
  }

  return 1; // Unification failure
}

// Helper: Update a substitution by replacing/updating a variable's binding
Subst *update_substitution(Subst *subst, const char *var, Type *new_type) {
  if (new_type->kind == T_VAR) {
    return subst;
  }
  // Remove old binding if it exists
  Subst *new_subst = NULL;

  for (Subst *s = subst; s; s = s->next) {
    if (!CHARS_EQ(s->var, var)) {
      // Keep bindings for other variables
      new_subst = subst_extend(new_subst, s->var, s->type);
    }
  }

  new_subst = subst_extend(new_subst, var, new_type);
  return new_subst;
}

TypeClass *find_typeclass(TypeClass *impls, const char *name) {
  for (TypeClass *tc = impls; tc; tc = tc->next) {
    if (CHARS_EQ(tc->name, name)) {
      return tc;
    }
  }
  return NULL;
}

Type *find_promoted_type(Type *var, Type *existing, Type *other_type) {

  if (other_type->kind == T_VAR && existing->kind != T_VAR) {
    return existing;
  }
  if (types_equal(existing, other_type)) {
    return existing;
  }

  TypeClass *tc = var->required;
  if (!tc) {
    return other_type;
  }

  TypeClass *ex_tc = find_typeclass(existing->implements, tc->name);

  TypeClass *other_tc = find_typeclass(other_type->implements, tc->name);
  if (!other_tc) {
    return NULL;
  }
  if (ex_tc->rank >= other_tc->rank) {
    return existing;
  } else {
    return other_type;
  }

  return existing;
}

Subst *solve_constraints(Constraint *constraints) {
  Subst *subst = NULL;

  while (constraints) {
    Constraint *current = constraints;
    constraints = constraints->next;

    const char *var_name = current->var->data.T_VAR;

    Type *new_type = apply_substitution(subst, current->type);
    Type *existing = find_in_subst(subst, var_name);

    if (!existing) {
      subst = subst_extend(subst, var_name, new_type);
      continue;
    }

    Type *existing_subst = apply_substitution(subst, existing);
    if (types_equal(existing_subst, new_type)) {
      continue;
    }

    TICtx ur = {.constraints = constraints};

    if (unify(existing_subst, new_type, &ur) != 0) {
      Type *promoted =
          find_promoted_type(current->var, existing_subst, new_type);
      if (promoted) {
        subst = update_substitution(subst, var_name, promoted);
        continue;
      } else {
        return NULL;
      }
    }

    // Apply direct substitutions
    if (ur.subst) {
      for (Subst *s = ur.subst; s; s = s->next) {
        subst = subst_extend(subst, s->var, s->type);
      }
    }

    constraints = ur.constraints;
    subst = update_substitution(subst, var_name, new_type);
    continue;
  }

  return subst;
}
