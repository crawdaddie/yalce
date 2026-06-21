#ifndef _LANG_BACKEND_LLVM_CONSTRUCTORS_H
#define _LANG_BACKEND_LLVM_CONSTRUCTORS_H

#include "common.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_cons_type_constructor(Ast *ast, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder);

#endif
