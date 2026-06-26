#include "./infer_lambda.h"
#include "serde.h"
#include "type_ser.h"

Type *infer_lambda(Ast *ast, TICtx *ctx) {
  size_t len = ast->data.AST_LAMBDA.len;
  Type **param_types = len ? t_alloc(sizeof(Type *) * len) : NULL;
  Type *self_type = NULL;
  int lambda_scope_depth = ctx->scope + 1;
  LambdaScope lambda_scope = {
      .fn_ast = ast,
      .base_scope = lambda_scope_depth,
      .parent = ctx->current_scope,
  };
  TICtx child = *ctx;

  child.current_fn_ast = ast;
  child.current_scope = &lambda_scope;
  child.current_fn_base_scope = lambda_scope_depth;
  child.scope = lambda_scope_depth;
  child.yielded_type = NULL;

  if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
    self_type = next_tvar();
    child.env =
        env_extend(child.env, ast->data.AST_LAMBDA.fn_name.chars, self_type);
  }

  AstList *param = ast->data.AST_LAMBDA.params;
  for (size_t i = 0; i < len && param; i++, param = param->next) {
    Type *pt = next_tvar();
    param_types[i] = pt;
    TypeEnv *param_boundary = child.env;
    if (bind_pattern(param->ast, pt, &child) != 0) {
      return type_error(param->ast, "Unsupported lambda parameter");
    }
    set_env_slice_scope(child.env, param_boundary, lambda_scope_depth);
    set_env_slice_yield_boundary(child.env, param_boundary,
                                 ast->data.AST_LAMBDA.num_yields);
  }

  Type *body_type = infer_expr(ast->data.AST_LAMBDA.body, &child);
  if (!body_type) {
    return NULL;
  }

  ctx->constraints = child.constraints;
  ctx->predicates = child.predicates;
  ctx->subst = child.subst;

  Type *fn_type = body_type;
  if (child.yielded_type) {
    // printf("[LAMBDA]\n");
    // print_type(fn_type);
    fn_type = create_coroutine_instance_type(fn_type);
  }
  for (size_t i = len; i > 0; i--) {
    fn_type = type_fn(param_types[i - 1], fn_type);
  }

  if (ast->data.AST_LAMBDA.num_closed_vals > 0) {
    int closed_len = ast->data.AST_LAMBDA.num_closed_vals;
    Type **closed_types = t_alloc(sizeof(Type *) * (size_t)closed_len);
    int i = 0;
    for (AstList *closed_vals = ast->data.AST_LAMBDA.closed_vals; closed_vals;
         closed_vals = closed_vals->next, i++) {

      closed_types[i] = closed_vals->ast->type;
    }
    Type *closure_env_type = create_tuple_type(closed_len, closed_types);
    fn_type->closure_meta = closure_env_type;
    ast->type = fn_type;
  }

  if (ast->data.AST_LAMBDA.is_coroutine && fn_type->kind == T_FN) {
    fn_type->data.T_FN.attributes =
        set_attr(fn_type->data.T_FN.attributes, FN_ATTR_COROUTINE_CONSTRUCTOR);
  }

  if (self_type) {
    add_constraint(ctx, self_type, fn_type);
  }

  return fn_type;
}
