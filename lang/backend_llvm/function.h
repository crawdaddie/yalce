#ifndef _LANG_BACKEND_LLVM_FUNCTION_H
#define _LANG_BACKEND_LLVM_FUNCTION_H
#include "common.h"
#include "types/type.h"
#include "types/typeclass.h"
#include "llvm-c/Types.h"

LLVMTypeRef fn_prototype(Type *fn_type, int fn_len, TypeEnv *env,
                         LLVMModuleRef module);

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder);

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder);

LLVMValueRef get_specific_callable(JITSymbol *sym, const char *sym_name,
                                   Type *expected_fn_type, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

typedef LLVMValueRef (*LLVMBinopMethod)(LLVMValueRef, LLVMValueRef,
                                        LLVMModuleRef, LLVMBuilderRef);

LLVMTypeRef codegen_for_func_sig();
LLVMValueRef codegen_build_for(LLVMTypeRef for_func_type, LLVMModuleRef module,
                               LLVMBuilderRef builder);
#endif
