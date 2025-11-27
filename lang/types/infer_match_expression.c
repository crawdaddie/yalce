#include "./infer_match_expression.h"
#include "serde.h"
#include "types/type_ser.h"
#include <stdio.h>

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
    Type *pattern_type = pattern_ast->type;

    pattern_ast->type = pattern_type;
    pattern_types[i] = pattern_type;

    branch_envs[i] = c.env;

    if (unify(scrutinee_type, pattern_type, ctx)) {
      fprintf(stderr, "Error: cannot unify scrutinee with pattern type in "
                      "match expression\n");
      return NULL;
    }

    if (guard) {
      infer(guard, &c);
      (ast->data.AST_MATCH.branches + i * 2)->type = pattern_type;
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
      return type_error(body_ast, "Cannot infer body type for case %d", i + 1);
    }

    if (unify(result_type, case_body_type, &body_ctx)) {
      return type_error(body_ast, "Case %d returns incompatible type", i + 1);
    }

    if (i > 0) {
      unify(pattern_types[i], pattern_types[i - 1], &body_ctx);
      unify(case_body_types[i], case_body_types[i - 1], &body_ctx);
    }
    // printf("body %d: ", i);
    // print_constraints(body_ctx.constraints);

    ctx->constraints = body_ctx.constraints;
  }

  Subst *sols = solve_constraints(ctx->constraints);
  // print_subst(sols);

  ctx->subst = compose_subst(ctx->subst, sols);

  // printf("after match\n");
  // print_constraints(ctx->constraints);
  Type *final_scrutinee = apply_substitution(ctx->subst, scrutinee_type);
  ast->data.AST_MATCH.expr->type = final_scrutinee;

  Type *final_result = apply_substitution(ctx->subst, result_type);

  if (ctx->scope == 0) {
    apply_substitution_to_lambda_body(ast, ctx->subst);
  }
  return final_result;
}
