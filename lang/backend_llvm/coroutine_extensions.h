#ifndef _BACKEND_LLVM_COROUTINE_EXT_H
#define _BACKEND_LLVM_COROUTINE_EXT_H

#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef CorMapHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef IterHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);
#endif
