#ifndef _LANG_BACKEND_LLVM_JIT_H
#define _LANG_BACKEND_LLVM_JIT_H
#include "./common.h"
#include "llvm-c/ExecutionEngine.h"

#include <llvm-c/LLJIT.h> // ORC LLJIT C API
#include <llvm-c/Orc.h>   // ORC core C API
int jit(int argc, char **argv);

int prepare_ex_engine(JITLangCtx *ctx, LLVMExecutionEngineRef *engine,
                      LLVMModuleRef module);

int start_jit(JITLangCtx *kernel_ctx, ht *kernel_table,
              LLVMContextRef *context_ref, LLVMModuleRef *module_ref,
              LLVMBuilderRef *builder_ref);

#endif
