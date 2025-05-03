#ifndef _LANG_BACKEND_LLVM_LOOP_H
#define _LANG_BACKEND_LLVM_LOOP_H

#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_loop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);
#endif
