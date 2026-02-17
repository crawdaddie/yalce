#ifndef AUDIO_JIT_H
#define AUDIO_JIT_H

#include "backend_llvm/common.h"
#include <llvm-c/Types.h>

LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

#endif
