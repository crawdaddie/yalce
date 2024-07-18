#ifndef _LANG_BACKEND_LLVM_JIT_H
#define _LANG_BACKEND_LLVM_JIT_H
#include "llvm-c/ExecutionEngine.h"
int jit(int argc, char **argv);

int prepare_ex_engine(LLVMExecutionEngineRef *engine, LLVMModuleRef module);
#endif
