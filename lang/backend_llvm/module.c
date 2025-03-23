#include "./module.h"
#include "codegen.h"
#include "globals.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <string.h>
LLVMValueRef codegen_module(Ast *ast, JITLangCtx *ctx, ht *generic_storage,
                            LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *module_type = ast->md;
  LLVMTypeRef llvm_module_type =
      type_to_llvm_type(module_type, ctx->env, module);

  // LLVMValueRef module_alloca = alloc(llvm_module_type, ctx, builder);
  LLVMValueRef module_struct = LLVMGetUndef(llvm_module_type);

  STACK_ALLOC_CTX_PUSH(module_ctx, ctx)
  int len = ast->data.AST_LAMBDA.body->data.AST_BODY.len;
  Ast **stmts = ast->data.AST_LAMBDA.body->data.AST_BODY.stmts;
  for (int i = 0; i < len; i++) {
    Ast *stmt = stmts[i];
    if (stmt->tag == AST_LET) {
      Type *member_type = module_type->data.T_CONS.args[i];
      if (member_type->kind == T_FN && is_generic(member_type)) {
        JITLangCtx _mod_generic_ctx = *ctx;
        if (generic_storage) {
          _mod_generic_ctx.frame->table = generic_storage;
        }
        create_generic_fn_binding(stmt->data.AST_LET.binding,
                                  stmt->data.AST_LET.expr, &_mod_generic_ctx);

        // JITSymbol *symbol =
        //     create_generic_fn_symbol(stmt->data.AST_LET.expr, &module_ctx);
        // bind_
        //
        continue;
      }

      LLVMValueRef member_val = codegen(stmt, &module_ctx, module, builder);
      // LLVMValueRef module_member_gep =
      //     codegen_tuple_gep(i, module_alloca, llvm_module_type, builder);
      // LLVMBuildStore(builder, member_val, module_member_gep);
      module_struct = LLVMBuildInsertValue(builder, module_struct, member_val,
                                           i, "module_member");
    }
  }

  return module_struct;
}

LLVMValueRef bind_module_chars(const char *chars, int chars_len,
                               Type *module_type, LLVMValueRef module_val,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  uint64_t id_hash = hash_string(chars, chars_len);

  LLVMTypeRef llvm_module_type =
      type_to_llvm_type(module_type, ctx->env, module);

  if (ctx->stack_ptr == 0) {

    JITSymbol *sym = new_symbol(STYPE_TOP_LEVEL_VAR, module_type, module_val,
                                llvm_module_type);

    codegen_set_global(sym, module_val, module_type, llvm_module_type, ctx,
                       module, builder);

    ht_set_hash(ctx->frame->table, chars, id_hash, sym);
    return _TRUE;
  } else {
    JITSymbol *sym =
        new_symbol(STYPE_LOCAL_VAR, module_type, module_val, llvm_module_type);
    ht_set_hash(ctx->frame->table, chars, id_hash, sym);
    return _TRUE;
  }

  return module_val;
}
LLVMValueRef bind_module(Ast *binding, Type *module_type,
                         LLVMValueRef module_val, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder) {

  const char *chars = binding->data.AST_IDENTIFIER.value;
  int chars_len = binding->data.AST_IDENTIFIER.length;
  return bind_module_chars(chars, chars_len, module_type, module_val, ctx,
                           module, builder);
}

LLVMValueRef bind_parametrized_module(Ast *binding, Ast *module_ast,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  return NULL;
}
