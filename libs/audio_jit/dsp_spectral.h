#ifndef _LIBS_AUDIO_JIT_DSP_SPECTRAL_H
#define _LIBS_AUDIO_JIT_DSP_SPECTRAL_H

#include "../../lang/parse.h"
#include "dsp_build_expr.h"
#include <llvm-c/Types.h>
#include <stdbool.h>

// type Bin = Ptr;
// type FreqSignal = Ptr;
//
// let Spectral = module () ->
//   let fft = extern fn Int -> Int
//                   -> (T -> FreqSignal)
//                   -> Array of Signal
//                   -> FreqSignal;
//
//   let ifft = FreqSignal -> Signal;
//
//   let freq_map = extern fn (Bin -> Bin) -> FreqSignal -> FreqSignal;
//   let freq_zip = extern fn (Bin -> Bin -> Bin)
//                       -> FreqSignal -> FreqSignal -> FreqSignal;
//
//   let mag   = extern fn Bin -> Double;
//   let phase = extern fn Bin -> Double;
//   let polar = extern fn Double -> Double -> Bin;
// ;

typedef enum {
  FDOM_INVALID = 0,
  FDOM_INPUT,
  FDOM_MAP,
  FDOM_ZIP,
  FDOM_CONST,
} FDomKind;

// Opaque bin value in compiled spectral code.
// Runtime representation can stay as LLVMValueRef for now.
typedef struct {
  LLVMValueRef val;
} BinValue;

// Frequency-domain stream handle used only inside a spectral region.
typedef struct {
  FDomKind kind;
  int lanes;
  LLVMValueRef handle;
  LLVMValueRef *vec;
  Type *type;
} FDomValue;

#define FDOM_SCALAR(k, h) ((FDomValue){.kind = (k), .lanes = 1, .handle = (h)})
#define FDOM_NULL ((FDomValue){0})

static inline int fdom_lane_count(FDomValue v) {
  return v.lanes > 0 ? v.lanes : 0;
}

static inline LLVMValueRef fdom_lane(FDomValue v, int i) {
  return v.lanes > 1 ? v.vec[i % v.lanes] : v.handle;
}

// Top-level fused spectral region:
//   fft frame_size hop_size spectral_fn [inputs...]
//
// This returns a normal sample-domain DspValue because the region includes
// analysis, spectral processing, and synthesis.
DspValue dsp_fft_region(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder);

DspValue dsp_ifft_region(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder);

// Spectral-domain combinators used only while compiling the spectral function.
FDomValue dsp_freq_map(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder);

FDomValue dsp_freq_zip(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder);

// Bin-level primitives used inside spectral lambdas.
LLVMValueRef dsp_bin_mag(LLVMValueRef bin, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef dsp_bin_phase(LLVMValueRef bin, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef dsp_bin_polar(LLVMValueRef r, LLVMValueRef theta,
                           LLVMModuleRef module, LLVMBuilderRef builder);

// AST-facing wrappers for builtin dispatch.
LLVMValueRef dsp_mag(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                     LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef dsp_phase(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef dsp_polar(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder);

// Optional domain/type checks if you want explicit guarding in build_expr/fn
// application.
bool dsp_is_freq_signal_type(Type *t);
bool dsp_is_bin_type(Type *t);

#endif
