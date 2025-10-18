#include "./binding.h"
#include "./globals.h"
#include "./symbols.h"
#include "./types.h"
#include "match.h"
#include "llvm-c/Core.h"
#include <stdint.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

void set_var_bindings(BindList *bl, JITLangCtx *ctx, LLVMModuleRef module,
                      LLVMBuilderRef builder) {

  // Iterate through the binding list and add each binding to the context
  for (BindList *b = bl; b != NULL; b = b->next) {
    if (ast_is_placeholder_id(b->binding)) {
      continue; // Skip placeholder bindings like '_'
    }

    const char *chars = b->binding->data.AST_IDENTIFIER.value;
    uint64_t id_hash =
        hash_string(chars, b->binding->data.AST_IDENTIFIER.length);

    if (ctx->stack_ptr == 0) {

      LLVMTypeRef llvm_type = bl->val_type;
      LLVMValueRef val = bl->val;
      Type *type = bl->type;

      JITSymbol *ex_sym = ht_get_hash(ctx->frame->table, chars, id_hash);

      JITSymbol *sym;
      if (ex_sym != NULL) {
        // printf("restore existing symbol\n");
        // print_ast(binding);
        ex_sym->val = val;
        ex_sym->llvm_type = llvm_type;
        ex_sym->symbol_type = type;
        if (ex_sym->storage) {
          LLVMBuildStore(builder, val, ex_sym->storage);
        }
        sym = ex_sym;
      } else {
        sym = new_symbol(STYPE_TOP_LEVEL_VAR, type, val, llvm_type);
        codegen_set_global(chars, sym, val, type, llvm_type, ctx, module,
                           builder);
      }

      ht_set_hash(ctx->frame->table, chars, id_hash, sym);
      continue;
    }

    // Local binding
    JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, b->type, b->val, b->val_type);
    ht_set_hash(ctx->frame->table, chars, id_hash, sym);
  }
}

LLVMValueRef codegen_pattern_binding(Ast *pattern, LLVMValueRef val,
                                     Type *val_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {

  BindList *bl = NULL;
  LLVMTypeRef llvm_val_type = type_to_llvm_type(val_type, ctx, module);
  LLVMValueRef test_result = LLVMConstInt(LLVMInt1Type(), 1, 0);

  test_pattern_rec(pattern, &bl, &test_result, val, llvm_val_type, val_type,
                   ctx, module, builder);

  set_var_bindings(bl, ctx, module, builder);

  while (bl != NULL) {
    BindList *next = bl->next;
    free(bl);
    bl = next;
  }

  return test_result;
}
