#include "../../lang/serde.h"
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
