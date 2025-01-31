#ifndef _LANG_BACKEND_LLVM_COROUTINE_H
#define _LANG_BACKEND_LLVM_COROUTINE_H
#include "common.h"
#include "parse.h"

LLVMValueRef create_coroutine_constructor_binding(Ast *binding, Ast *ast,
                                                  JITLangCtx *ctx,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder);

LLVMValueRef create_coroutine_instance_from_constructor(JITSymbol *sym,
                                                        Ast *args, int args_len,
                                                        JITLangCtx *ctx,
                                                        LLVMModuleRef module,
                                                        LLVMBuilderRef builder);

LLVMValueRef yield_from_coroutine_instance(JITSymbol *sym, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef WrapCoroutineWithEffectHandler(Ast *ast, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder);

LLVMValueRef MapCoroutineHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

#endif
