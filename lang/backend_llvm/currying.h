#ifndef _LANG_BACKEND_LLVM_CURRYING_H
#define _LANG_BACKEND_LLVM_CURRYING_H

#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef create_curried_fn_binding(Ast *binding, Ast *app, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);
#endif
