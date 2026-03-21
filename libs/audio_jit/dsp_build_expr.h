#ifndef _LIBS_AUDIO_JIT_BUILD_DSP_EXPR_H
#define _LIBS_AUDIO_JIT_BUILD_DSP_EXPR_H

#include "../../lang/parse.h"
#include <llvm-c/Types.h>

#include "./audio_jit.h"

#include "./compile_synth.h"

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder);
#endif
