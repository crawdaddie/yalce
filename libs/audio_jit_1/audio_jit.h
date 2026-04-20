#ifndef AUDIO_JIT_H
#define AUDIO_JIT_H

#include "../lang/backend_llvm/common.h"
extern int STYPE_AUDIO_JIT_SYM;

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
