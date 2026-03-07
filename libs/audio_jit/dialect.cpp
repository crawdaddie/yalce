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

extern "C" {
#include "../../engine/node.h"
}

using namespace mlir;

// Feedback delay.
// State layout:
//   state_offset: [write_pos: i32] (4 bytes used, 8 allocated)
//   buf_offset:   [delay ring buffer: f64[buf_size]]
// read_pos is computed as (write_pos - delay_samps + buf_size) % buf_size so
// the read head is always exactly delay_samps behind the write head.
extern "C" double ylc_delay_fb_state(void *state_raw, int32_t state_offset,
                                     int32_t buf_offset, int32_t buf_size,
                                     double input, double fb,
                                     int32_t delay_samps) {
  int32_t *write_pos = (int32_t *)((char *)state_raw + state_offset);
  double *buf = (double *)((char *)state_raw + buf_offset);
  if (delay_samps <= 0) {
    delay_samps = 1;
  }
  if (delay_samps >= buf_size) {
    delay_samps = buf_size - 1;
  }
  int32_t read_pos = (*write_pos - delay_samps + buf_size) % buf_size;
  double delayed = buf[read_pos];
  double out = input + delayed;
  buf[*write_pos] = fb * out;
  *write_pos = (*write_pos + 1) % buf_size;
  return out;
}

// Lerp Feedback delay.
// State layout:
//   state_offset: [write_pos: i32] (4 bytes used, 8 allocated)
//   buf_offset:   [delay ring buffer: f64[buf_size]]
// delay_secs is divided by spf to get a fractional sample count; linear
// interpolation between adjacent buffer samples gives sub-sample accuracy.
extern "C" double ylc_delay1_fb_state(void *state_raw, int32_t state_offset,
                                      int32_t buf_offset, int32_t buf_size,
                                      double input, double fb,
                                      double delay_secs, double spf) {
  int32_t *write_pos = (int32_t *)((char *)state_raw + state_offset);
  double *buf = (double *)((char *)state_raw + buf_offset);
  double delay_samps_f = delay_secs / spf;
  if (delay_samps_f < 1.0)
    delay_samps_f = 1.0;
  if (delay_samps_f >= buf_size)
    delay_samps_f = buf_size - 1;
  int32_t delay_samps_i = (int32_t)delay_samps_f;
  double frac = delay_samps_f - delay_samps_i;
  // read_pos0 is floor(delay) samples behind write_pos,
  // read_pos1 is one sample further back for the lerp upper bound.
  int32_t read_pos0 = (*write_pos - delay_samps_i + buf_size) % buf_size;
  int32_t read_pos1 = (read_pos0 - 1 + buf_size) % buf_size;
  double delayed = buf[read_pos0] * (1.0 - frac) + buf[read_pos1] * frac;
  double out = input + delayed;
  buf[*write_pos] = fb * out;
  *write_pos = (*write_pos + 1) % buf_size;
  return out;
}

// Schroeder allpass delay.
// State layout:
//   state_offset: [write_pos: i32] (4 bytes used, 8 allocated)
//   buf_offset:   [delay ring buffer: f64[buf_size]]
// w = input + g * delayed;  out = delayed - g * input;  buf[write_pos] = w.
// Flat magnitude response: safe to chain. g=0 is a pure feedforward delay.
extern "C" double ylc_allpass_state(void *state_raw, int32_t state_offset,
                                    int32_t buf_offset, int32_t buf_size,
                                    double input, double g,
                                    int32_t delay_samps) {
  int32_t *write_pos = (int32_t *)((char *)state_raw + state_offset);
  double *buf = (double *)((char *)state_raw + buf_offset);
  if (delay_samps <= 0)
    delay_samps = 1;
  if (delay_samps >= buf_size)
    delay_samps = buf_size - 1;
  int32_t read_pos = (*write_pos - delay_samps + buf_size) % buf_size;
  double delayed = buf[read_pos];
  buf[*write_pos] = input + g * delayed;
  *write_pos = (*write_pos + 1) % buf_size;
  return delayed - g * input;
}

// Schroeder allpass with linear interpolation.
// State layout:
//   state_offset: [write_pos: i32] (4 bytes used, 8 allocated)
//   buf_offset:   [delay ring buffer: f64[buf_size]]
// delay_secs is divided by spf to get a fractional sample count; lerp between
// adjacent buffer samples gives sub-sample accuracy.
extern "C" double ylc_allpass1_state(void *state_raw, int32_t state_offset,
                                     int32_t buf_offset, int32_t buf_size,
                                     double input, double g,
                                     double delay_secs, double spf) {
  int32_t *write_pos = (int32_t *)((char *)state_raw + state_offset);
  double *buf = (double *)((char *)state_raw + buf_offset);
  double delay_samps_f = delay_secs / spf;
  if (delay_samps_f < 1.0)
    delay_samps_f = 1.0;
  if (delay_samps_f >= buf_size)
    delay_samps_f = buf_size - 1;
  int32_t delay_samps_i = (int32_t)delay_samps_f;
  double frac = delay_samps_f - delay_samps_i;
  int32_t read_pos0 = (*write_pos - delay_samps_i + buf_size) % buf_size;
  int32_t read_pos1 = (read_pos0 - 1 + buf_size) % buf_size;
  double delayed = buf[read_pos0] * (1.0 - frac) + buf[read_pos1] * frac;
  buf[*write_pos] = input + g * delayed;
  *write_pos = (*write_pos + 1) % buf_size;
  return delayed - g * input;
}

// =============================================================================
// DspDialect constructor — registers all ops.
// =============================================================================

DspDialect::DspDialect(MLIRContext *ctx)
    : Dialect("dsp", ctx, TypeID::get<DspDialect>()) {
  addOperations<InletOp, OutletOp, PhasorOp, ImpulseOp, PhasorTrigOp,
                TableLookupOp, BufReadOp, LinscaleOp, DelayOp, Delay1Op,
                AllpassOp, Allpass1Op, WhiteNoiseOp>();
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
    auto fmf =
        LLVM::FastmathFlagsAttr::get(r.getContext(), LLVM::FastmathFlags::fast);
    Value phase = r.create<LLVM::LoadOp>(loc, f64, phase_ptr);
    Value step = r.create<LLVM::FMulOp>(loc, operands[1], operands[2], fmf);
    Value advanced = r.create<LLVM::FAddOp>(loc, phase, step, fmf);

    // Wrap to [0, 1): overflow → 0.0, underflow (negative freq) → 1.0.
    // Two chained selects keep the logic branchless.
    Value one = r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr(1.0));
    Value zero = r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr(0.0));
    Value ovf =
        r.create<LLVM::FCmpOp>(loc, LLVM::FCmpPredicate::oge, advanced, one);
    Value udf =
        r.create<LLVM::FCmpOp>(loc, LLVM::FCmpPredicate::olt, advanced, zero);
    Value next = r.create<LLVM::SelectOp>(loc, ovf, zero, advanced);
    next = r.create<LLVM::SelectOp>(loc, udf, one, next);

    r.create<LLVM::StoreOp>(loc, next, phase_ptr);
    r.replaceOp(op, phase); // return pre-advance phase
    return success();
  }
};

// PhasorTrigOp: phasor-backed trigger returning trig only.
// State layout: [phase: f64].
// Returns trig. trig = 1.0 when phase == 0.0 else 0.0.
struct PhasorTrigOpLowering : public ConversionPattern {
  PhasorTrigOpLowering(MLIRContext *ctx)
      : ConversionPattern(PhasorTrigOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto imp = cast<PhasorTrigOp>(op);
    auto loc = op->getLoc();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i64 = r.getI64Type();

    // GEP: state_ptr + state_offset → &phase
    Value off_val = r.create<LLVM::ConstantOp>(
        loc, i64, r.getI64IntegerAttr(imp.getStateOffset()));
    Value phase_ptr = r.create<LLVM::GEPOp>(loc, ptr, r.getI8Type(),
                                            operands[0], ValueRange{off_val});

    auto fmf =
        LLVM::FastmathFlagsAttr::get(r.getContext(), LLVM::FastmathFlags::fast);

    Value phase = r.create<LLVM::LoadOp>(loc, f64, phase_ptr);

    // trig = (phase == 0.0) ? 1.0 : 0.0
    Value zero = r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr(0.0));
    Value one = r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr(1.0));
    Value is_zero =
        r.create<LLVM::FCmpOp>(loc, LLVM::FCmpPredicate::oeq, phase, zero);
    Value trig = r.create<LLVM::UIToFPOp>(loc, f64, is_zero);

    // Advance phase (same wrap semantics as PhasorOpLowering).
    Value step = r.create<LLVM::FMulOp>(loc, operands[1], operands[2], fmf);
    Value advanced = r.create<LLVM::FAddOp>(loc, phase, step, fmf);
    Value ovf =
        r.create<LLVM::FCmpOp>(loc, LLVM::FCmpPredicate::oge, advanced, one);
    Value udf =
        r.create<LLVM::FCmpOp>(loc, LLVM::FCmpPredicate::olt, advanced, zero);
    Value next = r.create<LLVM::SelectOp>(loc, ovf, zero, advanced);
    next = r.create<LLVM::SelectOp>(loc, udf, one, next);

    r.create<LLVM::StoreOp>(loc, next, phase_ptr);

    r.replaceOp(op, ValueRange{trig});
    return success();
  }
};

//  t      = (input - domain_a) / (domain_b - domain_a)   // normalize to [0,1]
// output = range_a + t * (range_b - range_a)             // lerp into output
// range
struct LinscaleOpLowering : public ConversionPattern {
  LinscaleOpLowering(MLIRContext *ctx)
      : ConversionPattern(LinscaleOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto loc = op->getLoc();
    auto fmf =
        LLVM::FastmathFlagsAttr::get(r.getContext(), LLVM::FastmathFlags::fast);
    // operands[] are the already-lowered SSA values in order of addOperands():
    // [domain_a, domain_b, range_a, range_b, input]
    Value num = r.create<LLVM::FSubOp>(loc, operands[4], operands[0], fmf);
    Value denom = r.create<LLVM::FSubOp>(loc, operands[1], operands[0], fmf);
    Value t = r.create<LLVM::FDivOp>(loc, num, denom, fmf);
    Value span = r.create<LLVM::FSubOp>(loc, operands[3], operands[2], fmf);
    Value scaled = r.create<LLVM::FMulOp>(loc, t, span, fmf);
    r.replaceOpWithNewOp<LLVM::FAddOp>(op, operands[2], scaled, fmf);
    return success();
  }
};

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

    auto fmf =
        LLVM::FastmathFlagsAttr::get(r.getContext(), LLVM::FastmathFlags::fast);
    // d_index = phase * size
    Value size_f =
        r.create<LLVM::ConstantOp>(loc, f64, r.getF64FloatAttr((double)size));
    Value d_index = r.create<LLVM::FMulOp>(loc, phase, size_f, fmf);

    // integer index and fractional part
    Value index = r.create<LLVM::FPToSIOp>(loc, i64, d_index);
    Value index_f = r.create<LLVM::SIToFPOp>(loc, f64, index);
    Value frac = r.create<LLVM::FSubOp>(loc, d_index, index_f, fmf);

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
    Value diff = r.create<LLVM::FSubOp>(loc, b_val, a, fmf);
    Value interp = r.create<LLVM::FAddOp>(
        loc, a, r.create<LLVM::FMulOp>(loc, frac, diff, fmf), fmf);

    r.replaceOp(op, interp);
    return success();
  }
};

// BufReadOp: lerp into a runtime-sized f64 buffer.
// operands: [phase: f64, buf_ptr: !llvm.ptr, size: i64]
// size is a runtime value so idx1 wraps with modulo instead of bitmask.
struct BufReadOpLowering : public ConversionPattern {
  BufReadOpLowering(MLIRContext *ctx)
      : ConversionPattern(BufReadOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto loc = op->getLoc();
    auto f64 = r.getF64Type();
    auto i64 = r.getI64Type();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());

    Value phase = operands[2];    // f64 in [0, 1)
    Value buf_ptr = operands[1];  // !llvm.ptr — f64[]
    Value size_i64 = operands[0]; // i64 — number of elements

    auto fmf =
        LLVM::FastmathFlagsAttr::get(r.getContext(), LLVM::FastmathFlags::fast);

    // d_index = phase * (f64)size  (in [0, size))
    Value size_f = r.create<LLVM::SIToFPOp>(loc, f64, size_i64);
    Value d_index = r.create<LLVM::FMulOp>(loc, phase, size_f, fmf);

    // integer floor and fractional part
    Value index = r.create<LLVM::FPToSIOp>(loc, i64, d_index);
    Value index_f = r.create<LLVM::SIToFPOp>(loc, f64, index);
    Value frac = r.create<LLVM::FSubOp>(loc, d_index, index_f, fmf);

    // idx0 in [0, size-1]; idx1 = (idx0 + 1) % size
    Value one_i64 =
        r.create<LLVM::ConstantOp>(loc, i64, r.getI64IntegerAttr(1));
    Value idx0 = index;
    Value idx1 = r.create<LLVM::URemOp>(
        loc, r.create<LLVM::AddOp>(loc, index, one_i64), size_i64);

    Value ptr_a =
        r.create<LLVM::GEPOp>(loc, ptr, f64, buf_ptr, ValueRange{idx0});
    Value ptr_b =
        r.create<LLVM::GEPOp>(loc, ptr, f64, buf_ptr, ValueRange{idx1});
    Value a = r.create<LLVM::LoadOp>(loc, f64, ptr_a);
    Value b_val = r.create<LLVM::LoadOp>(loc, f64, ptr_b);

    // lerp: a + frac * (b - a)
    Value diff = r.create<LLVM::FSubOp>(loc, b_val, a, fmf);
    r.replaceOp(op,
                r.create<LLVM::FAddOp>(
                    loc, a, r.create<LLVM::FMulOp>(loc, frac, diff, fmf), fmf));
    return success();
  }
};

// DelayOp: calls ylc_delay_fb_state(state_raw, state_offset, buf_offset,
//          buf_size, input, fb, delay_samps) → f64
// operands: [state_ptr, inputs_ptr, input, fb, spf, delay_time]
struct DelayOpLowering : public ConversionPattern {
  DelayOpLowering(MLIRContext *ctx)
      : ConversionPattern(DelayOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto delay = cast<DelayOp>(op);
    auto loc = op->getLoc();
    auto mod = op->getParentOfType<ModuleOp>();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i32 = r.getI32Type();

    // ylc_delay_fb_state(state_raw, state_offset, buf_offset, buf_size,
    //                    input, fb, delay_samps) -> f64
    auto fn_ty = LLVM::LLVMFunctionType::get(
        f64, {ptr, i32, i32, i32, f64, f64, i32}, false);
    auto fn = declare_extern(mod, r, "ylc_delay_fb_state", fn_ty);

    Value state_off = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(delay.getStateOffset()));
    Value buf_off = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(delay.getBufOffset()));
    Value buf_size = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(delay.getBufSize()));

    auto fmf = arith::FastMathFlagsAttr::get(r.getContext(),
                                             arith::FastMathFlags::fast);
    Value delay_samps = r.create<arith::FPToSIOp>(
        loc, i32, r.create<arith::DivFOp>(loc, operands[5], operands[4], fmf));

    r.replaceOpWithNewOp<LLVM::CallOp>(
        op, fn,
        ValueRange{operands[0], state_off, buf_off, buf_size, operands[2],
                   operands[3], delay_samps});
    return success();
  }
};

struct Delay1OpLowering : public ConversionPattern {
  Delay1OpLowering(MLIRContext *ctx)
      : ConversionPattern(Delay1Op::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto delay = cast<Delay1Op>(op);
    auto loc = op->getLoc();
    auto mod = op->getParentOfType<ModuleOp>();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i32 = r.getI32Type();

    // ylc_delay1_fb_state(state_raw, state_offset, buf_offset, buf_size,
    //                     input, fb, delay_secs, spf) -> f64
    auto fn_ty = LLVM::LLVMFunctionType::get(
        f64, {ptr, i32, i32, i32, f64, f64, f64, f64}, false);
    auto fn = declare_extern(mod, r, "ylc_delay1_fb_state", fn_ty);

    Value state_off = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(delay.getStateOffset()));
    Value buf_off = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(delay.getBufOffset()));
    Value buf_size = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(delay.getBufSize()));

    auto fmf = arith::FastMathFlagsAttr::get(r.getContext(),
                                             arith::FastMathFlags::fast);

    r.replaceOpWithNewOp<LLVM::CallOp>(
        op, fn,
        ValueRange{operands[0], state_off, buf_off, buf_size, operands[2],
                   operands[3], operands[5], operands[4]});
    return success();
  }
};

// AllpassOp: calls ylc_allpass_state(state_raw, state_offset, buf_offset,
//            buf_size, input, g, delay_samps) → f64
// operands: [state_ptr, inputs_ptr, input, g, spf, delay_time]
struct AllpassOpLowering : public ConversionPattern {
  AllpassOpLowering(MLIRContext *ctx)
      : ConversionPattern(AllpassOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto ap = cast<AllpassOp>(op);
    auto loc = op->getLoc();
    auto mod = op->getParentOfType<ModuleOp>();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i32 = r.getI32Type();

    // ylc_allpass_state(state_raw, state_offset, buf_offset, buf_size,
    //                   input, g, delay_samps) -> f64
    auto fn_ty = LLVM::LLVMFunctionType::get(
        f64, {ptr, i32, i32, i32, f64, f64, i32}, false);
    auto fn = declare_extern(mod, r, "ylc_allpass_state", fn_ty);

    Value state_off = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(ap.getStateOffset()));
    Value buf_off = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(ap.getBufOffset()));
    Value buf_size = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(ap.getBufSize()));

    auto fmf = arith::FastMathFlagsAttr::get(r.getContext(),
                                             arith::FastMathFlags::fast);
    Value delay_samps = r.create<arith::FPToSIOp>(
        loc, i32, r.create<arith::DivFOp>(loc, operands[5], operands[4], fmf));

    r.replaceOpWithNewOp<LLVM::CallOp>(
        op, fn,
        ValueRange{operands[0], state_off, buf_off, buf_size, operands[2],
                   operands[3], delay_samps});
    return success();
  }
};

// Allpass1Op: calls ylc_allpass1_state(state_raw, state_offset, buf_offset,
//             buf_size, input, g, delay_secs, spf) → f64
// operands: [state_ptr, inputs_ptr, input, g, spf, delay_time]
struct Allpass1OpLowering : public ConversionPattern {
  Allpass1OpLowering(MLIRContext *ctx)
      : ConversionPattern(Allpass1Op::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto ap = cast<Allpass1Op>(op);
    auto loc = op->getLoc();
    auto mod = op->getParentOfType<ModuleOp>();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i32 = r.getI32Type();

    // ylc_allpass1_state(state_raw, state_offset, buf_offset, buf_size,
    //                    input, g, delay_secs, spf) -> f64
    auto fn_ty = LLVM::LLVMFunctionType::get(
        f64, {ptr, i32, i32, i32, f64, f64, f64, f64}, false);
    auto fn = declare_extern(mod, r, "ylc_allpass1_state", fn_ty);

    Value state_off = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(ap.getStateOffset()));
    Value buf_off = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(ap.getBufOffset()));
    Value buf_size = r.create<LLVM::ConstantOp>(
        loc, i32, r.getI32IntegerAttr(ap.getBufSize()));

    // operands[5]=delay_time, operands[4]=spf
    r.replaceOpWithNewOp<LLVM::CallOp>(
        op, fn,
        ValueRange{operands[0], state_off, buf_off, buf_size, operands[2],
                   operands[3], operands[5], operands[4]});
    return success();
  }
};

extern "C" double white_noise_sample() {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * 2. - 1.;
  return rand_double;
}

struct WhiteNoiseLowering : public ConversionPattern {
  WhiteNoiseLowering(MLIRContext *ctx)
      : ConversionPattern(WhiteNoiseOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto loc = op->getLoc();
    auto mod = op->getParentOfType<ModuleOp>();
    auto f64 = r.getF64Type();

    auto fn_ty = LLVM::LLVMFunctionType::get(f64, {}, false);
    auto fn = declare_extern(mod, r, "white_noise_sample", fn_ty);

    r.replaceOpWithNewOp<LLVM::CallOp>(op, fn, ValueRange{});
    return success();
  }
};
// // BufplayOp and EnvAslrOp: stub lowerings (TODO: full implementations).
// struct BufplayOpLowering : public ConversionPattern {
//   BufplayOpLowering(MLIRContext *ctx)
//       : ConversionPattern(BufplayOp::getOperationName(), 1, ctx) {}
//   LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value>,
//                                 ConversionPatternRewriter &r) const override
//                                 {
//     r.replaceOpWithNewOp<LLVM::ConstantOp>(op, r.getF64Type(),
//                                            r.getF64FloatAttr(0.0));
//     return success();
//   }
// };
//
// struct EnvAslrOpLowering : public ConversionPattern {
//   EnvAslrOpLowering(MLIRContext *ctx)
//       : ConversionPattern(EnvAslrOp::getOperationName(), 1, ctx) {}
//   LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value>,
//                                 ConversionPatternRewriter &r) const override
//                                 {
//     r.replaceOpWithNewOp<LLVM::ConstantOp>(op, r.getF64Type(),
//                                            r.getF64FloatAttr(1.0));
//     return success();
//   }
// };

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
    patterns
        .add<InletOpLowering, OutletOpLowering, PhasorOpLowering,
             // ImpulseOpLowering,
             PhasorTrigOpLowering, LinscaleOpLowering, TableLookupOpLowering,
             BufReadOpLowering, DelayOpLowering, Delay1OpLowering,
             AllpassOpLowering, Allpass1OpLowering, WhiteNoiseLowering>(
            &getContext());

    if (failed(applyPartialConversion(getOperation(), target,
                                      std::move(patterns))))
      signalPassFailure();
  }
};

std::unique_ptr<Pass> mlir::createDspToLLVMPass() {
  return std::make_unique<DspToLLVMPass>();
}
