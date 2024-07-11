#ifndef _LANG_BACKEND_FUNCTION_CURRYING_H
#define _LANG_BACKEND_FUNCTION_CURRYING_H

#include "common.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_curry_fn(Ast *curry, LLVMValueRef func,
                              unsigned int func_params_len, JITLangCtx *ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder);
#endif
