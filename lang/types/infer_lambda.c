#include "./infer_lambda.h"
#include "./infer_binding.h"
#include "serde.h"
#include "types/type_expressions.h"
#include "types/unification.h"

void initial_lambda_signature(int num_params, Type **param_types,
                              AstList *type_annotation_list, AstList *params,
                              TICtx *ctx) {

  for (int i = 0; i < num_params; i++,
           type_annotation_list = type_annotation_list
                                      ? type_annotation_list->next
                                      : NULL,
           params = params->next) {
    Ast *param_ast = params->ast;
    if (param_ast->tag == AST_VOID) {
      param_types[i] = &t_void;
      continue;
    }

    Ast *type_annotation =
        type_annotation_list ? type_annotation_list->ast : NULL;

    param_types[i] =
        type_annotation
            ? instantiate(compute_type_expression(type_annotation, ctx), ctx)
            : next_tvar();
  }
}

void bind_lambda_params(Ast *ast, Type **param_types, TICtx *ctx) {
  int i = 0;
  for (AstList *pl = ast->data.AST_LAMBDA.params; pl; pl = pl->next, i++) {
    Ast *param_pattern = pl->ast;
    Type *param_type = param_types[i];

    if (!bind_pattern_recursive(
            param_pattern, param_type,
            (binding_md){BT_FN_PARAM, {.FN_PARAM = {.scope = ctx->scope}}},
            ctx)) {
      type_error(ctx, param_pattern, "Cannot bind lambda parameter");
      return;
    }
  }
}

void bind_recursive_ref(Ast *ast, Type *function_type, TICtx *ctx) {
  if (ast->data.AST_LAMBDA.fn_name.chars == NULL) {
    return;
  }
  const char *fn_name = ast->data.AST_LAMBDA.fn_name.chars;
  Scheme var_scheme = {.vars = NULL, .type = function_type};
  ctx->env = env_extend(ctx->env, fn_name, var_scheme.vars, var_scheme.type);
  ctx->env->md = (binding_md){BT_RECURSIVE_REF};
}

void unify_recursive_ref(Ast *ast, Type *recursive_fn_type, Type *result_type,
                         TICtx *ctx) {

  // If this is a recursive function, unify the recursive reference with the
  // final type
  if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
    TICtx unify_ctx = {};
    if (unify(recursive_fn_type, result_type, &unify_ctx)) {
      type_error(ctx, ast, "Recursive function type mismatch");
    }

    // Apply the unification results
    if (unify_ctx.subst) {
      ctx->subst = compose_subst(unify_ctx.subst, ctx->subst);
      *result_type = *apply_substitution(unify_ctx.subst, result_type);
    }
  }
}

Type *create_coroutine_inst(Type *ret_type) {
  Type *instance_type = type_fn(&t_void, create_option_type(ret_type));
  Type **arg = talloc(sizeof(Type *));
  arg[0] = instance_type;
  instance_type = create_cons_type(TYPE_NAME_COROUTINE_INSTANCE, 1, arg);
  return instance_type;
}

Type *create_coroutine_lambda(Type *fn_type, TICtx *ctx) {
  Type *return_type = fn_return_type(fn_type);

  Type *instance_type = create_coroutine_inst(return_type);

  Type *f = fn_type;
  while (f->data.T_FN.to->kind == T_FN) {
    f = f->data.T_FN.to;
  }

  f->data.T_FN.to = instance_type;

  Type **a = talloc(sizeof(Type *));
  a[0] = fn_type;
  Type *constructor_type =
      create_cons_type(TYPE_NAME_COROUTINE_CONSTRUCTOR, 1, a);

  return constructor_type;
}

Type *infer_lambda(Ast *ast, TICtx *ctx) {
  Ast *body = ast->data.AST_LAMBDA.body;
  int num_params = ast->data.AST_LAMBDA.len;

  Type *param_types[num_params];

  initial_lambda_signature(num_params, param_types,
                           ast->data.AST_LAMBDA.type_annotations,
                           ast->data.AST_LAMBDA.params, ctx);

  TICtx lambda_ctx = *ctx;
  lambda_ctx.current_fn_ast = ast;
  lambda_ctx.scope = ctx->scope + 1;
  lambda_ctx.current_fn_base_scope = lambda_ctx.scope;

  bind_lambda_params(ast, param_types, &lambda_ctx);

  // Create a fresh type variable for the recursive function reference
  Type *recursive_fn_type = next_tvar();
  bind_recursive_ref(ast, recursive_fn_type, &lambda_ctx);

  Type *body_type = infer(body, &lambda_ctx);
  if (!body_type) {
    return type_error(ctx, body, "Cannot infer lambda body type");
  }

  Subst *ls = solve_constraints(lambda_ctx.constraints);

  for (int i = 0; i < num_params; i++) {
    param_types[i] = apply_substitution(lambda_ctx.subst, param_types[i]);
  }
  body_type = apply_substitution(lambda_ctx.subst, body_type);

  // Build the final function type
  Type *result_type = body_type;
  for (int i = num_params - 1; i >= 0; i--) {
    result_type = type_fn(param_types[i], result_type);
  }

  // If this is a recursive function, unify the recursive reference with the
  // final type
  // unify_recursive_ref(ast, recursive_fn_type, result_type, &lambda_ctx);
  // printf("rec types??\n");
  // print_type(recursive_fn_type);
  // print_type(result_type);
  //
  ctx->subst = lambda_ctx.subst;

  if (lambda_ctx.yielded_type) {
    return create_coroutine_lambda(result_type, ctx);
  }

  return result_type;
}
