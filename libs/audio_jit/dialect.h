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
class BufplayOp;
class EnvAslrOp;
class DelayOp;

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

// dsp.bufplay %state_ptr, %rate, %trig {state_offset} : (!llvm.ptr, f64, f64)
// -> f64 Stateful buffer player. State: phase f64 + prev_trig f64 (16 bytes).
class BufplayOp
    : public Op<BufplayOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<3>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.bufplay"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value rate, Value trig, int32_t state_offset) {
    s.addOperands({state_ptr, rate, trig});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addTypes(b.getF64Type());
  }
  Value getStatePtr() { return getOperand(0); }
  Value getRate() { return getOperand(1); }
  Value getTrig() { return getOperand(2); }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
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

// dsp.delay %state_ptr, %inputs_ptr, %input, %fb {state_offset, inlet_idx}
//           : (!llvm.ptr, !llvm.ptr, f64, f64) -> f64
// Feedback delay. State (8 bytes at state_offset): [read_pos: i32, write_pos: i32].
// inputs[inlet_idx] is a hidden delay-buffer node (delay line lives in its
// output.buf, allocated with calloc so the pool is never invalidated).
// out = input + buf[read_pos];  buf[write_pos] = fb * out;  advance both.
class DelayOp
    : public Op<DelayOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<4>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.delay"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"state_offset", "inlet_idx"};
    return n;
  }
  static void build(OpBuilder &b, OperationState &s, Value state_ptr,
                    Value inputs_ptr, Value input, Value fb,
                    int32_t state_offset, int32_t inlet_idx) {
    s.addOperands({state_ptr, inputs_ptr, input, fb});
    s.addAttribute("state_offset", b.getI32IntegerAttr(state_offset));
    s.addAttribute("inlet_idx", b.getI32IntegerAttr(inlet_idx));
    s.addTypes(b.getF64Type());
  }
  int32_t getStateOffset() {
    return (*this)->getAttrOfType<IntegerAttr>("state_offset").getInt();
  }
  int32_t getInletIdx() {
    return (*this)->getAttrOfType<IntegerAttr>("inlet_idx").getInt();
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
