#include "codegen.h"
#include "dialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"

using namespace mlir;

static Value get_static_table(void *table_data, DspBuildCtx &ctx) {
  auto i64_ty = ctx.b.getI64Type();
  auto ptr_ty = LLVM::LLVMPointerType::get(ctx.b.getContext());
  Value addr = ctx.b.create<LLVM::ConstantOp>(
      ctx.loc, i64_ty, ctx.b.getI64IntegerAttr((int64_t)(uintptr_t)table_data));
  return ctx.b.create<LLVM::IntToPtrOp>(ctx.loc, ptr_ty, addr);
}

static Value emit_table_osc(Value freq, void *table_data, int32_t table_size,
                            DspBuildCtx &ctx) {
  int off = ctx.state_offset;
  ctx.state_offset += 8;
  Value phase =
      ctx.b.create<PhasorOp>(ctx.loc, ctx.state_ptr, freq, ctx.spf, off)
          ->getResult(0);
  Value table_ptr = get_static_table(table_data, ctx);
  return ctx.b.create<TableLookupOp>(ctx.loc, phase, table_ptr, table_size)
      ->getResult(0);
}

static bool value_is_const_zero(Value v) {
  if (!v) {
    return false;
  }
  if (auto c = v.getDefiningOp<arith::ConstantFloatOp>()) {
    return c.value().isZero();
  }
  if (auto c = v.getDefiningOp<arith::ConstantIntOp>()) {
    return c.value() == 0;
  }
  if (auto c = v.getDefiningOp<LLVM::ConstantOp>()) {
    auto attr = c.getValueAttr();
    if (auto fa = dyn_cast<FloatAttr>(attr)) {
      return fa.getValue().isZero();
    }
    if (auto ia = dyn_cast<IntegerAttr>(attr)) {
      return ia.getInt() == 0;
    }
  }
  return false;
}

Value SinOscHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  return emit_table_osc(freq, (void *)sin_table, SIN_TABSIZE, ctx);
}

Value SqOscHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  return emit_table_osc(freq, (void *)sq_table, SQ_TABSIZE, ctx);
}

Value SawOscHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  return emit_table_osc(freq, (void *)saw_table, SAW_TABSIZE, ctx);
}

Value PhasorHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  int off = reserve_state(ctx, 8);
  auto &b = ctx.b;
  auto loc = ctx.loc;
  return b.create<PhasorOp>(loc, ctx.state_ptr, freq, ctx.spf, off)
      ->getResult(0);
}

Value ImpulseHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Value phasor = PhasorHandler(ast, ctx, jit_ctx);
  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto zero =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(0.0));
  Value cmp =
      b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OEQ, phasor, zero);
  return b.create<arith::UIToFPOp>(loc, b.getF64Type(), cmp);
}

Value PhasorTrigHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);

  auto &b = ctx.b;
  auto loc = ctx.loc;

  if (value_is_const_zero(freq)) {
    int off = reserve_state(ctx, 8);
    auto f64 = b.getF64Type();
    auto i64 = b.getI64Type();
    auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
    Value off_val =
        b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(off));
    Value flag_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                           ctx.state_ptr, ValueRange{off_val});
    Value flag = b.create<LLVM::LoadOp>(loc, f64, flag_ptr);
    Value zero = b.create<arith::ConstantFloatOp>(loc, f64, APFloat(0.0));
    Value one = b.create<arith::ConstantFloatOp>(loc, f64, APFloat(1.0));
    Value not_fired =
        b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OEQ, flag, zero);
    Value trig = b.create<arith::SelectOp>(loc, not_fired, one, zero);
    b.create<LLVM::StoreOp>(loc, one, flag_ptr);
    return trig;
  }

  int off = reserve_state(ctx, 8);
  auto op = b.create<PhasorTrigOp>(loc, ctx.state_ptr, freq, ctx.spf, off);
  return op->getResult(0);
}
