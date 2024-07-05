#ifndef _LANG_BACKEND_LLVM_CODEGEN_H
#define _LANG_BACKEND_LLVM_CODEGEN_H
#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
#endif
