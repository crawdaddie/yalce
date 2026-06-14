#include "../lang/parse.h"
#include "../lang/types/builtins.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

static void check_constraint_count(TICtx *ctx, int expected,
                                   const char *test_name) {
  int count = 0;
  for (Constraint *c = ctx->constraints; c != NULL; c = c->next) {
    count++;
  }
  if (count == expected) {
    printf("✅ PASS: %s (constraints: %d)\n", test_name, count);
    tests_passed++;
  } else {
    printf("❌ FAIL: %s (expected %d, got %d)\n", test_name, expected, count);
    tests_failed++;
  }
}

static void check_constraint_exists(TICtx *ctx, Type *var, Type *type,
                                    const char *test_name) {
  for (Constraint *c = ctx->constraints; c != NULL; c = c->next) {
    if (types_equal(c->var, var) && types_equal(c->type, type)) {
      printf("✅ PASS: %s\n", test_name);
      tests_passed++;
      return;
    }
  }
  printf("❌ FAIL: %s (constraint not found)\n", test_name);
  tests_failed++;
}

static void test_add_constraint_basic() {
  printf("\n--- test_add_constraint_basic ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar = next_tvar();
  Type *tint = &t_int;

  add_constraint(&ctx, tvar, tint);

  check_constraint_count(&ctx, 1, "Single constraint added");
  check_constraint_exists(&ctx, tvar, tint, "Constraint matches var and type");
}

static void test_add_constraint_duplicate() {
  printf("\n--- test_add_constraint_duplicate ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar = next_tvar();
  Type *tint = &t_int;

  add_constraint(&ctx, tvar, tint);
  add_constraint(&ctx, tvar, tint);

  check_constraint_count(&ctx, 1, "Duplicate constraint not added");
}

static void test_add_constraint_multiple() {
  printf("\n--- test_add_constraint_multiple ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar1 = next_tvar();
  Type *tvar2 = next_tvar();
  Type *tint = &t_int;
  Type *tstring = &t_string;

  add_constraint(&ctx, tvar1, tint);
  add_constraint(&ctx, tvar2, tstring);

  check_constraint_count(&ctx, 2, "Multiple constraints added");
  check_constraint_exists(&ctx, tvar1, tint, "First constraint exists");
  check_constraint_exists(&ctx, tvar2, tstring, "Second constraint exists");
}

static void test_add_constraint_order() {
  printf("\n--- test_add_constraint_order ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar1 = next_tvar();
  Type *tvar2 = next_tvar();
  Type *tvar3 = next_tvar();
  Type *tint = &t_int;

  add_constraint(&ctx, tvar1, tint);
  add_constraint(&ctx, tvar2, tint);
  add_constraint(&ctx, tvar3, tint);

  check_constraint_count(&ctx, 3, "Three constraints added");

  int found_count = 0;
  for (Constraint *c = ctx.constraints; c != NULL; c = c->next) {
    if (types_equal(c->type, tint)) {
      found_count++;
    }
  }
  if (found_count == 3) {
    printf("✅ PASS: All constraints have correct type\n");
    tests_passed++;
  } else {
    printf("❌ FAIL: Expected 3 constraints with type Int, got %d\n",
           found_count);
    tests_failed++;
  }
}

static void test_add_constraint_null_handling() {
  printf("\n--- test_add_constraint_null_handling ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar = next_tvar();

  add_constraint(&ctx, NULL, tvar);
  add_constraint(&ctx, tvar, NULL);

  check_constraint_count(&ctx, 0, "Null constraints not added");
}

static void check_subst_contains(Subst *subst, const char *var, Type *type,
                                 const char *test_name) {
  for (Subst *s = subst; s != NULL; s = s->next) {
    if (strcmp(s->var, var) == 0 && types_equal(s->type, type)) {
      printf("✅ PASS: %s\n", test_name);
      tests_passed++;
      return;
    }
  }
  printf("❌ FAIL: %s (substitution not found)\n", test_name);
  tests_failed++;
}

static void test_solve_constraints_basic() {
  printf("\n--- test_solve_constraints_basic ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar = next_tvar();
  add_constraint(&ctx, tvar, &t_int);

  Subst *subst = solve_constraints(ctx.constraints);

  if (subst) {
    printf("✅ PASS: Substitution created\n");
    tests_passed++;
    check_subst_contains(subst, tvar->data.T_VAR, &t_int, "tvar mapped to Int");
  } else {
    printf("❌ FAIL: Substitution is NULL\n");
    tests_failed++;
  }
}

static void test_solve_constraints_multiple() {
  printf("\n--- test_solve_constraints_multiple ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar1 = next_tvar();
  Type *tvar2 = next_tvar();
  add_constraint(&ctx, tvar1, &t_int);
  add_constraint(&ctx, tvar2, &t_string);

  Subst *subst = solve_constraints(ctx.constraints);

  int count = 0;
  for (Subst *s = subst; s != NULL; s = s->next)
    count++;

  if (count == 2) {
    printf("✅ PASS: Two substitutions created\n");
    tests_passed++;
  } else {
    printf("❌ FAIL: Expected 2 substitutions, got %d\n", count);
    tests_failed++;
  }
}

static void test_solve_constraints_chained() {
  printf("\n--- test_solve_constraints_chained ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar1 = next_tvar();
  Type *tvar2 = next_tvar();

  add_constraint(&ctx, tvar1, tvar2);
  add_constraint(&ctx, tvar2, &t_int);

  Subst *subst = solve_constraints(ctx.constraints);

  if (subst) {
    printf("✅ PASS: Chained constraints solved\n");
    tests_passed++;
  } else {
    printf("❌ FAIL: Chained constraints failed to solve\n");
    tests_failed++;
  }
}

static void test_apply_substitution() {
  printf("\n--- test_apply_substitution ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar = next_tvar();
  add_constraint(&ctx, tvar, &t_int);

  Subst *subst = solve_constraints(ctx.constraints);

  Type *result = apply_substitution(subst, tvar);

  if (types_equal(result, &t_int)) {
    printf("✅ PASS: Substitution applied to type variable\n");
    tests_passed++;
  } else {
    printf("❌ FAIL: Substitution not applied correctly\n");
    tests_failed++;
  }
}

static void test_apply_substitution_nested() {
  printf("\n--- test_apply_substitution_nested ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar = next_tvar();
  Type *fn_type = type_fn(tvar, &t_int);
  // t_alloc(sizeof(Type));
  //   fn_type->kind = T_FN;
  //   fn_type->data.T_FN.from = tvar;
  //   fn_type->data.T_FN.to = &t_int;

  add_constraint(&ctx, tvar, &t_string);

  Subst *subst = solve_constraints(ctx.constraints);
  Type *result = apply_substitution(subst, fn_type);

  if (result && result->kind == T_FN && result->data.T_FN.from &&
      is_string_type(result->data.T_FN.from)) {
    printf("✅ PASS: Substitution applied to nested type\n");
    tests_passed++;
  } else {
    printf("❌ FAIL: Substitution not applied to nested type (kind=%d)\n",
           result ? result->kind : -1);
    tests_failed++;
  }
}

static void test_compose_subst() {
  printf("\n--- test_compose_subst ---\n");
  reset_type_var_counter();

  Subst *s1 = t_alloc(sizeof(Subst));
  s1->var = "t0";
  s1->type = &t_int;
  s1->next = NULL;

  Subst *s2 = t_alloc(sizeof(Subst));
  s2->var = "t1";
  s2->type = &t_string;
  s2->next = NULL;

  Subst *composed = compose_subst(s1, s2);

  int count = 0;
  for (Subst *s = composed; s != NULL; s = s->next)
    count++;

  if (count == 2) {
    printf("✅ PASS: Substitutions composed\n");
    tests_passed++;
  } else {
    printf("❌ FAIL: Expected 2 substitutions, got %d\n", count);
    tests_failed++;
  }
}

static void test_infer_solve() {
  printf("\n--- test_infer_solve ---\n");
  reset_type_var_counter();

  TICtx ctx = {0};
  ctx.env = NULL;
  ctx.constraints = NULL;

  Type *tvar = next_tvar();
  add_constraint(&ctx, tvar, &t_int);

  Solution sol = {0};
  int result = infer_solve(&ctx, &sol);

  if (result == 0 && sol.subst != NULL) {
    printf("✅ PASS: infer_solve returns solution\n");
    tests_passed++;
  } else {
    printf("❌ FAIL: infer_solve failed\n");
    tests_failed++;
  }
}

int main() {
  printf("=== Testing add_constraint ===\n");

  test_add_constraint_basic();
  test_add_constraint_duplicate();
  test_add_constraint_multiple();
  test_add_constraint_order();
  test_add_constraint_null_handling();

  printf("\n=== Testing solve_constraints ===\n");

  test_solve_constraints_basic();
  test_solve_constraints_multiple();
  test_solve_constraints_chained();
  test_apply_substitution();
  test_apply_substitution_nested();
  test_compose_subst();
  test_infer_solve();

  printf("\n=== Results: %d passed, %d failed ===\n", tests_passed,
         tests_failed);

  return tests_failed > 0 ? 1 : 0;
}
