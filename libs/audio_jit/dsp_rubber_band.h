#ifndef _LIBS_AUDIO_JIT_DSP_RUBBER_BAND_H
#define _LIBS_AUDIO_JIT_DSP_RUBBER_BAND_H

#include "../../lang/parse.h"
#include "./compile_synth.h"
#include "./dsp_build_expr.h"
#include <llvm-c/Types.h>

DspValue dsp_rubberband_bufplay(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                LLVMModuleRef module,
                                LLVMBuilderRef builder);
DspValue dsp_rubberband_bufplay_finer(Ast *ast, DspBuildCtx *dsp_ctx,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder);

#endif
