#ifndef _LIBS_AUDIO_JIT_BUILD_DSP_EXPR_H
#define _LIBS_AUDIO_JIT_BUILD_DSP_EXPR_H

#include "../../lang/parse.h"
#include <llvm-c/Types.h>

#include "./audio_jit.h"

#include "./compile_synth.h"

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef dsp_consume_state_cursor(LLVMValueRef cursor_ptr,
                                      LLVMBuilderRef builder, int size,
                                      int align, const char *name);
LLVMValueRef dsp_consume_frame_state(DspBuildCtx *dsp_ctx,
                                     LLVMBuilderRef builder, int size,
                                     int align, const char *name);

LLVMValueRef dsp_consume_init_state(DspBuildCtx *dsp_ctx,
                                    LLVMBuilderRef builder, int size, int align,
                                    const char *name);
#endif
