
#ifndef _LIBS_AUDIO_JIT_DSP_FN_APPLICATION_H
#define _LIBS_AUDIO_JIT_DSP_FN_APPLICATION_H
#include "../../lang/parse.h"
#include "./compile_synth.h"
#include <llvm-c/Types.h>
LLVMValueRef dsp_fn_application(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder);

bool ast_is_const(Ast *ast, JITLangCtx *jit_ctx);
#endif
