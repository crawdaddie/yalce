#include "inference.h"
#include "builtins.h"
#include "serde.h"
#include "types/type.h"
#include "types/type_declaration.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

Type *infer_application(Ast *ast, TICtx *ctx);
Type *infer_fn_application(Ast *ast, TICtx *ctx);
Type *infer_cons_application(Ast *ast, TICtx *ctx);
Type *infer_yield_stmt(Ast *ast, TICtx *ctx);
Type *infer_lambda(Ast *ast, TICtx *ctx);
Type *infer_let_binding(Ast *ast, TICtx *ctx);
Type *infer_match_expr(Ast *ast, TICtx *ctx);

void apply_substitutions_rec(Ast *ast, Substitution *subst);
Substitution *solve_constraints(TypeConstraint *constraints);

#define IS_PRIMITIVE_TYPE(t) ((1 << t->kind) & TYPE_FLAGS_PRIMITIVE)
#define CHARS_EQ(a, b) (strcmp(a, b) == 0)

Type *unify_in_ctx(Type *t1, Type *t2, TICtx *ctx, Ast *node);

void _print_location(Ast *ast, FILE *fstream) {
  loc_info *loc = ast->loc_info;

  if (!loc || !loc->src || !loc->src_content) {
    print_ast_err(ast);
    return;
  }

  fprintf(fstream, " %s %d:%d\n", loc->src, loc->line, loc->col);

  const char *start = loc->src_content;
  const char *offset = start + loc->absolute_offset;

  while (offset > start && *offset != '\n') {
    offset--;
  }

  if (offset > start) {
    offset++;
  }

  while (*offset && *offset != '\n') {
    fputc(*offset, fstream);
    offset++;
  }

  fprintf(fstream, "\n");
  fprintf(fstream, "%*c", loc->col - 1, ' ');
  fprintf(fstream, "^");
  fprintf(fstream, "\n");
}

void *type_error(TICtx *ctx, Ast *node, const char *fmt, ...) {
  FILE *err_stream = ctx->err_stream ? ctx->err_stream : stderr;
  va_list args;
  va_start(args, fmt);

  vfprintf(err_stream, fmt, args);
  if (node && node->loc_info) {
    _print_location(node, err_stream);
  } else if (node) {
    print_ast_err(node);
  }
  va_end(args);
  return NULL;
}

Type *env_lookup(TypeEnv *env, const char *name) {
  while (env) {
    if (env->name && strcmp(env->name, name) == 0) {
      return env->type;
    }

    env = env->next;
  }

  return lookup_builtin_type(name);
}

void print_subst(Substitution *c);

Type *create_list_type(Ast *ast, const char *cons_name, TICtx *ctx) {

  if (ast->data.AST_LIST.len == 0) {
    Type *t = talloc(sizeof(Type));
    Type **el = talloc(sizeof(Type *));
    el[0] = next_tvar();
    *t = (Type){T_CONS, {.T_CONS = {cons_name, el, 1}}};
    return t;
  }

  int len = ast->data.AST_LIST.len;
  Type *el_type = infer(ast->data.AST_LIST.items, ctx);

  for (int i = 1; i < len; i++) {
    Ast *el = ast->data.AST_LIST.items + i;
    Type *_el_type = infer(el, ctx);
    if (_el_type->kind == T_VAR) {
      unify_in_ctx(_el_type, el_type, ctx, ast);
    } else if

        (!types_equal(el_type, _el_type)) {
      return type_error(
          ctx, ast,
          "Typecheck Error: typechecking list literal - all elements must "
          "be of the same type");
    }
    el_type = _el_type;
  }
  Type *type = talloc(sizeof(Type));
  Type **contained = talloc(sizeof(Type *));
  contained[0] = el_type;
  *type = (Type){T_CONS, {.T_CONS = {cons_name, contained, 1}}};
  return type;
}

Type *infer(Ast *ast, TICtx *ctx) {
  Type *type = NULL;
  switch (ast->tag) {
  case AST_INT: {
    type = &t_int;
    break;
  }
  case AST_DOUBLE: {
    type = &t_num;
    break;
  }
  case AST_STRING: {
    type = &t_string;
    break;
  }
  case AST_CHAR: {
    type = &t_char;
    break;
  }
  case AST_BOOL: {
    type = &t_bool;
    break;
  }
  case AST_VOID: {
    type = &t_void;
    break;
  }

  case AST_ARRAY: {
    type = create_list_type(ast, TYPE_NAME_ARRAY, ctx);
    break;
  }

  case AST_LIST: {
    type = create_list_type(ast, TYPE_NAME_LIST, ctx);
    break;
  }
  case AST_TUPLE: {

    int len = ast->data.AST_LIST.len;

    Type **cons_args = talloc(sizeof(Type *) * len);
    bool is_struct_of_coroutines = false;

    for (int i = 0; i < len; i++) {

      Ast *member = ast->data.AST_LIST.items + i;
      Type *mtype = infer(member, ctx);
      cons_args[i] = mtype;
      // if (is_coroutine_type(mtype)) {
      //   is_struct_of_coroutines = true;
      // }
    }

    type = talloc(sizeof(Type));

    *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_TUPLE, cons_args, len}}};

    if (ast->data.AST_LIST.items[0].tag == AST_LET) {
      const char **names = talloc(sizeof(char *) * len);
      for (int i = 0; i < len; i++) {
        Ast *member = ast->data.AST_LIST.items + i;
        names[i] = member->data.AST_LET.binding->data.AST_IDENTIFIER.value;
      }
      type->data.T_CONS.names = names;
    }
    // if (is_struct_of_coroutines) {
    //   for (int i = 0; i < len; i++) {
    //     if (is_coroutine_type(type->data.T_CONS.args[i])) {
    //       type->data.T_CONS.args[i] =
    //           type_of_option(fn_return_type(type->data.T_CONS.args[i]));
    //     }
    //   }
    //   type = type_fn(&t_void, create_option_type(type));
    //   type->is_coroutine_instance = true;
    // }

    break;
  }

  case AST_TYPE_DECL: {
    type = type_declaration(ast, &ctx->env);
    break;
  }

  case AST_FMT_STRING: {

    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      infer(member, ctx);
    }

    type = &t_string;
    break;
  }

  case AST_BODY: {
    Ast *stmt;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      stmt = ast->data.AST_BODY.stmts[i];

      Type *t = infer(stmt, ctx);

      if (!t) {
        print_ast_err(stmt);
        return NULL;
      }
      type = t;
    }
    break;
  }

  case AST_IDENTIFIER: {

    const char *name = ast->data.AST_IDENTIFIER.value;
    type = env_lookup(ctx->env, name);

    if (type && type->kind == T_CREATE_NEW_GENERIC) {
      type = type->data.T_CREATE_NEW_GENERIC(NULL);
    }

    if (type == NULL) {
      type = next_tvar();
    }

    break;
  }

  case AST_APPLICATION: {
    type = infer_application(ast, ctx);
    break;
  }

  case AST_LET: {
    type = infer_let_binding(ast, ctx);
    break;
  }

  case AST_LAMBDA: {
    type = infer_lambda(ast, ctx);
    break;
  }
  case AST_MATCH: {
    // static int match = 0;
    // printf("infer match %d\n", match);
    // match++;
    type = infer_match_expr(ast, ctx);
    break;
  }
  case AST_EXTERN_FN: {
    Ast *sig = ast->data.AST_EXTERN_FN.signature_types;
    int params_count = sig->data.AST_LIST.len - 1;

    if (sig->tag == AST_FN_SIGNATURE) {
      Type *f = compute_type_expression(sig->data.AST_LIST.items + params_count,
                                        ctx->env);
      sig->data.AST_LIST.items[params_count].md = f;

      for (int i = params_count - 1; i >= 0; i--) {
        Type *p =
            compute_type_expression(sig->data.AST_LIST.items + i, ctx->env);

        sig->data.AST_LIST.items[i].md = p;

        f = type_fn(p, f);
      }
      type = f;
    }

    // Type *f = compute_type_expression(sig->data.AST_LIST.items +
    // params_count,
    //                                   ctx->env);

    break;
  }

  case AST_YIELD: {
    type = infer_yield_stmt(ast, ctx);
    break;
  }
  default: {
    return type_error(
        ctx, ast, "Typecheck Error: inference not implemented for AST Node\n");
  }
  }

  ast->md = type;
  return type;
}

Type *infer_yield_stmt(Ast *ast, TICtx *ctx) {

  Ast *yield_expr = ast->data.AST_YIELD.expr;

  infer(yield_expr, ctx);
  Type *yield_expr_type = yield_expr->md;

  if (is_coroutine_type(yield_expr_type)) {
    yield_expr_type = type_of_option(fn_return_type(yield_expr_type));
  }

  if (ctx->yielded_type == NULL) {
    ctx->yielded_type = yield_expr_type;

  } else {
    Type *prev_yield_type = ctx->yielded_type;

    if (!unify_in_ctx(prev_yield_type, yield_expr_type, ctx, yield_expr)) {
      return type_error(ctx, ast,
                        "Error: yielded values must be of the same type!");
    }

    ctx->yielded_type = yield_expr_type;
  }
  ctx->current_fn_ast->data.AST_LAMBDA.num_yields++;

  // if (yield_expr->tag == AST_APPLICATION &&
  //     strcmp(
  //         yield_expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
  //         "arithmetic") == 0) {
  //   printf("???\n");
  //   print_type(yield_expr_type);
  // }
  return yield_expr_type;
}

Type *struct_of_fns_to_return(Type *cons) {
  Type **results = talloc(sizeof(Type *) * cons->data.T_CONS.num_args);
  for (int i = 0; i < cons->data.T_CONS.num_args; i++) {
    Type *t = cons->data.T_CONS.args[i];
    if (t->kind == T_FN) {

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

Type *infer_schedule_event_callback(Ast *ast, TICtx *ctx) {
  if (ast->data.AST_APPLICATION.len != 3) {
    return type_error(ctx, ast, "run_in_scheduler must have 3 args\n");
  }

  infer(ast->data.AST_APPLICATION.args,
        ctx); // first arg is concrete schedule fn impl
  Type *val_generator_type = infer(ast->data.AST_APPLICATION.args + 2, ctx);
  if (!is_struct_of_void_fns(val_generator_type)) {
    return type_error(
        ctx, ast->data.AST_APPLICATION.args + 2,
        "value generator must consist of constants or () -> xx void funcs");
  }
  Type *sink_fn = infer(ast->data.AST_APPLICATION.args + 1, ctx);
  Type *val_struct = sink_fn->data.T_FN.from;
  if (sink_fn->data.T_FN.to->kind != T_FN) {
    return type_error(ctx, ast->data.AST_APPLICATION.args + 1,
                      "not enough args in sink fn");
  }
  Type *frame_offset_arg = sink_fn->data.T_FN.to->data.T_FN.from;

  TICtx _ctx = {};
  unify_in_ctx(frame_offset_arg, &t_int, &_ctx,
               (ast->data.AST_APPLICATION.args + 1)->data.AST_LAMBDA.params +
                   1);
  Type *concrete_val_struct = struct_of_fns_to_return(val_generator_type);

  unify_in_ctx(val_struct, concrete_val_struct, &_ctx,
               (ast->data.AST_APPLICATION.args + 1)->data.AST_LAMBDA.params);

  Substitution *subst = solve_constraints(_ctx.constraints);

  apply_substitutions_rec(ast, subst);
  (ast->data.AST_APPLICATION.args + 1)->md =
      apply_substitution(subst, (ast->data.AST_APPLICATION.args + 1)->md);
}

Type *infer_application(Ast *ast, TICtx *ctx) {
  Type *fn_type = infer(ast->data.AST_APPLICATION.function, ctx);

  if (ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
      CHARS_EQ(ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
               TYPE_NAME_RUN_IN_SCHEDULER)) {
    infer_schedule_event_callback(ast, ctx);
    return &t_void;
  }

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
  }
}

bool constraints_match(TypeConstraint *constraints, Type *t1, Type *t2) {
  for (TypeConstraint *c = constraints; c; c = c->next) {
    Type *_t1 = c->t1;
    Type *_t2 = c->t2;
    if (types_equal(t1, _t1) && types_equal(t2, _t2)) {
      return true;
    }
  }
  return false;
}

TypeConstraint *constraints_extend(TypeConstraint *constraints, Type *t1,
                                   Type *t2) {
  if (constraints_match(constraints, t1, t2)) {
    return constraints;
  }

  TypeConstraint *c = talloc(sizeof(TypeConstraint));
  c->t1 = t1;
  c->t2 = t2;
  c->next = constraints;
  return c;
}

void print_constraints(TypeConstraint *c) {
  if (!c) {
    return;
  }
  printf("constraints:\n");
  for (TypeConstraint *con = c; con != NULL; con = con->next) {

    if (con->t1->kind == T_VAR) {
      printf("%s : ", con->t1->data.T_VAR);
      print_type(con->t2);
    } else {
      print_type(con->t1);
      print_type(con->t2);
    }
    // if (con->src) {
    //   print_ast(con->src);
    // };
  }
}

void print_subst(Substitution *c) {
  if (!c) {
    return;
  }
  for (Substitution *con = c; con != NULL; con = con->next) {

    printf("subst: ");
    if (con->from->kind == T_VAR) {
      printf("%s with ", con->from->data.T_VAR);
      print_type(con->to);
    } else {
      print_type(con->from);
      printf("with ");
      print_type(con->to);
    }
  }
}

Type *unify_in_ctx(Type *t1, Type *t2, TICtx *ctx, Ast *node) {
  // printf("unify in ctx\n");
  // print_type(t1);
  // print_type(t2);

  if (types_equal(t1, t2)) {
    return t1;
  }

  if (IS_PRIMITIVE_TYPE(t1)) {
    ctx->constraints = constraints_extend(ctx->constraints, t2, t1);
    ctx->constraints->src = node;
    return t1;
  }

  if (!is_generic(t2)) {
    for (TypeClass *tc = t1->implements; tc; tc = tc->next) {
      if (!type_implements(t2, tc)) {

        char buf[20];
        return type_error(
            ctx, node,
            "Typecheck error type %s does not implement typeclass '%s' \n",
            type_to_string(t2, buf), tc->name);
      }
    }
  }

  if (t2->kind == T_VAR) {
    for (TypeClass *tc = t1->implements; tc; tc = tc->next) {
      typeclasses_extend(t2, tc);
    }
  }

  if (t1->kind == T_CONS && t2->kind == T_CONS &&
      (strcmp(t1->data.T_CONS.name, t1->data.T_CONS.name) == 0)) {
    int num_args = t1->data.T_CONS.num_args;

    for (int i = 0; i < num_args; i++) {

      Type *_t1 = t1->data.T_CONS.args[i];
      Type *_t2 = t2->data.T_CONS.args[i];

      if (!unify_in_ctx(_t1, _t2, ctx, node)) {
        return NULL;
      }
    }
  } else if (t1->kind == T_FN && t2->kind == T_FN) {
    ctx->constraints = constraints_extend(ctx->constraints, t1->data.T_FN.from,
                                          t2->data.T_FN.from);

    return unify_in_ctx(t1->data.T_FN.to, t2->data.T_FN.to, ctx, node);

  } else if (t1->kind == T_CONS && IS_PRIMITIVE_TYPE(t2)) {
    // printf("unify fail\n");
    // print_type(t1);
    // print_type(t2);
    return t2;
  } else if (t2->kind == T_VAR && t1->kind != T_VAR) {
    return unify_in_ctx(t2, t1, ctx, node);
  } else {
    ctx->constraints = constraints_extend(ctx->constraints, t1, t2);
    ctx->constraints->src = node;
  }

  return t1;
}

Type *infer_fn_application(Ast *ast, TICtx *ctx) {
  Type *fn_type = ast->data.AST_APPLICATION.function->md;

  if (fn_type->is_recursive_fn_ref) {
    fn_type = deep_copy_type(fn_type);
  }
  Type *_fn_type;

  int len = ast->data.AST_APPLICATION.len;
  Type *arg_types[len];

  TICtx app_ctx = {.scope = ctx->scope + 1};

  for (size_t i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    Ast *arg = ast->data.AST_APPLICATION.args + i;
    arg_types[i] = infer(arg, ctx);

    // For each argument, add a constraint that the function's parameter type
    // must match the argument type
    Type *param_type = fn_type->data.T_FN.from;
    // printf("fn app arg\n");
    // print_type(param_type);
    // print_type(arg_types[i]);
    if (!unify_in_ctx(param_type, arg_types[i], &app_ctx,
                      ast->data.AST_APPLICATION.args + i)) {

      // printf("%s\n", param_type->alias);
      // print_type(param_type);
      // print_type(arg_types[i]);
      //
      // printf("param type unify fail\n");
      return NULL;
    }

    if (i < ast->data.AST_APPLICATION.len - 1) {
      if (fn_type->data.T_FN.to->kind != T_FN) {
        return type_error(ctx, ast, "Too many arguments provided to function");
      }
      fn_type = fn_type->data.T_FN.to;
    }
  }

  Substitution *subst = solve_constraints(app_ctx.constraints);

  _fn_type = apply_substitution(subst, _fn_type);
  ast->data.AST_APPLICATION.function->md = _fn_type;

  Type *res_type = _fn_type;

  for (int i = 0; i < len; i++) {

    Ast *arg = ast->data.AST_APPLICATION.args + i;

    if (!((Type *)arg->md)->is_fn_param) {
      arg->md = apply_substitution(subst, arg->md);
    }

    if (is_generic(arg->md) &&
        !types_equal(arg->md, res_type->data.T_FN.from)) {
      ctx->constraints = constraints_extend(ctx->constraints, arg->md,
                                            res_type->data.T_FN.from);
      ctx->constraints->src = arg;
    }

    if (is_generic(arg_types[i]) && !types_equal(arg_types[i], arg->md)) {
      unify_in_ctx(arg_types[i], arg->md, ctx, arg);

      // printf("unify args:\n");
      // print_ast(arg);
      // print_type(arg_types[i]);
      // print_type(arg->md);
      // ctx->constraints =
      //     constraints_extend(ctx->constraints, arg_types[i], arg->md);
      // ctx->constraints->src = arg;
    }

    res_type = res_type->data.T_FN.to;
  }

  return res_type;
}

Type *find_variant_member(Type *variant, const char *name) {
  for (int i = 0; i < variant->data.T_CONS.num_args; i++) {
    Type *mem = variant->data.T_CONS.args[i];
    if (strcmp(mem->data.T_CONS.name, name) == 0) {
      return mem;
    }
  }
  return NULL;
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
  for (int i = 0; i < cons->data.T_CONS.num_args; i++) {

    Type *cons_arg = cons->data.T_CONS.args[i];

    Type *arg_type;
    if (!(arg_type = infer(ast->data.AST_APPLICATION.args + i, ctx))) {
      return type_error(
          ctx, ast, "Could not infer argument type in cons %s application\n",
          cons->data.T_CONS.name);
    }

    if (!unify_in_ctx(cons_arg, arg_type, &app_ctx, ast)) {
      return type_error(ctx, ast,
                        "Could not constrain type variable to function type\n");
    }
  }

  Substitution *subst = solve_constraints(app_ctx.constraints);
  return apply_substitution(subst, fn_type);
}

#define LIST_CONS_OPERATOR "::"

bool is_list_cons_operator(Ast *f) {
  return f->tag == AST_IDENTIFIER &&
         (strcmp(f->data.AST_IDENTIFIER.value, LIST_CONS_OPERATOR) == 0);
}

Type *infer_pattern(Ast *pattern, TICtx *ctx) {
  switch (pattern->tag) {
  case AST_IDENTIFIER: {
    const char *name = pattern->data.AST_IDENTIFIER.value;
    Type *lookup = env_lookup(ctx->env, name);
    Type *type;

    if (lookup && lookup->kind == T_CREATE_NEW_GENERIC) {
      lookup = lookup->data.T_CREATE_NEW_GENERIC(NULL);
      return lookup;
    }

    if (lookup != NULL && is_variant_type(lookup) &&
        strcmp(lookup->data.T_CONS.name, name) != 0) {
      return deep_copy_type(lookup);
    }
    return next_tvar();
  }

  case AST_TUPLE: {
    int len = pattern->data.AST_LIST.len;
    Type **member_types = talloc(sizeof(Type *) * len);

    for (int i = 0; i < len; i++) {
      member_types[i] = infer_pattern(&pattern->data.AST_LIST.items[i], ctx);
      if (!member_types[i])
        return NULL;
    }

    Type *tuple_type = create_tuple_type(len, member_types);

    return tuple_type;
  }

  case AST_APPLICATION: {
    if (is_list_cons_operator(pattern->data.AST_APPLICATION.function)) {
      Type *list_el_type = next_tvar();
      pattern->data.AST_APPLICATION.args->md = list_el_type;
      Type *type = talloc(sizeof(Type));
      Type **contained = talloc(sizeof(Type *));
      contained[0] = list_el_type;
      *type = (Type){T_CONS, {.T_CONS = {TYPE_NAME_LIST, contained, 1}}};
      pattern->data.AST_APPLICATION.args->md = list_el_type;
      pattern->data.AST_APPLICATION.args[1].md = type;
      return type;
    }

    Type *t = infer(pattern, ctx);
    return t;
    break;
  }
  default: {
    return infer(pattern, ctx);
  }
  }
}

TypeEnv *bind_in_env(TypeEnv *env, Ast *binding, Type *expr_type, int scope) {

  switch (binding->tag) {
  case AST_IDENTIFIER: {

    const char *name = binding->data.AST_IDENTIFIER.value;
    if (CHARS_EQ(name, "_")) {
      break;
    }
    expr_type->scope = scope;
    env = env_extend(env, name, expr_type);
    break;
  }

  case AST_TUPLE: {
    for (int i = 0; i < binding->data.AST_LIST.len; i++) {
      Ast *b = binding->data.AST_LIST.items + i;
      env = bind_in_env(env, b, expr_type->data.T_CONS.args[i], scope);
    }
    break;
  }
  case AST_APPLICATION: {
    if (is_list_cons_operator(binding->data.AST_APPLICATION.function)) {
      Type *el_type = expr_type->data.T_CONS.args[0];
      env =
          bind_in_env(env, binding->data.AST_APPLICATION.args, el_type, scope);
      env = bind_in_env(env, binding->data.AST_APPLICATION.args + 1, expr_type,
                        scope);
      break;
    }

    Ast *fn_id = binding->data.AST_APPLICATION.function;
    const char *fn_name = fn_id->data.AST_IDENTIFIER.value;
    Type *cons = expr_type;

    if (is_variant_type(expr_type)) {

      cons = find_variant_member(expr_type, fn_name);

      if (!cons) {
        fprintf(stderr, "Error: %s not found in variant %s\n", fn_name,
                cons->data.T_CONS.name);
        return NULL;
      }
    }

    for (int i = 0; i < binding->data.AST_APPLICATION.len; i++) {
      env = bind_in_env(env, binding->data.AST_APPLICATION.args + i,
                        cons->data.T_CONS.args[i], scope);
    }
    break;
  }
  }
  return env;
}

void bind_in_ctx(TICtx *ctx, Ast *binding, Type *expr_type) {
  ctx->env = bind_in_env(ctx->env, binding, expr_type, ctx->scope);
}

Type *infer_let_binding(Ast *ast, TICtx *ctx) {

  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;

  Type *expr_type;
  if (!(expr_type = infer(expr, ctx))) {
    return type_error(ctx, ast->data.AST_LET.expr,
                      "Typecheck Error: Could not infer expr type in let\n");
  }

  Type *binding_type;
  if (!(binding_type = infer_pattern(binding, ctx))) {
    return type_error(ctx, ast->data.AST_LET.binding,
                      "Typecheck Error: Could not infer binding type in let\n");
  }

  if (!unify_in_ctx(binding_type, expr_type, ctx, ast)) {
    return type_error(ctx, ast->data.AST_LET.binding,
                      "Typecheck Error: Could not unify binding type with "
                      "expression type in let\n");
    return NULL;
  }

  Ast *in_expr = ast->data.AST_LET.in_expr;
  if (in_expr != NULL) {
    TICtx body_ctx = *ctx;
    body_ctx.scope++;
    bind_in_ctx(&body_ctx, ast->data.AST_LET.binding, expr_type);

    Type *res_type = infer(ast->data.AST_LET.in_expr, &body_ctx);
    ctx->constraints = body_ctx.constraints;
    return res_type;
  }

  bind_in_ctx(ctx, ast->data.AST_LET.binding, expr_type);
  return expr_type;
}

Type *infer_lambda(Ast *ast, TICtx *ctx) {
  TICtx body_ctx = *ctx;
  body_ctx.scope++;
  body_ctx.current_fn_ast = ast;

  int num_params = ast->data.AST_LAMBDA.len;

  Type **param_types = talloc(sizeof(Type *) * num_params);

  for (int i = 0; i < num_params; i++) {
    Ast *param = &ast->data.AST_LAMBDA.params[i];
    Ast *def =
        ast->data.AST_LAMBDA.defaults ? ast->data.AST_LAMBDA.defaults[i] : NULL;

    Type *param_type;
    if (def != NULL) {
      param_type = compute_type_expression(def, ctx->env);
    } else {
      param_type = infer_pattern(param, &body_ctx);
    }

    param_type->is_fn_param = true;
    param_type->scope = body_ctx.scope;
    param_types[i] = param_type;
    bind_in_ctx(&body_ctx, param, param_types[i]);
  }

  bool is_named = ast->data.AST_LAMBDA.fn_name.chars != NULL;
  const char *name = ast->data.AST_LAMBDA.fn_name.chars;
  Type *fn_type_var = NULL;
  if (is_named) {
    fn_type_var = next_tvar();
    fn_type_var->is_recursive_fn_ref = true;

    Ast rec_fn_name_binding = {
        AST_IDENTIFIER,
        {.AST_IDENTIFIER = {.value = name,
                            .length = ast->data.AST_LAMBDA.fn_name.length}}};

    bind_in_ctx(&body_ctx, &rec_fn_name_binding, fn_type_var);
  }

  Type *body_type = infer(ast->data.AST_LAMBDA.body, &body_ctx);

  if (!body_type) {
    return type_error(ctx, ast,
                      "Typecheck Error: could not infer function body type\n");
  }

  Type *actual_fn_type = body_type;
  for (int i = num_params - 1; i >= 0; i--) {
    Type *new_fn = talloc(sizeof(Type));
    new_fn->kind = T_FN;
    new_fn->data.T_FN.from = param_types[i];
    new_fn->data.T_FN.to = actual_fn_type;
    actual_fn_type = new_fn;
  }

  ast->md = actual_fn_type;

  for (TypeConstraint *c = body_ctx.constraints; c; c = c->next) {
    Type *t1 = c->t1;
    Type *t2 = c->t2;
    if (t1->scope <= ctx->scope) {
      ctx->constraints = constraints_extend(ctx->constraints, t1, t2);
    }
  }
  // print_constraints(body_ctx.constraints);

  Substitution *subst = solve_constraints(body_ctx.constraints);
  // printf("lambda subs\n");
  // print_subst(subst);

  ast->md = apply_substitution(subst, ast->md);

  if (is_named) {
    apply_substitutions_rec(ast->data.AST_LAMBDA.body, subst);
  }

  if (body_ctx.yielded_type != NULL) {
    ast->md = coroutine_constructor_type_from_fn_type(ast->md);
  }

  return ast->md;
}

Type *infer_match_expr(Ast *ast, TICtx *ctx) {

  Type *result = next_tvar();
  Ast *expr = ast->data.AST_MATCH.expr;
  Type *expr_type;
  if (!(expr_type = infer(expr, ctx))) {
    return type_error(
        ctx, expr, "Typecheck Error: Could not infer match expression type\n");
  }
  int len = ast->data.AST_MATCH.len;
  Type *last_branch_type = NULL;
  for (int i = 0; i < ast->data.AST_MATCH.len; i++) {

    Ast *branch_pattern = &ast->data.AST_MATCH.branches[2 * i];
    Ast *guard_clause = NULL;

    if (branch_pattern->tag == AST_MATCH_GUARD_CLAUSE) {
      guard_clause = branch_pattern->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
      branch_pattern = branch_pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    }

    Ast *branch_body = &ast->data.AST_MATCH.branches[1 + (2 * i)];

    Type *pattern_type;
    if (!(pattern_type = infer_pattern(branch_pattern, ctx))) {
      return type_error(ctx, branch_pattern,
                        "Typecheck Error: Could not infer pattern type\n");
    }

    if (!unify_in_ctx(expr_type, pattern_type, ctx, branch_pattern)) {
      return type_error(ctx, branch_pattern,
                        "Typecheck Error: Could not unify pattern type with "
                        "matched value type\n");
    }

    TICtx branch_ctx = *ctx;

    branch_ctx.scope++;
    bind_in_ctx(&branch_ctx, branch_pattern, pattern_type);

    Type *guard_clause_type;
    if (guard_clause &&
        !(guard_clause_type = infer(guard_clause, &branch_ctx))) {
      return type_error(
          ctx, guard_clause,
          "Typecheck Error: Could not guard clause in match branch\n");
    }

    Type *branch_type;
    if (!(branch_type = infer(branch_body, &branch_ctx))) {
      return type_error(
          ctx, branch_body,
          "Typecheck Error: Could not infer type of match branch body\n");
    }

    if (!unify_in_ctx(result, branch_type, &branch_ctx, branch_body)) {
      return type_error(ctx, branch_body,
                        "Inconsistent types in match branches\n");
    }

    if (last_branch_type != NULL) {

      if (!unify_in_ctx(last_branch_type, branch_type, &branch_ctx,
                        branch_body)) {
        return type_error(ctx, branch_body,
                          "Inconsistent types in match branches\n");
      }
    }

    last_branch_type = branch_type;
    ctx->constraints = branch_ctx.constraints;
  }
  return result;
}

bool _is_option_type(Type *t) {
  return (t->alias != NULL) && (strcmp(t->alias, "Option") == 0);
}

bool occurs_check(Type *var, Type *t) {
  if (t->kind == T_VAR) {
    return strcmp(var->data.T_VAR, t->data.T_VAR) == 0;
  }

  if (t->kind == T_FN) {
    return occurs_check(var, t->data.T_FN.from) ||
           occurs_check(var, t->data.T_FN.to);
  }

  if (t->kind == T_CONS || t->kind == T_TYPECLASS_RESOLVE) {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      if (occurs_check(var, t->data.T_CONS.args[i])) {
        return true;
      }
    }
  }

  return false;
}

Type *apply_substitution(Substitution *subst, Type *t) {
  if (!subst) {
    return t;
  }

  if (!t) {
    return NULL;
  }

  switch (t->kind) {

  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_STRING: {
    return t;
  }
  case T_VAR: {
    Substitution *s = subst;
    while (s) {
      if (types_equal(s->from, t)) {
        Type *to = s->to;
        if (is_generic(to)) {
          return apply_substitution(subst, to);
        } else {
          return to;
        }
      }
      s = s->next;
    }
    break;
  }
  case T_TYPECLASS_RESOLVE: {

    Type *new_t = talloc(sizeof(Type));
    *new_t = *t;

    new_t->data.T_CONS.args = talloc(sizeof(Type *) * t->data.T_CONS.num_args);
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      new_t->data.T_CONS.args[i] =
          apply_substitution(subst, t->data.T_CONS.args[i]);
    }
    return resolve_tc_rank(new_t);
  }

  case T_CONS: {
    Type *new_t = talloc(sizeof(Type));
    *new_t = *t;
    new_t->data.T_CONS.args = talloc(sizeof(Type *) * t->data.T_CONS.num_args);
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      new_t->data.T_CONS.args[i] =
          apply_substitution(subst, t->data.T_CONS.args[i]);
    }
    return new_t;
  }
  case T_FN: {
    Type *new_t = talloc(sizeof(Type));
    *new_t = *t;
    new_t->data.T_FN.from = apply_substitution(subst, t->data.T_FN.from);
    new_t->data.T_FN.to = apply_substitution(subst, t->data.T_FN.to);
    return new_t;
  }
  default: {
    // printf("apply subst\n");
    // print_type(t);
    // print_subst(subst);
    break;
  }
  }
  return t;
}

Substitution *substitutions_extend(Substitution *subst, Type *t1, Type *t2) {
  if (types_equal(t1, t2)) {
    return subst;
  }

  Substitution *new_subst = talloc(sizeof(Substitution));
  new_subst->from = t1;
  new_subst->to = t2;
  new_subst->next = subst;

  return new_subst;
}

bool cons_types_match(Type *t1, Type *t2) {
  return (t1->kind == T_CONS) && (t2->kind == T_CONS) &&
         (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) == 0);
}

Substitution *solve_constraints(TypeConstraint *constraints) {
  Substitution *subst = NULL;

  while (constraints) {

    Type *t1 = apply_substitution(subst, constraints->t1);
    Type *t2 = apply_substitution(subst, constraints->t2);

    if (!t1 || !t2) {
      constraints = constraints->next;
      continue;
    }

    if (t1->kind == T_CONS && ((1 << t2->kind) & TYPE_FLAGS_PRIMITIVE)) {
      print_type(t2);
      print_type(t1);

      TICtx _ctx = {.err_stream = NULL};
      return type_error(
          &_ctx, constraints->src,
          "Cannot constrain cons type to primitive simple type\n");
    }

    if (t1->kind == t2->kind && IS_PRIMITIVE_TYPE(t1)) {
      constraints = constraints->next;
      continue;
    }

    if (t1->kind == T_VAR) {
      if (occurs_check(t1, t2)) {
        constraints = constraints->next;
        continue;
      }

      subst = substitutions_extend(subst, t1, t2);
    } else if (t2->kind == T_VAR) {
      if (occurs_check(t2, t1)) {
        constraints = constraints->next;
        continue;
      }

      subst = substitutions_extend(subst, t2, t1);

    } else if (IS_PRIMITIVE_TYPE(t1) && t2->kind == T_TYPECLASS_RESOLVE) {
      for (int i = 0; i < t2->data.T_CONS.num_args; i++) {
        subst = substitutions_extend(subst, t2->data.T_CONS.args[i], t1);
      }
    } else if (IS_PRIMITIVE_TYPE(t2) && t1->kind == T_TYPECLASS_RESOLVE) {

      for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
        subst = substitutions_extend(subst, t1->data.T_CONS.args[i], t2);
      }
    } else if (cons_types_match(t1, t2)) {
      if (is_variant_type(t1) && is_variant_type(t2)) {
        for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
          Type *mem1 = t1->data.T_CONS.args[i];
          Type *mem2 = t2->data.T_CONS.args[i];

          if (mem1->kind == T_CONS && mem1->data.T_CONS.num_args > 0) {
            TypeConstraint *next = talloc(sizeof(TypeConstraint));
            next->next = constraints->next;
            next->t1 = mem1;
            next->t2 = mem2;
            next->src = constraints->src;
            constraints->next = next;
            continue;
          }
        }
      } else if (is_generic(t1) && (!is_generic(t2))) {
        for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
          subst = substitutions_extend(subst, t1->data.T_CONS.args[i],
                                       t2->data.T_CONS.args[i]);
        }
      } else {
        for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
          subst = substitutions_extend(subst, t1->data.T_CONS.args[i],
                                       t2->data.T_CONS.args[i]);
        }
      }
    } else if (t1->kind == T_FN && t2->kind == T_FN) {
      Type *f1 = t1;
      Type *f2 = t2;
      while (f1->kind == T_FN && f2->kind == T_FN) {
        subst =
            substitutions_extend(subst, f1->data.T_FN.from, f2->data.T_FN.from);

        f1 = f1->data.T_FN.to;
        f2 = f2->data.T_FN.to;
      }

      subst = substitutions_extend(subst, f1, f2);
    } else if (t1->kind == T_EMPTY_LIST && is_list_type(t2)) {
      *t1 = *t2;
    } else if (is_pointer_type(t1) && t2->kind == T_FN) {
    } else {

      TICtx _ctx = {.err_stream = NULL};
      char buf1[100] = {};
      char buf2[100] = {};

      type_error(&_ctx, constraints->src,
                 "Constraint solving type mismatch %s != %s\n",
                 t1->alias ? t1->alias : type_to_string(t1, buf1),
                 t2->alias ? t2->alias : type_to_string(t2, buf2));
    }

    constraints = constraints->next;
  }

  return subst;
}

void apply_substitutions_rec(Ast *ast, Substitution *subst) {
  if (!ast) {
    return;
  }

  switch (ast->tag) {
  case AST_TUPLE:
  case AST_ARRAY:
  case AST_LIST: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      apply_substitutions_rec(ast->data.AST_LIST.items + i, subst);
    }

    ast->md = apply_substitution(subst, ast->md);
    break;
  }

  case AST_INT:
  case AST_DOUBLE:
  case AST_CHAR:
  case AST_BOOL:
  case AST_STRING: {
    break;
  }

  case AST_FMT_STRING: {

    // int arity = ast->data.AST_LIST.len;
    // for (int i = 0; i < arity; i++) {
    // Ast *member = ast->data.AST_LIST.items + i;
    // if (member->tag == AST_IDENTIFIER &&
    //     CHARS_EQ(member->data.AST_IDENTIFIER.value, "x")) {
    //   print_ast(member);
    //   print_type(member->md);
    //   print_type(member->md);
    // }
    // apply_substitutions_rec(member, subst);
    // }

    // ast->md = apply_substitution(subst, ast->md);
    break;
  }

  case AST_LAMBDA: {
    apply_substitutions_rec(ast->data.AST_LAMBDA.body, subst);
    break;
  }

  case AST_BODY: {
    Type *fin;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      apply_substitutions_rec(ast->data.AST_BODY.stmts[i], subst);
      fin = ast->data.AST_BODY.stmts[i]->md;
    }
    ast->md = fin;
    break;
  }

  case AST_APPLICATION: {

    apply_substitutions_rec(ast->data.AST_APPLICATION.function, subst);
    //
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      // print_ast(ast->data.AST_APPLICATION.args + i);
      // print_type((ast->data.AST_APPLICATION.args + i)->md);
      apply_substitutions_rec(ast->data.AST_APPLICATION.args + i, subst);
    }
    ast->md = apply_substitution(subst, ast->md);
    break;
  }

  case AST_LET: {
    apply_substitutions_rec(ast->data.AST_LET.expr, subst);
    Type *override = ast->data.AST_LET.expr->md;

    if (ast->data.AST_LET.in_expr) {
      apply_substitutions_rec(ast->data.AST_LET.in_expr, subst);
      override = ast->data.AST_LET.in_expr->md;
    }

    ast->md = override;
    break;
  }

  case AST_MATCH: {
    apply_substitutions_rec(ast->data.AST_MATCH.expr, subst);
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      apply_substitutions_rec(ast->data.AST_MATCH.branches + (2 * i), subst);
      apply_substitutions_rec(ast->data.AST_MATCH.branches + (2 * i) + 1,
                              subst);
    }

    ast->md = apply_substitution(subst, ast->md);
    break;
  }

  case AST_MATCH_GUARD_CLAUSE: {
    apply_substitutions_rec(ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr, subst);
    break;
  }

  case AST_YIELD: {
    apply_substitutions_rec(ast->data.AST_YIELD.expr, subst);
    ast->md = apply_substitution(subst, ast);
    break;
  }

  default: {
    ast->md = apply_substitution(subst, ast->md);
    break;
  }
  }
}

Type *solve_program_constraints(Ast *prog, TICtx *ctx) {
  Substitution *subst = solve_constraints(ctx->constraints);
  //
  // print_constraints(ctx->constraints);
  // print_subst(subst);
  //
  if (ctx->constraints && !subst) {
    return NULL;
  }

  apply_substitutions_rec(prog, subst);

  return prog->md;
}
