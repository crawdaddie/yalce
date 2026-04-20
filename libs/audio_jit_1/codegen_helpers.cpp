#include "codegen.h"
#include "dialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"

using namespace mlir;

Value LinscaleHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value domain_a = build_dsp_expr(args, ctx, jit_ctx);
  Value domain_b = build_dsp_expr(args + 1, ctx, jit_ctx);
  Value range_a = build_dsp_expr(args + 2, ctx, jit_ctx);
  Value range_b = build_dsp_expr(args + 3, ctx, jit_ctx);
  Value input = build_dsp_expr(args + 4, ctx, jit_ctx);
  return ctx.b
      .create<LinscaleOp>(ctx.loc, domain_a, domain_b, range_a, range_b, input)
      ->getResult(0);
}

Value BipolarLinscaleHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  auto &b = ctx.b;
  auto loc = ctx.loc;
  Ast *args = ast->data.AST_APPLICATION.args;
  Value domain_a =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(-1.0));
  Value domain_b =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(1.0));
  Value range_a = build_dsp_expr(args, ctx, jit_ctx);
  Value range_b = build_dsp_expr(args + 1, ctx, jit_ctx);
  Value input = build_dsp_expr(args + 2, ctx, jit_ctx);
  return ctx.b
      .create<LinscaleOp>(ctx.loc, domain_a, domain_b, range_a, range_b, input)
      ->getResult(0);
}

Value UnipolarLinscaleHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  auto &b = ctx.b;
  auto loc = ctx.loc;
  Ast *args = ast->data.AST_APPLICATION.args;
  Value domain_a =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(0.0));
  Value domain_b =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(1.0));
  Value range_a = build_dsp_expr(args, ctx, jit_ctx);
  Value range_b = build_dsp_expr(args + 1, ctx, jit_ctx);
  Value input = build_dsp_expr(args + 2, ctx, jit_ctx);
  return ctx.b
      .create<LinscaleOp>(ctx.loc, domain_a, domain_b, range_a, range_b, input)
      ->getResult(0);
}

Value FoldHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  auto &b = ctx.b;
  auto loc = ctx.loc;
  Ast *args = ast->data.AST_APPLICATION.args;

  Value limit_a = build_dsp_expr(args, ctx, jit_ctx);
  Value limit_b = build_dsp_expr(args + 1, ctx, jit_ctx);
  Value input = build_dsp_expr(args + 2, ctx, jit_ctx);

  auto f64 = b.getF64Type();
  auto floor_fn =
      declare_extern(ctx.mod, b, "llvm.floor.f64",
                     LLVM::LLVMFunctionType::get(f64, {f64}, false));
  auto fabs_fn = declare_extern(ctx.mod, b, "llvm.fabs.f64",
                                LLVM::LLVMFunctionType::get(f64, {f64}, false));

  auto fmf =
      arith::FastMathFlagsAttr::get(b.getContext(), arith::FastMathFlags::fast);
  Value two = b.create<arith::ConstantFloatOp>(loc, f64, APFloat(2.0));
  Value range = b.create<arith::SubFOp>(loc, limit_b, limit_a, fmf);
  Value dbl = b.create<arith::MulFOp>(loc, two, range, fmf);
  Value shifted = b.create<arith::SubFOp>(loc, input, limit_a, fmf);
  Value floored =
      b.create<LLVM::CallOp>(
           loc, floor_fn,
           ValueRange{b.create<arith::DivFOp>(loc, shifted, dbl, fmf)})
          .getResult();
  Value t = b.create<arith::SubFOp>(
      loc, shifted, b.create<arith::MulFOp>(loc, dbl, floored, fmf), fmf);
  Value abs_val = b.create<LLVM::CallOp>(
                       loc, fabs_fn,
                       ValueRange{b.create<arith::SubFOp>(loc, t, range, fmf)})
                      .getResult();
  return b.create<arith::AddFOp>(
      loc, limit_a, b.create<arith::SubFOp>(loc, range, abs_val, fmf), fmf);
}

Value WrapHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  auto &b = ctx.b;
  auto loc = ctx.loc;
  Ast *args = ast->data.AST_APPLICATION.args;

  Value limit_a = build_dsp_expr(args, ctx, jit_ctx);
  Value limit_b = build_dsp_expr(args + 1, ctx, jit_ctx);
  Value input = build_dsp_expr(args + 2, ctx, jit_ctx);

  auto f64 = b.getF64Type();
  auto floor_fn =
      declare_extern(ctx.mod, b, "llvm.floor.f64",
                     LLVM::LLVMFunctionType::get(f64, {f64}, false));

  auto fmf =
      arith::FastMathFlagsAttr::get(b.getContext(), arith::FastMathFlags::fast);
  Value range = b.create<arith::SubFOp>(loc, limit_b, limit_a, fmf);
  Value shifted = b.create<arith::SubFOp>(loc, input, limit_a, fmf);
  Value t = b.create<LLVM::CallOp>(
                 loc, floor_fn,
                 ValueRange{b.create<arith::DivFOp>(loc, shifted, range, fmf)})
                .getResult();
  return b.create<arith::SubFOp>(
      loc, input, b.create<arith::MulFOp>(loc, range, t, fmf), fmf);
}
