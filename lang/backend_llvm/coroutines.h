#ifndef _LANG_BACKEND_LLVM_COROUTINES_H
#define _LANG_BACKEND_LLVM_COROUTINES_H
#include "common.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);

LLVMValueRef codegen_coroutine_instance(Ast *args, int args_len,
                                        JITSymbol *symbol, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef codegen_coroutine_next(Ast *application, JITSymbol *sym,
                                    Type *expected_fn_type, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder);
LLVMTypeRef get_coroutine_instance_type(LLVMTypeRef params_obj_type,
                                        LLVMTypeRef generator_fn_type);
#endif
