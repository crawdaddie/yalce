#ifndef _BACKEND_LLVM_COROUTINE_EXT_H
#define _BACKEND_LLVM_COROUTINE_EXT_H
#include "./coroutines.h"
#include "./coroutines_private.h"

LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);
#endif
