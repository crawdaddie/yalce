#ifndef _LIBS_AUDIO_JIT_DSP_FORK_H
#define _LIBS_AUDIO_JIT_DSP_FORK_H
#include "./dsp_build_expr.h"
#define _FORK_OPERATOR_ID "~"

DspValue dsp_fork(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                  LLVMModuleRef module, LLVMBuilderRef builder);

#endif
