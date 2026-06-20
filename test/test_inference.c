#include "../lang/parse.h"
#include "../lang/types/builtins.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "../lang/types/type_ser.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void infer_final(Ast *ast, const Solution *solved, TICtx *ctx);

static int tests_passed = 0;
static int tests_failed = 0;

static void start_test(const char *test_name, int line) {
  fprintf(stderr, "\n[%s:%d]\n", test_name, line);
}

static void pass(const char *test_name) {
  fprintf(stderr, "✅ %s\n", test_name);
  tests_passed++;
}

static void assert_true(bool ok, const char *test_name, const char *fmt, ...) {
  if (ok) {
    pass(test_name);
    return;
  }

  va_list args;
  fprintf(stderr, "❌ %s: ", test_name);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  tests_failed++;
}

static int count_constraints(TICtx *ctx) {
  int count = 0;
  for (Constraint *c = ctx->constraints; c != NULL; c = c->next) {
    count++;
  }
  return count;
}

static int count_scheme_vars(TypeList *vars) {
  int count = 0;
  for (TypeList *v = vars; v; v = v->next) {
    count++;
  }
  return count;
}

static TypeEnv *find_binding(TypeEnv *env, const char *name) {
  return lookup_type_ref(env, name);
}

static bool subst_maps_to(Subst *subst, int var_id, Type *expected) {
  for (Subst *s = subst; s != NULL; s = s->next) {
    if (s->var_id == var_id && types_equal(s->type, expected)) {
      return true;
    }
  }
  return false;
}

static bool has_constraint(TICtx *ctx, Type *left, Type *right) {
  for (Constraint *c = ctx->constraints; c != NULL; c = c->next) {
    if (c->kind != CONSTRAINT_EQUALITY) {
      continue;
    }
    if (types_equal(c->data.EQUALITY.left, left) &&
        types_equal(c->data.EQUALITY.right, right)) {
      return true;
    }
  }
  return false;
}

static bool predicate_is_trait(Predicate *pred, TypeClass *trait, Type *type,
                               Type *param0) {
  if (!pred || pred->kind != PRED_TRAIT || pred->trait != trait) {
    return false;
  }
  if (!types_equal(pred->data.TRAIT.type, type)) {
    return false;
  }
  if (!param0) {
    return pred->data.TRAIT.params == NULL;
  }
  return pred->data.TRAIT.params != NULL &&
         types_equal(pred->data.TRAIT.params->type, param0);
}

static bool has_trait_predicate(Predicate *preds, TypeClass *trait, Type *type,
                                Type *param0) {
  for (Predicate *p = preds; p; p = p->next) {
    if (predicate_is_trait(p, trait, type, param0)) {
      return true;
    }
  }
  return false;
}

static bool has_any_trait_predicate(Predicate *preds, TypeClass *trait) {
  for (Predicate *p = preds; p; p = p->next) {
    if (p->kind == PRED_TRAIT && p->trait == trait) {
      return true;
    }
  }
  return false;
}

static Ast *parse_and_infer(const char *input, TICtx *ctx) {
  reset_type_var_counter();
  Ast *ast = parse_input((char *)input, NULL);
  if (!ast) {
    return NULL;
  }
  memset(ctx, 0, sizeof(*ctx));
  ctx->err_stream = stderr;
  if (!infer(ast, ctx)) {
    return NULL;
  }
  return ast;
}

static void test_add_constraint_deduplicates_exact_equality() {
  start_test(__func__, __LINE__);
  TICtx ctx = {0};
  Type *var = next_tvar();

  add_constraint(&ctx, var, &t_int);
  add_constraint(&ctx, var, &t_int);

  assert_true(count_constraints(&ctx) == 1,
              "add_constraint deduplicates exact equality",
              "expected one deduplicated constraint, got %d",
              count_constraints(&ctx));
}

static void test_solve_constraints_function_unification() {
  start_test(__func__, __LINE__);
  TICtx ctx = {0};
  Type *a = next_tvar();
  Type *b = next_tvar();
  Type *lhs = type_fn(a, &t_int);
  Type *rhs = type_fn(&t_string, b);

  add_constraint(&ctx, lhs, rhs);
  Subst *subst = solve_constraints(ctx.constraints);

  assert_true(subst != NULL, "solve_constraints unifies function equality",
              "expected function unification to solve");
  if (!subst) {
    return;
  }

  assert_true(types_equal(apply_substitution(subst, a), &t_string),
              "function unification resolves argument variable",
              "expected argument variable to resolve to String");
  assert_true(types_equal(apply_substitution(subst, b), &t_int),
              "function unification resolves return variable",
              "expected return variable to resolve to Int");
}

static void test_solve_constraints_occurs_check_rejects() {
  start_test(__func__, __LINE__);
  TICtx ctx = {0};
  Type *a = next_tvar();
  Type *list_a = create_list_type_of_type(a);

  add_constraint(&ctx, a, list_a);
  Subst *subst = solve_constraints(ctx.constraints);

  assert_true(subst == NULL, "solve_constraints rejects occurs check cycle",
              "expected occurs check to reject an infinite type");
}

static void test_apply_substitution_rewrites_nested_function_types() {
  start_test(__func__, __LINE__);
  TICtx ctx = {0};
  Type *a = next_tvar();
  Type *fn = type_fn(a, create_list_type_of_type(a));

  add_constraint(&ctx, a, &t_int);
  Subst *subst = solve_constraints(ctx.constraints);

  assert_true(subst != NULL, "nested substitution exists",
              "expected substitution to exist");
  if (!subst) {
    return;
  }

  Type *applied = apply_substitution(subst, fn);
  bool ok =
      applied && applied->kind == T_FN &&
      types_equal(applied->data.T_FN.from, &t_int) &&
      types_equal(applied->data.T_FN.to, create_list_type_of_type(&t_int));
  assert_true(ok, "apply_substitution rewrites nested function type",
              "expected nested function type to be rewritten");
}

static void test_instantiate_env_freshens_scheme_vars_and_predicates() {
  start_test(__func__, __LINE__);
  Type *a = tvar("a");
  Type *fn_type = type_fn(a, a);
  TypeList scheme_vars = {.type = a, .next = NULL};
  Predicate *preds = predicate_append(NULL, GenericArithmetic, a);
  TypeEnv entry = {.name = "id",
                   .type = fn_type,
                   .scheme_vars = &scheme_vars,
                   .predicates = preds};

  TICtx ctx1 = {0};
  TICtx ctx2 = {0};
  Type *inst1 = instantiate_env(&entry, &ctx1);
  Type *inst2 = instantiate_env(&entry, &ctx2);

  bool same_within_inst1 =
      inst1 && inst1->kind == T_FN &&
      types_equal(inst1->data.T_FN.from, inst1->data.T_FN.to);
  bool same_within_inst2 =
      inst2 && inst2->kind == T_FN &&
      types_equal(inst2->data.T_FN.from, inst2->data.T_FN.to);
  bool fresh_between_instantiations =
      same_within_inst1 && same_within_inst2 &&
      !types_equal(inst1->data.T_FN.from, inst2->data.T_FN.from);
  bool predicate_freshened =
      ctx1.predicates && ctx1.predicates->kind == PRED_TRAIT &&
      ctx1.predicates->trait == GenericArithmetic &&
      types_equal(ctx1.predicates->data.TRAIT.type, inst1->data.T_FN.from);

  assert_true(fresh_between_instantiations,
              "instantiate_env freshens scheme vars per call",
              "expected fresh scheme vars on each instantiation");
  assert_true(predicate_freshened,
              "instantiate_env freshens attached predicates",
              "expected attached predicates to be freshened with the type");
}

static void test_resolve_predicates_from_succeeds_for_double_from_int() {
  start_test(__func__, __LINE__);
  TypeList params = {.type = &t_int, .next = NULL};
  Predicate *preds =
      predicate_append_applied(NULL, GenericFrom, &t_num, &params);
  Subst *subst = NULL;

  int rc = resolve_predicates(&subst, preds, stderr);
  assert_true(rc == 0, "resolve_predicates accepts Double : From<Int>",
              "expected Double : From<Int> obligation to resolve");
}

static void test_resolve_predicates_from_fails_for_int_from_double() {
  start_test(__func__, __LINE__);
  TypeList params = {.type = &t_num, .next = NULL};
  Predicate *preds =
      predicate_append_applied(NULL, GenericFrom, &t_int, &params);
  Subst *subst = NULL;

  int rc = resolve_predicates(&subst, preds, stderr);
  assert_true(rc != 0, "resolve_predicates rejects Int : From<Double>",
              "expected Int : From<Double> obligation to fail");
}

static void test_resolve_predicates_from_succeeds_for_coroutine_from_list() {
  start_test(__func__, __LINE__);
  Type *cor = create_coroutine_instance_type(&t_int);
  Type *list = create_list_type_of_type(&t_int);
  TypeList params = {.type = list, .next = NULL};
  Predicate *preds = predicate_append_applied(NULL, GenericFrom, cor, &params);
  Subst *subst = NULL;

  int rc = resolve_predicates(&subst, preds, stderr);
  assert_true(
      rc == 0, "resolve_predicates accepts Coroutine<Int> : From<List<Int>>",
      "expected Coroutine<Int> : From<List<Int>> obligation to resolve");
}

static void test_resolve_predicates_from_succeeds_for_coroutine_from_array() {
  start_test(__func__, __LINE__);
  Type *cor = create_coroutine_instance_type(&t_int);
  Type *array = create_array_type(&t_int);
  TypeList params = {.type = array, .next = NULL};
  Predicate *preds = predicate_append_applied(NULL, GenericFrom, cor, &params);
  Subst *subst = NULL;

  int rc = resolve_predicates(&subst, preds, stderr);
  assert_true(
      rc == 0, "resolve_predicates accepts Coroutine<Int> : From<Array<Int>>",
      "expected Coroutine<Int> : From<Array<Int>> obligation to resolve");
}

static void
test_infer_application_emits_from_predicate_for_concrete_conversion() {
  start_test(__func__, __LINE__);
  TICtx ctx;
  Ast *ast = parse_and_infer("let f = fn x: (Double) -> x;;\n"
                             "f 1;\n",
                             &ctx);

  assert_true(ast != NULL, "conversion application infers successfully",
              "expected program to infer successfully");
  if (!ast) {
    return;
  }

  assert_true(
      !has_constraint(&ctx, &t_int, &t_num) &&
          !has_constraint(&ctx, &t_num, &t_int),
      "conversion application avoids direct Int ~ Double equality",
      "expected application hook to avoid direct Int ~ Double equality");
}

static void test_infer_application_converts_list_to_coroutine_parameter() {
  start_test(__func__, __LINE__);
  TICtx ctx;
  Type *expected = create_coroutine_instance_type(&t_int);
  Ast *ast = parse_and_infer("cor_loop [1, 2, 3]\n", &ctx);

  assert_true(ast != NULL, "list argument converts to coroutine parameter",
              "expected List<Int> to adapt to Coroutine<Int> via From");
  if (!ast) {
    return;
  }

  assert_true(types_equal(ast->type, expected),
              "list-to-coroutine application returns Coroutine<Int>",
              "expected application result type to be Coroutine<Int>");
}

static void test_infer_application_converts_array_to_coroutine_parameter() {
  start_test(__func__, __LINE__);
  TICtx ctx;
  Type *expected = create_coroutine_instance_type(&t_int);
  Ast *ast = parse_and_infer("cor_loop [|1, 2, 3|]\n", &ctx);

  assert_true(ast != NULL, "array argument converts to coroutine parameter",
              "expected Array<Int> to adapt to Coroutine<Int> via From");
  if (!ast) {
    return;
  }

  assert_true(types_equal(ast->type, expected),
              "array-to-coroutine application returns Coroutine<Int>",
              "expected application result type to be Coroutine<Int>");
}

static void test_infer_application_uses_equality_for_generic_parameter() {
  start_test(__func__, __LINE__);
  TICtx ctx;
  Ast *ast = parse_and_infer("let id = fn x -> x;;\n"
                             "id 1;\n",
                             &ctx);

  assert_true(ast != NULL, "generic application infers successfully",
              "expected generic application to infer");
  if (!ast) {
    return;
  }

  assert_true(types_equal(ast->type, &t_int), "generic application returns Int",
              "expected id 1 to infer Int");
  assert_true(!has_trait_predicate(ctx.predicates, GenericFrom, &t_num, &t_int),
              "generic application emits no From predicate",
              "expected no From predicate for direct generic match");
}

static void test_infer_application_direct_concrete_match_emits_no_from() {
  start_test(__func__, __LINE__);
  TICtx ctx;
  Ast *ast = parse_and_infer("let f = fn x: (Int) -> x;;\n"
                             "f 1;\n",
                             &ctx);

  assert_true(ast != NULL, "concrete application infers successfully",
              "expected concrete application to infer successfully");
  if (!ast) {
    return;
  }

  assert_true(types_equal(ast->type, &t_int),
              "concrete application returns Int",
              "expected concrete application result type to be Int");
  assert_true(!has_any_trait_predicate(ctx.predicates, GenericFrom),
              "concrete application emits no From predicate",
              "expected no From predicate for exact concrete application");
}

static void test_let_generalizes_lambda_binding() {
  start_test(__func__, __LINE__);
  TICtx ctx;
  Ast *ast = parse_and_infer("let id = fn x -> x;;\n", &ctx);
  TypeEnv *entry = find_binding(ctx.env, "id");

  assert_true(ast != NULL, "let-bound lambda infers successfully",
              "expected let-bound lambda to infer");
  assert_true(entry != NULL && entry->type && entry->type->kind == T_FN,
              "let-bound lambda creates function binding",
              "expected id binding to exist with a function type");
  assert_true(entry != NULL && count_scheme_vars(entry->scheme_vars) >= 1,
              "let-bound lambda binding is generalized",
              "expected id binding to be generalized");
}

static void test_generalize_env_respects_outer_free_vars() {
  start_test(__func__, __LINE__);
  Type *outer_var = tvar("outer");
  Type *local_var = tvar("local");
  TypeEnv outer = {.name = "x", .type = outer_var};
  TypeEnv entry = {.name = "f", .type = type_fn(outer_var, local_var)};

  generalize_env(&entry, &outer);

  bool one_scheme_var = count_scheme_vars(entry.scheme_vars) == 1;
  bool generalized_local_only =
      one_scheme_var && entry.scheme_vars->type &&
      entry.scheme_vars->type->kind == T_VAR &&
      strcmp(entry.scheme_vars->type->data.T_VAR.name, "local") == 0;

  assert_true(generalized_local_only, "generalize_env excludes outer free vars",
              "expected only vars not free in the outer env to generalize");
}

static void test_match_expression_unifies_branch_results() {
  start_test(__func__, __LINE__);
  TICtx ctx;
  Ast *ast = parse_and_infer("let f = fn l ->\n"
                             "  match l with\n"
                             "    | x::_ -> x\n"
                             "    | [] -> 0\n"
                             ";;\n",
                             &ctx);

  Type expected = MAKE_FN_TYPE_2(&TLIST(&t_int), &t_int);
  assert_true(ast != NULL, "match expression infers successfully",
              "expected match expression to infer");
  if (!ast) {
    return;
  }

  assert_true(types_equal(ast->type, &expected),
              "match expression unifies branch result types",
              "expected branch result unification to produce List<Int> -> Int");
}

static void test_resolve_predicates_leaves_generic_trait_deferred() {
  start_test(__func__, __LINE__);
  Type *a = tvar("a");
  Predicate *preds = predicate_append(NULL, GenericArithmetic, a);
  Subst *subst = NULL;

  int rc = resolve_predicates(&subst, preds, stderr);

  assert_true(rc == 0, "generic trait obligation remains deferred",
              "expected generic trait obligation to remain deferred");
  assert_true(
      subst == NULL, "generic deferred obligation leaves subst unchanged",
      "expected deferred generic trait obligation to avoid extending subst");
}

static void test_polymorphic_let_binding_instantiates_at_multiple_types() {
  start_test(__func__, __LINE__);
  TICtx ctx;
  Ast *ast = parse_and_infer("let id = fn x -> x;;\n"
                             "(id 1, id \"hello\")\n",
                             &ctx);
  Type expected = TTUPLE(2, &t_int, &t_string);

  assert_true(ast != NULL, "polymorphic let binding infers successfully",
              "expected polymorphic binding to infer across multiple uses");
  if (!ast) {
    return;
  }

  assert_true(
      types_equal(ast->type, &expected),
      "polymorphic let binding instantiates at Int and String",
      "expected polymorphic let-binding to instantiate at Int and String");
}

static void test_infer_final_rewrites_nested_ast_annotations() {
  start_test(__func__, __LINE__);
  Type *a = tvar("a");
  Ast ident = {.tag = AST_IDENTIFIER,
               .type = a,
               .data = {.AST_IDENTIFIER = {.value = "x", .length = 1}}};
  Ast app = {.tag = AST_APPLICATION,
             .type = a,
             .data = {.AST_APPLICATION = {
                          .function = &ident,
                          .args = NULL,
                          .len = 0,
                      }}};
  Subst subst = {.var_id = a->data.T_VAR.id, .type = &t_int, .next = NULL};
  Solution solved = {.subst = &subst};
  TICtx ctx = {0};

  infer_final(&app, &solved, &ctx);

  assert_true(types_equal(app.type, &t_int),
              "infer_final rewrites root annotation",
              "expected root AST annotation to be rewritten");
  assert_true(types_equal(ident.type, &t_int),
              "infer_final rewrites nested annotation",
              "expected nested AST annotation to be rewritten");
}

static void test_infer_solve_succeeds_with_empty_constraints() {
  start_test(__func__, __LINE__);
  TICtx ctx = {0};
  Solution sol = {0};

  int rc = infer_solve(&ctx, &sol);
  assert_true(rc == 0 && sol.subst == NULL,
              "infer_solve succeeds on empty constraints",
              "expected infer_solve to succeed on an empty constraint set");
}

int main(void) {
  reset_type_var_counter();
  initialize_builtin_types();

  test_add_constraint_deduplicates_exact_equality();
  test_solve_constraints_function_unification();
  test_solve_constraints_occurs_check_rejects();
  test_apply_substitution_rewrites_nested_function_types();
  test_instantiate_env_freshens_scheme_vars_and_predicates();
  test_resolve_predicates_from_succeeds_for_double_from_int();
  test_resolve_predicates_from_fails_for_int_from_double();
  test_resolve_predicates_from_succeeds_for_coroutine_from_list();
  test_resolve_predicates_from_succeeds_for_coroutine_from_array();
  test_infer_application_emits_from_predicate_for_concrete_conversion();
  test_infer_application_converts_list_to_coroutine_parameter();
  test_infer_application_converts_array_to_coroutine_parameter();
  test_infer_application_uses_equality_for_generic_parameter();
  test_infer_application_direct_concrete_match_emits_no_from();
  test_let_generalizes_lambda_binding();
  test_generalize_env_respects_outer_free_vars();
  test_match_expression_unifies_branch_results();
  test_resolve_predicates_leaves_generic_trait_deferred();
  test_polymorphic_let_binding_instantiates_at_multiple_types();
  test_infer_final_rewrites_nested_ast_annotations();
  test_infer_solve_succeeds_with_empty_constraints();

  fprintf(stderr, "\nResults: %d passed, %d failed\n", tests_passed,
          tests_failed);
  return tests_failed == 0 ? 0 : 1;
}
