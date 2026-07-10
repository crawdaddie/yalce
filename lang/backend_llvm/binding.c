#include "./binding.h"
#include "../parse.h"
#include "./globals.h"
#include "./symbols.h"
#include "./types.h"
#include "common.h"
#include "match.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

static void refresh_bound_symbol(JITSymbol *sym, symbol_type type_tag, Type *type,
                                 LLVMValueRef val, LLVMTypeRef llvm_type,
                                 LLVMBuilderRef builder) {
  sym->type = type_tag;
  sym->symbol_type = type;
  sym->val = val;
  sym->llvm_type = llvm_type;
  if (sym->storage) {
    LLVMBuildStore(builder, val, sym->storage);
  }
}

static JITSymbol *upsert_identifier_symbol(const char *chars, uint64_t id_hash,
                                           symbol_type top_level_tag,
                                           symbol_type local_tag, Type *type,
                                           LLVMValueRef val,
                                           LLVMTypeRef llvm_type,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  JITSymbol *ex_sym = ht_get_hash(ctx->frame->table, chars, id_hash);

  if (ctx->stack_ptr == 0) {
    if (ex_sym != NULL) {
      refresh_bound_symbol(ex_sym, top_level_tag, type, val, llvm_type, builder);
      ht_set_hash(ctx->frame->table, chars, id_hash, ex_sym);
      return ex_sym;
    }

    JITSymbol *sym = new_symbol(top_level_tag, type, val, llvm_type);
    codegen_set_global(chars, sym, val, type, llvm_type, ctx, module, builder);
    ht_set_hash(ctx->frame->table, chars, id_hash, sym);
    return sym;
  }

  if (ex_sym != NULL && ex_sym->storage) {
    refresh_bound_symbol(ex_sym, ex_sym->type, type, val, llvm_type, builder);
    return ex_sym;
  }

  JITSymbol *sym = new_symbol(local_tag, type, val, llvm_type);
  ht_set_hash(ctx->frame->table, chars, id_hash, sym);
  return sym;
}

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

    LLVMTypeRef llvm_type = bl->val_type;
    LLVMValueRef val = bl->val;
    Type *type = bl->type;
    upsert_identifier_symbol(chars, id_hash, STYPE_TOP_LEVEL_VAR, STYPE_LOCAL_VAR,
                             type, val, llvm_type, ctx, module, builder);
  }
}

LLVMValueRef bind_local_value_with_storage(Ast *id, LLVMValueRef val,
                                           LLVMValueRef storage, Type *val_type,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {

  if (ast_is_placeholder_id(id)) {
    return val;
  }

  const char *chars = id->data.AST_IDENTIFIER.value;
  uint64_t id_hash = hash_string(chars, id->data.AST_IDENTIFIER.length);

  LLVMTypeRef llvm_type = type_to_llvm_type(val_type, ctx, module);

  JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, val_type, val, llvm_type);
  sym->storage = storage;
  ht_set_hash(ctx->frame->table, chars, id_hash, sym);

  return val;
}

LLVMValueRef bind_value(Ast *id, LLVMValueRef val, Type *val_type,
                        JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  if (ast_is_placeholder_id(id)) {
    return val;
  }

  const char *chars = id->data.AST_IDENTIFIER.value;
  uint64_t id_hash = hash_string(chars, id->data.AST_IDENTIFIER.length);

  LLVMTypeRef llvm_type = type_to_llvm_type(val_type, ctx, module);

  upsert_identifier_symbol(chars, id_hash, STYPE_TOP_LEVEL_VAR, STYPE_LOCAL_VAR,
                           val_type, val, llvm_type, ctx, module, builder);
  return val;
}

LLVMValueRef codegen_pattern_binding(Ast *pattern, LLVMValueRef val,
                                     Type *val_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  if (pattern->tag == AST_VOID) {
    return val;
  }

  test_pattern(pattern, val, val_type, ctx, module, builder);

  // BindList *bl = NULL;
  // LLVMTypeRef llvm_val_type = type_to_llvm_type(val_type, ctx, module);
  // LLVMValueRef test_result = LLVMConstInt(LLVMInt1Type(), 1, 0);
  //
  // set_var_bindings(bl, ctx, module, builder);
  //
  // while (bl != NULL) {
  //   BindList *next = bl->next;
  //   free(bl);
  //   bl = next;
  // }

  return val;
}
