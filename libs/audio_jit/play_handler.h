#ifndef __PLAY_HANDLER_H
#define __PLAY_HANDLER_H

#include "../../lang/backend_llvm/common.h"
#include "../../lang/backend_llvm/jit.h" // JITLangCtx

LLVMValueRef play_module_handler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);
#endif
