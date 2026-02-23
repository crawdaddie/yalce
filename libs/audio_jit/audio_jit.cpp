// =============================================================================
// audio_jit — DSP dialect → LLVM IR compiler for Yalce
//
// Pipeline:
//   AST → dsp.* ops (in MLIR module)
//       → DspToLLVMPass (inline all ops to LLVM dialect)
//       → SCFToControlFlow + CFToLLVM + ArithToLLVM + ReconcileUnrealizedCasts
//       → translateModuleToLLVMIR
//       → LLVM O3 (loop-fusion, vectorisation, inlining across synths)
//       → linkModules into MCJIT
// =============================================================================

extern "C" {
#include "audio_jit.h"
#include "../../engine/audio_graph.h"
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/backend_llvm/module.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/common.h"
#include "../../lang/format_utils.h"
#include "../../lang/ht.h"
#include "../../lang/modules.h"
#include "../../lang/parse.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/inference.h"
}

#include "dialect.h"

#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm-c/Core.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
// engine/common.h defines PI as M_PI; undef before LLVM headers use PI as an
// identifier.
#undef PI
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

// =============================================================================
// Wavetable data — embedded at compile time from pre-generated CSV files.
// Avoids any runtime symbol lookup; the address is a true compile-time
// constant.
// =============================================================================
//
#define SQ_TABSIZE (1 << 11)
#define SIN_TABSIZE (1 << 11)
#define SAW_TABSIZE (1 << 11)
#define GRAIN_WINDOW_TABSIZE (1 << 9)
static const double sin_table[SIN_TABSIZE] = {
#include "../../engine/assets/sin_table.csv"
};
static const double sq_table[SQ_TABSIZE] = {
#include "../../engine/assets/sq_table.csv"
};
static const double saw_table[SAW_TABSIZE] = {
#include "../../engine/assets/saw_table.csv"
};
static const double grain_win[GRAIN_WINDOW_TABSIZE] = {
#include "../../engine/assets/grain_win.csv"
};

int STYPE_AUDIO_JIT_SYM;
int STYPE_AUDIO_JIT_BUILTIN_HANDLER;

using namespace mlir;

// =============================================================================
// Runtime helpers — resolved by RTLD_GLOBAL when the .so is loaded.
// =============================================================================

extern "C" double ylc_read_inlet(void *inputs_raw, int32_t idx, int64_t frame) {
  return reinterpret_cast<Node **>(inputs_raw)[idx]->output.buf[frame];
}
extern "C" void ylc_write_output(void *node_raw, int64_t frame, double val) {
  reinterpret_cast<Node *>(node_raw)->output.buf[frame] = val;
}
extern "C" void *ylc_get_output_buf(void *node_raw) {
  return reinterpret_cast<Node *>(node_raw)->output.buf;
}
// Return the data pointer of inputs[inlet_idx] — hoisted before the sample
// loop.
extern "C" void *ylc_buf_inlet_data(void *inputs_raw, int32_t inlet_idx) {
  return reinterpret_cast<Node **>(inputs_raw)[inlet_idx]->output.buf;
}
// Return the element count of inputs[inlet_idx] — hoisted before the sample
// loop.
extern "C" int64_t ylc_buf_inlet_size(void *inputs_raw, int32_t inlet_idx) {
  return (int64_t)reinterpret_cast<Node **>(inputs_raw)[inlet_idx]->output.size;
}

// Return the inlet Node* itself — hoisted before the sample loop by
// BufRefHandler.
extern "C" void *ylc_get_inlet_node(void *inputs_raw, int32_t inlet_idx) {
  return reinterpret_cast<Node **>(inputs_raw)[inlet_idx];
}
// Read a sample from a buffer node using lerp interpolation.
// node_raw is the Node* returned by ylc_get_inlet_node.  phase is in [0, 1).
extern "C" double ylc_bufread(void *node_raw, double phase) {
  Node *node = reinterpret_cast<Node *>(node_raw);
  double *buf = node->output.buf;
  int64_t sz = (int64_t)node->output.size;
  if (sz <= 0) {
    return 0.0;
  }
  double d_index = phase * (double)sz;
  int64_t index = (int64_t)d_index;

  if (index < 0) {
    index = 0;
  }

  if (index >= sz) {
    index = sz - 1;
  }

  double frac = d_index - (double)index;
  double a = buf[index];
  double b_val = buf[(index + 1) % sz];

  return a + frac * (b_val - a);
}

extern "C" double grain_samp(double *buf, int64_t buf_size, double trig,
                             double prev_trig, double pos, double rate,
                             double width, double spf, int32_t max_grains,
                             double *rates, double *phases, double *widths,
                             double *remaining_secs, double *starts,
                             int32_t *active, int32_t *active_grains) {
  double sample = 0.0;
  if (prev_trig < 0.5 && trig >= 0.5 && *active_grains < max_grains) {
    for (int i = 0; i < max_grains; i++) {
      if (active[i] == 0) {
        rates[i] = rate;
        phases[i] = 0.0;
        starts[i] = pos * buf_size;
        widths[i] = width;
        remaining_secs[i] = width;
        active[i] = 1;
        (*active_grains)++;
        break;
      }
    }
  }

  for (int i = 0; i < max_grains; i++) {
    if (active[i]) {
      double r = rates[i];
      double p = phases[i];
      double s = starts[i];
      double w = widths[i];
      double rem = remaining_secs[i];

      double d_index = s + (p * buf_size);
      int index = (int)d_index;
      double frac = d_index - index;

      double a = buf[index % buf_size];
      double b_val = buf[(index + 1) % buf_size];

      double grain_elapsed = 1.0 - (rem / w);
      double env_val = pow2table_read(grain_elapsed, GRAIN_WINDOW_TABSIZE,
                                      (double *)grain_win);

      sample += env_val * ((1.0 - frac) * a + (frac * b_val));
      phases[i] += (r / buf_size);

      remaining_secs[i] -= spf;
      if (remaining_secs[i] <= 0.0) {
        active[i] = 0;
        (*active_grains)--;
      }
    }
  }

  return sample;
}

// Return the element count of a buffer node.
extern "C" int64_t ylc_bufsize(void *node_raw) {
  Node *node = reinterpret_cast<Node *>(node_raw);
  return (int64_t)node->output.size;
}

// Describes a hidden delay-buffer inlet baked into a compiled synth.
// Passed as a compile-time-constant array to ylc_create_audio_node so that
// node creation, delay-buffer allocation, and inlet wiring all happen in one
// place.
struct BufInputSpec {
  int inlet_idx;
  double max_delay_sec;
};

// Called at runtime (from JIT'd code) to wrap a compiled perform function
// in a Node and register it in the audio graph.
extern "C" Node *ylc_create_audio_node(perform_func_t perform, int num_inputs,
                                       int state_bytes) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, state_bytes);
  int saved_idx = node->node_index;
  int state_off =
      state_bytes > 0 ? state_offset_ptr_in_graph(graph, state_bytes) : 0;
  *node = (Node){
      .perform = perform,
      .node_index = saved_idx,
      .num_inputs = num_inputs,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .state_size = state_bytes,
      .state_offset = state_off,
      .meta = (char *)"audio_jit_synth",
  };

  if (state_bytes > 0) {
    node->state_ptr = state_ptr(graph, node);
    memset(node->state_ptr, 0, state_bytes);
  }

  return graph_embed(node);
}

// Return the state buffer pointer of a node (used by JIT-generated init code).
extern "C" void *ylc_node_get_state(Node *node) { return node->state_ptr; }

// Wire one hidden delay-buffer inlet into a freshly created DSP node.
// Creates a zeroed circular buffer of max_delay_sec duration and plugs it as
// inputs[inlet_idx].  We allocate the Node directly (without graph_embed) so
// the delay buffer does NOT get appended to _chain_head/_chain_tail — if it
// did, play_node() would see a non-NULL chain tail, call chain(), and set
// write_to_output on the zero-buffer node instead of the JIT synth node,
// producing silence.
extern "C" void ylc_attach_buf_inlet(Node *node, int32_t inlet_idx,
                                     double max_delay_sec) {
  int max_sz = (int)(max_delay_sec / ctx_spf());
  if (max_sz <= 0)
    max_sz = 1;
  Node *buf = (Node *)calloc(1, sizeof(Node));
  *buf = (Node){
      .perform = NULL,
      .num_inputs = 0,
      .output = (Signal){.layout = 1,
                         .size = max_sz,
                         .buf = (double *)calloc(max_sz, sizeof(double))},
      .meta = (char *)"delay_buf",
  };
  plug_input_in_graph(inlet_idx, node, buf);
}

// =============================================================================
// Global MLIR context
// =============================================================================

static std::unique_ptr<MLIRContext> g_mlir_ctx;

static MLIRContext *get_mlir_ctx() {
  if (!g_mlir_ctx) {
    g_mlir_ctx = std::make_unique<MLIRContext>();
    g_mlir_ctx
        ->loadDialect<DspDialect, arith::ArithDialect, cf::ControlFlowDialect,
                      scf::SCFDialect, LLVM::LLVMDialect>();
    registerBuiltinDialectTranslation(*g_mlir_ctx);
    registerLLVMDialectTranslation(*g_mlir_ctx);
  }
  return g_mlir_ctx.get();
}

// =============================================================================
// Location helper
// =============================================================================

static Location ast_loc(Ast *ast, MLIRContext *ctx) {
  if (ast && ast->loc_info) {
    auto *li = ast->loc_info;
    auto file =
        mlir::StringAttr::get(ctx, li->src_file ? li->src_file : "<unknown>");
    return mlir::FileLineColLoc::get(file, li->line, li->col);
  }
  return mlir::UnknownLoc::get(ctx);
}

// =============================================================================
// Hoistable call — an extern function call with all-constant args that should
// be evaluated once at node-creation time and stored in the state buffer.
// =============================================================================

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
};

// =============================================================================
// Build context threaded through AST → DSP op emission
// =============================================================================

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
  std::vector<BufInputSpec> buf_inputs;
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
  // External buffer nodes wired as hidden inlets at node-creation time.
  std::vector<BufRefSpec> buf_ref_inputs;
};

static int reserve_state(DspBuildCtx &ctx, int bytes) {
  int off = ctx.state_offset;
  ctx.state_offset += bytes;
  return off;
}

// Hoist array literal into state and return base offset (i64) as MLIR value.
static Value emit_array_state(Ast *array_ast, DspBuildCtx &ctx,
                              const std::string &name) {
  int len = (int)array_ast->data.AST_LIST.len;
  int arr_base = ctx.state_offset;
  ctx.state_offset += 8 * len;

  ctx.array_states[name] = ArrayStateSpec{arr_base, len};

  // Record literal initializers for node-creation time.
  for (int i = 0; i < len; i++) {
    Ast *el = &array_ast->data.AST_LIST.items[i];
    double v = 0.0;
    if (el->tag == AST_DOUBLE) {
      v = el->data.AST_DOUBLE.value;
    } else if (el->tag == AST_FLOAT) {
      v = (double)el->data.AST_FLOAT.value;
    } else if (el->tag == AST_INT) {
      v = (double)el->data.AST_INT.value;
    } else {
      fprintf(stderr,
              "Error: array literal elements must be numeric constants\n");
      print_ast(el);
      return {};
    }
    int off = arr_base + (i * 8);
    ctx.array_inits.push_back({off, v});
  }

  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto i64_ty = b.getI64Type();
  return b.create<LLVM::ConstantOp>(loc, i64_ty, b.getI64IntegerAttr(arr_base));
}

// =============================================================================
// AST → DSP ops
// =============================================================================

static Value build_dsp_expr(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx);

// Check if an AST subtree is a compile-time constant (delegates to the type
// system's is_constant_expr, using jit_ctx->env for scope lookup).
static bool ast_is_const(Ast *ast, JITLangCtx *jit_ctx) {
  TICtx ti = {};
  ti.env = jit_ctx->env;
  return is_constant_expr(ast, &ti);
}

// Try to extract a constant argument value from a literal AST node.
// Returns nullopt if the node isn't a simple literal.
static std::optional<HoistableCallArg> ast_to_const_arg(Ast *ast,
                                                        ::Type *param_ty) {
  if (ast->tag == AST_DOUBLE)
    return HoistableCallArg{HoistableCallArg::F64, ast->data.AST_DOUBLE.value,
                            0};
  if (ast->tag == AST_FLOAT)
    return HoistableCallArg{HoistableCallArg::F64,
                            (double)ast->data.AST_FLOAT.value, 0};
  if (ast->tag == AST_INT) {
    if (param_ty && param_ty->kind == T_NUM)
      return HoistableCallArg{HoistableCallArg::F64,
                              (double)ast->data.AST_INT.value, 0};
    return HoistableCallArg{HoistableCallArg::I32, 0.0,
                            (int32_t)ast->data.AST_INT.value};
  }
  return std::nullopt;
}

static Value get_static_table(const char *table_name, void *table_data,
                              DspBuildCtx &ctx) {

  auto i64_ty = ctx.b.getI64Type();
  auto ptr_ty = LLVM::LLVMPointerType::get(ctx.b.getContext());
  Value addr = ctx.b.create<LLVM::ConstantOp>(
      ctx.loc, i64_ty, ctx.b.getI64IntegerAttr((int64_t)(uintptr_t)table_data));
  Value ptr = ctx.b.create<LLVM::IntToPtrOp>(ctx.loc, ptr_ty, addr);
  return ptr;
}

// Wavetable oscillator: PhasorOp manages state, TableLookupOp does inline lerp.
// table_data is the host pointer; table_size must be a power of 2.
static Value emit_table_osc(Value freq, const char *table_name,
                            void *table_data, int32_t table_size,
                            DspBuildCtx &ctx) {
  int off = ctx.state_offset;
  ctx.state_offset += 8;
  Value phase =
      ctx.b.create<PhasorOp>(ctx.loc, ctx.state_ptr, freq, ctx.spf, off)
          ->getResult(0);
  Value table_ptr = get_static_table(table_name, table_data, ctx);
  return ctx.b.create<TableLookupOp>(ctx.loc, phase, table_ptr, table_size)
      ->getResult(0);
}

static mlir::Type ylc_to_mlir_type(::Type *t, mlir::MLIRContext *ctx) {
  if (!t)
    return LLVM::LLVMVoidType::get(ctx);
  switch (t->kind) {
  case T_NUM:
    return mlir::Float64Type::get(ctx);
  case T_INT:
    return mlir::IntegerType::get(ctx, 32);
  case T_UINT64:
    return mlir::IntegerType::get(ctx, 64);
  case T_BOOL:
    return mlir::IntegerType::get(ctx, 1);
  case T_VOID:
    return LLVM::LLVMVoidType::get(ctx);
  default:
    return LLVM::LLVMPointerType::get(ctx);
  }
}

static LLVM::LLVMFunctionType ylc_fn_to_mlir(::Type *t,
                                             mlir::MLIRContext *ctx) {
  llvm::SmallVector<mlir::Type> params;
  while (t->kind == T_FN) {
    if (t->data.T_FN.from->kind != T_VOID) // skip unit args
      params.push_back(ylc_to_mlir_type(t->data.T_FN.from, ctx));
    t = t->data.T_FN.to;
  }
  return LLVM::LLVMFunctionType::get(ylc_to_mlir_type(t, ctx), params, false);
}

typedef Value (*BuiltinDSPHandler)(Ast *, DspBuildCtx &ctx,
                                   JITLangCtx *jit_ctx);

Value SinOscHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;

  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  return emit_table_osc(freq, "sin_table", (void *)sin_table, SIN_TABSIZE, ctx);
}

Value SqOscHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  return emit_table_osc(freq, "sq_table", (void *)sq_table, SQ_TABSIZE, ctx);
}

Value SawOscHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  return emit_table_osc(freq, "saw_table", (void *)saw_table, SAW_TABSIZE, ctx);
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
  // Ast *args = ast->data.AST_APPLICATION.args;
  // Value freq = build_dsp_expr(args, ctx, jit_ctx);
  // int off = ctx.state_offset;
  // ctx.state_offset += 8;
  // return ctx.b.create<ImpulseOp>(ctx.loc, ctx.state_ptr, freq, ctx.spf, off)
  //     ->getResult(0);
  //
  //
  auto &b = ctx.b;
  auto loc = ctx.loc;

  auto zero =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(0.));

  Value cmp =
      b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OEQ, phasor, zero);
  // TODO: cast boolean value as an f64
  return b.create<arith::UIToFPOp>(loc, b.getF64Type(), cmp);
}

// Like ImpulseHandler, but emits dsp.phasor_trig (trig, prev_trig).
// Returns trig (result 0). prev_trig (result 1) is available to callers that
// need it at the op level.
Value PhasorTrigHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  int off = reserve_state(ctx, 16); // phase + prev_trig (2x f64)

  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto op = b.create<PhasorTrigOp>(loc, ctx.state_ptr, freq, ctx.spf, off);
  return op->getResult(0);
}

// bufref node_symbol -> !llvm.ptr
// Registers node_symbol as a hidden inlet (wired at node-creation time) and
// hoists ylc_get_inlet_node(inputs_ptr, inlet_idx) before the sample loop,
// returning the Node* as an opaque ptr for use by BufReadHandler.
Value BufRefHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  JITSymbol *sym = lookup_id_ast(args, jit_ctx);
  if (!sym) {
    fprintf(stderr, "bufref: unresolved symbol\n");
    return {};
  }

  int hidden_idx = ctx.next_hidden_inlet++;
  ctx.buf_ref_inputs.push_back({hidden_idx, sym});

  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
  auto i32_ty = b.getI32Type();

  auto fn_ty = LLVM::LLVMFunctionType::get(ptr_ty, {ptr_ty, i32_ty}, false);
  auto fn = declare_extern(ctx.mod, b, "ylc_get_inlet_node", fn_ty);

  OpBuilder::InsertionGuard guard(b);
  b.restoreInsertionPoint(ctx.hoist_ip);
  Value idx_val =
      b.create<LLVM::ConstantOp>(loc, i32_ty, b.getI32IntegerAttr(hidden_idx));
  Value node_ptr =
      b.create<LLVM::CallOp>(loc, fn, ValueRange{ctx.inputs_ptr, idx_val})
          .getResult();
  ctx.hoist_ip = b.saveInsertionPoint();
  return node_ptr;
}

#define ON_TRIG(trig, expr)                                                    \
  Value prev_trig;                                                             \
  Value prev_ptr;                                                              \
  bool has_prev_ptr = false;                                                   \
  if (auto op = trig.getDefiningOp<PhasorTrigOp>()) {                          \
    prev_trig = op->getResult(1);                                              \
  } else {                                                                     \
    int prev_off = ctx.state_offset;                                           \
    ctx.state_offset += 8;                                                     \
    Value prev_off_val =                                                       \
        b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(prev_off));   \
    prev_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),               \
                                     ctx.state_ptr, ValueRange{prev_off_val}); \
    prev_trig = b.create<LLVM::LoadOp>(loc, f64, prev_ptr)->getResult(0);      \
    has_prev_ptr = true;                                                       \
  }                                                                            \
                                                                               \
  auto half = b.create<arith::ConstantFloatOp>(loc, f64, APFloat(0.5));        \
                                                                               \
  Value prev_lt = b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OLT,      \
                                          prev_trig, half);                    \
  Value curr_ge =                                                              \
      b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OGE, trig, half);     \
  Value rising = b.create<arith::AndIOp>(loc, prev_lt, curr_ge);               \
                                                                               \
  auto if_op = b.create<scf::IfOp>(loc, rising);                               \
  auto &then_blk = if_op.getThenRegion().front();                              \
  b.setInsertionPointToStart(&then_blk);                                       \
  expr;                                                                        \
  b.setInsertionPointAfter(if_op);                                             \
  if (has_prev_ptr) {                                                          \
    b.create<LLVM::StoreOp>(loc, trig, prev_ptr);                              \
  }

Value SahHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {

  Ast *args = ast->data.AST_APPLICATION.args;
  Value trig = build_dsp_expr(&args[1], ctx, jit_ctx);
  Value input = build_dsp_expr(&args[0], ctx, jit_ctx);
  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto f64 = b.getF64Type();
  auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
  auto i64 = b.getI64Type();

  // State: phase (f64) + prev_trig (f64)
  int val_off = ctx.state_offset;
  ctx.state_offset += 8;

  Value val_off_val =
      b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(val_off));

  Value val_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                        ctx.state_ptr, ValueRange{val_off_val});

  ON_TRIG(trig, ({ b.create<LLVM::StoreOp>(loc, input, val_ptr); }));

  return b.create<LLVM::LoadOp>(loc, f64, val_ptr);
}

Value GrainsHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;

  if (!ast_is_const(&args[0], jit_ctx) || args[0].tag != AST_INT) {
    fprintf(stderr, "Error - max_grains needs to be a constant\n");
    return {};
  }

  int max_grains = args[0].data.AST_INT.value;

  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto f64 = b.getF64Type();
  auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
  auto i64 = b.getI64Type();

  // std::vector<HoistableCallArg> hc_args;
  // auto arg = ast_to_const_arg(&args[0], &t_int);
  // hc_args.push_back(*arg);
  //
  // HoistableCall::RetKind rk = HoistableCall::I32;
  // // Allocate a state slot and record the call
  // int slot = ctx.state_offset;
  // ctx.state_offset += 8;
  // ctx.hoistable_calls.push_back({slot, "", hc_args, rk});
  //
  // OpBuilder::InsertionGuard guard(b);
  // b.restoreInsertionPoint(ctx.hoist_ip);
  // Value slot_ptr = b.create<LLVM::GEPOp>(
  //     loc, ptr_ty, b.getI8Type(), ctx.state_ptr,
  //     ValueRange{b.create<LLVM::ConstantOp>(loc, b.getI64Type(),
  //                                           b.getI64IntegerAttr(slot))});
  // mlir::Type load_ty = (rk == HoistableCall::I32) ?
  // (mlir::Type)b.getI32Type()
  //                                                 :
  //                                                 (mlir::Type)b.getF64Type();
  // Value loaded = b.create<LLVM::LoadOp>(loc, load_ty, slot_ptr);
  // ctx.hoist_ip = b.saveInsertionPoint();
  // Value max_grains = loaded;
  // Value max_grains= build_dsp_expr(&args[0], ctx, jit_ctx);

  Value buf = build_dsp_expr(&args[1], ctx, jit_ctx);
  Value rate = build_dsp_expr(&args[2], ctx, jit_ctx);
  Value pos = build_dsp_expr(&args[3], ctx, jit_ctx);
  Value width = build_dsp_expr(&args[4], ctx, jit_ctx);
  Value trig = build_dsp_expr(&args[5], ctx, jit_ctx);

  Value prev_trig;
  Value prev_ptr;
  bool has_prev_ptr = false;
  if (auto op = trig.getDefiningOp<PhasorTrigOp>()) {
    prev_trig = op->getResult(1);
  } else {
    int prev_off = reserve_state(ctx, 8);
    Value prev_off_val =
        b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(prev_off));
    prev_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(), ctx.state_ptr,
                                     ValueRange{prev_off_val});
    prev_trig = b.create<LLVM::LoadOp>(loc, f64, prev_ptr)->getResult(0);
    has_prev_ptr = true;
  }

  int active_grains_off = reserve_state(ctx, 4);
  int arrays_off = ctx.state_offset;
  ctx.state_offset += (max_grains * 8   // rates
                       + max_grains * 8 // phases
                       + max_grains * 8 // widths
                       + max_grains * 8 // remaining_secs
                       + max_grains * 8 // starts
                       + max_grains * 4 // active flags (i32)
  );

  auto i32 = b.getI32Type();
  Value arrays_off_val =
      b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(arrays_off));
  Value base_ptr = b.create<LLVM::GEPOp>(
      loc, ptr_ty, b.getI8Type(), ctx.state_ptr, ValueRange{arrays_off_val});

  auto i32_size = b.create<LLVM::ConstantOp>(
      loc, i64, b.getI64IntegerAttr(sizeof(int32_t) * max_grains));
  auto f64_size = b.create<LLVM::ConstantOp>(
      loc, i64, b.getI64IntegerAttr(sizeof(double) * max_grains));

  Value rates_ptr = base_ptr;
  Value phases_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                           rates_ptr, ValueRange{f64_size});
  Value widths_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                           phases_ptr, ValueRange{f64_size});
  Value remaining_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                              widths_ptr, ValueRange{f64_size});
  Value starts_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                           remaining_ptr, ValueRange{f64_size});
  Value active_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                           starts_ptr, ValueRange{f64_size});

  Value active_off_val = b.create<LLVM::ConstantOp>(
      loc, i64, b.getI64IntegerAttr(active_grains_off));
  Value active_grains_ptr = b.create<LLVM::GEPOp>(
      loc, ptr_ty, b.getI8Type(), ctx.state_ptr, ValueRange{active_off_val});

  auto fn_ty = LLVM::LLVMFunctionType::get(
      f64,
      {ptr_ty, i64, f64, f64, f64, f64, f64, f64, i32, ptr_ty, ptr_ty, ptr_ty,
       ptr_ty, ptr_ty, ptr_ty, ptr_ty},
      false);
  auto fn = declare_extern(ctx.mod, b, "grain_samp", fn_ty);

  auto bufsize_ty = LLVM::LLVMFunctionType::get(i64, {ptr_ty}, false);
  auto bufsize_fn = declare_extern(ctx.mod, b, "ylc_bufsize", bufsize_ty);
  Value buf_size_i64 =
      b.create<LLVM::CallOp>(loc, bufsize_fn, ValueRange{buf}).getResult();

  auto get_buf_ty = LLVM::LLVMFunctionType::get(ptr_ty, {ptr_ty}, false);
  auto get_buf_fn =
      declare_extern(ctx.mod, b, "ylc_get_output_buf", get_buf_ty);
  Value buf_ptr =
      b.create<LLVM::CallOp>(loc, get_buf_fn, ValueRange{buf}).getResult();

  Value max_grains_val =
      b.create<LLVM::ConstantOp>(loc, i32, b.getI32IntegerAttr(max_grains));
  Value sample =
      b.create<LLVM::CallOp>(loc, fn,
                             ValueRange{buf_ptr, buf_size_i64, trig, prev_trig,
                                        pos, rate, width, ctx.spf,
                                        max_grains_val, rates_ptr, phases_ptr,
                                        widths_ptr, remaining_ptr, starts_ptr,
                                        active_ptr, active_grains_ptr})
          .getResult();

  if (has_prev_ptr) {
    b.create<LLVM::StoreOp>(loc, trig, prev_ptr);
  }

  return sample;
}

Value BufPlayHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {

  Ast *args = ast->data.AST_APPLICATION.args;
  Value node_ptr = build_dsp_expr(&args[0], ctx, jit_ctx);
  Value rate = build_dsp_expr(&args[1], ctx, jit_ctx);
  Value start_pos = build_dsp_expr(&args[2], ctx, jit_ctx);
  Value trig = build_dsp_expr(&args[3], ctx, jit_ctx);
  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto f64 = b.getF64Type();
  auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
  auto i64 = b.getI64Type();

  // State: phase (f64) + prev_trig (f64)
  int phase_off = ctx.state_offset;
  ctx.state_offset += 8;

  Value phase_off_val =
      b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(phase_off));

  Value phase_ptr = b.create<LLVM::GEPOp>(
      loc, ptr_ty, b.getI8Type(), ctx.state_ptr, ValueRange{phase_off_val});

  ON_TRIG(trig, ({ b.create<LLVM::StoreOp>(loc, start_pos, phase_ptr); }));

  // Load phase after possible reset.
  Value phase = b.create<LLVM::LoadOp>(loc, f64, phase_ptr)->getResult(0);

  // Wrap phase into [0, 1)
  auto one = b.create<arith::ConstantFloatOp>(loc, f64, APFloat(1.0));
  Value phase_mod = b.create<arith::RemFOp>(loc, phase, one);
  Value phase_lt0 = b.create<arith::CmpFOp>(
      loc, arith::CmpFPredicate::OLT, phase_mod,
      b.create<arith::ConstantFloatOp>(loc, f64, APFloat(0.0)));
  Value phase_wrapped = b.create<arith::SelectOp>(
      loc, phase_lt0, b.create<arith::AddFOp>(loc, phase_mod, one), phase_mod);

  // Sample using bufread(node_ptr, phase)
  auto bufread_ty = LLVM::LLVMFunctionType::get(f64, {ptr_ty, f64}, false);
  auto bufread_fn = declare_extern(ctx.mod, b, "ylc_bufread", bufread_ty);
  Value sample = b.create<LLVM::CallOp>(loc, bufread_fn,
                                        ValueRange{node_ptr, phase_wrapped})
                     .getResult();

  // Compute phase increment: rate / buf_size
  auto bufsize_ty = LLVM::LLVMFunctionType::get(i64, {ptr_ty}, false);
  auto bufsize_fn = declare_extern(ctx.mod, b, "ylc_bufsize", bufsize_ty);
  Value bufsize_i64 =
      b.create<LLVM::CallOp>(loc, bufsize_fn, ValueRange{node_ptr}).getResult();

  Value zero_i64 = b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(0));
  Value size_ok = b.create<arith::CmpIOp>(loc, arith::CmpIPredicate::sgt,
                                          bufsize_i64, zero_i64);
  Value bufsize_f64 = b.create<arith::SIToFPOp>(loc, f64, bufsize_i64);
  Value bufsize_safe =
      b.create<arith::SelectOp>(loc, size_ok, bufsize_f64, one);
  Value inc = b.create<arith::DivFOp>(loc, rate, bufsize_safe);

  Value next_phase = b.create<arith::AddFOp>(loc, phase, inc);
  Value next_mod = b.create<arith::RemFOp>(loc, next_phase, one);
  Value next_lt0 = b.create<arith::CmpFOp>(
      loc, arith::CmpFPredicate::OLT, next_mod,
      b.create<arith::ConstantFloatOp>(loc, f64, APFloat(0.0)));
  Value next_wrapped = b.create<arith::SelectOp>(
      loc, next_lt0, b.create<arith::AddFOp>(loc, next_mod, one), next_mod);

  b.create<LLVM::StoreOp>(loc, next_wrapped, phase_ptr);

  return sample;
}

// bufread node_ptr phase -> f64
// node_ptr is the !llvm.ptr returned by BufRefHandler (a Node*).
// Calls ylc_bufread(node_ptr, phase) which does lerp from node->output.buf.
Value BufReadHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value node_ptr = build_dsp_expr(&args[0], ctx, jit_ctx);
  Value phase = build_dsp_expr(&args[1], ctx, jit_ctx);
  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto f64 = b.getF64Type();
  auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
  auto fn_ty = LLVM::LLVMFunctionType::get(f64, {ptr_ty, f64}, false);
  auto fn = declare_extern(ctx.mod, b, "ylc_bufread", fn_ty);
  return b.create<LLVM::CallOp>(loc, fn, ValueRange{node_ptr, phase})
      .getResult();
}

Value LinscaleHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {

  Ast *args = ast->data.AST_APPLICATION.args;
  Value domain_a = build_dsp_expr(args, ctx, jit_ctx);
  Value domain_b = build_dsp_expr(args + 1, ctx, jit_ctx);
  Value range_a = build_dsp_expr(args + 2, ctx, jit_ctx);
  Value range_b = build_dsp_expr(args + 3, ctx, jit_ctx);
  Value input = build_dsp_expr(args + 4, ctx, jit_ctx);
  // Ast *args = ast->data.AST_APPLICATION.args;
  // Value freq = build_dsp_expr(args, ctx, jit_ctx);
  // int off = ctx.state_offset;
  // ctx.state_offset += 8;
  auto &b = ctx.b;
  auto loc = ctx.loc;
  return ctx.b
      .create<LinscaleOp>(ctx.loc, domain_a, domain_b, range_a, range_b, input)
      ->getResult(0);
}

Value BipolarLinscaleHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {

  auto &b = ctx.b;
  auto loc = ctx.loc;
  Ast *args = ast->data.AST_APPLICATION.args;
  Value domain_a =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(-1.));

  Value domain_b =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(1.));
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
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(0.));

  Value domain_b =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(1.));

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
  // t = shifted mod double_range  (same wrap formula)
  Value floored =
      b.create<LLVM::CallOp>(
           loc, floor_fn,
           ValueRange{b.create<arith::DivFOp>(loc, shifted, dbl, fmf)})
          .getResult();
  Value t = b.create<arith::SubFOp>(
      loc, shifted, b.create<arith::MulFOp>(loc, dbl, floored, fmf), fmf);
  // result = limit_a + range - |t - range|
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

  // floor intrinsic: no libm call, LLVM lowers to a single instruction
  auto f64 = b.getF64Type();
  auto floor_fn =
      declare_extern(ctx.mod, b, "llvm.floor.f64",
                     LLVM::LLVMFunctionType::get(f64, {f64}, false));

  auto fmf =
      arith::FastMathFlagsAttr::get(b.getContext(), arith::FastMathFlags::fast);
  // result = input - range * floor((input - limit_a) / range)
  Value range = b.create<arith::SubFOp>(loc, limit_b, limit_a, fmf);
  Value shifted = b.create<arith::SubFOp>(loc, input, limit_a, fmf);
  Value t = b.create<LLVM::CallOp>(
                 loc, floor_fn,
                 ValueRange{b.create<arith::DivFOp>(loc, shifted, range, fmf)})
                .getResult();
  return b.create<arith::SubFOp>(
      loc, input, b.create<arith::MulFOp>(loc, range, t, fmf), fmf);
}

// Extract a compile-time constant from a numeric AST node.
static double ast_const_double(Ast *a) {
  if (!a) {
    return 0.0;
  }
  if (a->tag == AST_DOUBLE) {
    return a->data.AST_DOUBLE.value;
  }
  if (a->tag == AST_FLOAT) {
    return (double)a->data.AST_FLOAT.value;
  }
  if (a->tag == AST_INT) {
    return (double)a->data.AST_INT.value;
  }
  return 0.0;
}

// delay delay_time max_delay fb input → f64
// Creates a hidden inlet holding the circular delay buffer.
// State: [write_pos: i32] (4 bytes used, 8 allocated for alignment).
// delay_time is a per-sample f64 value (may vary within the loop).
// read_pos is derived at runtime as (write_pos - delay_samps + buf_sz) %
// buf_sz.
static Value DelayHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Ast *fn = ast->data.AST_APPLICATION.function;

  double max_delay = ast_const_double(&args[1]);
  Value delay_time = build_dsp_expr(&args[0], ctx, jit_ctx);
  Value fb = build_dsp_expr(&args[2], ctx, jit_ctx);
  Value input = build_dsp_expr(&args[3], ctx, jit_ctx);

  if (strcmp(fn->data.AST_IDENTIFIER.value, "delay1") == 0) {
    int off = ctx.state_offset;
    ctx.state_offset +=
        8; // 8 bytes for alignment; only write_pos (i32) is used
    int hidden_idx = ctx.next_hidden_inlet++;
    ctx.buf_inputs.push_back({hidden_idx, max_delay});

    return ctx.b
        .create<Delay1Op>(ctx.loc, ctx.state_ptr, ctx.inputs_ptr, input, fb,
                          ctx.spf, delay_time, off, hidden_idx)
        ->getResult(0);
  }

  int off = ctx.state_offset;
  ctx.state_offset += 8; // 8 bytes for alignment; only write_pos (i32) is used
  int hidden_idx = ctx.next_hidden_inlet++;
  ctx.buf_inputs.push_back({hidden_idx, max_delay});

  return ctx.b
      .create<DelayOp>(ctx.loc, ctx.state_ptr, ctx.inputs_ptr, input, fb,
                       ctx.spf, delay_time, off, hidden_idx)
      ->getResult(0);
}

// allpass delay_time max_delay g input → f64
// Schroeder allpass: w = input + g*delayed; out = delayed - g*input.
static Value AllpassHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  double max_delay = ast_const_double(&args[1]);
  Value delay_time = build_dsp_expr(&args[0], ctx, jit_ctx);
  Value g = build_dsp_expr(&args[2], ctx, jit_ctx);
  Value input = build_dsp_expr(&args[3], ctx, jit_ctx);

  int off = ctx.state_offset;
  ctx.state_offset += 8; // 8 bytes for alignment; only write_pos (i32) is used
  int hidden_idx = ctx.next_hidden_inlet++;
  ctx.buf_inputs.push_back({hidden_idx, max_delay});

  return ctx.b
      .create<AllpassOp>(ctx.loc, ctx.state_ptr, ctx.inputs_ptr, input, g,
                         ctx.spf, delay_time, off, hidden_idx)
      ->getResult(0);
}

static Value Allpass1Handler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  double max_delay = ast_const_double(&args[1]);
  Value delay_time = build_dsp_expr(&args[0], ctx, jit_ctx);
  Value g = build_dsp_expr(&args[2], ctx, jit_ctx);
  Value input = build_dsp_expr(&args[3], ctx, jit_ctx);

  int off = ctx.state_offset;
  ctx.state_offset += 8; // 8 bytes for alignment; only write_pos (i32) is used
  int hidden_idx = ctx.next_hidden_inlet++;
  ctx.buf_inputs.push_back({hidden_idx, max_delay});

  return ctx.b
      .create<Allpass1Op>(ctx.loc, ctx.state_ptr, ctx.inputs_ptr, input, g,
                          ctx.spf, delay_time, off, hidden_idx)
      ->getResult(0);
}

static Value WhiteNoiseHandler(Ast *ast, DspBuildCtx &ctx,
                               JITLangCtx *jit_ctx) {

  return ctx.b.create<WhiteNoiseOp>(ctx.loc)->getResult(0);
}

// uniformly distributed double between min and max
extern "C" double ylc_rand_double_range(double min, double max) {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * (max - min) + min;
  // printf("rand double range %f %f -> %f\n", min, max, rand_double);
  return rand_double;
}
static Value LFNoiseHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *fn = ast->data.AST_APPLICATION.function;

  Ast *args = ast->data.AST_APPLICATION.args;
  Value range_a = build_dsp_expr(&args[0], ctx, jit_ctx);
  Value range_b = build_dsp_expr(&args[1], ctx, jit_ctx);

  Value freq = build_dsp_expr(&args[2], ctx, jit_ctx);
  int off = ctx.state_offset;
  ctx.state_offset += 8;

  auto &b = ctx.b;
  auto loc = ctx.loc;

  auto phasor =
      b.create<PhasorOp>(loc, ctx.state_ptr, freq, ctx.spf, off)->getResult(0);

  auto zero =
      b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(0.));

  Value cmp =
      b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OEQ, phasor, zero);

  if (strcmp(fn->data.AST_IDENTIFIER.value, "lfnoise0") == 0) {
    off = ctx.state_offset;

    ctx.state_offset += 8;
    auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
    auto i64_ty = b.getI64Type();
    auto f64_ty = b.getF64Type();

    Value off_val =
        b.create<LLVM::ConstantOp>(loc, i64_ty, b.getI64IntegerAttr(off));
    Value slot_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                           ctx.state_ptr, ValueRange{off_val});

    auto if_op = b.create<scf::IfOp>(loc, cmp);
    auto &then_blk = if_op.getThenRegion().front();
    b.setInsertionPointToStart(&then_blk);
    auto fn_ty = LLVM::LLVMFunctionType::get(f64_ty, {f64_ty, f64_ty}, false);
    auto fn_op = declare_extern(ctx.mod, b, "rand_double_range", fn_ty);
    Value new_val =
        b.create<LLVM::CallOp>(loc, fn_op, ValueRange{range_a, range_b})
            .getResult();
    b.create<LLVM::StoreOp>(loc, new_val, slot_ptr);
    // b.create<scf::YieldOp>(loc);

    b.setInsertionPointAfter(if_op);
    return b.create<LLVM::LoadOp>(loc, f64_ty, slot_ptr);
  }

  if (strcmp(fn->data.AST_IDENTIFIER.value, "lfnoise1") == 0) {
    int prev_off = ctx.state_offset;
    ctx.state_offset += 8;
    int next_off = ctx.state_offset;
    ctx.state_offset += 8;

    auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
    auto i64_ty = b.getI64Type();
    auto f64_ty = b.getF64Type();

    Value prev_off_val =
        b.create<LLVM::ConstantOp>(loc, i64_ty, b.getI64IntegerAttr(prev_off));
    Value next_off_val =
        b.create<LLVM::ConstantOp>(loc, i64_ty, b.getI64IntegerAttr(next_off));
    Value prev_ptr = b.create<LLVM::GEPOp>(
        loc, ptr_ty, b.getI8Type(), ctx.state_ptr, ValueRange{prev_off_val});
    Value next_ptr = b.create<LLVM::GEPOp>(
        loc, ptr_ty, b.getI8Type(), ctx.state_ptr, ValueRange{next_off_val});

    auto if_op = b.create<scf::IfOp>(loc, cmp);
    auto &then_blk = if_op.getThenRegion().front();
    b.setInsertionPointToStart(&then_blk);

    Value next_val = b.create<LLVM::LoadOp>(loc, f64_ty, next_ptr);
    b.create<LLVM::StoreOp>(loc, next_val, prev_ptr);

    auto fn_ty = LLVM::LLVMFunctionType::get(f64_ty, {f64_ty, f64_ty}, false);
    auto fn_op = declare_extern(ctx.mod, b, "rand_double_range", fn_ty);
    Value new_val =
        b.create<LLVM::CallOp>(loc, fn_op, ValueRange{range_a, range_b})
            .getResult();
    b.create<LLVM::StoreOp>(loc, new_val, next_ptr);

    b.setInsertionPointAfter(if_op);

    Value prev_val =
        b.create<LLVM::LoadOp>(loc, f64_ty, prev_ptr)->getResult(0);
    Value next_val2 =
        b.create<LLVM::LoadOp>(loc, f64_ty, next_ptr)->getResult(0);
    Value delta =
        b.create<arith::SubFOp>(loc, next_val2, prev_val)->getResult(0);
    Value scaled = b.create<arith::MulFOp>(loc, phasor, delta)->getResult(0);
    Value out = b.create<arith::AddFOp>(loc, prev_val, scaled)->getResult(0);
    return out;
  }

  return ctx.b.create<WhiteNoiseOp>(ctx.loc)->getResult(0);
}

static Value SpfHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  return ctx.spf;
}

static Value ClampHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  auto &b = ctx.b;
  auto loc = ctx.loc;
  auto clamp_threshold = build_dsp_expr(&args[0], ctx, jit_ctx);
  auto val = build_dsp_expr(&args[1], ctx, jit_ctx);

  if (!clamp_threshold || !val) {
    fprintf(stderr, "Error could not compute clamp operator\n");
    return {};
  }

  Value cmp = b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OGT, val,
                                      clamp_threshold)
                  ->getResult(0);
  return b.create<arith::SelectOp>(loc, cmp, clamp_threshold, val)
      ->getResult(0);
}

static Value TrigFnHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  print_ast(ast);
  return {};
}

static Value TrigChooseHandler(Ast *ast, DspBuildCtx &ctx,
                               JITLangCtx *jit_ctx) {

  Ast *args = ast->data.AST_APPLICATION.args;
  Value arr = build_dsp_expr(&args[0], ctx, jit_ctx);

  auto &b = ctx.b;
  auto loc = ctx.loc;

  auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
  auto i64 = b.getI64Type();
  auto f64 = b.getF64Type();

  int out_val_off = ctx.state_offset;
  ctx.state_offset += 8;
  Value out_val_off_val =
      b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(out_val_off));
  Value out_val_ptr = b.create<LLVM::GEPOp>(
      loc, ptr_ty, b.getI8Type(), ctx.state_ptr, ValueRange{out_val_off_val});

  Value trig_sig = build_dsp_expr(&args[1], ctx, jit_ctx);
  ON_TRIG(trig_sig, ({
            int len = -1;
            if (args[0].tag == AST_IDENTIFIER) {
              std::string name(args[0].data.AST_IDENTIFIER.value,
                               args[0].data.AST_IDENTIFIER.length);
              auto it = ctx.array_states.find(name);
              if (it != ctx.array_states.end()) {
                len = it->second.len;
              }
            } else if (args[0].tag == AST_ARRAY) {
              len = (int)args[0].data.AST_LIST.len;
            }
            if (len <= 0) {
              fprintf(stderr, "trig_choose: could not resolve array length\n");
              print_ast(ast);
              return {};
            }

            // rand() % len
            auto rand_ty =
                LLVM::LLVMFunctionType::get(b.getI32Type(), {}, false);
            auto rand_fn = declare_extern(ctx.mod, b, "rand", rand_ty);
            Value r =
                b.create<LLVM::CallOp>(loc, rand_fn, ValueRange{}).getResult();

            Value len_i32 = b.create<LLVM::ConstantOp>(
                loc, b.getI32Type(), b.getI32IntegerAttr(len));
            Value idx_i32 = b.create<LLVM::URemOp>(loc, r, len_i32);

            Value idx_i64 = b.create<LLVM::ZExtOp>(loc, i64, idx_i32);
            Value stride =
                b.create<LLVM::ConstantOp>(loc, i64, b.getI64IntegerAttr(8));
            Value idx_bytes = b.create<LLVM::MulOp>(loc, idx_i64, stride);
            Value elem_off = b.create<LLVM::AddOp>(loc, arr, idx_bytes);

            Value elem_ptr =
                b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(), ctx.state_ptr,
                                      ValueRange{elem_off});
            Value new_val = b.create<LLVM::LoadOp>(loc, f64, elem_ptr);

            b.create<LLVM::StoreOp>(loc, new_val, out_val_ptr);
          }));

  // Value prev_trig;
  // Value prev_ptr;
  // bool has_prev_ptr = false;
  // if (auto op = trig_sig.getDefiningOp<PhasorTrigOp>()) {
  //   prev_trig = op->getResult(1);
  // } else {
  //   auto off = ctx.state_offset;
  //   ctx.state_offset += 8;
  //   Value off_val =
  //       b.create<LLVM::ConstantOp>(loc, i64_ty, b.getI64IntegerAttr(off));
  //   prev_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
  //   ctx.state_ptr,
  //                                    ValueRange{off_val});
  //   prev_trig = b.create<LLVM::LoadOp>(loc, f64_ty, prev_ptr)->getResult(0);
  //   has_prev_ptr = true;
  // }
  // auto half =
  //     b.create<arith::ConstantFloatOp>(loc, b.getF64Type(), APFloat(0.5));
  //
  // Value prev_lt =
  //     b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OLT, prev_trig,
  //     half);
  // Value curr_ge =
  //     b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OGE, trig_sig,
  //     half);
  //
  // Value cmp = b.create<arith::AndIOp>(loc, prev_lt, curr_ge);
  //
  // auto if_op = b.create<scf::IfOp>(loc, cmp);
  // auto &then_blk = if_op.getThenRegion().front();
  // b.setInsertionPointToStart(&then_blk);
  //
  // int len = -1;
  // if (args[0].tag == AST_IDENTIFIER) {
  //   std::string name(args[0].data.AST_IDENTIFIER.value,
  //                    args[0].data.AST_IDENTIFIER.length);
  //   auto it = ctx.array_states.find(name);
  //   if (it != ctx.array_states.end()) {
  //     len = it->second.len;
  //   }
  // } else if (args[0].tag == AST_ARRAY) {
  //   len = (int)args[0].data.AST_LIST.len;
  // }
  // if (len <= 0) {
  //   fprintf(stderr, "trig_choose: could not resolve array length\n");
  //   print_ast(ast);
  //   return {};
  // }
  //
  // // rand() % len
  // auto rand_ty = LLVM::LLVMFunctionType::get(b.getI32Type(), {}, false);
  // auto rand_fn = declare_extern(ctx.mod, b, "rand", rand_ty);
  // Value r = b.create<LLVM::CallOp>(loc, rand_fn, ValueRange{}).getResult();
  //
  // Value len_i32 =
  //     b.create<LLVM::ConstantOp>(loc, b.getI32Type(),
  //     b.getI32IntegerAttr(len));
  // Value idx_i32 = b.create<LLVM::URemOp>(loc, r, len_i32);
  //
  // Value idx_i64 = b.create<LLVM::ZExtOp>(loc, i64_ty, idx_i32);
  // Value stride =
  //     b.create<LLVM::ConstantOp>(loc, i64_ty, b.getI64IntegerAttr(8));
  // Value idx_bytes = b.create<LLVM::MulOp>(loc, idx_i64, stride);
  // Value elem_off = b.create<LLVM::AddOp>(loc, arr, idx_bytes);
  //
  // Value elem_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
  //                                        ctx.state_ptr,
  //                                        ValueRange{elem_off});
  // Value new_val = b.create<LLVM::LoadOp>(loc, f64_ty, elem_ptr);
  //
  // b.create<LLVM::StoreOp>(loc, new_val, out_val_ptr);
  //
  // b.setInsertionPointAfter(if_op);
  // if (has_prev_ptr) {
  //   b.create<LLVM::StoreOp>(loc, trig_sig, prev_ptr);
  // / }
  return b.create<LLVM::LoadOp>(loc, f64, out_val_ptr);
}

struct InlinedCtx {
  DspBuildCtx child;
  DspBuildCtx &parent;

  explicit InlinedCtx(DspBuildCtx &parent)
      : parent(parent), child{.b = parent.b,
                              .mod = parent.mod,
                              .loc = parent.loc,
                              .node_ptr = parent.node_ptr,
                              .state_ptr = parent.state_ptr,
                              .inputs_ptr = parent.inputs_ptr,
                              .spf = parent.spf,
                              .frame_idx = parent.frame_idx,
                              .state_offset = parent.state_offset,
                              .next_hidden_inlet = parent.next_hidden_inlet,
                              .hoist_ip = parent.hoist_ip,
                              .array_states = parent.array_states,
                              .array_inits = parent.array_inits} {}

  ~InlinedCtx() {
    parent.b = child.b;
    parent.hoist_ip = child.hoist_ip;
    parent.array_states = std::move(child.array_states);
    parent.array_inits = std::move(child.array_inits);
    parent.state_offset = child.state_offset;
    parent.next_hidden_inlet = child.next_hidden_inlet;
    parent.buf_inputs.insert(parent.buf_inputs.end(), child.buf_inputs.begin(),
                             child.buf_inputs.end());
    parent.hoistable_calls.insert(parent.hoistable_calls.end(),
                                  child.hoistable_calls.begin(),
                                  child.hoistable_calls.end());
    parent.buf_ref_inputs.insert(parent.buf_ref_inputs.end(),
                                 child.buf_ref_inputs.begin(),
                                 child.buf_ref_inputs.end());
  }
};
static Value inline_dsp_subexpr(JITSymbol *f, Ast *args, DspBuildCtx &ctx,
                                JITLangCtx *jit_ctx) {

  Ast *fn_ast = (Ast *)f->symbol_data._USER_DEFINED_SYMBOL;

  // Evaluate args first so any phasors inside them advance ctx.state_offset
  // before we snapshot it into the child context.
  llvm::SmallVector<std::pair<std::string, Value>> arg_vals;
  int inlet_idx = 0;
  for (AstList *p = fn_ast->data.AST_LAMBDA.params; p;
       p = p->next, inlet_idx++) {
    Ast *param = p->ast;
    if (param->tag != AST_IDENTIFIER)
      continue;
    std::string pname(param->data.AST_IDENTIFIER.value,
                      param->data.AST_IDENTIFIER.length);
    arg_vals.push_back(
        {std::move(pname), build_dsp_expr(args + inlet_idx, ctx, jit_ctx)});
  }

  // Create InlinedCtx *after* args are evaluated so state_offset is current.
  InlinedCtx inlined(ctx);
  for (auto &[name, val] : arg_vals)
    inlined.child.locals[name] = val;

  auto res =
      build_dsp_expr(fn_ast->data.AST_LAMBDA.body, inlined.child, jit_ctx);
  // res.dump();
  return res;
}

const char *can_parse_list_expr(Ast *fn) {

  const char *fn_name;
  if (fn->tag == AST_IDENTIFIER) {
    fn_name = fn->data.AST_IDENTIFIER.value;
    return fn_name;

  } else if (fn->tag == AST_RECORD_ACCESS &&
             fn->data.AST_RECORD_ACCESS.record->tag == AST_IDENTIFIER &&
             (strcmp(
                  fn->data.AST_RECORD_ACCESS.record->data.AST_IDENTIFIER.value,
                  "Arrays") == 0 ||
              strcmp(
                  fn->data.AST_RECORD_ACCESS.record->data.AST_IDENTIFIER.value,
                  "Lists") == 0)) {
    fn_name = fn->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;
    return fn_name;
  }

  return nullptr;
}

Value list_expr_handlers(Ast *ast, const char *fn_name, DspBuildCtx &ctx,
                         JITLangCtx *jit_ctx) {
  if (strcmp(fn_name, "fold") == 0) {
    Ast *fold_fn = ast->data.AST_APPLICATION.args;      // fn acc el -> body
    Ast *init_ast = ast->data.AST_APPLICATION.args + 1; // initial accumulator
    Ast *list_ast = ast->data.AST_APPLICATION.args + 2; // list literal

    if (list_ast->tag != AST_LIST) {
      fprintf(stderr,
              "Error: Lists.fold in audio compiler requires a list literal\n");
      return {};
    }
    if (fold_fn->tag != AST_LAMBDA) {
      fprintf(
          stderr,
          "Error: Lists.fold in audio compiler requires an inline lambda\n");
      return {};
    }

    // Extract the two parameter names (acc, element) from the lambda.
    AstList *params = fold_fn->data.AST_LAMBDA.params;
    if (!params || !params->next) {
      fprintf(stderr,
              "Error: fold lambda must have exactly 2 params (acc, element)\n");
      return {};
    }
    std::string acc_name(params->ast->data.AST_IDENTIFIER.value,
                         params->ast->data.AST_IDENTIFIER.length);
    std::string el_name(params->next->ast->data.AST_IDENTIFIER.value,
                        params->next->ast->data.AST_IDENTIFIER.length);

    // Evaluate the initial accumulator in the caller's context.
    Value acc = build_dsp_expr(init_ast, ctx, jit_ctx);

    // For each list element, inline the fold body with acc and el bound.
    // InlinedCtx snapshots state_offset/buf_inputs from ctx and propagates
    // them back on destruction, so stateful ops (allpass, delay…) in the
    // body get unique state slots across iterations.
    Ast *items = list_ast->data.AST_LIST.items;
    size_t len = list_ast->data.AST_LIST.len;
    for (size_t i = 0; i < len; i++) {
      Value el = build_dsp_expr(items + i, ctx, jit_ctx);
      {
        InlinedCtx inlined(ctx);
        inlined.child.locals = ctx.locals; // inherit outer scope for closures
        inlined.child.locals[acc_name] = acc;
        inlined.child.locals[el_name] = el;
        acc = build_dsp_expr(fold_fn->data.AST_LAMBDA.body, inlined.child,
                             jit_ctx);
      } // destructor writes state_offset and buf_inputs back to ctx
    }

    return acc;
  }

  // Lists.map (fn el -> body) list — compile-time unroll, side-effects only
  // (use Lists.sum to accumulate results).
  if (strcmp(fn_name, "map") == 0) {
    Ast *map_fn = ast->data.AST_APPLICATION.args;
    Ast *list_ast = ast->data.AST_APPLICATION.args + 1;

    if (map_fn->tag != AST_LAMBDA || list_ast->tag != AST_LIST) {
      fprintf(
          stderr,
          "Error: Lists.map requires an inline lambda and a list literal\n");
      return {};
    }
    AstList *params = map_fn->data.AST_LAMBDA.params;
    if (!params)
      return {};
    std::string el_name(params->ast->data.AST_IDENTIFIER.value,
                        params->ast->data.AST_IDENTIFIER.length);

    Ast *items = list_ast->data.AST_LIST.items;
    size_t len = list_ast->data.AST_LIST.len;
    Value last = {};
    for (size_t i = 0; i < len; i++) {
      Value el = build_dsp_expr(items + i, ctx, jit_ctx);
      {
        InlinedCtx inlined(ctx);
        inlined.child.locals = ctx.locals;
        inlined.child.locals[el_name] = el;
        last = build_dsp_expr(map_fn->data.AST_LAMBDA.body, inlined.child,
                              jit_ctx);
      }
    }
    return last;
  }

  // Lists.sum list — where list is a literal or a Lists.map call
  if (strcmp(fn_name, "sum") == 0) {
    Ast *list_arg = ast->data.AST_APPLICATION.args;

    // Unwrap Lists.map f list piped into sum
    if (list_arg->tag == AST_APPLICATION) {
      const char *inner =
          can_parse_list_expr(list_arg->data.AST_APPLICATION.function);
      if (inner && strcmp(inner, "map") == 0) {
        Ast *map_fn = list_arg->data.AST_APPLICATION.args;
        Ast *list_ast = list_arg->data.AST_APPLICATION.args + 1;
        if (map_fn->tag != AST_LAMBDA || list_ast->tag != AST_LIST) {
          fprintf(stderr, "Error: Lists.sum(Lists.map ...) requires inline "
                          "lambda and list literal\n");
          return {};
        }
        AstList *params = map_fn->data.AST_LAMBDA.params;
        if (!params)
          return {};
        std::string el_name(params->ast->data.AST_IDENTIFIER.value,
                            params->ast->data.AST_IDENTIFIER.length);
        Ast *items = list_ast->data.AST_LIST.items;
        size_t len = list_ast->data.AST_LIST.len;
        Value acc = {};
        for (size_t i = 0; i < len; i++) {
          Value el = build_dsp_expr(items + i, ctx, jit_ctx);
          Value mapped;
          {
            InlinedCtx inlined(ctx);
            inlined.child.locals = ctx.locals;
            inlined.child.locals[el_name] = el;
            mapped = build_dsp_expr(map_fn->data.AST_LAMBDA.body, inlined.child,
                                    jit_ctx);
          }
          acc =
              acc ? ctx.b.create<arith::AddFOp>(ctx.loc, acc, mapped) : mapped;
        }
        return acc;
      }
    }

    // Plain literal list
    if (list_arg->tag == AST_LIST) {
      Ast *items = list_arg->data.AST_LIST.items;
      size_t len = list_arg->data.AST_LIST.len;
      if (len == 0)
        return {};
      Value acc = build_dsp_expr(items, ctx, jit_ctx);
      for (size_t i = 1; i < len; i++)
        acc = ctx.b.create<arith::AddFOp>(
            ctx.loc, acc, build_dsp_expr(items + i, ctx, jit_ctx));
      return acc;
    }

    fprintf(stderr,
            "Error: Lists.sum requires a list literal or Lists.map result\n");
    return {};
  }

  return {};
}

static Value build_dsp_expr(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  if (!ast)
    return {};
  auto &b = ctx.b;
  auto loc = ctx.loc;

  switch (ast->tag) {

  case AST_DOUBLE:
    return b.create<arith::ConstantFloatOp>(
        loc, b.getF64Type(), APFloat(ast->data.AST_DOUBLE.value));
  case AST_FLOAT:
    return b.create<arith::ConstantFloatOp>(
        loc, b.getF64Type(), APFloat((double)ast->data.AST_FLOAT.value));
  case AST_INT:
    return b.create<arith::ConstantFloatOp>(
        loc, b.getF64Type(), APFloat((double)ast->data.AST_INT.value));

  case AST_IDENTIFIER: {
    std::string name(ast->data.AST_IDENTIFIER.value,
                     ast->data.AST_IDENTIFIER.length);
    auto it = ctx.locals.find(name);
    if (it != ctx.locals.end()) {
      return it->second;
    }

    JITSymbol *sym;
    if ((sym = lookup_id_ast(ast, jit_ctx))) {
      fprintf(stderr, "audio_jit: found '%s'\n", name.c_str());
      return {};
    }
    fprintf(stderr, "audio_jit: unresolved '%s'\n", name.c_str());
    return {};
  }
  case AST_RECORD_ACCESS: {

    JITSymbol *sym;
    if ((sym = lookup_id_ast(ast, jit_ctx))) {
      if (ast->type && ast->type->kind == T_NUM) {
        double v = 0.0;
        if (sym->val && LLVMIsAConstantFP(sym->val)) {
          LLVMBool loses = 0;
          v = LLVMConstRealGetDouble(sym->val, &loses);
        } else {
          fprintf(stderr, "Error: record access numeric constant not found\n");
          print_ast_err(ast);
          return {};
        }
        return b.create<arith::ConstantFloatOp>(loc, b.getF64Type(),
                                                APFloat(v));
      }
      return {};
    }
    return {};
  }

  case AST_LET: {
    // Hoist array literals into state storage with literal initialization.
    if (ast->data.AST_LET.binding &&
        ast->data.AST_LET.binding->tag == AST_IDENTIFIER &&
        ast->data.AST_LET.expr && ast->data.AST_LET.expr->tag == AST_ARRAY) {
      Ast *arr = ast->data.AST_LET.expr;
      std::string n(ast->data.AST_LET.binding->data.AST_IDENTIFIER.value,
                    ast->data.AST_LET.binding->data.AST_IDENTIFIER.length);

      int len = (int)arr->data.AST_LIST.len;
      int base = ctx.state_offset;
      ctx.state_offset += 8 * len;
      ctx.array_states[n] = ArrayStateSpec{base, len};

      // Emit the array value as its base offset (i64).
      Value base_val = emit_array_state(arr, ctx, n);
      ctx.locals[n] = base_val;

      if (ast->data.AST_LET.in_expr)
        return build_dsp_expr(ast->data.AST_LET.in_expr, ctx, jit_ctx);
      return base_val;
    }

    Value val = build_dsp_expr(ast->data.AST_LET.expr, ctx, jit_ctx);
    if (ast->data.AST_LET.binding &&
        ast->data.AST_LET.binding->tag == AST_IDENTIFIER) {
      std::string n(ast->data.AST_LET.binding->data.AST_IDENTIFIER.value,
                    ast->data.AST_LET.binding->data.AST_IDENTIFIER.length);

      if (val) {
        ctx.locals[n] = val;
      }
    }
    if (ast->data.AST_LET.in_expr)
      return build_dsp_expr(ast->data.AST_LET.in_expr, ctx, jit_ctx);
    return val;
  }

  case AST_BODY: {
    Value last{};
    for (AstList *l = ast->data.AST_BODY.stmts; l; l = l->next)
      last = build_dsp_expr(l->ast, ctx, jit_ctx);
    return last;
  }

  case AST_APPLICATION: {
    Ast *fn = ast->data.AST_APPLICATION.function;
    Ast *args = ast->data.AST_APPLICATION.args;
    size_t nargs = ast->data.AST_APPLICATION.len;

    JITLangCtx *fn_ctx = jit_ctx;
    JITSymbol *f = nullptr;

    if (fn->tag == AST_RECORD_ACCESS) {
      Ast *rec = fn->data.AST_RECORD_ACCESS.record;
      Ast *mem = fn->data.AST_RECORD_ACCESS.member;
      JITSymbol *mod_sym = lookup_id_ast(rec, jit_ctx);
      if (mod_sym && mod_sym->type == STYPE_MODULE) {
        fn_ctx = mod_sym->symbol_data.STYPE_MODULE.ctx;
        f = lookup_id_ast(mem, fn_ctx);
      } else {
        f = lookup_id_ast(fn, jit_ctx);
      }
    } else {
      f = lookup_id_ast(fn, jit_ctx);
    }

    if (!f) {
      fprintf(stderr, "Error: could not resolve symbol: ");
      print_ast_err(fn);
      return {};
    }

    if (f && f->type == STYPE_AUDIO_JIT_BUILTIN_HANDLER &&
        f->symbol_data._USER_DEFINED_SYMBOL) {
      BuiltinDSPHandler handler =
          (BuiltinDSPHandler)f->symbol_data._USER_DEFINED_SYMBOL;
      return handler(ast, ctx, jit_ctx);
    }

    if (f && f->type == STYPE_AUDIO_JIT_SYM) {
      return inline_dsp_subexpr(f, args, ctx, fn_ctx);
    }

    if (f && f->symbol_type->kind == T_FN) {
      const char *fn_name = nullptr;

      if (f->type == STYPE_GENERIC_FUNCTION) {
        const char *list_fn_name = can_parse_list_expr(fn);

        if (list_fn_name && (strcmp(list_fn_name, "fold") == 0 ||
                             strcmp(list_fn_name, "map") == 0 ||
                             strcmp(list_fn_name, "sum") == 0)) {
          return list_expr_handlers(ast, list_fn_name, ctx, jit_ctx);
        }

        fprintf(stderr, "Error: generic function instantiation in audio "
                        "compiler not yet supported\n");
        print_ast_err(ast);
        return nullptr;
      }

      if (f->type == STYPE_LAZY_EXTERN_FUNCTION) {
        fn_name = f->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast->data
                      .AST_EXTERN_FN.fn_name.chars;

      } else if (f->type == STYPE_FUNCTION) {
        fn_name = LLVMGetValueName(f->val);
      }

      // --- Hoistable call detection ---
      // If all args are compile-time constants, evaluate this call once at node
      // creation time and load the result from a state slot in the perform fn.
      if (fn_name) {
        bool all_const = true;
        for (size_t i = 0; i < nargs && all_const; i++) {
          if (!ast_is_const(&args[i], jit_ctx)) {
            all_const = false;
            break;
          }
        }

        if (all_const) {
          // Collect typed constant args
          std::vector<HoistableCallArg> hc_args;
          ::Type *pt = f->symbol_type;
          bool extract_ok = true;
          for (size_t i = 0; i < nargs && pt->kind == T_FN;
               i++, pt = pt->data.T_FN.to) {
            if (pt->data.T_FN.from->kind == T_VOID)
              continue;
            auto arg = ast_to_const_arg(&args[i], pt->data.T_FN.from);
            if (!arg) {
              extract_ok = false;
              break;
            }
            hc_args.push_back(*arg);
          }

          if (extract_ok) {
            // Determine return type
            ::Type *ret_t = f->symbol_type;
            while (ret_t->kind == T_FN)
              ret_t = ret_t->data.T_FN.to;
            HoistableCall::RetKind rk = (ret_t->kind == T_INT)
                                            ? HoistableCall::I32
                                            : HoistableCall::F64;

            // Allocate a state slot and record the call
            int slot = ctx.state_offset;
            ctx.state_offset += 8;
            ctx.hoistable_calls.push_back({slot, fn_name, hc_args, rk});

            // Emit a load from the state slot, hoisted before the scf.for loop
            auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
            OpBuilder::InsertionGuard guard(b);
            b.restoreInsertionPoint(ctx.hoist_ip);
            Value slot_ptr = b.create<LLVM::GEPOp>(
                loc, ptr_ty, b.getI8Type(), ctx.state_ptr,
                ValueRange{b.create<LLVM::ConstantOp>(
                    loc, b.getI64Type(), b.getI64IntegerAttr(slot))});
            mlir::Type load_ty = (rk == HoistableCall::I32)
                                     ? (mlir::Type)b.getI32Type()
                                     : (mlir::Type)b.getF64Type();
            Value loaded = b.create<LLVM::LoadOp>(loc, load_ty, slot_ptr);
            ctx.hoist_ip = b.saveInsertionPoint();
            return loaded;
          }
        }
      }
      // --- end hoistable detection ---
      //

      auto fn_ty = ylc_fn_to_mlir(f->symbol_type, b.getContext());
      auto fn_op = declare_extern(ctx.mod, b, fn_name, fn_ty);

      llvm::SmallVector<Value> call_args;
      ::Type *param_t = f->symbol_type;
      for (size_t i = 0; i < nargs && param_t->kind == T_FN;
           i++, param_t = param_t->data.T_FN.to) {

        if (param_t->data.T_FN.from->kind == T_VOID) {
          continue;
        }

        Value v = build_dsp_expr(&args[i], ctx, jit_ctx);
        if (!v) {
          return {};
        }

        call_args.push_back(v);
      }

      auto call = b.create<LLVM::CallOp>(loc, fn_op, call_args);

      ::Type *ret_t = f->symbol_type;
      while (ret_t->kind == T_FN) {
        ret_t = ret_t->data.T_FN.to;
      }

      if (ret_t->kind == T_VOID) {
        return {};
      }

      auto r = call.getResult();
      return r;
    }

    if (fn->tag == AST_IDENTIFIER) {

      const char *name = fn->data.AST_IDENTIFIER.value;

      if (strcmp(name, "*") == 0 && nargs == 2) {
        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);
        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        return b.create<arith::MulFOp>(loc, l, r);
      }

      if (strcmp(name, "/") == 0 && nargs == 2) {

        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);
        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        auto res = b.create<arith::DivFOp>(loc, l, r);
        return res;
      }

      if (strcmp(name, "+") == 0 && nargs == 2) {

        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);

        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        return b.create<arith::AddFOp>(loc, l, r);
      }

      if (strcmp(name, "-") == 0 && nargs == 2) {

        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);
        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        return b.create<arith::SubFOp>(loc, l, r);
      }

      if (strcmp(name, "==") == 0 && nargs == 2) {
        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);
        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        Value cmp =
            b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OEQ, l, r);
        // TODO: cast boolean value as an f64
        return b.create<arith::UIToFPOp>(loc, b.getF64Type(), cmp);
        // return b.create<rith::
      }

      if (strcmp(name, "<=") == 0 && nargs == 2) {
        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);
        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        Value cmp =
            b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OLE, l, r);
        return b.create<arith::UIToFPOp>(loc, b.getF64Type(), cmp);
      }

      if (strcmp(name, ">=") == 0 && nargs == 2) {
        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);
        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        Value cmp =
            b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OGE, l, r);
        return b.create<arith::UIToFPOp>(loc, b.getF64Type(), cmp);
      }

      if (strcmp(name, "array_set") == 0) {
        if (nargs != 3) {
          fprintf(stderr, "Error: array_set expects (state, idx, val)\n");
          print_ast(ast);
          return {};
        }
        Value base = build_dsp_expr(&args[0], ctx, jit_ctx);
        if (!base || base.getType() != b.getI64Type()) {
          fprintf(stderr, "Error: array_set base must be i64 state offset\n");
          print_ast(ast);
          return {};
        }
        if (args[1].tag != AST_INT) {
          fprintf(stderr, "Error: array_set index must be an int literal\n");
          print_ast(ast);
          return {};
        }
        int idx = (int)args[1].data.AST_INT.value;
        // bounds check if we know the array name
        if (args[0].tag == AST_IDENTIFIER) {
          std::string name(args[0].data.AST_IDENTIFIER.value,
                           args[0].data.AST_IDENTIFIER.length);
          auto it = ctx.array_states.find(name);
          if (it != ctx.array_states.end()) {
            if (idx < 0 || idx >= it->second.len) {
              fprintf(stderr, "Error: array_set index out of bounds\n");
              print_ast(ast);
              return {};
            }
          }
        }

        Value idx_val = b.create<LLVM::ConstantOp>(
            loc, b.getI64Type(), b.getI64IntegerAttr((int64_t)idx));
        Value stride = b.create<LLVM::ConstantOp>(loc, b.getI64Type(),
                                                  b.getI64IntegerAttr(8));
        Value idx_bytes = b.create<LLVM::MulOp>(loc, idx_val, stride);
        Value off = b.create<LLVM::AddOp>(loc, base, idx_bytes);

        auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
        Value slot_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                               ctx.state_ptr, ValueRange{off});
        Value val = build_dsp_expr(&args[2], ctx, jit_ctx);
        if (!val)
          return {};
        b.create<LLVM::StoreOp>(loc, val, slot_ptr);
        return val;
      }

      if (strcmp(name, "array_at") == 0) {
        if (nargs != 2) {
          fprintf(stderr, "Error: array_at expects (state, idx)\n");
          print_ast(ast);
          return {};
        }
        Value base = build_dsp_expr(&args[0], ctx, jit_ctx);
        if (!base || base.getType() != b.getI64Type()) {
          fprintf(stderr, "Error: array_at base must be i64 state offset\n");
          print_ast(ast);
          return {};
        }
        if (args[1].tag != AST_INT) {
          fprintf(stderr, "Error: array_at index must be an int literal\n");
          print_ast(ast);
          return {};
        }
        int idx = (int)args[1].data.AST_INT.value;
        if (args[0].tag == AST_IDENTIFIER) {
          std::string name(args[0].data.AST_IDENTIFIER.value,
                           args[0].data.AST_IDENTIFIER.length);
          auto it = ctx.array_states.find(name);
          if (it != ctx.array_states.end()) {
            if (idx < 0 || idx >= it->second.len) {
              fprintf(stderr, "Error: array_at index out of bounds\n");
              print_ast(ast);
              return {};
            }
          }
        }

        Value idx_val = b.create<LLVM::ConstantOp>(
            loc, b.getI64Type(), b.getI64IntegerAttr((int64_t)idx));
        Value stride = b.create<LLVM::ConstantOp>(loc, b.getI64Type(),
                                                  b.getI64IntegerAttr(8));
        Value idx_bytes = b.create<LLVM::MulOp>(loc, idx_val, stride);
        Value off = b.create<LLVM::AddOp>(loc, base, idx_bytes);

        auto ptr_ty = LLVM::LLVMPointerType::get(b.getContext());
        Value slot_ptr = b.create<LLVM::GEPOp>(loc, ptr_ty, b.getI8Type(),
                                               ctx.state_ptr, ValueRange{off});
        return b.create<LLVM::LoadOp>(loc, b.getF64Type(), slot_ptr);
      }
    }
    break;
  }
  case AST_ARRAY: {
    std::string n = "anonymous_array::" + std::to_string(ctx.state_offset);
    return emit_array_state(ast, ctx, n);
  }
  default:
    break;
  }
  return {};
}

// =============================================================================
// Build the MLIR module: LLVM perform function wrapping an scf.for over
// nframes, body filled with DSP ops.
// =============================================================================

struct DspModuleResult {
  OwningOpRef<ModuleOp> mod;
  int state_bytes;
  std::vector<BufInputSpec> buf_inputs;
  std::vector<HoistableCall> hoistable_calls;
  std::vector<ArrayInitSpec> array_inits;
  std::vector<BufRefSpec> buf_ref_inputs;
};

static DspModuleResult build_dsp_module(Ast *lambda, const std::string &fn_name,
                                        JITLangCtx *jit_ctx,
                                        MLIRContext *mlir_ctx) {
  auto loc = ast_loc(lambda, mlir_ctx);
  OwningOpRef<ModuleOp> module_ref = ModuleOp::create(loc);
  ModuleOp mod = *module_ref;

  OpBuilder b(mlir_ctx);

  // perform_func_t: void *perform(Node*, void*, Node**, int nframes, double
  // spf)
  auto ptr_ty = LLVM::LLVMPointerType::get(mlir_ctx);
  auto i32_ty = b.getI32Type();
  auto f64_ty = b.getF64Type();

  auto fn_type = LLVM::LLVMFunctionType::get(
      ptr_ty, {ptr_ty, ptr_ty, ptr_ty, i32_ty, f64_ty}, false);

  b.setInsertionPointToEnd(mod.getBody());
  auto fn = b.create<LLVM::LLVMFuncOp>(loc, fn_name, fn_type);
  auto *entry = fn.addEntryBlock(b);
  b.setInsertionPointToStart(entry);

  // Count real (lambda param) inputs so hidden inlets start after them.
  int num_real_inputs = 0;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next)
    if (p->ast->tag != AST_VOID)
      num_real_inputs++;

  DspBuildCtx ctx{b,
                  mod,
                  loc,
                  fn.getArgument(0), // node_ptr
                  fn.getArgument(1), // state_ptr
                  fn.getArgument(2), // inputs_ptr
                  fn.getArgument(4), // spf
                  Value{},           // frame_idx — set inside the loop
                  0};
  ctx.next_hidden_inlet = num_real_inputs;

  // scf.for %i = 0 to nframes step 1
  Value zero = b.create<arith::ConstantIndexOp>(loc, 0);
  Value one = b.create<arith::ConstantIndexOp>(loc, 1);
  Value nframes =
      b.create<arith::IndexCastOp>(loc, b.getIndexType(), fn.getArgument(3));

  // Create the for loop first (no body builder callback) so the ForOp is a
  // fully-constructed operation before we set the hoist insertion point.
  auto for_op = b.create<scf::ForOp>(loc, zero, nframes, one);

  // Hoist point: just before the for_op in the entry block.  This is a stable
  // iterator — unlike block::end(), it doesn't shift when ops are appended.
  b.setInsertionPoint(for_op);
  ctx.hoist_ip = b.saveInsertionPoint();

  // Build the loop body explicitly.
  b.setInsertionPointToStart(for_op.getBody());
  ctx.frame_idx = for_op.getInductionVar();
  ctx.b = b;

  // Bind lambda params to inlet reads (each param → dsp.inlet idx)
  int inlet_idx = 0;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p;
       p = p->next, inlet_idx++) {
    Ast *param = p->ast;
    if (param->tag != AST_IDENTIFIER)
      continue;
    std::string pname(param->data.AST_IDENTIFIER.value,
                      param->data.AST_IDENTIFIER.length);
    ctx.locals[pname] =
        ctx.b
            .create<InletOp>(loc, ctx.inputs_ptr, for_op.getInductionVar(),
                             inlet_idx)
            ->getResult(0);
  }

  Value result = build_dsp_expr(lambda->data.AST_LAMBDA.body, ctx, jit_ctx);
  int state_bytes = ctx.state_offset;

  if (result)
    ctx.b.create<OutletOp>(loc, ctx.node_ptr, for_op.getInductionVar(), result);

  // Resume insertion in the entry block after the for loop.
  // Note: scf::ForOp::build without a body builder calls ensureTerminator,
  // which auto-inserts scf.yield — do not add a second one.
  b.setInsertionPointAfter(for_op);

  // Return node->output.buf via the helper.
  auto get_buf_ty = LLVM::LLVMFunctionType::get(ptr_ty, {ptr_ty}, false);
  auto get_buf_fn = declare_extern(mod, b, "ylc_get_output_buf", get_buf_ty);
  Value out_buf =
      b.create<LLVM::CallOp>(loc, get_buf_fn, ValueRange{ctx.node_ptr})
          .getResult();
  b.create<LLVM::ReturnOp>(loc, ValueRange{out_buf});

  return {std::move(module_ref), state_bytes,     ctx.buf_inputs,
          ctx.hoistable_calls,   ctx.array_inits, ctx.buf_ref_inputs};
}

// =============================================================================
// Pass pipelines
// =============================================================================

static LogicalResult runMLIRPasses(ModuleOp mod, MLIRContext *ctx) {
  PassManager pm(ctx);
  pm.addPass(createDspToLLVMPass());                // dsp.* → LLVM dialect
  pm.addPass(createSCFToControlFlowPass());         // scf.for → cf
  pm.addPass(createConvertControlFlowToLLVMPass()); // cf → llvm
  pm.addPass(createArithToLLVMConversionPass());    // arith.* → llvm
  pm.addPass(createReconcileUnrealizedCastsPass());
  return pm.run(mod);
}

static void runLLVMOptPasses(llvm::Module &mod) {

  llvm::PassBuilder pb;
  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;
  pb.registerModuleAnalyses(mam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerFunctionAnalyses(fam);
  pb.registerLoopAnalyses(lam);
  pb.crossRegisterProxies(lam, fam, cgam, mam);
  auto mpm = pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
  mpm.run(mod, mam);
}

// =============================================================================
// BuiltinHandler entry point
// =============================================================================

static int g_synth_id = 0;

void hoist_constant_calls(std::vector<HoistableCall> hoistable_calls,
                          LLVMValueRef node_val, LLVMModuleRef module_ref,
                          LLVMBuilderRef builder) {

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef f64_ty = LLVMDoubleTypeInContext(llvm_ctx);
  LLVMTypeRef void_ty = LLVMVoidTypeInContext(llvm_ctx);

  // --- Emit hoistable-call initialization ---
  // For each call that was identified as having all-constant args, call the
  // extern function now (at node-creation time) and store the result into the
  // node's state buffer at the pre-allocated slot.
  // ylc_node_get_state(Node*) -> void*
  LLVMTypeRef get_state_params[] = {ptr_ty};
  LLVMTypeRef get_state_fn_ty =
      LLVMFunctionType(ptr_ty, get_state_params, 1, 0);
  LLVMValueRef get_state_fn =
      LLVMGetNamedFunction(module_ref, "ylc_node_get_state");
  if (!get_state_fn) {
    get_state_fn =
        LLVMAddFunction(module_ref, "ylc_node_get_state", get_state_fn_ty);
    LLVMSetLinkage(get_state_fn, LLVMExternalLinkage);
  }
  LLVMValueRef state_ptr_val = LLVMBuildCall2(
      builder, get_state_fn_ty, get_state_fn, {&node_val}, 1, "state_ptr");

  LLVMTypeRef i64_ty = LLVMInt64TypeInContext(llvm_ctx);
  LLVMTypeRef i8_ty = LLVMInt8TypeInContext(llvm_ctx);

  for (auto &hc : hoistable_calls) {
    if (hc.fn_name.empty()) {
      // hoistable call is just a const value
      auto &arg = hc.args[0];
      auto init_val = arg.kind == HoistableCallArg::F64
                          ? LLVMConstReal(f64_ty, arg.f64val)
                          : LLVMConstInt(i32_ty, (uint64_t)arg.i32val, true);

      // GEP into state buffer at the allocated slot offset
      LLVMValueRef gep_idx = LLVMConstInt(i64_ty, (uint64_t)hc.state_offset, 0);
      LLVMValueRef slot_ptr =
          LLVMBuildGEP2(builder, i8_ty, state_ptr_val, &gep_idx, 1, "slot");
      LLVMBuildStore(builder, init_val, slot_ptr);
      continue;
    }
    // Build arg type/value lists
    std::vector<LLVMTypeRef> arg_tys;
    std::vector<LLVMValueRef> arg_vals;
    for (auto &arg : hc.args) {
      if (arg.kind == HoistableCallArg::F64) {
        arg_tys.push_back(f64_ty);
        arg_vals.push_back(LLVMConstReal(f64_ty, arg.f64val));
      } else {
        arg_tys.push_back(i32_ty);
        arg_vals.push_back(LLVMConstInt(i32_ty, (uint64_t)arg.i32val, true));
      }
    }

    LLVMTypeRef ret_ty = (hc.ret_kind == HoistableCall::I32) ? i32_ty : f64_ty;
    LLVMTypeRef call_fn_ty =
        LLVMFunctionType(ret_ty, arg_tys.data(), (unsigned)arg_tys.size(), 0);
    LLVMValueRef call_fn = LLVMGetNamedFunction(module_ref, hc.fn_name.c_str());
    if (!call_fn) {
      call_fn = LLVMAddFunction(module_ref, hc.fn_name.c_str(), call_fn_ty);
      LLVMSetLinkage(call_fn, LLVMExternalLinkage);
    }

    LLVMValueRef init_val =
        LLVMBuildCall2(builder, call_fn_ty, call_fn, arg_vals.data(),
                       (unsigned)arg_vals.size(), "hoist_val");

    // GEP into state buffer at the allocated slot offset
    LLVMValueRef gep_idx = LLVMConstInt(i64_ty, (uint64_t)hc.state_offset, 0);
    LLVMValueRef slot_ptr =
        LLVMBuildGEP2(builder, i8_ty, state_ptr_val, &gep_idx, 1, "slot");
    LLVMBuildStore(builder, init_val, slot_ptr);
  }
}

void hoist_array_inits(std::vector<ArrayInitSpec> inits, LLVMValueRef node_val,
                       LLVMModuleRef module_ref, LLVMBuilderRef builder) {
  if (inits.empty())
    return;
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef f64_ty = LLVMDoubleTypeInContext(llvm_ctx);
  LLVMTypeRef i64_ty = LLVMInt64TypeInContext(llvm_ctx);
  LLVMTypeRef i8_ty = LLVMInt8TypeInContext(llvm_ctx);

  LLVMTypeRef get_state_params[] = {ptr_ty};
  LLVMTypeRef get_state_fn_ty =
      LLVMFunctionType(ptr_ty, get_state_params, 1, 0);
  LLVMValueRef get_state_fn =
      LLVMGetNamedFunction(module_ref, "ylc_node_get_state");
  if (!get_state_fn) {
    get_state_fn =
        LLVMAddFunction(module_ref, "ylc_node_get_state", get_state_fn_ty);
    LLVMSetLinkage(get_state_fn, LLVMExternalLinkage);
  }
  LLVMValueRef state_ptr_val = LLVMBuildCall2(
      builder, get_state_fn_ty, get_state_fn, {&node_val}, 1, "state_ptr");

  for (auto &init : inits) {
    LLVMValueRef gep_idx = LLVMConstInt(i64_ty, (uint64_t)init.offset, 0);
    LLVMValueRef slot_ptr =
        LLVMBuildGEP2(builder, i8_ty, state_ptr_val, &gep_idx, 1, "slot");
    LLVMBuildStore(builder, LLVMConstReal(f64_ty, init.value), slot_ptr);
  }
}
void wire_node_bufs(std::vector<BufInputSpec> buf_inputs, LLVMValueRef node_val,
                    LLVMModuleRef module_ref, LLVMBuilderRef builder) {

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef f64_ty = LLVMDoubleTypeInContext(llvm_ctx);
  LLVMTypeRef void_ty = LLVMVoidTypeInContext(llvm_ctx);
  LLVMTypeRef attach_params[] = {ptr_ty, i32_ty, f64_ty};
  LLVMTypeRef attach_fn_ty = LLVMFunctionType(void_ty, attach_params, 3, 0);
  LLVMValueRef attach_fn =
      LLVMGetNamedFunction(module_ref, "ylc_attach_buf_inlet");
  if (!attach_fn) {
    attach_fn =
        LLVMAddFunction(module_ref, "ylc_attach_buf_inlet", attach_fn_ty);
    LLVMSetLinkage(attach_fn, LLVMExternalLinkage);
  }
  for (auto &buf : buf_inputs) {
    LLVMValueRef args[] = {
        node_val,
        LLVMConstInt(i32_ty, buf.inlet_idx, 0),
        LLVMConstReal(f64_ty, buf.max_delay_sec),
    };
    LLVMBuildCall2(builder, attach_fn_ty, attach_fn, args, 3, "");
  }
}

extern "C" LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *jit_ctx,
                                              LLVMModuleRef module_ref,
                                              LLVMBuilderRef builder) {
  Ast *lambda = ast->data.AST_APPLICATION.args;
  if (!lambda || lambda->tag != AST_LAMBDA) {
    fprintf(stderr, "compile_audio_fn: expected fn () -> ...\n");
    return LLVMConstInt(LLVMInt32Type(), 0, 0);
  }

  // Count lambda params — used as num_inputs for the Node
  int num_inputs = 0;

  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    if (p->ast->tag != AST_VOID) {
      num_inputs++;
    }
  }

  std::string fn_name = "synth_perform_" + std::to_string(g_synth_id++);

  MLIRContext *mlir_ctx = get_mlir_ctx();
  auto result = build_dsp_module(lambda, fn_name, jit_ctx, mlir_ctx);
  auto &mlir_mod = result.mod;
  int state_bytes = result.state_bytes;
  auto &buf_inputs = result.buf_inputs;
  if (!mlir_mod) {
    fprintf(stderr, "compile_audio_fn: DSP IR build failed\n");
    return LLVMConstInt(LLVMInt32Type(), 0, 0);
  }

  fprintf(stderr, COLOR_MAGENTA STYLE_BOLD "=== DSP IR ===\n");
  mlir_mod->dump();
  fprintf(stderr, "==============\n" STYLE_RESET_ALL);

  if (failed(runMLIRPasses(*mlir_mod, mlir_ctx))) {
    fprintf(stderr, "compile_audio_fn: lowering failed\n");
    return LLVMConstInt(LLVMInt32Type(), 0, 0);
  }

  llvm::Module *mcjit = reinterpret_cast<llvm::Module *>(module_ref);
  auto llvm_mod = mlir::translateModuleToLLVMIR(*mlir_mod, mcjit->getContext());
  if (!llvm_mod) {
    fprintf(stderr, "compile_audio_fn: LLVM IR export failed\n");
    return LLVMConstInt(LLVMInt32Type(), 0, 0);
  }

  runLLVMOptPasses(*llvm_mod);

  if (llvm::Linker::linkModules(*mcjit, std::move(llvm_mod))) {
    fprintf(stderr, "compile_audio_fn: link failed\n");
    return LLVMConstInt(LLVMInt32Type(), 0, 0);
  }

  int num_total_inputs =
      num_inputs + (int)buf_inputs.size() + (int)result.buf_ref_inputs.size();
  fprintf(stderr,
          "compile_audio_fn: OK name=%s real_inputs=%d delay_bufs=%d "
          "buf_refs=%d state=%d bytes\n",
          fn_name.c_str(), num_inputs, (int)buf_inputs.size(),
          (int)result.buf_ref_inputs.size(), state_bytes);

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef f64_ty = LLVMDoubleTypeInContext(llvm_ctx);
  LLVMTypeRef void_ty = LLVMVoidTypeInContext(llvm_ctx);

  // --- ylc_create_audio_node(perform, num_total_inputs, state_bytes) → Node*
  LLVMTypeRef create_params[] = {ptr_ty, i32_ty, i32_ty};
  LLVMTypeRef create_fn_ty = LLVMFunctionType(ptr_ty, create_params, 3, 0);
  LLVMValueRef create_fn =
      LLVMGetNamedFunction(module_ref, "ylc_create_audio_node");
  if (!create_fn) {
    create_fn =
        LLVMAddFunction(module_ref, "ylc_create_audio_node", create_fn_ty);
    LLVMSetLinkage(create_fn, LLVMExternalLinkage);
  }

  LLVMValueRef dsp_fn = LLVMGetNamedFunction(module_ref, fn_name.c_str());
  LLVMValueRef create_args[] = {
      dsp_fn,
      LLVMConstInt(i32_ty, num_total_inputs, 0),
      LLVMConstInt(i32_ty, state_bytes, 0),
  };
  LLVMValueRef node_val =
      LLVMBuildCall2(builder, create_fn_ty, create_fn, create_args, 3, "node");

  // Wire delay-buffer inlets.
  if (!buf_inputs.empty())
    wire_node_bufs(buf_inputs, node_val, module_ref, builder);

  // Wire external buffer-ref inlets (existing nodes plugged as hidden inputs).
  if (!result.buf_ref_inputs.empty()) {
    LLVMTypeRef attach_params[] = {i32_ty, ptr_ty, ptr_ty};
    LLVMTypeRef attach_fn_ty = LLVMFunctionType(void_ty, attach_params, 3, 0);
    LLVMValueRef attach_fn =
        LLVMGetNamedFunction(module_ref, "plug_input_in_graph");

    if (!attach_fn) {
      attach_fn =
          LLVMAddFunction(module_ref, "plug_input_in_graph", attach_fn_ty);
      LLVMSetLinkage(attach_fn, LLVMExternalLinkage);
    }

    for (auto &bri : result.buf_ref_inputs) {
      LLVMValueRef source = bri.sym->val;
      if (bri.sym->type == STYPE_TOP_LEVEL_VAR && bri.sym->storage)
        source = LLVMBuildLoad2(builder, ptr_ty, bri.sym->storage, "buf_node");
      LLVMValueRef args[] = {LLVMConstInt(i32_ty, bri.inlet_idx, 0), node_val,
                             source};
      LLVMBuildCall2(builder, attach_fn_ty, attach_fn, args, 3, "");
    }
  }

  // --- Emit hoistable-call initialization ---
  // For each call that was identified as having all-constant args, call the
  // extern function now (at node-creation time) and store the result into the
  // node's state buffer at the pre-allocated slot.
  if (!result.hoistable_calls.empty()) {
    hoist_constant_calls(result.hoistable_calls, node_val, module_ref, builder);
  }
  if (!result.array_inits.empty()) {
    hoist_array_inits(result.array_inits, node_val, module_ref, builder);
  }

  return node_val;
}

extern "C" LLVMValueRef _register_let(Ast *let, JITLangCtx *jit_ctx,
                                      LLVMModuleRef module_ref,
                                      LLVMBuilderRef builder) {

  Ast *binding = let->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;

  JITSymbol *sym =
      new_symbol((symbol_type)STYPE_AUDIO_JIT_SYM, nullptr, nullptr, nullptr);
  ht *stack = jit_ctx->frame->table;
  Ast *expr = let->data.AST_LET.expr;
  sym->symbol_data._USER_DEFINED_SYMBOL = expr;
  ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

LLVMValueRef audio_jit_inline_module(Ast *binding, Ast *module_ast,
                                     JITLangCtx *ctx,
                                     LLVMModuleRef llvm_module_ref,
                                     LLVMBuilderRef builder) {

  YLCModule _module = {
      .type = module_ast->type,
      .ast = module_ast,
  };

  YLCModule *module = &_module;
  JITSymbol *module_symbol;

  if (module->ast) {
    ::Type *module_type = module->type;
    int mod_len = module_type->data.T_CONS.num_args;
    Ast *module_ast = module->ast;

    module_symbol = create_module_symbol(module_type, NULL, module_ast, ctx,
                                         llvm_module_ref);

    JITLangCtx *mod_ctx = module_symbol->symbol_data.STYPE_MODULE.ctx;
    if (module_ast->data.AST_LAMBDA.body->tag != AST_BODY) {
      _register_let(module_ast->data.AST_LAMBDA.body, mod_ctx, llvm_module_ref,
                    builder);
    } else {

      AST_LIST_ITER(module_ast->data.AST_LAMBDA.body->data.AST_BODY.stmts, ({
                      _register_let(l->ast, mod_ctx, llvm_module_ref, builder);
                    }));
    }

    const char *mod_binding = binding->data.AST_IDENTIFIER.value;
    int mod_binding_len = binding->data.AST_IDENTIFIER.length;

    ht_set_hash(ctx->frame->table, mod_binding,
                hash_string(mod_binding, mod_binding_len), module_symbol);

    module->ref = module_symbol;
  }

  // return module_symbol->val;
  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

extern "C" LLVMValueRef RegisterAudioOpHandler(Ast *ast, JITLangCtx *jit_ctx,
                                               LLVMModuleRef module_ref,
                                               LLVMBuilderRef builder) {
  if (ast->tag == AST_APPLICATION &&
      ast->data.AST_APPLICATION.args->tag == AST_LET) {

    Ast *expr = ast->data.AST_APPLICATION.args->data.AST_LET.expr;

    if (expr->tag == AST_LAMBDA) {
      return _register_let(ast->data.AST_APPLICATION.args, jit_ctx, module_ref,
                           builder);
    }

    if (expr->tag == AST_MODULE) {
      return audio_jit_inline_module(
          ast->data.AST_APPLICATION.args->data.AST_LET.binding, expr, jit_ctx,
          module_ref, builder);
    }
  }

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

// =============================================================================
// Library constructor
// =============================================================================
__attribute__((constructor)) static void ylc_audio_jit_init() {

  if (!ylc_jit_ctx) {
    fprintf(stderr, "libaudio_jit: no JIT context at load time\n");
    return;
  }

  STYPE_AUDIO_JIT_SYM = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_BUILTIN_HANDLER = REGISTERED_JIT_SYMBOL_TYPE++;
  ht *stack = ylc_jit_ctx->frame->table;

  ({
    JITSymbol *sym =
        new_symbol(STYPE_GENERIC_FUNCTION, nullptr, nullptr, nullptr);
    sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =
        CompileAudioFnHandler;
    const char *name = "compile_audio_fn";
    ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
    fprintf(stderr, "libaudio_jit: registered compile_audio_fn\n");
  });

  ({
    JITSymbol *sym =
        new_symbol(STYPE_GENERIC_FUNCTION, nullptr, nullptr, nullptr);
    sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =
        RegisterAudioOpHandler;
    const char *name = "register_audio_op";
    ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
    fprintf(stderr, "libaudio_jit: registered register_audio_op\n");
  });

#define DSP_BUILTIN(name, handler, fn_type)                                    \
  ({                                                                           \
    JITSymbol *sym = new_symbol((symbol_type)STYPE_AUDIO_JIT_BUILTIN_HANDLER,  \
                                fn_type, nullptr, nullptr);                    \
    sym->symbol_data._USER_DEFINED_SYMBOL = (void *)handler;                   \
    ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);            \
    add_builtin(name, fn_type);                                                \
    fprintf(stderr, "libaudio_jit: registered " name "\n");                    \
  })

  DSP_BUILTIN("phasor", PhasorHandler, type_fn(&t_num, &t_num));
  DSP_BUILTIN("sin_osc", SinOscHandler, type_fn(&t_num, &t_num));
  DSP_BUILTIN("sq_osc", SqOscHandler, type_fn(&t_num, &t_num));
  DSP_BUILTIN("saw_osc", SawOscHandler, type_fn(&t_num, &t_num));
  DSP_BUILTIN("trig", PhasorTrigHandler, type_fn(&t_num, &t_num));
  DSP_BUILTIN(
      "linscale", LinscaleHandler,
      type_fn(&t_num,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num)))));

  // bufref node_sym -> !llvm.ptr (Node* of the inlet, hoisted before loop)
  DSP_BUILTIN("bufref", BufRefHandler, type_fn(&t_num, &t_num));
  // bufread node_ptr phase -> f64  (lerp from node->output.buf)
  DSP_BUILTIN("bufread", BufReadHandler,
              type_fn(&t_num, type_fn(&t_num, &t_num)));

  DSP_BUILTIN("bipolar_scale", BipolarLinscaleHandler,
              type_fn(&t_num, type_fn(&t_num, &t_num)));

  DSP_BUILTIN("unipolar_scale", UnipolarLinscaleHandler,
              type_fn(&t_num, type_fn(&t_num, &t_num)));

  DSP_BUILTIN("wrap", WrapHandler,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num))));

  DSP_BUILTIN("fold", FoldHandler,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num))));

  // delay delay_time max_delay fb input → num
  DSP_BUILTIN(
      "delay", DelayHandler,
      type_fn(&t_num,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num)))));

  DSP_BUILTIN(
      "delay1", DelayHandler,
      type_fn(&t_num,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num)))));

  // allpass delay_time max_delay g input → num
  DSP_BUILTIN(
      "allpass", AllpassHandler,
      type_fn(&t_num,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num)))));

  DSP_BUILTIN(
      "allpass1", Allpass1Handler,
      type_fn(&t_num,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num)))));

  DSP_BUILTIN("white", WhiteNoiseHandler, type_fn(&t_void, &t_num));
  DSP_BUILTIN("lfnoise0", LFNoiseHandler,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num))));

  DSP_BUILTIN("lfnoise1", LFNoiseHandler,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num))));

  DSP_BUILTIN("spf", SpfHandler, type_fn(&t_void, &t_num));
  DSP_BUILTIN("clampup", ClampHandler,
              type_fn(&t_num, type_fn(&t_num, &t_num)));

  DSP_BUILTIN("trig_fn", TrigFnHandler, nullptr);
  DSP_BUILTIN("trig_choose", TrigChooseHandler, nullptr);
  DSP_BUILTIN(
      "bufplay", BufPlayHandler,
      type_fn(&t_ptr,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num)))));

  DSP_BUILTIN("sah", SahHandler, type_fn(&t_num, type_fn(&t_num, &t_num)));
  DSP_BUILTIN(
      "grains", GrainsHandler,

      type_fn(
          &t_ptr,
          type_fn(&t_num,
                  type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num))))));
}
