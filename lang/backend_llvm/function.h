#ifndef _LANG_BACKEND_LLVM_FUNCTION_H
#define _LANG_BACKEND_LLVM_FUNCTION_H

#include "common.h"
#include "types/type.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef codegen_fn_type(Type *fn_type, int fn_len, TypeEnv *env,
                            LLVMModuleRef module);

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);
LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder);

LLVMValueRef get_specific_callable(JITSymbol *sym, const char *sym_name,
                                   Type *expected_fn_type, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);
#endif
