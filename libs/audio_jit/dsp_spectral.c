#include "./dsp_spectral.h"
#include "./common.h"
#include <string.h>

FDomValue dsp_fft_region(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *fn_ast = ast->data.AST_APPLICATION.args + 2;
  Ast *fft_size_ast = ast->data.AST_APPLICATION.args;
  if (fft_size_ast->tag != AST_INT) {
    return (FDomValue){};
  }

  Ast *hop_size_ast = ast->data.AST_APPLICATION.args + 1;

  if (hop_size_ast->tag != AST_INT) {

    return (FDomValue){};
  }
  int fft_size = fft_size_ast->data.AST_INT.value;
  int hop_size = fft_size_ast->data.AST_INT.value;

  return (FDomValue){};
}

// Spectral-domain combinators used only while compiling the spectral function.
FDomValue dsp_freq_map(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {}

FDomValue dsp_freq_zip(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {}

// Bin-level primitives used inside spectral lambdas.
LLVMValueRef dsp_bin_mag(LLVMValueRef bin, LLVMModuleRef module,
                         LLVMBuilderRef builder) {}

LLVMValueRef dsp_bin_phase(LLVMValueRef bin, LLVMModuleRef module,
                           LLVMBuilderRef builder) {}

LLVMValueRef dsp_bin_polar(LLVMValueRef r, LLVMValueRef theta,
                           LLVMModuleRef module, LLVMBuilderRef builder) {}

// AST-facing wrappers for builtin dispatch.
LLVMValueRef dsp_mag(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                     LLVMModuleRef module, LLVMBuilderRef builder) {}

LLVMValueRef dsp_phase(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {}

LLVMValueRef dsp_polar(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {}

// Optional domain/type checks if you want explicit guarding in build_expr/fn
// application.
bool dsp_is_freq_signal_type(Type *t) {}
bool dsp_is_bin_type(Type *t) {}

static FDomValue spectral_process(Ast *ast, DspBuildCtx *dsp_ctx,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  if (ast->tag == AST_APPLICATION) {
    Ast *f = ast->data.AST_APPLICATION.function;
    if (is_ident(f, "fft")) {
      return dsp_fft_region(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "freq_map")) {
      return dsp_freq_map(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "freq_zip")) {
      return dsp_freq_zip(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "mag")) {
      return dsp_freq_zip(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "phase")) {
      return dsp_freq_zip(ast, dsp_ctx, ctx, module, builder);
    }

    if (is_ident(f, "polar")) {
      return dsp_freq_zip(ast, dsp_ctx, ctx, module, builder);
    }
  }
}
static DspValue dsp_ifft(FDomValue) { return DSP_ZERO; }

DspValue dsp_ifft_region(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder) {

  FDomValue spectral_val = spectral_process(ast->data.AST_APPLICATION.args,
                                            dsp_ctx, ctx, module, builder);
  return dsp_ifft(spectral_val);
}
