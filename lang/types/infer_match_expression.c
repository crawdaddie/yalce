#include "./infer_match_expression.h"
#include "serde.h"
#include "types/infer_binding.h"
#include "types/unification.h"

Type *__infer_match_expression(Ast *ast, TICtx *ctx) {

  Type *scrutinee_type = infer(ast->data.AST_MATCH.expr, ctx);
  if (!scrutinee_type) {
    scrutinee_type = next_tvar();
  }

  Type *result_type = next_tvar();

  int num_cases = ast->data.AST_MATCH.len;

  for (int i = 0; i < num_cases; i++) {

    Ast *pattern_ast = ast->data.AST_MATCH.branches + i * 2;

    Ast *body_ast = ast->data.AST_MATCH.branches + i * 2 + 1;

    Subst *current_subst = ctx->subst;
    TypeEnv *env_subst = apply_subst_env(current_subst, ctx->env);
    Type *scrutinee_subst = apply_substitution(current_subst, scrutinee_type);

    // Step 5: CREATE EXPECTED PATTERN TYPE aGd bind pattern to it
    Type *expected_pattern_type = next_tvar();

    TypeEnv *case_env = env_subst;
    Type *pattern_result = bind_pattern_recursive(
        pattern_ast, expected_pattern_type, &case_env, ctx);

    if (!pattern_result) {
      return type_error(ctx, pattern_ast, "Pattern binding failed in case %d",
                        i + 1);
    }

    UnifyResult pattern_ur = {.subst = NULL, .constraints = NULL, .inf = NULL};
    if (unify(scrutinee_subst, expected_pattern_type, &pattern_ur)) {
      return type_error(ctx, pattern_ast,
                        "Pattern in case %d incompatible with scrutinee type",
                        i + 1);
    }

    Subst *pattern_constraint_solution = NULL;
    if (pattern_ur.constraints) {
      pattern_constraint_solution = solve_constraints(pattern_ur.constraints);
      if (!pattern_constraint_solution) {
        return type_error(ctx, pattern_ast,
                          "Cannot solve pattern constraints for case %d",
                          i + 1);
      }
    }

    Subst *pattern_combined_subst =
        compose_subst(pattern_constraint_solution, pattern_ur.subst);
    ctx->subst = compose_subst(pattern_combined_subst, ctx->subst);

    case_env = apply_subst_env(ctx->subst, case_env);

    Type *updated_scrutinee = apply_substitution(ctx->subst, scrutinee_type);

    // Step 10: Infer body type in the extended environment
    TICtx case_ctx = {
        .env = case_env, .subst = ctx->subst, .err_stream = ctx->err_stream};

    Type *case_body_type = infer(body_ast, &case_ctx);
    if (!case_body_type) {
      return type_error(ctx, body_ast, "Cannot infer body type for case %d",
                        i + 1);
    }

    UnifyResult body_ur = {.subst = NULL, .constraints = NULL, .inf = NULL};
    if (unify(result_type, case_body_type, &body_ur)) {
      return type_error(ctx, body_ast, "Case %d returns incompatible type",
                        i + 1);
    }

    Subst *body_constraint_solution = NULL;
    if (body_ur.constraints) {
      body_constraint_solution = solve_constraints(body_ur.constraints);
      if (!body_constraint_solution) {
        return type_error(ctx, body_ast,
                          "Cannot solve body constraints for case %d", i + 1);
      }
    }

    Subst *body_combined_subst =
        compose_subst(body_constraint_solution, body_ur.subst);
    ctx->subst = compose_subst(body_combined_subst, case_ctx.subst);
  }

  Type *final_result = apply_substitution(ctx->subst, result_type);
  Type *final_scrutinee = apply_substitution(ctx->subst, scrutinee_type);

  return final_result;
}

Type *infer_match_expression(Ast *ast, TICtx *ctx) {

  Type *scrutinee_type = infer(ast->data.AST_MATCH.expr, ctx);

  Type *result_type = next_tvar();

  int num_cases = ast->data.AST_MATCH.len;

  UnifyResult pattern_unification = {};

  TypeEnv *branch_envs[num_cases];
  for (int i = 0; i < num_cases; i++) {
    Ast *pattern_ast = ast->data.AST_MATCH.branches + i * 2;

    if (pattern_ast->tag == AST_MATCH_GUARD_CLAUSE) {
      pattern_ast = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    }

    TypeEnv *env = ctx->env;

    Type *pattern_type =
        bind_pattern_recursive(pattern_ast, scrutinee_type, &env, ctx);

    branch_envs[i] = env;

    if (unify(scrutinee_type, pattern_type, &pattern_unification)) {
      fprintf(stderr, "Error: cannot unify scrutinee with pattern type in "
                      "match expression\n");
      return NULL;
    }
  }

  Subst *subst;
  if (pattern_unification.constraints) {

    subst = solve_constraints(pattern_unification.constraints);
    ctx->subst = compose_subst(ctx->subst, subst);
  }

  Type *final_scrutinee = apply_substitution(ctx->subst, scrutinee_type);

  for (int i = 0; i < num_cases; i++) {
    branch_envs[i] = apply_subst_env(subst, branch_envs[i]);
  }

  UnifyResult body_ur = {.subst = NULL, .constraints = NULL, .inf = NULL};

  for (int i = 0; i < num_cases; i++) {
    Ast *body_ast = ast->data.AST_MATCH.branches + i * 2 + 1;
    TICtx body_ctx = *ctx;
    body_ctx.env = branch_envs[i];
    body_ctx.subst = body_ur.subst;

    Type *case_body_type = infer(body_ast, &body_ctx);

    if (!case_body_type) {
      return type_error(ctx, body_ast, "Cannot infer body type for case %d",
                        i + 1);
    }

    if (unify(result_type, case_body_type, &body_ur)) {
      return type_error(ctx, body_ast, "Case %d returns incompatible type",
                        i + 1);
    }
    body_ur.subst = body_ctx.subst;
  }

  Subst *body_constraint_solution;
  if (body_ur.constraints) {
    body_constraint_solution = solve_constraints(body_ur.constraints);
  }

  Subst *body_combined_subst =
      compose_subst(body_constraint_solution, body_ur.subst);

  ctx->subst = compose_subst(body_combined_subst, ctx->subst);

  Type *final_result = apply_substitution(ctx->subst, result_type);
  return final_result;
}
