#ifndef _LIBS_AUDIO_JIT_DSP_PAN_H
#define _LIBS_AUDIO_JIT_DSP_PAN_H
#include "./dsp_build_expr.h"

DspValue dsp_pan2(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                  LLVMModuleRef module, LLVMBuilderRef builder);
#endif
