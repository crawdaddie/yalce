#ifndef _LANG_BACKEND_BINOP_H
#define _LANG_BACKEND_BINOP_H

#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_binop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);
#endif
