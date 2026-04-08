// dsp_build_expr.h
#pragma once

extern "C" {
#include "../../lang/backend_llvm/common.h"
#include "../../lang/parse.h"
}
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/IR/Builders.h>
#include <mlir/IR/Value.h>

struct DspBuildCtx {
  mlir::func::FuncOp frame_fn; // the function we're building into
  mlir::OpBuilder &builder;
  mlir::Location loc;
  int sample_rate;
  double spf;
};

// Returns the mlir::Value for the sample produced by ast.
// Returns a null Value on error.
mlir::Value dsp_build_expr(Ast *ast, DspBuildCtx &dsp_ctx, JITLangCtx *ctx);

// Build the frame function body from a lambda AST, replacing any existing stub.
// Returns true on success.
bool dsp_build_frame(mlir::func::FuncOp frame_fn, mlir::OpBuilder &b,
                     mlir::Location loc, Ast *lambda, JITLangCtx *ctx,
                     int sample_rate);
