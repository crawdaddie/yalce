#ifndef _LANG_BACKEND_LLVM_SYMBOLS_H
#define _LANG_BACKEND_LLVM_SYMBOLS_H

#include "backend_llvm/common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_identifier(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

int codegen_lookup_id(const char *id, int length, JITLangCtx *ctx,
                      JITSymbol **result);

JITSymbol *lookup_id_mutable(const char *id, int length, JITLangCtx *ctx);

LLVMValueRef codegen_assignment(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef codegen_multiple_assignment(Ast *binding, LLVMValueRef expr_val,
                                         Type *expr_type, JITLangCtx *ctx,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         bool is_fn_param, int fn_param_idx);

LLVMValueRef codegen_single_assignment(Ast *id, LLVMValueRef expr_val,
                                       Type *expr_type, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder, bool is_fn_param,
                                       int fn_param_idx);

void bind_symbol_in_scope(const char *id, uint64_t id_hash, LLVMTypeRef type,
                          LLVMValueRef val, symbol_type sym_type,
                          JITLangCtx *ctx);

LLVMValueRef current_func(LLVMBuilderRef builder);

JITSymbol *lookup_id_ast(Ast *id, JITLangCtx *ctx);

#endif
