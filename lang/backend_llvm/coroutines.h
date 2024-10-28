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

LLVMValueRef codegen_coroutine_next(Ast *application, LLVMValueRef instance,
                                    LLVMTypeRef instance_type,
                                    LLVMTypeRef def_fn_type,
                                    JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder);
#endif
