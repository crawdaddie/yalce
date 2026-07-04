#include "./infer_application.h"
#include "serde.h"
#include "type_ser.h"
#include "types/builtins.h"
#include <string.h>

Type *callable_view(Type *type) {
  if (type && is_coroutine_type(type)) {
    return type_fn(&t_void, create_option_type(type->data.T_CONS.args[0]));
  }
  return type;
}

// Expand a Variadic template to match a target function type's arity.
// variadic = Variadic(Double -> Double), target = a -> b -> R (arity 2)
// → produces Double -> Double -> Double (repeating the last param).
static Type *extend_variadic_template(Type *variadic, Type *target) {
  Type *variadic_fn = variadic->data.T_CONS.args[0];
  int target_arity = fn_type_args_len(target);
  int template_arity = fn_type_args_len(variadic_fn);

  if (!variadic_fn || variadic_fn->kind != T_FN || target->kind != T_FN ||
      template_arity <= 0 || target_arity < template_arity) {
    return NULL;
  }

  Type **param_types = t_alloc(sizeof(Type *) * target_arity);
  Type *last_param_type = NULL;
  int fixed_prefix_arity = template_arity - 1;
  int i = 0;

  for (Type *v = variadic_fn; v && v->kind == T_FN; v = v->data.T_FN.to, i++) {
    last_param_type = v->data.T_FN.from;
    if (i < fixed_prefix_arity) {
      param_types[i] = deep_copy_type(v->data.T_FN.from);
    }
  }

  for (i = fixed_prefix_arity; i < target_arity; i++) {
    param_types[i] = deep_copy_type(last_param_type);
  }

  return create_type_multi_param_fn(
      target_arity, param_types, deep_copy_type(fn_return_type(variadic_fn)));
}

// Like extend_variadic_template, but for a target whose first parameter is a
// unit (void) argument — `fn () -> R`. Such a lambda has no real signal
// argument, so matching `Variadic(Double ... -> Double)` should yield `() ->
// R` (the void param preserved) rather than forcing the void arg to the
// template's repeated Double.
static Type *extend_variadic_template_void(Type *variadic, Type *target) {
  Type *variadic_fn = variadic->data.T_CONS.args[0];
  if (!variadic_fn || variadic_fn->kind != T_FN || target->kind != T_FN) {
    return NULL;
  }
  // `() -> R`: one void param, result is the target's return type.
  return type_fn(&t_void, deep_copy_type(fn_return_type(variadic_fn)));
}

// Check if a type carries a Variadic constraint (stored on meta).
// Returns the Variadic T_CONS if present, NULL otherwise.
static Type *get_variadic_constraint(Type *t) {
  if (t && t->meta) {
    Type *constraint = (Type *)t->meta;
    if (constraint->kind == T_CONS &&
        strcmp(constraint->data.T_CONS.name, "Variadic") == 0) {
      return constraint;
    }
  }
  return NULL;
}

static void constrain_argument_for_parameter(TICtx *ctx, Type *arg_type,
                                             Type *param_type, Ast *arg_ast) {
  // Variadic structural constraint: if the parameter type carries a Variadic
  // template (stored on meta), expand it to match the argument's arity and
  // add a constraint.  This constrains each lambda arg to the template's last
  // param type (e.g. Double) and the result to the template's return type.
  Type *variadic = get_variadic_constraint(param_type);
  if (variadic && arg_type && arg_type->kind == T_FN) {
    // A lambda with a unit parameter — `fn () -> ...` — has no real signal
    // argument. Match it against `Variadic(Double ... -> Double)` as `() ->
    // R` (the void param preserved) rather than forcing the void arg to the
    // template's repeated Double. The param is still a fresh tvar here (it is
    // only constrained to void later), so detect it from the lambda's AST.
    bool arg_has_void_param =
        arg_ast && arg_ast->tag == AST_LAMBDA &&
        arg_ast->data.AST_LAMBDA.params &&
        arg_ast->data.AST_LAMBDA.params->ast->tag == AST_VOID;
    Type *expanded = arg_has_void_param
                         ? extend_variadic_template_void(variadic, arg_type)
                         : extend_variadic_template(variadic, arg_type);
    if (expanded) {
      // Link the parameter (a tvar carrying the Variadic meta) to the
      // argument so the function's result type tracks the argument. Then
      // structurally constrain the argument to the expanded template, which
      // fixes each lambda parameter and the result to the template's types.
      add_constraint(ctx, param_type, arg_type);
      add_constraint(ctx, arg_type, expanded);
      return;
    }
  }

  bool structurally_compatible =
      types_match(param_type, arg_type) || types_match(arg_type, param_type);

  if (types_equal(arg_type, param_type) ||
      ((is_generic(arg_type) || is_generic(param_type)) &&
       structurally_compatible)) {
    add_constraint(ctx, arg_type, param_type);
    return;
  }

  if (is_coroutine_type(param_type)) {
    Type *yielded = param_type->data.T_CONS.args[0];
    if (is_list_type(arg_type)) {
      Type *elem = type_of_list(arg_type);
      if (elem) {
        add_constraint(ctx, yielded, elem);
      }
    } else if (is_array_type(arg_type)) {
      add_constraint(ctx, yielded, arg_type->data.T_CONS.args[0]);
    }
  }

  TypeList *from_params = t_alloc(sizeof(TypeList));
  from_params->type = arg_type;
  from_params->next = NULL;
  ctx->predicates = predicate_append_applied(ctx->predicates, GenericFrom,
                                             param_type, from_params);
}
Type *handle_closure_constants(Ast *ast, Type *type, TICtx *ctx) {
  if (!is_constant_expr(ast, ctx)) {
    return type;
  }

  int i = 0;
  Type *f = ast->data.AST_APPLICATION.function->type;
  for (; f->kind == T_FN && !is_closure(f); f = f->data.T_FN.to) {
    i++;
  }

  if (ast->data.AST_APPLICATION.len == i) {
    return type;
  }

  // The remaining callable after consuming the constant arguments is itself a
  // closure. We cannot turn it into a plain curried function because it still
  // needs its closure environment at runtime.
  if (is_closure(f)) {
    return type;
  }

  ast->data.AST_APPLICATION.is_curried_with_constants = true;
  Type *result = deep_copy_type(type);
  result->closure_meta = NULL;
  return result;
}

Type *infer_application(Ast *ast, TICtx *ctx) {
  Ast *fn_ast = ast->data.AST_APPLICATION.function;
  size_t nargs = ast->data.AST_APPLICATION.len;

  Type *fn_type = infer_expr(fn_ast, ctx);

  if (!fn_type) {
    return NULL;
  }

  int expected_args_len = fn_type_args_len(fn_type);

  Type *current = fn_type;

  Type *arg_types[nargs];
  for (size_t i = 0; i < nargs; i++) {
    current = callable_view(current);

    Type *arg_type = infer_expr(ast->data.AST_APPLICATION.args + i, ctx);
    arg_types[i] = arg_type;
    if (!arg_type)
      return NULL;

    if (current->kind == T_FN) {
      Type *param_type = current->data.T_FN.from;
      // A void (unit) parameter — `fn () -> ...` — denotes a function that
      // takes no real argument. When such a function is applied to an
      // argument (a common trigger idiom like `trig 0`), the argument is
      // ignored rather than constrained against `()`, which would otherwise
      // spuriously require `From((), [arg])`.
      if (param_type->kind != T_VOID) {
        constrain_argument_for_parameter(ctx, arg_type, param_type,
                                         ast->data.AST_APPLICATION.args + i);
      }
      current = current->data.T_FN.to;
    } else {
      // function position has too few params / is not a function
      Type *result = next_tvar();
      Type *expected = type_fn(arg_type, result);
      add_constraint(ctx, current, expected);
      current = result;
    }
  }
  if (expected_args_len > nargs) {
    Type **_arg_types = t_alloc(sizeof(Type *) * nargs);
    memcpy(_arg_types, arg_types, sizeof(Type *) * nargs);
    Type *closure_meta = create_tuple_type(nargs, _arg_types);
    current = deep_copy_type(current);
    current->closure_meta = closure_meta;
  }

  if (current->kind == T_FN) {
    return handle_closure_constants(ast, current, ctx);
  } else {
    return current;
  }
}
