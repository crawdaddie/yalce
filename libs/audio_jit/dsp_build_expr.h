#ifndef _LIBS_AUDIO_JIT_BUILD_DSP_EXPR_H
#define _LIBS_AUDIO_JIT_BUILD_DSP_EXPR_H

#include "../../lang/parse.h"
#include <llvm-c/Types.h>

#include "./audio_jit.h"

#include "./compile_synth.h"

typedef struct {
  int lanes;
  LLVMValueRef scalar;
  LLVMValueRef *vec;
} DspValue;

#define DSP_SCALAR(v)                                                          \
  (DspValue) { 1, v }

#define DSP_MULTI(l, v)                                                        \
  (DspValue) { .lanes = l, .scalar = *v, .vec = v }

#define DSP_NULL (DspValue){0}

DspValue dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
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

int max(int a, int b);
int min(int a, int b);
#endif
