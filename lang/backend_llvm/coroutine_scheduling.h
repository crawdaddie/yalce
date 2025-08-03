#ifndef _BACKEND_LLVM_COROUTINE_SCHED_H
#define _BACKEND_LLVM_COROUTINE_SCHED_H
#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);
#endif
