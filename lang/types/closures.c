#include "./closures.h"
#include "./common.h"
#include <string.h>

Type *_get_full_closure_type(int num, Type *f, AstList *cl) {
  if (num == 0) {
    return f;
  }
  return type_fn(cl->ast->md, _get_full_closure_type(num - 1, f, cl->next));
}

Type *get_full_fn_type_of_closure(Ast *closure) {

  Type *fn_type = closure->md;
  Type *t =
      _get_full_closure_type(closure->data.AST_LAMBDA.num_closure_free_vars,
                             fn_type, closure->data.AST_LAMBDA.params);
  return t;
}

void extend_closure_free_vars(Ast *fn, Ast *ref, Type *ref_type) {
  AstList *l = fn->data.AST_LAMBDA.params;
  while (l) {
    if (CHARS_EQ(ref->data.AST_IDENTIFIER.value,
                 l->ast->data.AST_IDENTIFIER.value)) {
      // avoid adding closure variable twice and also avoid closuring this
      // function's existing params
      return;
    }
    l = l->next;
  }
  ref->md = ref_type;
  fn->data.AST_LAMBDA.params =
      ast_list_extend_left(fn->data.AST_LAMBDA.params, ref);

  fn->data.AST_LAMBDA.type_annotations =
      ast_list_extend_left(fn->data.AST_LAMBDA.type_annotations, NULL);
  fn->data.AST_LAMBDA.num_closure_free_vars++;
}

void handle_closed_over_ref(Ast *ast, TypeEnv *ref, TICtx *ctx) {

  int this_scope = ctx->current_fn_scope;
  int ref_scope = ref->type->scope;
  int is_fn_param = ref->is_fn_param;
  int is_rec_fn_ref = ref->is_recursive_fn_ref;
  if (!is_fn_param && (!is_rec_fn_ref) && (ref_scope > 0) &&
      (this_scope > ref_scope)) {
    // printf("closure stufff???? this scope %d ref_scope %d is_fn_param %d
    // "
    //        "is_rec fn ref %d\n",
    //        this_scope, ref_scope, is_fn_param, ref->is_recursive_fn_ref);
    // print_ast(ctx->current_fn_ast);
    // print_ast(ast);
    extend_closure_free_vars(ctx->current_fn_ast, ast, ref->type);
  }
}
