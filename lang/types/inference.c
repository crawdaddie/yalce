#include "inference.h"
#include "./infer_application.h"
#include "builtins.h"
#include "modules.h"
#include "serde.h"
#include "types/common.h"
#include "types/type.h"
#include "types/type_declaration.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

Type *infer_yield_stmt(Ast *ast, TICtx *ctx);
Type *infer_lambda(Ast *ast, TICtx *ctx);
Type *infer_let_binding(Ast *ast, TICtx *ctx);
Type *infer_match_expr(Ast *ast, TICtx *ctx);
Type *infer_module(Ast *ast, TICtx *ctx);
Type *infer(Ast *ast, TICtx *ctx);
Type *infer_assignment(Ast *ast, TICtx *ctx);

Type *for_loop_binding(Ast *binding, Ast *expr, Ast *body, TICtx *ctx);
bool occurs_check(Type *var, Type *t);

void bind_in_ctx(TICtx *ctx, Ast *binding, Type *expr_type);
void apply_substitutions_rec(Ast *ast, Substitution *subst);
Substitution *solve_constraints(TypeConstraint *constraints);

Type *unify_in_ctx(Type *t1, Type *t2, TICtx *ctx, Ast *node);

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

TypeEnv *env_lookup_ref(TypeEnv *env, const char *name) {
  while (env) {
    if (env->name && strcmp(env->name, name) == 0) {
      return env;
    }

    env = env->next;
  }

  return NULL;
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

    for (int i = 0; i < len; i++) {

      Ast *member = ast->data.AST_LIST.items + i;
      Type *mtype = infer(member, ctx);
      cons_args[i] = mtype;
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

    break;
  }

  case AST_TYPE_DECL: {
    TypeEnv *env = ctx->env;
    type = type_declaration(ast, &env);
    ctx->env = env;
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

    TypeEnv *ref = env_lookup_ref(ctx->env, name);
    if (ref) {
      // printf("ast identifier %s scope: %d this scope: %d (is fn param
      // %d)\n",
      //        name, ref->type->scope, ctx->scope, ref->is_fn_param);
      ref->ref_count++;

      ast->data.AST_IDENTIFIER.is_fn_param = ref->is_fn_param;
      ast->data.AST_IDENTIFIER.is_recursive_fn_ref = ref->is_recursive_fn_ref;

      if (ctx->current_fn_ast &&
          ctx->current_fn_ast->data.AST_LAMBDA.num_yields >
              ref->type->yield_boundary) {

        ast->data.AST_IDENTIFIER.crosses_yield_boundary = true;
        Type *t = ref->type;

        // scan boundary crosser list
        bool ref_already_listed = false;

        for (AstList *bx =
                 ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers;
             bx; bx = bx->next) {

          if (CHARS_EQ(bx->ast->data.AST_IDENTIFIER.value, name)) {
            ref_already_listed = true;
            break;
          }
        }

        if ((t->scope >= ctx->current_fn_scope) && !ref_already_listed &&
            !(ast->data.AST_IDENTIFIER.is_fn_param ||
              ast->data.AST_IDENTIFIER.is_recursive_fn_ref)) {

          AstList *next = malloc(sizeof(AstList));
          next->ast = ast;
          next->next =
              ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers;
          ctx->current_fn_ast->data.AST_LAMBDA.yield_boundary_crossers = next;
          ctx->current_fn_ast->data.AST_LAMBDA.num_yield_boundary_crossers++;
        }
      }
      type = ref->type;
    }

    if (!ref) {
      Type *t = lookup_builtin_type(name);

      if (t && t->kind == T_CREATE_NEW_GENERIC) {
        type = t->data.T_CREATE_NEW_GENERIC(NULL);
      } else if (t) {
        type = t;
      } else {
        type = next_tvar();
      }
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
  case AST_MODULE: {
    type = infer_module(ast, ctx);
    break;
  }

  case AST_IMPORT: {
    const char *name = ast->data.AST_IMPORT.identifier;

    type = get_import_type(ast);
    if (ast->data.AST_IMPORT.import_all) {

      for (int i = 0; i < type->data.T_CONS.num_args; i++) {
        char *name = type->data.T_CONS.names[i];
        Ast binding = {AST_IDENTIFIER, .data = {.AST_IDENTIFIER = {
                                                    .value = name,
                                                    .length = strlen(name),
                                                }}};
        bind_in_ctx(ctx, &binding, type->data.T_CONS.args[i]);
      }
      break;
    }

    Ast binding = {AST_IDENTIFIER, .data = {.AST_IDENTIFIER = {
                                                .value = name,
                                                .length = strlen(name),
                                            }}};

    bind_in_ctx(ctx, &binding, type);
    break;
  }
  case AST_RECORD_ACCESS: {
    Type *rec_type = infer(ast->data.AST_RECORD_ACCESS.record, ctx);

    if (rec_type->kind == T_VAR && rec_type->is_recursive_type_ref) {
      const char *rec_type_name = rec_type->data.T_VAR;
      Type *record_type = env_lookup(ctx->env, rec_type_name);
      if (record_type) {
        rec_type = record_type;
      }
    }

    if (rec_type->kind != T_CONS) {
      return NULL;
    }

    if (rec_type->data.T_CONS.names == NULL) {
      return NULL;
    }

    const char *member_name =
        ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;

    for (int i = 0; i < rec_type->data.T_CONS.num_args; i++) {
      if (CHARS_EQ(rec_type->data.T_CONS.names[i], member_name)) {
        type = rec_type->data.T_CONS.args[i];
        ast->data.AST_RECORD_ACCESS.index = i;
        int array = is_array_type(type);
        int li = is_list_type(type);
        if (array || li) {
          Type *el_type = type->data.T_CONS.args[0];
          if (el_type->kind == T_VAR && el_type->is_recursive_type_ref &&
              CHARS_EQ(el_type->data.T_VAR, rec_type->data.T_CONS.name)) {
            if (array) {
              type = create_array_type(rec_type);
            } else {
              type = create_list_type_of_type(rec_type);
            }
          }
        }

        break;
      }
    }

    break;
  }
  case AST_RANGE_EXPRESSION: {
    Type *from = infer(ast->data.AST_RANGE_EXPRESSION.from, ctx);
    Type *to = infer(ast->data.AST_RANGE_EXPRESSION.to, ctx);
    unify_in_ctx(from, &t_int, ctx, ast->data.AST_RANGE_EXPRESSION.from);
    unify_in_ctx(to, &t_int, ctx, ast->data.AST_RANGE_EXPRESSION.to);
    type = &t_int;
    break;
  }

  case AST_LOOP: {
    Ast let = *ast;
    if (is_loop_of_iterable(ast)) {
      type = for_loop_binding(let.data.AST_LET.binding, let.data.AST_LET.expr,
                              let.data.AST_LET.in_expr, ctx);

      break;
    }
    let.tag = AST_LET;
    type = infer(&let, ctx);
    break;
  }
  case AST_BINOP: {
    if (ast->data.AST_BINOP.op == TOKEN_ASSIGNMENT) {
      type = infer_assignment(ast, ctx);
      break;
    }
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

  ctx->current_fn_ast->data.AST_LAMBDA.num_yields++;
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

  return yield_expr_type;
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

TypeEnv *bind_in_env(TypeEnv *env, Ast *binding, Type *expr_type, int scope,
                     Ast *current_fn) {

  switch (binding->tag) {
  case AST_IDENTIFIER: {

    const char *name = binding->data.AST_IDENTIFIER.value;
    if (CHARS_EQ(name, "_")) {
      break;
    }
    expr_type->scope = scope;
    expr_type->yield_boundary =
        current_fn ? current_fn->data.AST_LAMBDA.num_yields : 0;
    env = env_extend(env, name, expr_type);
    break;
  }

  case AST_TUPLE: {

    int len = binding->data.AST_LIST.len;
    if (expr_type->kind == T_VAR) {
      Type **cons_els = talloc(sizeof(Type *) * len);
      for (int i = 0; i < len; i++) {
        cons_els[i] = next_tvar();
      }
      *expr_type = (Type){T_CONS, .data = {.T_CONS = {.name = TYPE_NAME_TUPLE,
                                                      .args = cons_els,
                                                      .num_args = len}}};
    }

    for (int i = 0; i < len; i++) {
      Ast *b = binding->data.AST_LIST.items + i;
      env = bind_in_env(env, b, expr_type->data.T_CONS.args[i], scope,
                        current_fn);
    }
    break;
  }
  case AST_APPLICATION: {
    if (is_list_cons_operator(binding->data.AST_APPLICATION.function)) {
      Type *el_type = expr_type->data.T_CONS.args[0];
      env = bind_in_env(env, binding->data.AST_APPLICATION.args, el_type, scope,
                        current_fn);
      env = bind_in_env(env, binding->data.AST_APPLICATION.args + 1, expr_type,
                        scope, current_fn);
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
                        cons->data.T_CONS.args[i], scope, current_fn);
    }
    break;
  }
  }
  return env;
}

void bind_in_ctx(TICtx *ctx, Ast *binding, Type *expr_type) {
  ctx->env = bind_in_env(ctx->env, binding, expr_type, ctx->scope,
                         ctx->current_fn_ast);
}

Type *infer_let_binding(Ast *ast, TICtx *ctx) {

  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  Ast *in_expr = ast->data.AST_LET.in_expr;

  if (binding == NULL && expr->tag == AST_IMPORT && in_expr) {
    TICtx body_ctx = *ctx;
    body_ctx.scope++;
    infer(expr, &body_ctx);
    return infer(in_expr, &body_ctx);
  }

  if (binding != NULL && expr->tag == AST_IMPORT) {
    print_ast(ast);
    TICtx body_ctx = *ctx;
    body_ctx.scope++;
    infer(expr, &body_ctx);
    return infer(in_expr, &body_ctx);
  }

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

Type *for_loop_binding(Ast *binding, Ast *expr, Ast *body, TICtx *ctx) {

  Type *expr_type;
  if (!(expr_type = infer(expr, ctx))) {
    return type_error(ctx, expr,
                      "Typecheck Error: Could not infer expr type in loop\n");
  }
  if (is_coroutine_type(expr_type)) {
    expr_type = type_of_option(fn_return_type(expr_type));
  }

  TICtx body_ctx = *ctx;
  body_ctx.scope++;
  Type *binding_type;
  if (!(binding_type = infer_pattern(binding, &body_ctx))) {
    return type_error(
        ctx, binding,
        "Typecheck Error: Could not infer binding type in loop\n");
  }

  if (!unify_in_ctx(binding_type, expr_type, &body_ctx, binding)) {
    return type_error(ctx, binding,
                      "Typecheck Error: Could not unify binding type with "
                      "expression type in let\n");
    return NULL;
  }

  bind_in_ctx(&body_ctx, binding, expr_type);
  binding->md = expr_type;
  Type *res_type = infer(body, &body_ctx);
  ctx->constraints = body_ctx.constraints;
  return res_type;
}

Type *infer_lambda(Ast *ast, TICtx *ctx) {

  TICtx body_ctx = *ctx;
  body_ctx.scope++;
  body_ctx.current_fn_ast = ast;
  body_ctx.current_fn_scope = body_ctx.scope;

  int num_params = ast->data.AST_LAMBDA.len;

  Type **param_types = talloc(sizeof(Type *) * num_params);

  for (int i = 0; i < num_params; i++) {
    Ast *param = &ast->data.AST_LAMBDA.params[i];
    Ast *def = ast->data.AST_LAMBDA.type_annotations
                   ? ast->data.AST_LAMBDA.type_annotations[i]
                   : NULL;

    Type *param_type;
    if (def != NULL) {
      param_type = compute_type_expression(def, ctx->env);
    } else {
      param_type = infer_pattern(param, &body_ctx);
    }

    param_type->scope = body_ctx.scope;
    param_types[i] = param_type;
    bind_in_ctx(&body_ctx, param, param_types[i]);
    if (body_ctx.env) {
      body_ctx.env->is_fn_param = true;
    }
  }

  bool is_named = ast->data.AST_LAMBDA.fn_name.chars != NULL;
  const char *name = ast->data.AST_LAMBDA.fn_name.chars;
  Type *fn_type_var = NULL;

  if (is_named) {
    fn_type_var = next_tvar();

    Ast rec_fn_name_binding = {
        AST_IDENTIFIER,
        {.AST_IDENTIFIER = {.value = name,
                            .length = ast->data.AST_LAMBDA.fn_name.length}}};

    bind_in_ctx(&body_ctx, &rec_fn_name_binding, fn_type_var);
    if (body_ctx.env) {
      body_ctx.env->is_recursive_fn_ref = true;
    }
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

  Substitution *subst = solve_constraints(body_ctx.constraints);
#ifdef DEBUG_LAMBDA_CONSTRAINTS
  printf("## LAMBDA: %s\n", name);
  printf("constraints throughout lambda\n");
  print_constraints(body_ctx.constraints);
  print_subst(subst);
#endif

  ast->md = apply_substitution(subst, ast->md);

  // if (is_named) {
  apply_substitutions_rec(ast->data.AST_LAMBDA.body, subst);
  // }

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

    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      if (member->tag == AST_IDENTIFIER &&
          CHARS_EQ(member->data.AST_IDENTIFIER.value, "x")) {
      }
      apply_substitutions_rec(member, subst);
    }

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

    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      apply_substitutions_rec(ast->data.AST_APPLICATION.args + i, subst);
    }
    ast->md = apply_substitution(subst, ast->md);

    break;
  }
  case AST_LOOP:
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
bool is_loop_of_iterable(Ast *let) {
  return let->data.AST_LET.expr->tag == AST_APPLICATION &&
         let->data.AST_LET.expr->data.AST_APPLICATION.function->tag ==
             AST_IDENTIFIER &&
         CHARS_EQ(let->data.AST_LET.expr->data.AST_APPLICATION.function->data
                      .AST_IDENTIFIER.value,
                  "iter");
}

Type *solve_program_constraints(Ast *prog, TICtx *ctx) {
  Substitution *subst = solve_constraints(ctx->constraints);

  if (ctx->constraints && !subst) {
    return NULL;
  }

  apply_substitutions_rec(prog, subst);

  return prog->md;
}

Type *infer_module(Ast *ast, TICtx *ctx) {
  if (ast->data.AST_LAMBDA.len > 0) {
    // printf("infer parametrized module\n");
    for (int i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      Ast *param = ast->data.AST_LAMBDA.params + i;
    }
    return type_error(ctx, ast, "Error: parametrized modules not implemented");
  }

  Ast body;
  if (ast->data.AST_LAMBDA.body->tag != AST_BODY) {
    body = (Ast){
        AST_BODY,
        .data = {.AST_BODY = {.len = 1, .stmts = &ast->data.AST_LAMBDA.body}}};
  } else {
    body = *ast->data.AST_LAMBDA.body;
  }

  TICtx module_ctx = *ctx;
  TypeEnv *env_start = module_ctx.env;

  Ast *stmt;
  int len = body.data.AST_BODY.len;
  Type **member_types = talloc(sizeof(Type *) * len);
  const char **names = talloc(sizeof(char *) * len);

  for (int i = 0; i < len; i++) {
    stmt = body.data.AST_BODY.stmts[i];
    if (!((stmt->tag == AST_LET) || (stmt->tag == AST_TYPE_DECL) ||
          (stmt->tag == AST_IMPORT))) {
      return type_error(ctx, stmt,
                        "Please only have let statements and type declarations "
                        "in a module\n");
      return NULL;
    }

    Type *t = infer(stmt, &module_ctx);
    member_types[i] = t;

    if (stmt->tag == AST_TYPE_DECL) {
      names[i] = stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;

    } else if (stmt->tag == AST_IMPORT) {

      names[i] = stmt->data.AST_IMPORT.identifier;
    } else {
      names[i] = stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;
    }

    if (!t) {
      print_ast_err(stmt);
      return NULL;
    }
  }

  TypeEnv *env = module_ctx.env;

  Type *module_struct_type =
      create_cons_type(TYPE_NAME_MODULE, len, member_types);
  module_struct_type->data.T_CONS.names = names;
  ctx->env = env;
  return module_struct_type;
}

Type *infer_assignment(Ast *ast, TICtx *ctx) {
  Type *val_type = infer(ast->data.AST_BINOP.right, ctx);
  Type *var_type = infer(ast->data.AST_BINOP.left, ctx);
  print_type(val_type);
  print_type(var_type);
  return &t_void;
}
