#include "./infer_let.h"

static Predicate *predicate_filter_generic(Predicate *preds) {
  Predicate *result = NULL;
  for (Predicate *p = preds; p; p = p->next) {
    if (!predicate_is_generic(p)) {
      continue;
    }
    if (p->kind == PRED_TRAIT) {
      result = predicate_append_applied(result, p->trait, p->data.TRAIT.type,
                                        p->data.TRAIT.params);
    } else if (p->kind == PRED_COMPARABLE) {
      int n = 0;
      while (p->data.COMPARABLE.args && p->data.COMPARABLE.args[n]) {
        n++;
      }
      Type **args = t_alloc(sizeof(Type *) * (n + 1));
      for (int i = 0; i < n; i++) {
        args[i] = p->data.COMPARABLE.args[i];
      }
      args[n] = NULL;
      result = predicate_append_comparable(result, p->trait,
                                           p->data.COMPARABLE.witness, args);
    }
  }
  return result;
}

void finalize_env_slice(TypeEnv *slice_head, TypeEnv *boundary, Subst *subst) {
  int len = 0;
  for (TypeEnv *e = slice_head; e != boundary; e = e->next) {
    len++;
  }

  TypeEnv **entries = len ? t_alloc(sizeof(TypeEnv *) * len) : NULL;
  int i = len - 1;
  for (TypeEnv *e = slice_head; e != boundary; e = e->next, i--) {
    entries[i] = e;
  }

  for (int j = 0; j < len; j++) {
    TypeEnv *e = entries[j];
    e->type = apply_subst_to_type(subst, e->type);
    if (e->needs_generalization) {
      e->scheme_vars = NULL;
      if (e->can_generalize) {
        generalize_env(e, e->generalize_boundary);
      }
      e->needs_generalization = false;
    }
  }
}

int checkpoint_generalizable_slice(TypeEnv *slice_head, TypeEnv *boundary,
                                   TICtx *ctx) {
  Solution sol = {0};
  if (infer_solve(ctx, &sol) != 0) {
    return 1;
  }

  Subst *step_subst = sol.subst;
  Predicate *remaining_preds = NULL;
  if (ctx->predicates) {
    Predicate *resolved = predicate_apply_subst(step_subst, ctx->predicates);
    if (resolve_predicates(&step_subst, resolved, ctx->err_stream) != 0) {
      return 1;
    }
    remaining_preds = predicate_filter_generic(resolved);
  }

  ctx->subst = compose_subst(step_subst, ctx->subst);
  apply_subst_env(ctx->subst, ctx->env);
  for (TypeEnv *e = slice_head; e != boundary; e = e->next) {
    if (e->can_generalize) {
      e->predicates = predicate_duplicate(remaining_preds);
    }
  }
  ctx->predicates = NULL;
  finalize_env_slice(slice_head, boundary, NULL);
  ctx->constraints = NULL;
  return 0;
}

/*
 * Mark a contiguous stack slice of env entries as eligible for deferred
 * generalization.
 *
 * Why this exists:
 * During inference we bind names into the environment before equality
 * constraints have been solved. At that point, many types still contain raw
 * inference variables that will later be substituted to concrete or more
 * precise types.
 *
 * If we generalized eagerly when inserting into the env, we would freeze those
 * unsolved variables too early. That caused previous regressions where lambda
 * parameters, destructuring binds, and other local names were turned into
 * polymorphic bindings prematurely, so later reads would instantiate fresh
 * copies instead of refining the original variable through constraints.
 *
 * The safe approach is:
 * 1. Insert env entries monomorphically during inference.
 * 2. Remember which newly-added entries are true let/module export bindings
 *    that should become polymorphic later.
 * 3. After solving, apply substitution to their types and only then compute
 *    scheme_vars via generalize_env().
 *
 * `slice_head` is the current env head after introducing the new bindings.
 * `boundary` is the env pointer from before those bindings were introduced.
 * Because the env is a linked stack, iterating from `slice_head` down to
 * `boundary` gives exactly the bindings introduced by this let/module scope.
 *
 * For each entry in that slice we store:
 * - `can_generalize = true`: this binding is allowed to become polymorphic.
 * - `needs_generalization = true`: post-solve finalization still needs to run.
 * - `generalize_boundary = boundary`: when we later compute
 *   free(type) - free(env), we must subtract the env that was in scope at the
 *   binding site, not the final top-level env. Using the wrong boundary would
 *   over-generalize captured outer variables.
 *
 * Note that this helper only marks bindings. It does not mutate their type or
 * compute scheme vars. That work is intentionally deferred to the post-solve
 * finalization pass.
 */
void mark_generalizable_slice(TypeEnv *slice_head, TypeEnv *boundary) {
  for (TypeEnv *e = slice_head; e != boundary; e = e->next) {
    e->can_generalize = true;
    e->needs_generalization = true;
    e->generalize_boundary = boundary;
  }
}
Type *infer_let_expr(Ast *ast, TICtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  Ast *body = ast->data.AST_LET.in_expr;
  TypeEnv *outer_env = ctx->env;

  Type *expr_type = infer_expr(expr, ctx);
  if (!expr_type) {
    return NULL;
  }

  if (bind_pattern(binding, expr_type, ctx) != 0) {
    type_error(ast, "Unsupported let binding shape");
    return NULL;
  }

  set_env_slice_scope(ctx->env, outer_env, ctx->scope);
  if (ctx->current_fn_ast) {
    set_env_slice_yield_boundary(
        ctx->env, outer_env, ctx->current_fn_ast->data.AST_LAMBDA.num_yields);
  }

  mark_generalizable_slice(ctx->env, outer_env);
  if (checkpoint_generalizable_slice(ctx->env, outer_env, ctx) != 0) {
    ctx->env = outer_env;
    return NULL;
  }

  if (body) {
    Type *body_type = infer_expr(body, ctx);
    ctx->env = outer_env;
    return body_type;
  }

  return expr_type;
}
