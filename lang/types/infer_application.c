#include "./infer_application.h"
#include "types/builtins.h"

Type *callable_view(Type *type) {
  if (type && is_coroutine_type(type)) {
    return type_fn(&t_void, create_option_type(type->data.T_CONS.args[0]));
  }
  return type;
}
static void constrain_argument_for_parameter(TICtx *ctx, Type *arg_type,
                                             Type *param_type) {
  if (is_generic(arg_type) || is_generic(param_type) ||
      types_equal(arg_type, param_type)) {
    add_constraint(ctx, arg_type, param_type);
    return;
  }

  TypeList *from_params = t_alloc(sizeof(TypeList));
  from_params->type = arg_type;
  from_params->next = NULL;
  ctx->predicates = predicate_append_applied(ctx->predicates, GenericFrom,
                                             param_type, from_params);
}

Type *infer_application(Ast *ast, TICtx *ctx) {
  Ast *fn_ast = ast->data.AST_APPLICATION.function;
  size_t nargs = ast->data.AST_APPLICATION.len;

  Type *fn_type = infer_expr(fn_ast, ctx);
  if (!fn_type)
    return NULL;

  Type *current = fn_type;
  for (size_t i = 0; i < nargs; i++) {
    current = callable_view(current);
    Type *arg_type = infer_expr(ast->data.AST_APPLICATION.args + i, ctx);
    if (!arg_type)
      return NULL;

    if (current->kind == T_FN) {
      Type *param_type = current->data.T_FN.from;
      constrain_argument_for_parameter(ctx, arg_type, param_type);
      current = current->data.T_FN.to;
    } else {
      // function position has too few params / is not a function
      Type *result = next_tvar();
      Type *expected = type_fn(arg_type, result);
      add_constraint(ctx, current, expected);
      current = result;
    }
  }

  return current;
}
