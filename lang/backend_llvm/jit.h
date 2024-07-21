#ifndef _LANG_BACKEND_LLVM_JIT_H
#define _LANG_BACKEND_LLVM_JIT_H
#include "common.h"
#include "llvm-c/ExecutionEngine.h"
int jit(int argc, char **argv);

int prepare_ex_engine(JITLangCtx *ctx, LLVMExecutionEngineRef *engine,
                      LLVMModuleRef module);
#endif
