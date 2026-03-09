#pragma once

#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct Ast;

extern "C" {
#include "../../lang/backend_llvm/common.h"
}

namespace mlir {

struct HoistableCallArg {
  enum Kind { F64, I32 } kind;
  double f64val;
  int32_t i32val;
};

struct HoistableCall {
  int state_offset;
  std::string fn_name;
  std::vector<HoistableCallArg> args;
  enum RetKind { F64, I32 } ret_kind;
};

// Records a reference to an external buffer node that should be wired as a
// hidden inlet at node-creation time (via ylc_attach_node_inlet).
struct BufRefSpec {
  int inlet_idx;
  JITSymbol *sym; // resolved at build_dsp_expr time
};

struct ArrayInitSpec {
  int offset;
  double value;
};

struct ArrayStateSpec {
  int base_offset;
  int len;
  bool dynamic = false;
  int ptr_slot_offset = 0;
  int len_slot_offset = 0;
};

struct DynamicDelayAllocSpec {
  int ptr_slot_offset;
  int len_slot_offset;
  int inlet_idx;
};

// Build context threaded through AST → DSP op emission.
struct DspBuildCtx {
  OpBuilder b; // by value — callback rebinds this without corrupting the outer
               // builder
  ModuleOp mod;
  Location loc;
  Value node_ptr;   // !llvm.ptr  — the Node* itself
  Value state_ptr;  // !llvm.ptr  — opaque state block
  Value inputs_ptr; // !llvm.ptr  — Node** inputs array
  Value spf;        // f64        — seconds per frame
  Value frame_idx;  // index      — loop induction variable (set per-frame)
  int state_offset = 0;
  int next_hidden_inlet = 0; // next inlet index beyond the real lambda params
  std::unordered_map<std::string, Value> locals;
  // Insertion point before the scf.for loop — used to hoist values (e.g.
  // table pointers) that are loop-invariant.  Set after the ForOp is created;
  // updated each time a new hoisted op is emitted.
  OpBuilder::InsertPoint hoist_ip;

  // Array literals hoisted into the state buffer (name -> base offset/len).
  std::unordered_map<std::string, ArrayStateSpec> array_states;
  // Extern function calls with all-constant args, recorded during
  // build_dsp_expr and emitted as LLVM init calls in CompileAudioFnHandler.
  std::vector<HoistableCall> hoistable_calls;
  // Literal array element initializers to emit at node-creation time.
  std::vector<ArrayInitSpec> array_inits;
  // alloc_delay buffers whose sizes are computed in ctor from synth params.
  std::vector<DynamicDelayAllocSpec> dynamic_delay_allocs;
  // External buffer nodes wired as hidden inlets at node-creation time.
  std::vector<BufRefSpec> buf_ref_inputs;
};

using BuiltinDSPHandler = Value (*)(Ast *, DspBuildCtx &, JITLangCtx *);

} // namespace mlir
