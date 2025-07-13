#ifndef _LANG_BACKEND_LLVM_JIT_H
#define _LANG_BACKEND_LLVM_JIT_H
#include "backend_llvm/common.h"
#include "llvm-c/ExecutionEngine.h"
int jit(int argc, char **argv);

int prepare_ex_engine(JITLangCtx *ctx, LLVMExecutionEngineRef *engine,
                      LLVMModuleRef module);

int start_jit(JITLangCtx *kernel_ctx, ht *kernel_table,
              LLVMContextRef *context_ref, LLVMModuleRef *module_ref,
              LLVMBuilderRef *builder_ref);

extern int __BREAK_REPL_FOR_GUI_LOOP;
extern void (*break_repl_for_gui_loop_cb)(void);
#endif
