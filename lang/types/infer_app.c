
#include "./infer_app.h"
#include "./builtins.h"
#include "serde.h"
#include "types/unification.h"

/*
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

  // if (cons->data.T_CONS.args == NULL) {
  //   return cons;
  // }

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
*/

Type *infer_cons_application(Ast *ast, Scheme *cons_scheme, TICtx *ctx) {

  int len = ast->data.AST_APPLICATION.len;
  Ast *args = ast->data.AST_APPLICATION.args;

  Subst *inst_subst = NULL;

  int i = 0;

  Type *arg_types[len];

  for (int i = 0; i < len; i++) {
    arg_types[i] = infer(args + i, ctx);
  }

  Type *stype = deep_copy_type(cons_scheme->type);
  for (VarList *v = cons_scheme->vars; v; v = v->next, i++) {
    Type *t = arg_types[i];
    inst_subst = subst_extend(inst_subst, v->var, t);
  }

  Type *s = apply_substitution(inst_subst, stype);
  for (int i = 0; i < len; i++) {
    unify(arg_types[i], s->data.T_CONS.args[i], ctx);
  }
  ast->data.AST_APPLICATION.function->md = s;

  return s;

  // for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
  //
  //   printf("cons arg %d: ", i);
  //   print_type(cons_scheme->type->data.T_CONS.args[i]);
  //   print_type((ast->data.AST_APPLICATION.args + i)->md);
  //   // if (unify(ast->data.AST_APPLICATION.args[i].md,
  //   // inst->data.T_CONS.args[i],
  //   //           ctx)) {
  //   //   fprintf(stderr, "Unification of cons args failed\n");
  //   //   return NULL;
  //   // }
  // }
  //
  // return inst;
}
Type *coroutine_inst_to_callable(Type *cor) {
  return type_fn(&t_void, create_option_type(cor->data.T_CONS.args[0]));
}

Type *infer_app(Ast *ast, TICtx *ctx) {
  Ast *func = ast->data.AST_APPLICATION.function;
  Ast *args = ast->data.AST_APPLICATION.args;
  int num_args = ast->data.AST_APPLICATION.len;

  Type *func_type;
  if (func->tag == AST_IDENTIFIER) {

    Scheme *s = lookup_scheme(ctx->env, func->data.AST_IDENTIFIER.value);

    if (s == &array_at_scheme &&
        (ast->data.AST_APPLICATION.args + 1)->tag == AST_LIST) {

      // TODO: handle weird function list arg being interpreted as
      // array_at -eg f [(1,2)] is interpreted as array_at f (1,2)
      // workaround is to add a comma -> f [(1,2),]
      //
      // Ast *func = ast->data.AST_APPLICATION.args;
      // Ast *list_items = (ast->data.AST_APPLICATION.args + 1);
    }

    // if (s == &array_at_scheme_glob) {
    //   printf("array at\n");
    //   print_ast(ast);
    // }
    if (!s) {
      return NULL;
    }
    if (s->type->kind == T_CONS && !is_coroutine_constructor_type(s->type) &&
        !is_coroutine_type(s->type)) {
      return infer_cons_application(ast, s, ctx);
    }
  }

  // Step 1: Infer function type
  func_type = infer(func, ctx);

  if (is_coroutine_type(func_type)) {
    func_type = coroutine_inst_to_callable(func_type);
  } else if (is_coroutine_constructor_type(func_type)) {
    func_type = func_type->data.T_CONS.args[0];
  }

  if (!func_type) {
    return type_error(ctx, ast, "Cannot infer function type");
  }

  // Step 2: Infer argument types
  Type **arg_types = talloc(sizeof(Type *) * num_args);
  for (int i = 0; i < num_args; i++) {
    arg_types[i] = infer(args + i, ctx);

    if (!arg_types[i]) {
      return type_error(ctx, ast, "Cannot infer argument %d type", i + 1);
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
    type_error(ctx, ast, "Function application type mismatch : ");
    print_type_err(func_type);
    fprintf(stderr, "  != \n");
    print_type_err(expected_type);
    return NULL;
  }

  // print_constraints(unify_ctx.constraints);

  // print_constraints(unify_ctx.constraints);
  ctx->constraints = merge_constraints(ctx->constraints, unify_ctx.constraints);

  // printf("app constraints\n");
  // print_ast(ast);
  // print_constraints(unify_ctx.constraints);

  // Step 5: Solve constraints and apply substitutions
  Subst *solution = solve_constraints(unify_ctx.constraints);

  ctx->subst = compose_subst(solution, ctx->subst);
  expected_type = apply_substitution(solution, expected_type);
  ast->data.AST_APPLICATION.function->md = expected_type;

  Type *res = expected_type;

  for (int n = num_args; n; n--) {
    res = res->data.T_FN.to;
  }
  return res;
}
