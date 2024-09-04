#include "backend_llvm/match.h"
#include "globals.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type);
LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  return NULL;
}

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

LLVMValueRef match_values(Ast *binding, LLVMValueRef val, Type *val_type,
                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    const char *id_chars = binding->data.AST_IDENTIFIER.value;
    int id_len = binding->data.AST_IDENTIFIER.length;

    if (*(binding->data.AST_IDENTIFIER.value) == '_') {
      return _TRUE;
    }

    if (val_type->kind == T_FN && !(is_generic(val_type))) {
      LLVMTypeRef llvm_type = LLVMTypeOf(val);
      JITSymbol *sym = new_symbol(STYPE_FUNCTION, val_type, val, llvm_type);

      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);

      return _TRUE;
    }

    if (ctx->stack_ptr == 0) {

      LLVMTypeRef llvm_type = LLVMTypeOf(val);
      JITSymbol *sym =
          new_symbol(STYPE_TOP_LEVEL_VAR, val_type, val, llvm_type);

      codegen_set_global(sym, val, val_type, llvm_type, ctx, module, builder);
      ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                  hash_string(id_chars, id_len), sym);
      return _TRUE;
    }

    LLVMTypeRef llvm_type = LLVMTypeOf(val);
    JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, val_type, val, llvm_type);
    ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
                hash_string(id_chars, id_len), sym);

    return _TRUE;
  }

  default:
    return NULL;
  }
}
