#include "serde.h"
#include "types/common.h"
#include "types/type.h"
#include <string.h>

// forward decls
Type *infer(Ast *ast, TICtx *ctx);
Type *unify_in_ctx(Type *t1, Type *t2, TICtx *ctx, Ast *node);
void *type_error(TICtx *ctx, Ast *node, const char *fmt, ...);
Substitution *solve_constraints(TypeConstraint *constraints);
Type *apply_substitution(Substitution *subst, Type *t);
void bind_in_ctx(TICtx *ctx, Ast *binding, Type *expr_type);
void apply_substitutions_rec(Ast *ast, Substitution *subst);
TypeConstraint *constraints_extend(TypeConstraint *constraints, Type *t1,
                                   Type *t2);

Type *infer_application(Ast *ast, TICtx *ctx);
Type *infer_fn_application(Ast *ast, TICtx *ctx) {

  Type *fn_type = ast->data.AST_APPLICATION.function->md;

  if (ast->data.AST_IDENTIFIER.is_recursive_fn_ref) {
    fn_type = deep_copy_type(fn_type);
  }

  Type *_fn_type;

  int len = ast->data.AST_APPLICATION.len;
  Type *arg_types[len];

  TICtx app_ctx = {.scope = ctx->scope + 1};

  for (size_t i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    Ast *arg = ast->data.AST_APPLICATION.args + i;
    arg_types[i] = infer(arg, ctx);
    if (arg_types[i] == NULL) {
      fprintf(stderr, "Error could not infer argument %zu of ", i);
      print_ast_err(ast);
      print_ast_err(arg);
      return NULL;
    }

    if (arg_types[i]->kind == T_FN) {

      if ((!is_generic(fn_return_type(arg_types[i]))) &&
          (arg->tag == AST_LAMBDA) &&
          is_generic(arg->data.AST_LAMBDA.body->md)) {
        // TODO: this is a really fiddly and complex edge-case
        // when you have an anonymous function callback passed to a function
        // with type information constraints generated within the callback
        // aren't pushed back up to the app_ctx (???)
        unify_in_ctx(fn_return_type(arg_types[i]),
                     arg->data.AST_LAMBDA.body->md, &app_ctx, arg);
      }
    }

    // For each argument, add a constraint that the function's parameter type
    // must match the argument type
    Type *param_type = fn_type->data.T_FN.from;

    if (!unify_in_ctx(param_type, arg_types[i], &app_ctx,
                      ast->data.AST_APPLICATION.args + i)) {
      return NULL;
    }

    if (i < ast->data.AST_APPLICATION.len - 1) {
      if (fn_type->data.T_FN.to->kind != T_FN) {
        return type_error(
            ctx, ast, "Too many arguments provided to function %s",
            ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value);
      }
      fn_type = fn_type->data.T_FN.to;
    }
  }

  Substitution *subst = solve_constraints(app_ctx.constraints);
  // print_subst(subst);

  _fn_type = apply_substitution(subst, _fn_type);
  ast->data.AST_APPLICATION.function->md = _fn_type;

  Type *res_type = _fn_type;

  for (int i = 0; i < len; i++) {
    Ast *arg = ast->data.AST_APPLICATION.args + i;

    if (arg->tag == AST_IDENTIFIER && arg->data.AST_IDENTIFIER.is_fn_param) {
      arg->md = apply_substitution(subst, arg->md);
    }

    if (((Type *)arg->md)->kind == T_FN) {

      if ((!is_generic(fn_return_type(arg->md))) && (arg->tag == AST_LAMBDA) &&
          is_generic(arg->data.AST_LAMBDA.body->md)) {
        apply_substitutions_rec(arg, subst);
      }
    }

    if (is_generic(arg->md)) {
      apply_substitutions_rec(arg, subst);
    }

    if (is_generic(arg->md) &&
        !types_equal(arg->md, res_type->data.T_FN.from)) {

      ctx->constraints = constraints_extend(ctx->constraints, arg->md,
                                            res_type->data.T_FN.from);
      ctx->constraints->src = arg;
    }

    if (is_generic(arg_types[i]) && !types_equal(arg_types[i], arg->md)) {
      unify_in_ctx(arg_types[i], arg->md, ctx, arg);
    }

    res_type = res_type->data.T_FN.to;
  }

  return res_type;
}

bool is_struct_of_void_fns(Type *cons) {
  for (int i = 0; i < cons->data.T_CONS.num_args; i++) {
    Type *t = cons->data.T_CONS.args[i];
    if (t->kind == T_FN) {
      if (!is_void_func(t)) {
        // if member is fn, it must be () -> xx
        return false;
      }
    }
    // if member is not fn, that's ok
  }
  return true;
}

Type *struct_of_fns_to_return(Type *cons) {
  Type **results = talloc(sizeof(Type *) * cons->data.T_CONS.num_args);

  results[0] = &t_num;

  for (int i = 1; i < cons->data.T_CONS.num_args; i++) {
    Type *t = cons->data.T_CONS.args[i];
    if (is_coroutine_type(t)) {
      results[i] = fn_return_type(t);
      results[i] = type_of_option(results[i]);
    } else if (t->kind == T_FN) {
      results[i] = fn_return_type(t);

      if (results[i]->alias && CHARS_EQ(results[i]->alias, "Option")) {
        results[i] = type_of_option(results[i]);
      }
    } else {
      results[i] = t;
    }
  }
  return create_tuple_type(cons->data.T_CONS.num_args, results);
}

Type *infer_iter(Ast *ast, TICtx *ctx) {
  printf("infer iter\n");
  Type *t = infer(ast->data.AST_APPLICATION.args, ctx);
  print_type(t);
  Type *ret_type;
  switch (t->kind) {
  case T_CONS: {
    if (is_list_type(t)) {
      ret_type = t->data.T_CONS.args[0];
      break;
    }

    if (is_array_type(t)) {
      ret_type = t->data.T_CONS.args[0];
      break;
    }
  }
  case T_VAR: {
    // TODO:
    //   add typeclass 'with iter' and resolve later eg either list or array
    fprintf(stderr, "Type ");
    print_type_err(t);
    fprintf(stderr, "does not implement the iterable typeclass\n");
    break;
  }

  default: {
    fprintf(stderr, "Type ");
    print_type_err(t);
    fprintf(stderr, "does not implement the iterable typeclass\n");
    return NULL;
  }
  }
  Type *coroutine_fn = create_coroutine_instance_type(ret_type);
  Type *coroutine_constructor = type_fn(t, coroutine_fn);
  coroutine_constructor->is_coroutine_constructor = true;
  ast->data.AST_APPLICATION.function->md = coroutine_constructor;
  return coroutine_fn;
}

Type *find_variant_member(Type *variant, const char *name);

bool is_index_access_ast(Ast *application) {
  Ast *arg_ast = application->data.AST_APPLICATION.args;
  Type *arg_type = arg_ast->md;
  Type *cons = application->data.AST_APPLICATION.function->md;

  return is_list_type(arg_type) && arg_ast->tag == AST_LIST &&
         application->data.AST_APPLICATION.len == 1 && is_array_type(cons);
}
Type *infer_cons_application(Ast *ast, TICtx *ctx) {
  Type *fn_type = ast->data.AST_APPLICATION.function->md;

  Ast *fn_id = ast->data.AST_APPLICATION.function;
  const char *fn_name = fn_id->data.AST_IDENTIFIER.value;
  Type *cons = fn_type;

  if (is_variant_type(fn_type)) {
    cons = find_variant_member(fn_type, fn_name);
    if (!cons) {
      fprintf(stderr, "Error: %s not found in variant %s\n", fn_name,
              cons->data.T_CONS.name);
      return NULL;
    }
  }

  TICtx app_ctx = {};
  for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {

    Type *cons_arg = cons->data.T_CONS.args[i];
    Type *arg_type;

    if (!(arg_type = infer(ast->data.AST_APPLICATION.args + i, ctx))) {
      return type_error(
          ctx, ast, "Could not infer argument type in cons %s application\n",
          cons->data.T_CONS.name);
    }

    if (is_index_access_ast(ast)) {
      Ast *arg_ast = ast->data.AST_APPLICATION.args + i;
      Type *arg_type = arg_ast->md;
      unify_in_ctx(create_list_type_of_type(&t_int), arg_type, ctx, ast);

      return cons->data.T_CONS.args[0];
    }

    if (!unify_in_ctx(cons_arg, arg_type, &app_ctx, ast)) {
      return type_error(ctx, ast,
                        "Could not constrain type variable to function type\n");
    }

    if (is_generic(arg_type) && !(types_equal(arg_type, cons_arg))) {
      ctx->constraints =
          constraints_extend(ctx->constraints, arg_type, cons_arg);
    }
  }

  Substitution *subst = solve_constraints(app_ctx.constraints);
  Type *resolved_type = apply_substitution(subst, fn_type);
  apply_substitutions_rec(ast, subst);
  ast->data.AST_APPLICATION.function->md = resolved_type;
  return resolved_type;
}

Type *infer_application(Ast *ast, TICtx *ctx) {

  // print_ast(ast);
  // if (ast->data.AST_APPLICATION.function->tag == AST_RECORD_ACCESS) {
  //   printf("APPLICATION\n");
  //   print_ast(ast);
  // }
  // if (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
  //     CHARS_EQ(ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
  //              TYPE_NAME_REF)) {
  //   Type *val_type = infer(ast->data.AST_APPLICATION.args, ctx);
  //   val_type->is_ref = true;
  //   ast->data.AST_APPLICATION.function->md = type_fn(val_type, val_type);
  //   return val_type;
  // }
  if (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
      CHARS_EQ(ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
               "addrof")) {
    Type *arg_type = infer(ast->data.AST_APPLICATION.args, ctx);
    return arg_type;
  }

  if (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
      CHARS_EQ(ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
               "iter")) {
    return infer_iter(ast, ctx);
  }

  Type *fn_type = infer(ast->data.AST_APPLICATION.function, ctx);

  // if (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
  //     CHARS_EQ(ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
  //              TYPE_NAME_RUN_IN_SCHEDULER)) {
  //   infer_schedule_event_callback(ast, ctx);
  //   return &t_void;
  // }

  if (!fn_type) {
    return NULL;
  }

  switch (fn_type->kind) {
  case T_VAR: {
    int app_len = ast->data.AST_APPLICATION.len;

    Type *arg_types[app_len];

    for (int i = 0; i < app_len; i++) {
      Ast *arg = ast->data.AST_APPLICATION.args + i;
      Type *arg_type = infer(arg, ctx);
      if (!arg_type) {
        return type_error(ctx, arg,
                          "Could not infer argument type in var application\n");
      }
      arg_types[i] = arg_type;
    }

    Type *ret_type = next_tvar();

    Type *fn_constraint =
        create_type_multi_param_fn(app_len, arg_types, ret_type);

    unify_in_ctx(fn_constraint, fn_type, ctx, ast);

    return ret_type;
    break;
  }
  case T_CONS: {
    return infer_cons_application(ast, ctx);
  }
  case T_FN: {
    return infer_fn_application(ast, ctx);
  }
  default: {

    if (IS_PRIMITIVE_TYPE(fn_type)) {
      if (ast->data.AST_APPLICATION.args->tag == AST_LIST &&
          ast->data.AST_APPLICATION.args->data.AST_LIST.len == 0) {
        return create_list_type_of_type(fn_type);
      }

      Type *f = fn_type;
      for (int i = ast->data.AST_APPLICATION.len - 1; i >= 0; i--) {
        Type *t = infer(ast->data.AST_APPLICATION.args + i, ctx);
        f = type_fn(t, f);
      }
      // print_type(f);
      ast->data.AST_APPLICATION.function->md = f;
      return fn_type;
    }
    fprintf(stderr, "Error: constructor not implemented\n");
    return NULL;
  }
  }
}
