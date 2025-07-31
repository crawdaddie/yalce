#include "./currying.h"
#include "symbols.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef create_curried_fn_binding(Ast *binding, Ast *app, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  Ast *function_ast = app->data.AST_APPLICATION.function;
  int len = app->data.AST_APPLICATION.len;
  JITSymbol *callable_sym = lookup_id_ast(function_ast, ctx);
  Type *symbol_type = app->md;
  if (callable_sym->type == STYPE_FUNCTION ||
      callable_sym->type == STYPE_GENERIC_FUNCTION) {
    Type *original_callable_type = function_ast->md;

    LLVMValueRef *app_args = malloc(sizeof(LLVMValueRef) * len);

    for (int i = 0; i < len; i++) {
      app_args[i] =
          codegen(app->data.AST_APPLICATION.args + i, ctx, module, builder);
    }

    JITSymbol *curried_sym =
        new_symbol(STYPE_PARTIAL_EVAL_CLOSURE, symbol_type, NULL, NULL);

    curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.callable_sym =
        callable_sym;
    curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args = app_args;

    curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len = len;
    curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_args_len =
        fn_type_args_len(callable_sym->symbol_type);

    curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_callable_type =
        original_callable_type;
    const char *id_chars = binding->data.AST_IDENTIFIER.value;
    int id_len = binding->data.AST_IDENTIFIER.length;

    ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
                curried_sym);

  } else if (callable_sym->type == STYPE_PARTIAL_EVAL_CLOSURE) {

    LLVMValueRef *provided_args =
        callable_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args;
    int provided_args_len =
        callable_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len;
    int total_len =
        callable_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.original_args_len;
    Type *original_callable_type =
        callable_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE
            .original_callable_type;

    JITSymbol *original_callable_sym =
        (JITSymbol *)
            callable_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.callable_sym;

    LLVMValueRef *app_args =
        malloc(sizeof(LLVMValueRef) * (provided_args_len + len));
    for (int i = 0; i < provided_args_len; i++) {
      app_args[i] = provided_args[i];
    }
    for (int i = 0; i < len; i++) {
      app_args[provided_args_len + i] =
          codegen(app->data.AST_APPLICATION.args + i, ctx, module, builder);
    }

    JITSymbol *curried_sym =
        new_symbol(STYPE_PARTIAL_EVAL_CLOSURE, symbol_type, NULL, NULL);
    *curried_sym = *callable_sym;

    curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.args = app_args;
    curried_sym->symbol_data.STYPE_PARTIAL_EVAL_CLOSURE.provided_args_len =
        provided_args_len + len;

    const char *id_chars = binding->data.AST_IDENTIFIER.value;
    int id_len = binding->data.AST_IDENTIFIER.length;

    ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len),
                curried_sym);
  }
  return NULL;
}
