#include "./dsp_fork.h"

DspValue dsp_fork(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                  LLVMModuleRef module, LLVMBuilderRef builder) {

  if (ast->data.AST_APPLICATION.args->tag == AST_ARRAY ||
      ast->data.AST_APPLICATION.args->tag == AST_LIST) {
    Ast *arr_literal = ast->data.AST_APPLICATION.args;
    int len = arr_literal->data.AST_LIST.len;
    LLVMValueRef *chans = dsp_tmp_alloc(dsp_ctx, len * sizeof(LLVMValueRef), 8);
    for (int i = 0; i < len; i++) {
      DspValue val = dsp_build_expr(arr_literal->data.AST_LIST.items + i,
                                    dsp_ctx, ctx, module, builder);
      if (val.lanes > 1) {
        fprintf(
            stderr,
            "Error: please only include scalar values in the fork operator");
        return DSP_NULL;
      }

      chans[i] = val.scalar;
    }
    return DSP_MULTI(len, chans);
  }

  return DSP_NULL;
}

DspValue dsp_mix(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                 LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *out_layout_ast = ast->data.AST_APPLICATION.args;
  if (out_layout_ast->tag != AST_INT) {
    return DSP_NULL;
  }

  int out_layout = out_layout_ast->data.AST_INT.value;

  DspValue input = dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx,
                                  ctx, module, builder);
  int in_layout = input.lanes;
  if (input.lanes == 1) {
    return input;
  }

  if (input.lanes == out_layout) {
    return input;
  }

  if (out_layout <= 0) {
    return DSP_NULL;
  }

  if (out_layout == 1) {
    LLVMValueRef sum = input.vec[0];
    for (int i = 1; i < in_layout; i++) {
      sum = LLVMBuildFAdd(builder, sum, input.vec[i], "mix.sum");
    }
    return DSP_SCALAR(sum);
  }

  LLVMValueRef *out =
      dsp_tmp_alloc(dsp_ctx, sizeof(LLVMValueRef) * (size_t)out_layout, 8);
  if (!out) {
    return DSP_NULL;
  }

  LLVMTypeRef f64_ty = LLVMDoubleType();
  for (int ch = 0; ch < out_layout; ch++) {
    out[ch] = LLVMConstReal(f64_ty, 0.0);
  }

  double out_span = (double)(out_layout - 1);
  double in_span = (double)(in_layout - 1);

  for (int in_ch = 0; in_ch < in_layout; in_ch++) {
    double pos = (double)in_ch / in_span;
    double scaled = pos * out_span;
    int left = (int)scaled;
    int right = left < (out_layout - 1) ? left + 1 : left;
    double frac = scaled - (double)left;
    double left_gain = 1.0 - frac;
    double right_gain = frac;

    if (left_gain != 0.0) {
      LLVMValueRef left_term = input.vec[in_ch];
      if (left_gain != 1.0) {
        left_term =
            LLVMBuildFMul(builder, left_term, LLVMConstReal(f64_ty, left_gain),
                          "mix.left.term");
      }
      out[left] = LLVMBuildFAdd(builder, out[left], left_term, "mix.left");
    }

    if (right != left && right_gain != 0.0) {
      LLVMValueRef right_term = input.vec[in_ch];
      if (right_gain != 1.0) {
        right_term = LLVMBuildFMul(
            builder, right_term, LLVMConstReal(f64_ty, right_gain),
            "mix.right.term");
      }
      out[right] =
          LLVMBuildFAdd(builder, out[right], right_term, "mix.right");
    }
  }

  return DSP_MULTI(out_layout, out);
}
