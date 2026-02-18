#ifndef AUDIO_JIT_H
#define AUDIO_JIT_H

#include "../lang/backend_llvm/common.h"
#include <llvm-c/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

#ifdef __cplusplus
}
#endif

#endif
