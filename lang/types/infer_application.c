#include "./infer_application.h"
#include "./builtins.h"
#include "common.h"
#include "serde.h"
#include "types/type.h"
#include "types/type_ser.h"
#include <string.h>

Type *infer_fn_application(Type *func_type, Ast *ast, TICtx *ctx);

Type *create_fn_from_cons(Type *res, Type *cons) {

  Type *f = res;
  for (int i = cons->data.T_CONS.num_args - 1; i >= 0; i--) {
    f = type_fn(cons->data.T_CONS.args[i], f);
  }
  return f;
}

Type *infer_cons_application(Type *cons, Ast *ast, TICtx *ctx) {

  Type *f;
  if (is_sum_type(cons)) {
    Type *mem =
        extract_member_from_sum_type(cons, ast->data.AST_APPLICATION.function);
    if (!mem) {
      return NULL;
    }
    f = create_fn_from_cons(cons, mem);
  } else {
    f = create_fn_from_cons(cons, cons);
  }

  return infer_fn_application(f, ast, ctx);
}

// T-App: Γ ⊢ e₁ : τ₁    Γ ⊢ e₂ : τ₂    α fresh    S = unify(τ₁, τ₂ → α)
//        ──────────────────────────────────────────────────────────────
//                            Γ ⊢ e₁ e₂ : S(α)
Type *infer_application(Ast *ast, TICtx *ctx) {
  Ast *func = ast->data.AST_APPLICATION.function;

  // Step 1: Infer function type
  Type *func_type = infer(func, ctx);

  if (!func_type) {
    return type_error(ast, "Cannot infer type of applicable");
  }

  if (is_coroutine_type(func_type)) {
    Type f = MAKE_FN_TYPE_2(&t_void,
                            create_option_type(func_type->data.T_CONS.args[0]));
    return infer_fn_application(&f, ast, ctx);
  }

  if (is_coroutine_constructor_type(func_type)) {
    func_type = func_type->data.T_CONS.args[0];

    return infer_fn_application(func_type, ast, ctx);
  }

  if (func_type->kind == T_CONS) {
    return infer_cons_application(func_type, ast, ctx);
  }

  return infer_fn_application(func_type, ast, ctx);
}

Type *infer_fn_application(Type *func_type, Ast *ast, TICtx *ctx) {
  Ast *func = ast->data.AST_APPLICATION.function;
  int expected_args_len = fn_type_args_len(func_type);

  Ast *args = ast->data.AST_APPLICATION.args;
  int num_args = ast->data.AST_APPLICATION.len;

  // Step 2: Infer argument types
  Type **arg_types = t_alloc(sizeof(Type *) * num_args);
  for (int i = 0; i < num_args; i++) {
    arg_types[i] = infer(args + i, ctx);

    if (!arg_types[i]) {
      return type_error(ast, "Cannot infer argument %d type", i + 1);
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
    type_error(ast, "Function application type mismatch : ");
    print_type_err(func_type);
    fprintf(stderr, "  != \n");
    print_type_err(expected_type);
    return NULL;
  }

  ctx->constraints = merge_constraints(ctx->constraints, unify_ctx.constraints);

  // Step 5: Solve constraints and apply substitutions
  Subst *solution = solve_constraints(unify_ctx.constraints);

  ctx->subst = compose_subst(solution, ctx->subst);
  expected_type = apply_substitution(solution, expected_type);
  ast->data.AST_APPLICATION.function->md = expected_type;

  Type *res = expected_type;

  for (int n = num_args; n; n--) {
    res = res->data.T_FN.to;
  }

  if (expected_args_len > num_args) {
    printf("curried???\n");
    print_type(func_type);
    Type **_arg_types = t_alloc(sizeof(Type *) * num_args);
    memcpy(_arg_types, arg_types, sizeof(Type *) * num_args);
    Type *closure_meta = create_tuple_type(num_args, _arg_types);
    res->closure_meta = closure_meta;
    print_type(res);
  }
  // if (CHARS_EQ(fn_name, "array_at")) {
  //   printf("res type: \n");
  //   print_type(res);
  // }
  return res;
}
