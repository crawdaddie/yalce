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
#include "../../engine/node_util.h"
#include "../../engine/osc.h"
#include "../../lang/backend_llvm/function_extern.h"
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
#include "llvm/IR/Operator.h"
#include "llvm/Passes/PassBuilder.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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

// static const int YLC_SIN_TABSIZE = 1 << 11;
// static const int YLC_SQ_TABSIZE = 1 << 11;
// static const int YLC_SAW_TABSIZE = 1 << 11;

// Feedback delay. State at state_offset: [read_pos: i32, write_pos: i32] (8 bytes).
// inputs[inlet_idx] is the delay-buffer node (its output.buf is the delay line).
// out = input + buf[read_pos];  buf[write_pos] = fb * out;  advance both.
extern "C" double ylc_delay_fb(void *state_raw, int32_t state_offset,
                                void *inputs_raw, int32_t inlet_idx,
                                double input, double fb) {
  int32_t *read_pos = (int32_t *)((char *)state_raw + state_offset);
  int32_t *write_pos = read_pos + 1;
  Node **inputs = (Node **)inputs_raw;
  double *buf = inputs[inlet_idx]->output.buf;
  int buf_sz = inputs[inlet_idx]->output.size;
  double delayed = buf[*read_pos];
  double out = input + delayed;
  buf[*write_pos] = fb * out;
  *read_pos = (*read_pos + 1) % buf_sz;
  *write_pos = (*write_pos + 1) % buf_sz;
  return out;
}

// Attach a hidden delay-buffer node as inlet_idx of the DSP node.
// The delay line is calloc'd (not from the buffer pool), so it can never
// trigger a pool realloc that would invalidate other nodes' output.buf pointers.
// Also initialises read_pos in the node's state so the initial delay is correct.
extern "C" void ylc_attach_delay_buf(Node *node, int32_t inlet_idx,
                                     int32_t state_offset,
                                     double max_delay_sec,
                                     double init_delay_sec) {
  AudioGraph *graph = _graph;
  // Save index before allocate_node_in_graph may realloc graph->nodes.
  int node_idx = node->node_index;

  int max_sz = (int)(max_delay_sec / ctx_spf());
  if (max_sz <= 0) max_sz = 1;
  int delay_samps = (int)(init_delay_sec / ctx_spf());
  if (delay_samps >= max_sz) delay_samps = max_sz - 1;

  // Allocate delay line outside the pool — no pool realloc, no stale pointers.
  double *buf = (double *)calloc(max_sz, sizeof(double));

  // Create a hidden node to own the delay line buffer.
  // allocate_node_in_graph may move graph->nodes; re-lookup `node` afterwards.
  Node *buf_node = allocate_node_in_graph(graph, 0);
  *buf_node = (Node){
      .perform = NULL,
      .node_index = buf_node->node_index,
      .num_inputs = 0,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      .output = (Signal){.layout = 1, .size = max_sz, .buf = buf},
      .meta = (char *)"delay_buf",
  };

  // Re-lookup DSP node in the live (possibly reallocated) nodes array.
  Node *live_node = graph ? (graph->nodes + node_idx) : node;
  plug_input_in_graph(inlet_idx, live_node, buf_node);

  // Initialise read_pos so the delay starts at init_delay_sec.
  int32_t *read_pos = (int32_t *)((char *)live_node->state_ptr + state_offset);
  *read_pos = max_sz - delay_samps;
  // write_pos (read_pos + 1) stays 0 from the memset in ylc_create_audio_node.
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
// Build context threaded through AST → DSP op emission
// =============================================================================

// Describes a hidden delay-buffer inlet.
// ylc_attach_delay_buf is called once at node-creation time to:
//   - allocate the delay line (calloc, not pool)
//   - wire it as inputs[inlet_idx]
//   - initialise read_pos in state so the initial delay is correct
struct BufInputSpec {
  int inlet_idx;
  int state_offset;     // byte offset of [read_pos: i32, write_pos: i32]
  double max_delay_sec;
  double init_delay_sec;
};

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
  // Lazily fetched wavetable pointers, keyed by the C function that returns
  // them (e.g. "get_sin_table").  Each is hoisted before the loop exactly once.
  std::unordered_map<std::string, Value> hoisted_ptrs;
};

// =============================================================================
// AST → DSP ops
// =============================================================================

static Value build_dsp_expr(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx);

// Fetch a wavetable pointer once, hoisted before the scf.for loop.
// fn_name is the C accessor (e.g. "get_sin_table").
// Uses InsertionGuard to redirect ctx.b to hoist_ip and then restore it.
static Value get_hoisted_table(const char *fn_name, DspBuildCtx &ctx) {
  auto it = ctx.hoisted_ptrs.find(fn_name);
  if (it != ctx.hoisted_ptrs.end())
    return it->second;

  OpBuilder::InsertionGuard guard(ctx.b); // saves loop-body position
  ctx.b.restoreInsertionPoint(ctx.hoist_ip);

  auto ptr_ty = LLVM::LLVMPointerType::get(ctx.b.getContext());
  auto fn_ty = LLVM::LLVMFunctionType::get(ptr_ty, {}, false);
  auto fn = declare_extern(ctx.mod, ctx.b, fn_name, fn_ty);
  Value ptr = ctx.b.create<LLVM::CallOp>(ctx.loc, fn, ValueRange{}).getResult();
  ctx.hoisted_ptrs[fn_name] = ptr;
  ctx.hoist_ip = ctx.b.saveInsertionPoint();
  // guard restores ctx.b to the loop body
  return ptr;
}

// Wavetable oscillator: PhasorOp manages state, TableLookupOp does inline lerp.
// table_fn is the C accessor, table_size must be a power of 2.
static Value emit_table_osc(Value freq, const char *table_fn,
                            int32_t table_size, DspBuildCtx &ctx) {
  int off = ctx.state_offset;
  ctx.state_offset += 8;
  Value phase =
      ctx.b.create<PhasorOp>(ctx.loc, ctx.state_ptr, freq, ctx.spf, off)
          ->getResult(0);
  Value table_ptr = get_hoisted_table(table_fn, ctx);
  return ctx.b.create<TableLookupOp>(ctx.loc, phase, table_ptr, table_size)
      ->getResult(0);
}

static Value buildDspPipe(Ast *lhs, Ast *rhs, DspBuildCtx &ctx,
                          JITLangCtx *jit_ctx) {
  Value input = build_dsp_expr(lhs, ctx, jit_ctx);
  if (!input)
    return {};
  if (rhs->tag == AST_APPLICATION) {
    Ast *fn = rhs->data.AST_APPLICATION.function;
    Ast *args = rhs->data.AST_APPLICATION.args;
    if (fn->tag == AST_IDENTIFIER) {
      const char *name = fn->data.AST_IDENTIFIER.value;
      if (strcmp(name, "*") == 0) {
        Value r = build_dsp_expr(args, ctx, jit_ctx);
        return ctx.b.create<arith::MulFOp>(ctx.loc, input, r);
      }
      if (strcmp(name, "+") == 0) {
        Value r = build_dsp_expr(args, ctx, jit_ctx);
        return ctx.b.create<arith::AddFOp>(ctx.loc, input, r);
      }
    }
  }
  return input;
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
  return emit_table_osc(freq, "get_sin_table", SIN_TABSIZE, ctx);
}

Value SqOscHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  return emit_table_osc(freq, "get_sq_table", SQ_TABSIZE, ctx);
}

Value SawOscHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  return emit_table_osc(freq, "get_saw_table", SAW_TABSIZE, ctx);
}

Value PhasorHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;

  Value freq = build_dsp_expr(args, ctx, jit_ctx);
  int off = ctx.state_offset;
  ctx.state_offset += 8;

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
  if (!a) return 0.0;
  if (a->tag == AST_DOUBLE) return a->data.AST_DOUBLE.value;
  if (a->tag == AST_FLOAT) return (double)a->data.AST_FLOAT.value;
  if (a->tag == AST_INT) return (double)a->data.AST_INT.value;
  return 0.0;
}

// delay delay_time max_delay fb input → f64
// Creates a hidden inlet holding the circular delay buffer.
// State: [read_pos: i32][write_pos: i32]  (8 bytes)
static Value DelayHandler(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  Ast *args = ast->data.AST_APPLICATION.args;
  double delay_time = ast_const_double(&args[0]);
  double max_delay = ast_const_double(&args[1]);
  Value fb = build_dsp_expr(&args[2], ctx, jit_ctx);
  Value input = build_dsp_expr(&args[3], ctx, jit_ctx);

  int off = ctx.state_offset;
  ctx.state_offset += 8; // [read_pos: i32, write_pos: i32]
  int hidden_idx = ctx.next_hidden_inlet++;
  ctx.buf_inputs.push_back({hidden_idx, off, max_delay, delay_time});

  return ctx.b
      .create<DelayOp>(ctx.loc, ctx.state_ptr, ctx.inputs_ptr, input, fb,
                       off, hidden_idx)
      ->getResult(0);
}

//   if (strcmp(name, "sq_osc") == 0 && nargs >= 1) {
//     Value freq = build_dsp_expr(&args[0], ctx, jit_ctx);
//     return emit_table_osc(freq, "get_sq_table", YLC_SQ_TABSIZE, ctx);
//   }
//   if (strcmp(name, "saw_osc") == 0 && nargs >= 1) {
//     Value freq = build_dsp_expr(&args[0], ctx, jit_ctx);
//     return emit_table_osc(freq, "get_saw_table", YLC_SAW_TABSIZE, ctx);
//   }
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
                              .hoisted_ptrs = parent.hoisted_ptrs} {}

  ~InlinedCtx() {
    parent.b = child.b;
    parent.hoist_ip = child.hoist_ip;
    parent.hoisted_ptrs = std::move(child.hoisted_ptrs);
    parent.state_offset = child.state_offset;
    parent.next_hidden_inlet = child.next_hidden_inlet;
    parent.buf_inputs.insert(parent.buf_inputs.end(), child.buf_inputs.begin(),
                             child.buf_inputs.end());
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

  return build_dsp_expr(fn_ast->data.AST_LAMBDA.body, inlined.child, jit_ctx);
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
      printf("found this -- ");
      print_ast(ast);
      return {};
    }
    return {};
  }

  case AST_LET: {
    Value val = build_dsp_expr(ast->data.AST_LET.expr, ctx, jit_ctx);
    if (ast->data.AST_LET.binding &&
        ast->data.AST_LET.binding->tag == AST_IDENTIFIER) {
      std::string n(ast->data.AST_LET.binding->data.AST_IDENTIFIER.value,
                    ast->data.AST_LET.binding->data.AST_IDENTIFIER.length);
      if (val)
        ctx.locals[n] = val;
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

    JITSymbol *f = lookup_id_ast(fn, jit_ctx);

    if (f && f->type == STYPE_AUDIO_JIT_BUILTIN_HANDLER &&
        f->symbol_data._USER_DEFINED_SYMBOL) {
      BuiltinDSPHandler handler =
          (BuiltinDSPHandler)f->symbol_data._USER_DEFINED_SYMBOL;
      return handler(ast, ctx, jit_ctx);
    }
    if (f && f->type == STYPE_AUDIO_JIT_SYM) {
      return inline_dsp_subexpr(f, args, ctx, jit_ctx);
    }

    if (f && f->symbol_type->kind == T_FN) {
      const char *fn_name = nullptr;

      if (f->type == STYPE_GENERIC_FUNCTION) {
        fprintf(stderr, "Error: generic function instantiation in audio "
                        "compiler not yet supported\n");
        return nullptr;
      }

      if (f->type == STYPE_LAZY_EXTERN_FUNCTION) {
        fn_name = f->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast->data
                      .AST_EXTERN_FN.fn_name.chars;
      } else if (f->type == STYPE_FUNCTION) {
        fn_name = LLVMGetValueName(f->val);
      }

      auto fn_ty = ylc_fn_to_mlir(f->symbol_type, b.getContext());
      auto fn_op = declare_extern(ctx.mod, b, fn_name, fn_ty);

      llvm::SmallVector<Value> call_args;
      ::Type *param_t = f->symbol_type;
      for (size_t i = 0; i < nargs && param_t->kind == T_FN;
           i++, param_t = param_t->data.T_FN.to) {
        if (param_t->data.T_FN.from->kind == T_VOID)
          continue;
        Value v = build_dsp_expr(&args[i], ctx, jit_ctx);
        if (!v)
          return {};
        call_args.push_back(v);
      }

      auto call = b.create<LLVM::CallOp>(loc, fn_op, call_args);

      ::Type *ret_t = f->symbol_type;
      while (ret_t->kind == T_FN)
        ret_t = ret_t->data.T_FN.to;
      if (ret_t->kind == T_VOID)
        return {};
      return call.getResult();
    }

    if (fn->tag == AST_IDENTIFIER) {

      const char *name = fn->data.AST_IDENTIFIER.value;

      if (strcmp(name, "*") == 0 && nargs == 2) {
        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);
        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        return b.create<arith::MulFOp>(loc, l, r);
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
        // TODO: cast boolean value as an f64
        return b.create<arith::UIToFPOp>(loc, b.getF64Type(), cmp);
        // return b.create<rith::
      }

      if (strcmp(name, ">=") == 0 && nargs == 2) {
        Value l = build_dsp_expr(&args[0], ctx, jit_ctx);
        Value r = build_dsp_expr(&args[1], ctx, jit_ctx);
        Value cmp =
            b.create<arith::CmpFOp>(loc, arith::CmpFPredicate::OGE, l, r);
        return b.create<arith::UIToFPOp>(loc, b.getF64Type(), cmp);
      }
    }
    break;
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

  return {std::move(module_ref), state_bytes, ctx.buf_inputs};
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
  // llvm::FastMathFlags fmf;
  // fmf.setFast();
  // for (auto &F : mod)
  //   for (auto &BB : F)
  //     for (auto &I : BB)
  //       if (llvm::isa<llvm::FPMathOperator>(I))
  //         I.setFastMathFlags(fmf);

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

  int num_total_inputs = num_inputs + (int)buf_inputs.size();
  fprintf(stderr,
          "compile_audio_fn: OK name=%s real_inputs=%d delay_bufs=%d "
          "state=%d bytes\n",
          fn_name.c_str(), num_inputs, (int)buf_inputs.size(), state_bytes);

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

  // Attach hidden delay-buffer inlets.
  // ylc_attach_delay_buf(node, inlet_idx, state_offset, max_delay_sec,
  //                      init_delay_sec)
  // It allocates the delay line with calloc (not from pool) to avoid
  // invalidating any previously allocated output.buf pointers.
  if (!buf_inputs.empty()) {
    LLVMTypeRef delay_params[] = {ptr_ty, i32_ty, i32_ty, f64_ty, f64_ty};
    LLVMTypeRef delay_fn_ty = LLVMFunctionType(void_ty, delay_params, 5, 0);
    LLVMValueRef delay_fn =
        LLVMGetNamedFunction(module_ref, "ylc_attach_delay_buf");
    if (!delay_fn) {
      delay_fn =
          LLVMAddFunction(module_ref, "ylc_attach_delay_buf", delay_fn_ty);
      LLVMSetLinkage(delay_fn, LLVMExternalLinkage);
    }
    for (auto &buf : buf_inputs) {
      LLVMValueRef args[] = {node_val,
                             LLVMConstInt(i32_ty, buf.inlet_idx, 0),
                             LLVMConstInt(i32_ty, buf.state_offset, 0),
                             LLVMConstReal(f64_ty, buf.max_delay_sec),
                             LLVMConstReal(f64_ty, buf.init_delay_sec)};
      LLVMBuildCall2(builder, delay_fn_ty, delay_fn, args, 5, "");
    }
  }

  return node_val;
}

extern "C" LLVMValueRef _register_let(Ast *let, JITLangCtx *jit_ctx,
                                      LLVMModuleRef module_ref,
                                      LLVMBuilderRef builder) {
  // printf("register inline: ");
  // print_ast(let);

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
    fprintf(stderr, "libaudio_jit: registered register_dsp_op\n");
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
  DSP_BUILTIN("trig", ImpulseHandler, type_fn(&t_num, &t_num));
  DSP_BUILTIN(
      "linscale", LinscaleHandler,
      type_fn(&t_num,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num)))));

  DSP_BUILTIN("bipolar_scale", BipolarLinscaleHandler,
              type_fn(&t_num, type_fn(&t_num, &t_num)));

  DSP_BUILTIN("unipolar_scale", UnipolarLinscaleHandler,
              type_fn(&t_num, type_fn(&t_num, &t_num)));

  DSP_BUILTIN("wrap", WrapHandler,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num))));
  DSP_BUILTIN("fold", FoldHandler,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num, &t_num))));

  // delay delay_time max_delay fb input → num
  DSP_BUILTIN("delay", DelayHandler,
              type_fn(&t_num, type_fn(&t_num, type_fn(&t_num,
                                                       type_fn(&t_num, &t_num)))));
}
