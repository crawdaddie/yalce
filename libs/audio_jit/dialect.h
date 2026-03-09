// =============================================================================
// DSP dialect — op class definitions, dialect registration, declareExtern.
// Included by both dialect.cpp (lowering patterns) and audio_jit.cpp (builder).
// =============================================================================
#pragma once

#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Pass/Pass.h"

#include <memory>

namespace mlir {

// =============================================================================
// Forward declarations
// =============================================================================

class InletOp;
class OutletOp;
class PhasorOp;
class EnvAslrOp;
class DelayOp;
class Delay1Op;
class AllpassOp;
class Allpass1Op;
class Fdn4Op;
class Fdn4mOp;
class WhiteNoiseOp;
class BufReadOp;
class PhasorTrigOp;

// =============================================================================
// DspDialect
// =============================================================================

class DspDialect : public Dialect {
public:
  explicit DspDialect(MLIRContext *ctx);
  static StringRef getDialectNamespace() { return "dsp"; }
};

// =============================================================================
// DSP Ops
// =============================================================================

// dsp.inlet %inputs_ptr, %frame_idx {idx} : (!llvm.ptr, index) -> f64
// Reads one sample from inlet N at the current frame.
class InletOp
    : public Op<InletOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<2>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.inlet"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"idx"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value inputs_ptr,
                    Value frame_idx, int32_t idx) {
    s.addOperands({inputs_ptr, frame_idx});
    s.addAttribute("idx", b.getI32IntegerAttr(idx));
    s.addTypes(b.getF64Type());
  }
  Value getInputsPtr() { return getOperand(0); }
  Value getFrameIdx() { return getOperand(1); }
  int32_t getIdx() {
    return (*this)->getAttrOfType<IntegerAttr>("idx").getInt();
  }
};

// dsp.outlet %node_ptr, %frame_idx, %value : (!llvm.ptr, index, f64) -> ()
// Writes the final computed sample into node->output.buf[frame].
class OutletOp
    : public Op<OutletOp, OpTrait::ZeroRegions, OpTrait::ZeroResults,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<3>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.outlet"; }
  static ArrayRef<StringRef> getAttributeNames() { return {}; }
  static void build(OpBuilder &b, OperationState &s, Value node_ptr,
                    Value frame_idx, Value value) {
    s.addOperands({node_ptr, frame_idx, value});
  }
  Value getNodePtr() { return getOperand(0); }
  Value getFrameIdx() { return getOperand(1); }
  Value getValue() { return getOperand(2); }
};

// dsp.phasor %state_ptr, %freq, %spf {state_offset} : (!llvm.ptr, f64, f64) ->
// f64 Stateful phase accumulator. State: one f64 (phase in [0, 1)). Advances
// phase by freq*spf each frame, wraps at 1.0, returns raw phase. All waveform
// generators (ramp, sine, square, tri) derive from this primitive.
class PhasorOp
    : public Op<PhasorOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<3>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.phasor"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value freq, Value spf, int32_t state_offset) {
    s.addOperands({state_ptr, freq, spf});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addTypes(b.getF64Type());
  }
  Value getStatePtr() { return getOperand(0); }
  Value getFreq() { return getOperand(1); }
  Value getSpf() { return getOperand(2); }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
};

// dsp.impulse %state_ptr, %freq, %spf {state_offset} : (!llvm.ptr, f64, f64) ->
// f64 Same 8-byte state layout as PhasorOp. Returns 1.0 on the frame the phase
// wraps (i.e. once per period), 0.0 otherwise. No freq comparison at the
// call site — the wrap flag is internal to the op.
class ImpulseOp
    : public Op<ImpulseOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<3>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.impulse"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value freq, Value spf, int32_t state_offset) {
    s.addOperands({state_ptr, freq, spf});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
};

// dsp.phasor_trig %state_ptr, %freq, %spf {state_offset}
//                : (!llvm.ptr, f64, f64) -> f64
// Stateful phasor-backed trigger returning trig only.
// State: phase f64 (8 bytes).
// trig is 1.0 when phase == 0.0 (initially and on wrap), else 0.0.
class PhasorTrigOp
    : public Op<PhasorTrigOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<3>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.phasor_trig"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value freq, Value spf, int32_t state_offset) {
    s.addOperands({state_ptr, freq, spf});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
};

class LinscaleOp
    : public Op<LinscaleOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<5>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.linscale"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value domain_a,
                    Value domain_b, Value range_a, Value range_b, Value input) {
    s.addOperands({domain_a, domain_b, range_a, range_b, input});
    s.addTypes(b.getF64Type());
  }
};

// dsp.table_lookup %phase, %table_ptr {size} : (f64, !llvm.ptr) -> f64
// Stateless linear interpolation into a f64 wavetable of compile-time size
// (must be a power of 2). phase in [0, 1). Used by sin_osc and other
// wavetable oscillators built on top of PhasorOp.
class TableLookupOp
    : public Op<TableLookupOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<2>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.table_lookup"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"size"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value phase,
                    Value table_ptr, int32_t size) {
    s.addOperands({phase, table_ptr});
    s.addAttribute("size", b.getI32IntegerAttr(size));
    s.addTypes(b.getF64Type());
  }
  Value getPhase() { return getOperand(0); }
  Value getTablePtr() { return getOperand(1); }
  int32_t getSize() {
    return (*this)->getAttrOfType<IntegerAttr>("size").getInt();
  }
};

// dsp.bufread %phase, %buf_ptr, %size : (f64, !llvm.ptr, i64) -> f64
// Stateless linear interpolation into a f64 buffer of runtime size.
// phase in [0, 1). size is the number of f64 elements.
class BufReadOp
    : public Op<BufReadOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<3>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.bufread"; }
  static ArrayRef<StringRef> getAttributeNames() { return {}; }
  static void build(OpBuilder &b, OperationState &s, Value size, Value buf_ptr,
                    Value phase) {
    s.addOperands({size, buf_ptr, phase});
    s.addTypes(b.getF64Type());
  }
  Value getPhase() { return getOperand(2); }
  Value getBufPtr() { return getOperand(1); }
  Value getSize() { return getOperand(0); }
};

// dsp.env_aslr %state_ptr, %attack, %sus_lvl, %sus_dur, %release, %trig
//              {state_offset} : (!llvm.ptr, f64×5) -> f64
// Stateful ADSR envelope. State: phase f64 + stage i32 (12 bytes).
class EnvAslrOp
    : public Op<EnvAslrOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<6>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.env_aslr"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value attack, Value sus_lvl, Value sus_dur, Value release,
                    Value trig, int32_t state_offset) {
    s.addOperands({state_ptr, attack, sus_lvl, sus_dur, release, trig});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
};

// dsp.delay %state_ptr, %inputs_ptr, %input, %fb, %spf, %delay_time
//           {state_offset, buf_offset, buf_size}
//           : (!llvm.ptr, !llvm.ptr, f64, f64, f64, f64) -> f64
// Feedback delay. State (4 bytes used at state_offset): [write_pos: i32].
// delay_time is a per-sample f64 (seconds); delay_samps = (i32)(delay_time/spf)
// read_pos = (write_pos - delay_samps + buf_sz) % buf_sz.
class DelayOp
    : public Op<DelayOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<6>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.delay"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset", "buf_offset", "buf_size"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value inputs_ptr, Value input, Value fb, Value spf,
                    Value delay_time, int32_t state_offset, int32_t buf_offset,
                    int32_t buf_size) {
    s.addOperands({state_ptr, inputs_ptr, input, fb, spf, delay_time});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addAttribute("buf_offset", b.getI32IntegerAttr(buf_offset));
    s.addAttribute("buf_size", b.getI32IntegerAttr(buf_size));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
  int32_t getBufOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_offset").getInt();
  }
  int32_t getBufSize() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_size").getInt();
  }
};

class Delay1Op
    : public Op<Delay1Op, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<6>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.delay1"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset", "buf_offset", "buf_size"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value inputs_ptr, Value input, Value fb, Value spf,
                    Value delay_time, int32_t state_offset, int32_t buf_offset,
                    int32_t buf_size) {
    s.addOperands({state_ptr, inputs_ptr, input, fb, spf, delay_time});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addAttribute("buf_offset", b.getI32IntegerAttr(buf_offset));
    s.addAttribute("buf_size", b.getI32IntegerAttr(buf_size));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
  int32_t getBufOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_offset").getInt();
  }
  int32_t getBufSize() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_size").getInt();
  }
};

// dsp.allpass %state_ptr, %inputs_ptr, %input, %g, %spf, %delay_time
//             {state_offset, buf_offset, buf_size}
//             : (!llvm.ptr, !llvm.ptr, f64, f64, f64, f64) -> f64
// Schroeder allpass: w = input + g*delayed; out = delayed - g*input.
// Flat magnitude response — safe to chain. g=0 degenerates to pure ff delay.
class AllpassOp
    : public Op<AllpassOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<6>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.allpass"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset", "buf_offset", "buf_size"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value inputs_ptr, Value input, Value g, Value spf,
                    Value delay_time, int32_t state_offset, int32_t buf_offset,
                    int32_t buf_size) {
    s.addOperands({state_ptr, inputs_ptr, input, g, spf, delay_time});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addAttribute("buf_offset", b.getI32IntegerAttr(buf_offset));
    s.addAttribute("buf_size", b.getI32IntegerAttr(buf_size));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
  int32_t getBufOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_offset").getInt();
  }
  int32_t getBufSize() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_size").getInt();
  }
};

class Allpass1Op
    : public Op<Allpass1Op, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<6>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.allpass1"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset", "buf_offset", "buf_size"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value inputs_ptr, Value input, Value g, Value spf,
                    Value delay_time, int32_t state_offset, int32_t buf_offset,
                    int32_t buf_size) {
    s.addOperands({state_ptr, inputs_ptr, input, g, spf, delay_time});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addAttribute("buf_offset", b.getI32IntegerAttr(buf_offset));
    s.addAttribute("buf_size", b.getI32IntegerAttr(buf_size));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
  int32_t getBufOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_offset").getInt();
  }
  int32_t getBufSize() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_size").getInt();
  }
};

// dsp.fdn4 %state_ptr, %inputs_ptr, %input, %fb, %spf, %d1, %d2, %d3, %d4
//          {state_offset, buf0_offset, buf1_offset, buf2_offset, buf3_offset, buf_size}
//          : (!llvm.ptr, !llvm.ptr, f64, f64, f64, f64, f64, f64, f64) -> f64
class Fdn4Op
    : public Op<Fdn4Op, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<9>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.fdn4"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset", "buf0_offset", "buf1_offset",
                            "buf2_offset", "buf3_offset", "buf_size"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value inputs_ptr, Value input, Value fb, Value spf,
                    Value d1, Value d2, Value d3, Value d4, int32_t state_offset,
                    int32_t buf0_offset, int32_t buf1_offset, int32_t buf2_offset,
                    int32_t buf3_offset, int32_t buf_size) {
    s.addOperands({state_ptr, inputs_ptr, input, fb, spf, d1, d2, d3, d4});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addAttribute("buf0_offset", b.getI32IntegerAttr(buf0_offset));
    s.addAttribute("buf1_offset", b.getI32IntegerAttr(buf1_offset));
    s.addAttribute("buf2_offset", b.getI32IntegerAttr(buf2_offset));
    s.addAttribute("buf3_offset", b.getI32IntegerAttr(buf3_offset));
    s.addAttribute("buf_size", b.getI32IntegerAttr(buf_size));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
  int32_t getBuf0Offset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf0_offset").getInt();
  }
  int32_t getBuf1Offset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf1_offset").getInt();
  }
  int32_t getBuf2Offset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf2_offset").getInt();
  }
  int32_t getBuf3Offset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf3_offset").getInt();
  }
  int32_t getBufSize() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_size").getInt();
  }
};

// dsp.fdn4m %state_ptr, %inputs_ptr, %input, %fb, %spf, %d1, %d2, %d3, %d4,
// %moff
//           {state_offset, buf0_offset, buf1_offset, buf2_offset, buf3_offset,
//           buf_size}
//           : (!llvm.ptr, !llvm.ptr, f64, f64, f64, f64, f64, f64, f64, i64) ->
//           f64
class Fdn4mOp
    : public Op<Fdn4mOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<10>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.fdn4m"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset", "buf0_offset", "buf1_offset",
                            "buf2_offset", "buf3_offset", "buf_size"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value inputs_ptr, Value input, Value fb, Value spf, Value d1,
                    Value d2, Value d3, Value d4, Value matrix_offset,
                    int32_t state_offset, int32_t buf0_offset,
                    int32_t buf1_offset, int32_t buf2_offset,
                    int32_t buf3_offset, int32_t buf_size) {
    s.addOperands(
        {state_ptr, inputs_ptr, input, fb, spf, d1, d2, d3, d4, matrix_offset});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addAttribute("buf0_offset", b.getI32IntegerAttr(buf0_offset));
    s.addAttribute("buf1_offset", b.getI32IntegerAttr(buf1_offset));
    s.addAttribute("buf2_offset", b.getI32IntegerAttr(buf2_offset));
    s.addAttribute("buf3_offset", b.getI32IntegerAttr(buf3_offset));
    s.addAttribute("buf_size", b.getI32IntegerAttr(buf_size));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
  int32_t getBuf0Offset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf0_offset").getInt();
  }
  int32_t getBuf1Offset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf1_offset").getInt();
  }
  int32_t getBuf2Offset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf2_offset").getInt();
  }
  int32_t getBuf3Offset() {
    return (*this)->getAttrOfType<IntegerAttr>("buf3_offset").getInt();
  }
  int32_t getBufSize() {
    return (*this)->getAttrOfType<IntegerAttr>("buf_size").getInt();
  }
};

class WhiteNoiseOp
    : public Op<WhiteNoiseOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::ZeroOperands> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.white"; }
  static ArrayRef<StringRef> getAttributeNames() { return {}; }
  static void build(OpBuilder &b, OperationState &s) {
    s.addTypes(b.getF64Type());
  }
};

// =============================================================================
// Helpers
// =============================================================================

// Declares an external LLVM function in mod if not already present.
inline LLVM::LLVMFuncOp declare_extern(ModuleOp &mod, OpBuilder &b,
                                       StringRef name,
                                       LLVM::LLVMFunctionType fn_ty) {
  if (auto f = mod.lookupSymbol<LLVM::LLVMFuncOp>(name))
    return f;
  OpBuilder::InsertionGuard g(b);
  b.setInsertionPointToStart(mod.getBody());
  return b.create<LLVM::LLVMFuncOp>(mod.getLoc(), name, fn_ty);
}

// =============================================================================
// Pass factory
// =============================================================================

std::unique_ptr<Pass> createDspToLLVMPass();

} // namespace mlir
