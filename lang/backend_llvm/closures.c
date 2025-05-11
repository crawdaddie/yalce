#include "./closures.h"
#include "symbols.h"
#include "types/closures.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

bool is_lambda_with_closures(Ast *ast) {
  return ast->tag == AST_LAMBDA &&
         (ast->data.AST_LAMBDA.num_closure_free_vars > 0);
}

LLVMValueRef create_curried_generic_closure_binding(
    Ast *binding, Type *closure_type, Ast *closure, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {

  JITSymbol *curried_sym =
      new_symbol(STYPE_PARTIAL_EVAL_CLOSURE, closure_type, NULL, NULL);

  int len = closure->data.AST_LAMBDA.num_closure_free_vars;

  // closure->md = full_type;

  JITSymbol *original_sym = create_generic_fn_symbol(closure, ctx);

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.callable_sym =
      original_sym;

  LLVMValueRef *app_args = malloc(sizeof(LLVMValueRef) * len);
  AstList *l = closure->data.AST_LAMBDA.params;
  for (int i = 0; i < len; i++) {
    app_args[i] = codegen(l->ast, ctx, module, builder);
    l = l->next;
  }
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args = app_args;

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len = len;
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_args_len =
      len + closure->data.AST_LAMBDA.len;

  Type *full_type = get_full_fn_type_of_closure(closure);
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_callable_type =
      full_type;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
              curried_sym);
  return NULL;
}

LLVMValueRef create_curried_closure_binding(Ast *binding, Type *closure_type,
                                            Ast *closure, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {
  JITSymbol *curried_sym =
      new_symbol(STYPE_PARTIAL_EVAL_CLOSURE, closure_type, NULL, NULL);

  int len = closure->data.AST_LAMBDA.num_closure_free_vars;
  LLVMValueRef *app_args = malloc(sizeof(LLVMValueRef) * len);

  AstList *l = closure->data.AST_LAMBDA.params;
  for (int i = 0; i < len; i++) {
    app_args[i] = codegen(l->ast, ctx, module, builder);
    l = l->next;
  }

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.callable_sym =
      NULL; // <- this will be NULL, instead we compile the original lambda &
            // add it to the symbol
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args = app_args;

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len = len;
  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_args_len =
      len + closure->data.AST_LAMBDA.len;

  curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_callable_type =
      get_full_fn_type_of_closure(closure);

  LLVMValueRef func = codegen(closure, ctx, module, builder);
  curried_sym->val = func;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  printf("bind closure to %s\n", id_chars);
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
              curried_sym);
  return func;
}
