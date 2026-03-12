#include "./codegen.h"
#include "./codegen_context.h"
#include <mlir/IR/Value.h>

extern "C" {
#include "../../lang/serde.h"
}

// Extraction target: core expression builder and AST dispatch logic.
mlir::Value build_dsp_expr(Ast *ast, mlir::DspBuildCtx &ctx,
                           JITLangCtx *jit_ctx) {
  printf("build dsp\n");
  print_ast(ast);

  if (!ast)
    return {};
  auto &b = ctx.b;
  auto loc = ctx.loc;

  switch (ast->tag) {

  default:
    break;
  }
  return {};
}
