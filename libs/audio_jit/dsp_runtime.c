#include "../../lang/serde.h"
#include "./dsp_fork.h"

DspValue dsp_fork(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                  LLVMModuleRef module, LLVMBuilderRef builder) {
  printf("DSP forking value\n");
  print_ast(ast);
  if (ast->data.AST_APPLICATION.args->tag == AST_ARRAY ||
      ast->data.AST_APPLICATION.args->tag == AST_LIST) {
  }

  return DSP_NULL;
}
