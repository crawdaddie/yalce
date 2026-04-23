#ifndef _LIBS_AUDIO_JIT_BUILD_DSP_EXPR_H
#define _LIBS_AUDIO_JIT_BUILD_DSP_EXPR_H

#include "../../lang/parse.h"
#include <llvm-c/Types.h>

#include "./audio_jit.h"

#include "./compile_synth.h"
// DspValueMetaAttr

typedef uint64_t DspValueMetaAttr;
// Predefined attribute flags
#define DSP_ATTR_NONE 0x0000000000000000ULL
#define DSP_ATTR_COMPILE_CONST                                                 \
  0x0000000000000001ULL // compile-time constant - array literal or number
                        // literal or array_fill_const where size is const
// array_size (const) == const
// array_at (const)   != const

typedef struct {
  int lanes;
  LLVMValueRef scalar;
  LLVMValueRef *vec;
  // DspValueMetaAttr attr;
} DspValue;

#define DSP_SCALAR(v)                                                          \
  (DspValue) { 1, v }

#define DSP_MULTI(l, v)                                                        \
  (DspValue) { .lanes = l, .scalar = *v, .vec = v }

#define DSP_NULL (DspValue){0}
#define DSP_ZERO                                                               \
  (DspValue) { 1, LLVMConstReal(LLVMDoubleType(), 0) }

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
