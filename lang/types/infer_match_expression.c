#include "./infer_match_expression.h"
#include "serde.h"
#include "types/type_ser.h"
Type *infer_match_expression__(Ast *ast, TICtx *ctx) {

  Type *scrutinee_type = infer(ast->data.AST_MATCH.expr, ctx);

  Type *result_type = next_tvar();

  int num_cases = ast->data.AST_MATCH.len;

  TypeEnv *branch_envs[num_cases];
  Type *pattern_types[num_cases];

  for (int i = 0; i < num_cases; i++) {
    Ast *pattern_ast = ast->data.AST_MATCH.branches + i * 2;

    Ast *guard = NULL;

    if (pattern_ast->tag == AST_MATCH_GUARD_CLAUSE) {
      guard = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
      pattern_ast = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    }

    TICtx c = {.env = ctx->env};
    // Type *pattern_type = infer(pattern_ast, ctx);

    bind_type_in_ctx(pattern_ast, scrutinee_type, (binding_md){}, &c);

    Type *pattern_type = pattern_ast->md;
    pattern_types[i] = pattern_type;
    unify(pattern_type, scrutinee_type, ctx);

    branch_envs[i] = c.env;

    if (guard) {
      infer(guard, &c);
    }

    ctx->constraints = merge_constraints(ctx->constraints, c.constraints);
  }

  Subst *subst = solve_constraints(ctx->constraints);
  ctx->subst = compose_subst(ctx->subst, subst);

  for (int i = 0; i < num_cases; i++) {
    branch_envs[i] = apply_subst_env(subst, branch_envs[i]);
  }
  Type *case_body_types[num_cases];

  for (int i = 0; i < num_cases; i++) {
    Ast *body_ast = ast->data.AST_MATCH.branches + i * 2 + 1;
    TICtx body_ctx = *ctx;
    body_ctx.env = branch_envs[i];

    Type *case_body_type = infer(body_ast, &body_ctx);
    case_body_types[i] = case_body_type;

    if (!case_body_type) {
      return type_error(ctx, body_ast, "Cannot infer body type for case %d",
                        i + 1);
    }

    if (unify(result_type, case_body_type, &body_ctx)) {
      return type_error(ctx, body_ast, "Case %d returns incompatible type",
                        i + 1);
    }

    if (i > 0) {
      if (pattern_types[i] != NULL && pattern_types[i - 1] != NULL) {
        unify(pattern_types[i], pattern_types[i - 1], &body_ctx);
      }
      unify(case_body_types[i], case_body_types[i - 1], &body_ctx);
    }

    ctx->constraints = body_ctx.constraints;
  }

  Subst *sols = solve_constraints(ctx->constraints);

  ctx->subst = compose_subst(ctx->subst, sols);

  Type *final_scrutinee = apply_substitution(ctx->subst, scrutinee_type);
  ast->data.AST_MATCH.expr->md = final_scrutinee;

  // Type *final_result = result_type;
  Type *final_result = apply_substitution(ctx->subst, result_type);

  for (int i = 0; i < num_cases; i++) {
    Ast *pattern_ast = ast->data.AST_MATCH.branches + i * 2;

    Ast *guard = NULL;
    if (pattern_ast->tag == AST_MATCH_GUARD_CLAUSE) {
      guard = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
      apply_substitution_to_lambda_body(guard, ctx->subst);
      pattern_ast = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.test_expr;
      apply_substitution_to_lambda_body(pattern_ast, ctx->subst);

    } else {
      apply_substitution_to_lambda_body(pattern_ast, ctx->subst);
    }
  }

  // print_ast(ast);
  // print_subst(ctx->subst);

  return final_result;
}

Type *infer_match_expression(Ast *ast, TICtx *ctx) {

  Type *scrutinee_type = infer(ast->data.AST_MATCH.expr, ctx);

  Type *result_type = next_tvar();

  int num_cases = ast->data.AST_MATCH.len;

  TypeEnv *branch_envs[num_cases];
  Type *pattern_types[num_cases];

  for (int i = 0; i < num_cases; i++) {
    Ast *pattern_ast = ast->data.AST_MATCH.branches + i * 2;

    Ast *guard = NULL;
    if (pattern_ast->tag == AST_MATCH_GUARD_CLAUSE) {
      guard = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
      pattern_ast = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    }

    TICtx c = *ctx;

    // Type *pattern_type =
    //     bind_pattern_recursive(pattern_ast, scrutinee_type, (binding_md){},
    //     &c);
    //
    bind_type_in_ctx(pattern_ast, scrutinee_type, (binding_md){}, &c);
    Type *pattern_type = pattern_ast->md;

    pattern_ast->md = pattern_type;
    pattern_types[i] = pattern_type;

    branch_envs[i] = c.env;

    if (unify(scrutinee_type, pattern_type, ctx)) {
      fprintf(stderr, "Error: cannot unify scrutinee with pattern type in "
                      "match expression\n");
      return NULL;
    }

    if (guard) {
      infer(guard, &c);
      (ast->data.AST_MATCH.branches + i * 2)->md = pattern_type;
    }

    // ctx->constraints = merge_constraints(ctx->constraints, c.constraints);
  }

  Subst *subst = solve_constraints(ctx->constraints);
  ctx->subst = compose_subst(ctx->subst, subst);

  for (int i = 0; i < num_cases; i++) {
    branch_envs[i] = apply_subst_env(subst, branch_envs[i]);
  }
  Type *case_body_types[num_cases];

  for (int i = 0; i < num_cases; i++) {
    Ast *body_ast = ast->data.AST_MATCH.branches + i * 2 + 1;
    TICtx body_ctx = *ctx;
    body_ctx.env = branch_envs[i];

    Type *case_body_type = infer(body_ast, &body_ctx);
    case_body_types[i] = case_body_type;

    if (!case_body_type) {
      return type_error(ctx, body_ast, "Cannot infer body type for case %d",
                        i + 1);
    }

    if (unify(result_type, case_body_type, &body_ctx)) {
      return type_error(ctx, body_ast, "Case %d returns incompatible type",
                        i + 1);
    }

    if (i > 0) {
      unify(pattern_types[i], pattern_types[i - 1], &body_ctx);
      unify(case_body_types[i], case_body_types[i - 1], &body_ctx);
    }

    ctx->constraints = body_ctx.constraints;
  }

  Subst *sols = solve_constraints(ctx->constraints);

  ctx->subst = compose_subst(ctx->subst, sols);

  Type *final_scrutinee = apply_substitution(ctx->subst, scrutinee_type);
  ast->data.AST_MATCH.expr->md = final_scrutinee;

  Type *final_result = apply_substitution(ctx->subst, result_type);

  for (int i = 0; i < num_cases; i++) {
    Ast *pattern_ast = ast->data.AST_MATCH.branches + i * 2;

    Ast *guard = NULL;
    if (pattern_ast->tag == AST_MATCH_GUARD_CLAUSE) {
      guard = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
      apply_substitution_to_lambda_body(guard, ctx->subst);
      pattern_ast = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.test_expr;
      apply_substitution_to_lambda_body(pattern_ast, ctx->subst);

    } else {
      apply_substitution_to_lambda_body(pattern_ast, ctx->subst);
    }
    apply_substitution_to_lambda_body(ast->data.AST_MATCH.branches + i * 2 + 1,
                                      ctx->subst);
  }

  return final_result;
}
