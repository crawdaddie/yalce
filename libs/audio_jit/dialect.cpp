// =============================================================================
// DSP dialect — DspDialect constructor, lowering patterns, DspToLLVMPass.
// =============================================================================

#include "dialect.h"

#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/DialectConversion.h"

using namespace mlir;

// =============================================================================
// DspDialect constructor — registers all ops.
// =============================================================================

DspDialect::DspDialect(MLIRContext *ctx)
    : Dialect("dsp", ctx, TypeID::get<DspDialect>()) {
  addOperations<InletOp, OutletOp, PhasorOp, ImpulseOp, TableLookupOp,
                BufplayOp, EnvAslrOp>();
}

// =============================================================================
// Lowering patterns: dsp.* → LLVM dialect
// =============================================================================

struct InletOpLowering : public ConversionPattern {
  InletOpLowering(MLIRContext *ctx)
      : ConversionPattern(InletOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto inlet = cast<InletOp>(op);
    auto loc = op->getLoc();
    auto mod = op->getParentOfType<ModuleOp>();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i32 = r.getI32Type();
    auto i64 = r.getI64Type();

    auto fn_ty = LLVM::LLVMFunctionType::get(f64, {ptr, i32, i64}, false);
    auto fn = declare_extern(mod, r, "ylc_read_inlet", fn_ty);

    Value idx = r.create<LLVM::ConstantOp>(loc, i32,
                                           r.getI32IntegerAttr(inlet.getIdx()));
    Value frame = r.create<arith::IndexCastOp>(loc, i64, operands[1]);
    r.replaceOpWithNewOp<LLVM::CallOp>(op, fn,
                                       ValueRange{operands[0], idx, frame});
    return success();
  }
};

struct OutletOpLowering : public ConversionPattern {
  OutletOpLowering(MLIRContext *ctx)
      : ConversionPattern(OutletOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto loc = op->getLoc();
    auto mod = op->getParentOfType<ModuleOp>();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i64 = r.getI64Type();
    auto voidT = LLVM::LLVMVoidType::get(op->getContext());

    auto fn_ty = LLVM::LLVMFunctionType::get(voidT, {ptr, i64, f64}, false);
    auto fn = declare_extern(mod, r, "ylc_write_output", fn_ty);

    Value frame = r.create<arith::IndexCastOp>(loc, i64, operands[1]);
    r.replaceOpWithNewOp<LLVM::CallOp>(
        op, fn, ValueRange{operands[0], frame, operands[2]});
    return success();
  }
};

// PhasorOp: GEP into state at offset, load phase, advance by freq*spf,
// wrap to [0,1) via conditional subtract, store, return wrapped phase.
struct PhasorOpLowering : public ConversionPattern {
  PhasorOpLowering(MLIRContext *ctx)
      : ConversionPattern(PhasorOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto phasor = cast<PhasorOp>(op);
    auto loc = op->getLoc();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i64 = r.getI64Type();

    // GEP: state_ptr + state_offset → &phase
    Value off_val = r.create<LLVM::ConstantOp>(
        loc, i64, r.getI64IntegerAttr(phasor.getStateOffset()));
    Value phase_ptr = r.create<LLVM::GEPOp>(loc, ptr, r.getI8Type(),
                                            operands[0], ValueRange{off_val});

    // Load current phase — this is what we return (pre-advance semantics).
    Value phase = r.create<LLVM::LoadOp>(loc, f64, phase_ptr);
    Value step = r.create<LLVM::FMulOp>(loc, operands[1], operands[2]);
    Value advanced = r.create<LLVM::FAddOp>(loc, phase, step);

    // On wrap, reset state to exactly 0.0 so the next frame returns 0.0.
    // This makes (phasor == 0.0) a reliable wrap/start detector.
    Value one = r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr(1.0));
    Value zero = r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr(0.0));
    Value ovf =
        r.create<LLVM::FCmpOp>(loc, LLVM::FCmpPredicate::oge, advanced, one);
    Value next = r.create<LLVM::SelectOp>(loc, ovf, zero, advanced);

    r.create<LLVM::StoreOp>(loc, next, phase_ptr);
    r.replaceOp(op, phase); // return pre-advance phase
    return success();
  }
};

// // ImpulseOp: same phasor mechanics, but returns 1.0 on the wrap frame, 0.0
// // otherwise. The ovf flag is the result — no freq comparison needed
// externally. struct ImpulseOpLowering : public ConversionPattern {
//   ImpulseOpLowering(MLIRContext *ctx)
//       : ConversionPattern(ImpulseOp::getOperationName(), 1, ctx) {}
//   LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
//                                 ConversionPatternRewriter &r) const override
//                                 {
//     auto imp = cast<ImpulseOp>(op);
//     auto loc = op->getLoc();
//     auto ptr = LLVM::LLVMPointerType::get(op->getContext());
//     auto f64 = r.getF64Type();
//     auto i64 = r.getI64Type();
//
//     Value off_val = r.create<LLVM::ConstantOp>(
//         loc, i64, r.getI64IntegerAttr(imp.getStateOffset()));
//     Value phase_ptr = r.create<LLVM::GEPOp>(loc, ptr, r.getI8Type(),
//                                             operands[0],
//                                             ValueRange{off_val});
//
//     // Load pre-advance phase. Fire when it is exactly 0.0 — true at frame 0
//     // (zeroed initial state) and the frame after each wrap (because we store
//     // 0.0 on wrap, not the fractional remainder).
//     Value phase = r.create<LLVM::LoadOp>(loc, f64, phase_ptr);
//     Value step = r.create<LLVM::FMulOp>(loc, operands[1], operands[2]);
//     Value advanced = r.create<LLVM::FAddOp>(loc, phase, step);
//
//     Value one = r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr(1.0));
//     Value zero = r.create<LLVM::ConstantOp>(loc, f64,
//     r.getF64FloatAttr(0.0)); Value ovf =
//         r.create<LLVM::FCmpOp>(loc, LLVM::FCmpPredicate::oge, advanced, one);
//     Value next = r.create<LLVM::SelectOp>(loc, ovf, zero, advanced);
//     r.create<LLVM::StoreOp>(loc, next, phase_ptr);
//
//     Value fire =
//         r.create<LLVM::FCmpOp>(loc, LLVM::FCmpPredicate::oeq, phase, zero);
//     r.replaceOpWithNewOp<LLVM::SelectOp>(op, fire, one, zero);
//     return success();
//   }
// };

// TableLookupOp: fully inline GEP + linear interpolation — no function calls,
// fully visible to the LLVM optimizer.
struct TableLookupOpLowering : public ConversionPattern {
  TableLookupOpLowering(MLIRContext *ctx)
      : ConversionPattern(TableLookupOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto tbl = cast<TableLookupOp>(op);
    auto loc = op->getLoc();
    auto f64 = r.getF64Type();
    auto i64 = r.getI64Type();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());

    int32_t size = tbl.getSize();
    int64_t mask = (int64_t)size - 1;
    Value phase = operands[0];     // f64 in [0, 1)
    Value table_ptr = operands[1]; // !llvm.ptr — f64[]

    // d_index = phase * size
    Value size_f =
        r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr((double)size));
    Value d_index = r.create<LLVM::FMulOp>(loc, phase, size_f);

    // integer index and fractional part
    Value index = r.create<LLVM::FPToSIOp>(loc, i64, d_index);
    Value index_f = r.create<LLVM::SIToFPOp>(loc, f64, index);
    Value frac = r.create<LLVM::FSubOp>(loc, d_index, index_f);

    // idx0 = index & mask,  idx1 = (index + 1) & mask
    Value mask_c =
        r.create<LLVM::ConstantOp>(loc, i64, r.getI64IntegerAttr(mask));
    Value one_i64 =
        r.create<LLVM::ConstantOp>(loc, i64, r.getI64IntegerAttr(1));
    Value idx0 = r.create<LLVM::AndOp>(loc, index, mask_c);
    Value idx1 = r.create<LLVM::AndOp>(
        loc, r.create<LLVM::AddOp>(loc, index, one_i64), mask_c);

    // a = table[idx0], b = table[idx1]
    Value ptr_a =
        r.create<LLVM::GEPOp>(loc, ptr, f64, table_ptr, ValueRange{idx0});
    Value ptr_b =
        r.create<LLVM::GEPOp>(loc, ptr, f64, table_ptr, ValueRange{idx1});
    Value a = r.create<LLVM::LoadOp>(loc, f64, ptr_a);
    Value b_val = r.create<LLVM::LoadOp>(loc, f64, ptr_b);

    // lerp: a + frac * (b - a)
    Value diff = r.create<LLVM::FSubOp>(loc, b_val, a);
    Value interp =
        r.create<LLVM::FAddOp>(loc, a, r.create<LLVM::FMulOp>(loc, frac, diff));

    r.replaceOp(op, interp);
    return success();
  }
};

// BufplayOp and EnvAslrOp: stub lowerings (TODO: full implementations).
struct BufplayOpLowering : public ConversionPattern {
  BufplayOpLowering(MLIRContext *ctx)
      : ConversionPattern(BufplayOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value>,
                                ConversionPatternRewriter &r) const override {
    r.replaceOpWithNewOp<LLVM::ConstantOp>(op, r.getF64Type(),
                                           r.getF64FloatAttr(0.0));
    return success();
  }
};

struct EnvAslrOpLowering : public ConversionPattern {
  EnvAslrOpLowering(MLIRContext *ctx)
      : ConversionPattern(EnvAslrOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value>,
                                ConversionPatternRewriter &r) const override {
    r.replaceOpWithNewOp<LLVM::ConstantOp>(op, r.getF64Type(),
                                           r.getF64FloatAttr(1.0));
    return success();
  }
};

// =============================================================================
// DspToLLVMPass
// =============================================================================

struct DspToLLVMPass
    : public PassWrapper<DspToLLVMPass, OperationPass<ModuleOp>> {
  StringRef getName() const override { return "DspToLLVMPass"; }
  void runOnOperation() override {
    ConversionTarget target(getContext());
    target.addLegalDialect<LLVM::LLVMDialect, arith::ArithDialect,
                           scf::SCFDialect>();
    target.addIllegalDialect<DspDialect>();

    RewritePatternSet patterns(&getContext());
    patterns.add<InletOpLowering, OutletOpLowering, PhasorOpLowering,
                 // ImpulseOpLowering,
                 TableLookupOpLowering, BufplayOpLowering, EnvAslrOpLowering>(
        &getContext());

    if (failed(applyPartialConversion(getOperation(), target,
                                      std::move(patterns))))
      signalPassFailure();
  }
};

std::unique_ptr<Pass> mlir::createDspToLLVMPass() {
  return std::make_unique<DspToLLVMPass>();
}
