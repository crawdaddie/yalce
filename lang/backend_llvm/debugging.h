#ifndef _LANG_BACKEND_LLVM_DBG_H
#define _LANG_BACKEND_LLVM_DBG_H
#include "common.h"
#include <llvm-c/Types.h>

void init_debugging(const char *src_file, JITLangCtx *ctx,
                    LLVMModuleRef module);

void apply_debug_metadata(LLVMValueRef val, Ast *ast, JITLangCtx *ctx,
                          LLVMBuilderRef builder);
#endif
