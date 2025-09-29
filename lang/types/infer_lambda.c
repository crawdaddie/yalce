#include "./infer_lambda.h"
#include "./builtins.h"
#include "common.h"
#include "inference.h"
#include "serde.h"
#include "types/type_expressions.h"
#include "types/type_ser.h"
#include "types/unification.h"
#include <string.h>

void initial_lambda_signature(int num_params, Type **param_types,
                              AstList *type_annotation_list, AstList *params,
                              TICtx *ctx) {

  int i = 0;
  for (AstList *ps = params; ps; ps = ps->next, i++,
               type_annotation_list = type_annotation_list
                                          ? type_annotation_list->next
                                          : NULL) {

    Ast *param_ast = ps->ast;

    if (param_ast->tag == AST_VOID) {
      param_types[i] = &t_void;
      continue;
    }

    Ast *type_annotation =
        type_annotation_list ? type_annotation_list->ast : NULL;

    param_types[i] = type_annotation
                         ? compute_type_expression(type_annotation, ctx)
                         : next_tvar();
  }
}

void bind_lambda_params(Ast *ast, Type **param_types, TICtx *ctx) {
  int i = 0;
  for (AstList *pl = ast->data.AST_LAMBDA.params; pl; pl = pl->next, i++) {
    Ast *param_pattern = pl->ast;
    Type *param_type = param_types[i];

    if (bind_type_in_ctx(
            param_pattern, param_type,
            (binding_md){BT_FN_PARAM, {.FN_PARAM = {.scope = ctx->scope}}},
            ctx)) {

      type_error(param_pattern, "Cannot bind lambda parameter");
      return;
    }
  }
}

void bind_recursive_ref(Ast *ast, Type *function_type, TICtx *ctx) {
  if (ast->data.AST_LAMBDA.fn_name.chars == NULL) {
    return;
  }
  const char *fn_name = ast->data.AST_LAMBDA.fn_name.chars;
  Ast id = (Ast){
      .tag = AST_IDENTIFIER,
      {.AST_IDENTIFIER = {.value = fn_name,
                          .length = ast->data.AST_LAMBDA.fn_name.length}}};
  bind_type_in_ctx(&id, function_type, (binding_md){BT_RECURSIVE_REF}, ctx);
}

void unify_recursive_ref(Ast *ast, Type *recursive_fn_type, Type *result_type,
                         TICtx *ctx) {

  // If this is a recursive function, unify the recursive reference with the
  // final type
  if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
    TICtx unify_ctx = {};
    if (unify(recursive_fn_type, result_type, &unify_ctx)) {
      type_error(ast, "Recursive function type mismatch");
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
  Type **arg = t_alloc(sizeof(Type *));
  arg[0] = ret_type;
  // / instance_type;
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

  Type **a = t_alloc(sizeof(Type *));
  a[0] = fn_type;
  Type *constructor_type =
      create_cons_type(TYPE_NAME_COROUTINE_CONSTRUCTOR, 1, a);

  return constructor_type;
}

Type *create_closure(Ast *ast, Type *fn_type, TICtx *ctx) {
  int num = ast->data.AST_LAMBDA.num_closed_vals;

  if (num == 0) {
    return NULL;
  }
  Type **closed_types = t_alloc(sizeof(Type) * num);

  int i = 0;
  for (AstList *l = ast->data.AST_LAMBDA.closed_vals; l; l = l->next, i++) {
    // TODO: does the order here need to be reversed?
    // ast->data.AST_LAMBDA.closed_vals is in reverse order from when things are
    // used in the lambda
    // therefore the closed_types array will also be in reverse order
    closed_types[i] = l->ast->md;
  }
  fn_type->closure_meta = create_tuple_type(num, closed_types);
  return fn_type;
}

void apply_substitution_to_lambda_body(Ast *ast, Subst *subst) {
  if (!ast) {
    return;
  }

  switch (ast->tag) {

  case AST_INT: {
    break;
  }

  case AST_DOUBLE: {
    break;
  }

  case AST_STRING: {
    break;
  }

  case AST_CHAR: {
    break;
  }

  case AST_BOOL: {
    break;
  }
  case AST_VOID: {
    break;
  }

  case AST_FMT_STRING:
  case AST_ARRAY:
  case AST_LIST:
  case AST_TUPLE: {
    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *it = ast->data.AST_LIST.items + i;
      apply_substitution_to_lambda_body(it, subst);
    }
    break;
  }

  case AST_TYPE_DECL: {
    break;
  }

  case AST_BODY: {
    Ast *stmt;
    AST_LIST_ITER(ast->data.AST_BODY.stmts,
                  ({ apply_substitution_to_lambda_body(l->ast, subst); }));
    break;
  }

  case AST_IDENTIFIER: {
    break;
  }

  case AST_APPLICATION: {

    apply_substitution_to_lambda_body(ast->data.AST_APPLICATION.function,
                                      subst);

    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      apply_substitution_to_lambda_body(ast->data.AST_APPLICATION.args + i,
                                        subst);
    }

    break;
  }
  case AST_LOOP:
  case AST_LET: {
    apply_substitution_to_lambda_body(ast->data.AST_LET.binding, subst);
    apply_substitution_to_lambda_body(ast->data.AST_LET.expr, subst);
    apply_substitution_to_lambda_body(ast->data.AST_LET.in_expr, subst);
    break;
  }

  case AST_LAMBDA: {
    apply_substitution_to_lambda_body(ast->data.AST_LAMBDA.body, subst);
    break;
  }

  case AST_MATCH: {
    apply_substitution_to_lambda_body(ast->data.AST_MATCH.expr, subst);

    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {

      apply_substitution_to_lambda_body(ast->data.AST_MATCH.branches + 2 * i,
                                        subst);
      apply_substitution_to_lambda_body(
          ast->data.AST_MATCH.branches + 2 * i + 1, subst);
    }
    break;
  }

  case AST_MATCH_GUARD_CLAUSE: {

    apply_substitution_to_lambda_body(
        ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr, subst);

    apply_substitution_to_lambda_body(
        ast->data.AST_MATCH_GUARD_CLAUSE.test_expr, subst);
    //
    // print_type(ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr->data.AST_APPLICATION
    //                .function->md);
    //
    // printf("match test expr\n");
    // print_type(ast->data.AST_MATCH_GUARD_CLAUSE.test_expr->md);
    break;
  }
  //
  case AST_EXTERN_FN: {
    break;
  }

  case AST_YIELD: {
    apply_substitution_to_lambda_body(ast->data.AST_YIELD.expr, subst);
    break;
  }

  case AST_MODULE: {
    break;
  }

  case AST_TRAIT_IMPL: {
    break;
  }

  case AST_IMPORT: {
    break;
  }

  case AST_RECORD_ACCESS: {

    apply_substitution_to_lambda_body(ast->data.AST_RECORD_ACCESS.record,
                                      subst);
    apply_substitution_to_lambda_body(ast->data.AST_RECORD_ACCESS.member,
                                      subst);
    break;
  }

  case AST_RANGE_EXPRESSION: {

    apply_substitution_to_lambda_body(ast->data.AST_RANGE_EXPRESSION.from,
                                      subst);

    apply_substitution_to_lambda_body(ast->data.AST_RANGE_EXPRESSION.to, subst);
    break;
  }

  case AST_BINOP: {
    break;
  }

  default: {
    break;
  }
  }

  Type *t = ast->md;
  t = apply_substitution(subst, t);

  // TODO: when a Subst contains a chain of vars `a -> `b -> `c we could
  // eliminate `b and keep just `a -> `c
  if (ast->tag == AST_APPLICATION) {
    Type *n = t;
    while (n->kind == T_VAR) {
      Type *x = find_in_subst(subst, n->data.T_VAR);
      if (x) {
        n = x;
      } else {
        break;
      }
    }
    t = n;
  }
  ast->md = t;
}

// T-Lambda:  Γ, x₁ : α₁, x₂ : α₂, ..., xₙ : αₙ ⊢ e : τ    (α₁, α₂, ..., α esh)
//
//
//            ─────────────────────────────────────────────────────────────────────
//
//            Γ ⊢ λ x₁ x₂ ... xₙ. e : α₁ → α₂ → ... → αₙ → τ
//
Type *infer_lambda(Ast *ast, TICtx *ctx) {
  Ast *body = ast->data.AST_LAMBDA.body;
  int num_params = ast->data.AST_LAMBDA.len;

  Type *param_types[num_params];

  initial_lambda_signature(num_params, param_types,
                           ast->data.AST_LAMBDA.type_annotations,
                           ast->data.AST_LAMBDA.params, ctx);

  TICtx lctx = *ctx;
  lctx.current_fn_ast = ast;
  lctx.scope = ctx->scope + 1;
  lctx.current_fn_base_scope = lctx.scope;

  bind_lambda_params(ast, param_types, &lctx);

  // Create a fresh type variable for the recursive function reference
  Type *recursive_fn_type = next_tvar();
  bind_recursive_ref(ast, recursive_fn_type, &lctx);

  Type *body_type = infer(body, &lctx);
  if (!body_type) {
    return type_error(body, "Error: Cannot infer lambda body\n");
  }

  Subst *ls = solve_constraints(lctx.constraints);
  for (int i = 0; i < num_params; i++) {
    param_types[i] = apply_substitution(ls, param_types[i]);
    // param_types[i] = apply_substitution(lambda_ctx.subst, param_types[i]);
  }

  body_type = apply_substitution(ls, body_type);

  // Build the final function type
  Type *result_type = body_type;

  for (int i = num_params - 1; i >= 0; i--) {
    Type *t = param_types[i];
    t = apply_substitution(lctx.subst, t);
    result_type = type_fn(t, result_type);
  }

  if (ast->data.AST_LAMBDA.fn_name.chars) {
    // NB: maybe not do this for anon funcs
    //
    // eg in this context:
    //
    // type NoteCallback = Int -> Double -> ();
    // let register_note_on_handler = extern fn NoteCallback -> Int -> ();
    // register_note_on_handler (fn n vel -> vel + 0.0) 0
    // anonymous func arg would be incorrectly typed (the constraint from it
    // being a NoteCallback would be overriden)
    apply_substitution_to_lambda_body(body, ls);
  }

  ctx->subst = lctx.subst;

  if (lctx.yielded_type) {
    return create_coroutine_lambda(result_type, ctx);
  }

  Type *closure = create_closure(ast, result_type, ctx);

  if (closure) {
    return closure;
  }

  // printf("result type\n");
  // print_type(result_type);
  return result_type;
}
