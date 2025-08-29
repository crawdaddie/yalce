
#include "./infer_app.h"
#include "types/unification.h"

Type *infer_cons_application(Ast *ast, Scheme *cons_scheme, TICtx *ctx) {
  Type *inst =
      instantiate_with_args(cons_scheme, ast->data.AST_APPLICATION.args, ctx);
  return inst;
}

Type *infer_app(Ast *ast, TICtx *_ctx) {
  Ast *func = ast->data.AST_APPLICATION.function;
  Ast *args = ast->data.AST_APPLICATION.args;
  int num_args = ast->data.AST_APPLICATION.len;

  if (func->tag == AST_IDENTIFIER) {
    Scheme *s = lookup_scheme(_ctx->env, func->data.AST_IDENTIFIER.value);
    if (!s) {
      return NULL;
    }
    if (s->type->kind == T_CONS) {
      return infer_cons_application(ast, s, _ctx);
    }
  }

  // Step 1: Infer function type
  Type *func_type = infer(func, _ctx);
  if (!func_type) {
    return type_error(_ctx, ast, "Cannot infer function type");
  }

  // Step 2: Infer argument types
  Type **arg_types = talloc(sizeof(Type *) * num_args);
  for (int i = 0; i < num_args; i++) {
    arg_types[i] = infer(&args[i], _ctx);
    if (!arg_types[i]) {
      return type_error(_ctx, ast, "Cannot infer argument %d type", i + 1);
    }
  }

  // Step 3: Create expected function type
  Type *result_type = next_tvar();
  Type *expected_type = result_type;

  // Build expected type: arg1 -> arg2 -> ... -> result
  for (int i = num_args - 1; i >= 0; i--) {
    expected_type = type_fn(arg_types[i], expected_type);
  }

  // Step 4: Unify function type with expected type
  TICtx unify_ctx = {};
  if (unify(func_type, expected_type, &unify_ctx)) {
    print_type_err(func_type);
    print_type_err(expected_type);
    return type_error(_ctx, ast, "Function application type mismatch");
  }

  print_type_env(_ctx->env);
  print_constraints(unify_ctx.constraints);

  // Step 5: Solve constraints and apply substitutions
  Subst *solution = solve_constraints(unify_ctx.constraints);
  if (solution) {
    _ctx->subst = compose_subst(solution, _ctx->subst);
    return apply_substitution(solution, result_type);
  }

  return result_type;
}
