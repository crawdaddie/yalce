#include "./infer_match_expression.h"
#include "serde.h"
#include "types/infer_binding.h"
#include "types/unification.h"

Type *infer_match_expression(Ast *ast, TICtx *ctx) {

  Type *scrutinee_type = infer(ast->data.AST_MATCH.expr, ctx);

  Type *result_type = next_tvar();

  int num_cases = ast->data.AST_MATCH.len;

  TypeEnv *branch_envs[num_cases];
  for (int i = 0; i < num_cases; i++) {
    Ast *pattern_ast = ast->data.AST_MATCH.branches + i * 2;

    if (pattern_ast->tag == AST_MATCH_GUARD_CLAUSE) {
      pattern_ast = pattern_ast->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    }

    TICtx c = *ctx;

    Type *pattern_type =
        bind_pattern_recursive(pattern_ast, scrutinee_type, &c);

    branch_envs[i] = c.env;

    if (unify(scrutinee_type, pattern_type, ctx)) {
      fprintf(stderr, "Error: cannot unify scrutinee with pattern type in "
                      "match expression\n");
      return NULL;
    }
  }

  Subst *subst = solve_constraints(ctx->constraints);
  ctx->subst = compose_subst(ctx->subst, subst);

  for (int i = 0; i < num_cases; i++) {
    branch_envs[i] = apply_subst_env(subst, branch_envs[i]);
  }

  for (int i = 0; i < num_cases; i++) {
    Ast *body_ast = ast->data.AST_MATCH.branches + i * 2 + 1;
    TICtx body_ctx = *ctx;
    body_ctx.env = branch_envs[i];

    Type *case_body_type = infer(body_ast, &body_ctx);

    if (!case_body_type) {
      return type_error(ctx, body_ast, "Cannot infer body type for case %d",
                        i + 1);
    }

    if (unify(result_type, case_body_type, &body_ctx)) {
      return type_error(ctx, body_ast, "Case %d returns incompatible type",
                        i + 1);
    }
    ctx->constraints = body_ctx.constraints;
  }

  ctx->subst = solve_constraints(ctx->constraints);

  Type *final_scrutinee = apply_substitution(ctx->subst, scrutinee_type);
  ast->data.AST_MATCH.expr->md = final_scrutinee;

  Type *final_result = apply_substitution(ctx->subst, result_type);

  return final_result;
}
