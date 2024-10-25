#ifndef _LANG_BACKEND_LLVM_COROUTINES_H
#define _LANG_BACKEND_LLVM_COROUTINES_H
#include "common.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);
#endif
