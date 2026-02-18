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
#include "../../engine/node.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/common.h"
#include "../../lang/format_utils.h"
#include "../../lang/ht.h"
#include "../../lang/parse.h"
}

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
#include "mlir/Transforms/Passes.h"

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
#include <unordered_map>

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

// =============================================================================
// DSP Dialect
// =============================================================================

// Forward-declare all ops so the dialect constructor can register them.
class InletOp;
class OutletOp;
class OscOp;
class BufplayOp;
class EnvAslrOp;

class DspDialect : public Dialect {
public:
  explicit DspDialect(MLIRContext *ctx)
      : Dialect("dsp", ctx, TypeID::get<DspDialect>()) {
    addOperations<InletOp, OutletOp, OscOp, BufplayOp, EnvAslrOp>();
  }
  static StringRef getDialectNamespace() { return "dsp"; }
};

// =============================================================================
// DSP Ops (pure C++, no tablegen)
//
// All stateful ops carry a `state_offset` attribute (byte offset into the
// node's opaque state block) and a `state_ptr` operand so that the lowering
// pattern is self-contained — no need to walk parent ops.
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

// dsp.osc %state_ptr, %freq, %spf {state_offset} : (!llvm.ptr, f64, f64) -> f64
// Stateful sine oscillator. State: one f64 (phase accumulator).
// Advances phase by freq*spf each frame, returns sin(phase * 2π).
class OscOp : public Op<OscOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                        OpTrait::ZeroSuccessors, OpTrait::NOperands<3>::Impl> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "dsp.osc"; }
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

// dsp.bufplay %state_ptr, %rate, %trig {state_offset} : (!llvm.ptr, f64, f64)
// -> f64 Stateful buffer player. State: phase f64 + prev_trig f64 (16 bytes).
// The buffer pointer itself is baked into state at node-init time.
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
// Stateful attack-sustain-release envelope. State: phase f64 + stage i32 (12
// bytes).
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
// Helpers
// =============================================================================

static LLVM::LLVMFuncOp declareExtern(ModuleOp &mod, OpBuilder &b,
                                      StringRef name,
                                      LLVM::LLVMFunctionType fn_ty) {
  if (auto f = mod.lookupSymbol<LLVM::LLVMFuncOp>(name))
    return f;
  OpBuilder::InsertionGuard g(b);
  b.setInsertionPointToStart(mod.getBody());
  return b.create<LLVM::LLVMFuncOp>(mod.getLoc(), name, fn_ty);
}

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
  std::unordered_map<std::string, Value> locals;
};

// =============================================================================
// AST → DSP ops
// =============================================================================

static Value buildDspExpr(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx);

static Value buildDspPipe(Ast *lhs, Ast *rhs, DspBuildCtx &ctx,
                          JITLangCtx *jit_ctx) {
  Value input = buildDspExpr(lhs, ctx, jit_ctx);
  if (!input)
    return {};
  if (rhs->tag == AST_APPLICATION) {
    Ast *fn = rhs->data.AST_APPLICATION.function;
    Ast *args = rhs->data.AST_APPLICATION.args;
    if (fn->tag == AST_IDENTIFIER) {
      const char *name = fn->data.AST_IDENTIFIER.value;
      if (strcmp(name, "*") == 0) {
        Value r = buildDspExpr(args, ctx, jit_ctx);
        return ctx.b.create<arith::MulFOp>(ctx.loc, input, r);
      }
      if (strcmp(name, "+") == 0) {
        Value r = buildDspExpr(args, ctx, jit_ctx);
        return ctx.b.create<arith::AddFOp>(ctx.loc, input, r);
      }
    }
  }
  return input;
}

static Value buildDspExpr(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
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
    if (it != ctx.locals.end())
      return it->second;
    fprintf(stderr, "audio_jit: unresolved '%s'\n", name.c_str());
    return {};
  }

  case AST_LET: {
    Value val = buildDspExpr(ast->data.AST_LET.expr, ctx, jit_ctx);
    if (ast->data.AST_LET.binding &&
        ast->data.AST_LET.binding->tag == AST_IDENTIFIER) {
      std::string n(ast->data.AST_LET.binding->data.AST_IDENTIFIER.value,
                    ast->data.AST_LET.binding->data.AST_IDENTIFIER.length);
      if (val)
        ctx.locals[n] = val;
    }
    if (ast->data.AST_LET.in_expr)
      return buildDspExpr(ast->data.AST_LET.in_expr, ctx, jit_ctx);
    return val;
  }

  case AST_BODY: {
    Value last{};
    for (AstList *l = ast->data.AST_BODY.stmts; l; l = l->next)
      last = buildDspExpr(l->ast, ctx, jit_ctx);
    return last;
  }

  case AST_APPLICATION: {
    Ast *fn = ast->data.AST_APPLICATION.function;
    Ast *args = ast->data.AST_APPLICATION.args;
    size_t nargs = ast->data.AST_APPLICATION.len;
    if (fn->tag != AST_IDENTIFIER)
      break;
    const char *name = fn->data.AST_IDENTIFIER.value;

    if (strcmp(name, "|>") == 0 && nargs >= 2)
      return buildDspPipe(&args[0], &args[1], ctx, jit_ctx);

    // dsp.inlet N
    if (strcmp(name, "inlet") == 0 && nargs >= 1) {
      int idx = (args[0].tag == AST_INT) ? args[0].data.AST_INT.value
                                         : (int)args[0].data.AST_DOUBLE.value;
      return b.create<InletOp>(loc, ctx.inputs_ptr, ctx.frame_idx, idx)
          ->getResult(0);
    }

    // dsp.osc freq — allocates 8 bytes of state (one f64 phase)
    if (strcmp(name, "osc") == 0 && nargs >= 1) {
      Value freq = buildDspExpr(&args[0], ctx, jit_ctx);
      int off = ctx.state_offset;
      ctx.state_offset += 8;
      return b.create<OscOp>(loc, ctx.state_ptr, freq, ctx.spf, off)
          ->getResult(0);
    }

    // dsp.bufplay buf rate start trig — 16 bytes state (phase + prev_trig)
    if (strcmp(name, "bufplayer_trig_node") == 0 && nargs >= 4) {
      Value rate = buildDspExpr(&args[1], ctx, jit_ctx);
      Value trig = buildDspExpr(&args[3], ctx, jit_ctx);
      int off = ctx.state_offset;
      ctx.state_offset += 16;
      return b.create<BufplayOp>(loc, ctx.state_ptr, rate, trig, off)
          ->getResult(0);
    }

    // dsp.env_aslr a sl sd r trig — 12 bytes state (phase + stage)
    if (strcmp(name, "aslr_node") == 0 && nargs >= 5) {
      Value attack = buildDspExpr(&args[0], ctx, jit_ctx);
      Value sus_lvl = buildDspExpr(&args[1], ctx, jit_ctx);
      Value sus_dur = buildDspExpr(&args[2], ctx, jit_ctx);
      Value rel = buildDspExpr(&args[3], ctx, jit_ctx);
      Value trig = buildDspExpr(&args[4], ctx, jit_ctx);
      int off = ctx.state_offset;
      ctx.state_offset += 12;
      return b
          .create<EnvAslrOp>(loc, ctx.state_ptr, attack, sus_lvl, sus_dur, rel,
                             trig, off)
          ->getResult(0);
    }

    if (strcmp(name, "*") == 0 && nargs == 2) {
      Value l = buildDspExpr(&args[0], ctx, jit_ctx);
      Value r = buildDspExpr(&args[1], ctx, jit_ctx);
      return b.create<arith::MulFOp>(loc, l, r);
    }
    if (strcmp(name, "+") == 0 && nargs == 2) {
      Value l = buildDspExpr(&args[0], ctx, jit_ctx);
      Value r = buildDspExpr(&args[1], ctx, jit_ctx);
      return b.create<arith::AddFOp>(loc, l, r);
    }
    break;
  }
  default:
    break;
  }
  return {};
}
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
// Build the MLIR module: LLVM perform function wrapping an scf.for over
// nframes, body filled with DSP ops.  Returns {module, total_state_bytes}.
// =============================================================================

static std::pair<OwningOpRef<ModuleOp>, int>
build_dsp_module(Ast *lambda, JITLangCtx *jit_ctx, MLIRContext *mlir_ctx) {
  auto loc = ast_loc(lambda, mlir_ctx);
  OwningOpRef<ModuleOp> module_ref = ModuleOp::create(loc);
  ModuleOp mod = *module_ref;

  OpBuilder b(mlir_ctx);

  // perform_func_t: void *perform(Node*, void*, Node**, int nframes, double
  // spf)
  auto ptr_ty = LLVM::LLVMPointerType::get(mlir_ctx);
  auto i32_ty = b.getI32Type();
  auto f64_ty = b.getF64Type();
  auto void_ty = LLVM::LLVMVoidType::get(mlir_ctx);
  (void)void_ty;

  auto fn_type = LLVM::LLVMFunctionType::get(
      ptr_ty, {ptr_ty, ptr_ty, ptr_ty, i32_ty, f64_ty}, false);

  b.setInsertionPointToEnd(mod.getBody());
  auto fn = b.create<LLVM::LLVMFuncOp>(loc, "audio_synth_perform", fn_type);
  auto *entry = fn.addEntryBlock(b);
  b.setInsertionPointToStart(entry);

  DspBuildCtx ctx{b,
                  mod,
                  loc,
                  fn.getArgument(0), // node_ptr
                  fn.getArgument(1), // state_ptr
                  fn.getArgument(2), // inputs_ptr
                  fn.getArgument(4), // spf
                  Value{},           // frame_idx — set inside the loop
                  0};

  // scf.for %i = 0 to nframes step 1
  Value zero = b.create<arith::ConstantIndexOp>(loc, 0);
  Value one = b.create<arith::ConstantIndexOp>(loc, 1);
  Value nframes =
      b.create<arith::IndexCastOp>(loc, b.getIndexType(), fn.getArgument(3));

  int state_bytes = 0;
  b.create<scf::ForOp>(loc, zero, nframes, one, ValueRange{},
                       [&](OpBuilder &lb, Location ll, Value iv, ValueRange) {
                         ctx.frame_idx = iv;
                         ctx.b = lb;

                         Value result = buildDspExpr(
                             lambda->data.AST_LAMBDA.body, ctx, jit_ctx);
                         state_bytes = ctx.state_offset;

                         if (result)
                           ctx.b.create<OutletOp>(ll, ctx.node_ptr, iv, result);

                         ctx.b.create<scf::YieldOp>(ll);
                       });

  // Return node->output.buf via the helper.
  auto get_buf_ty = LLVM::LLVMFunctionType::get(ptr_ty, {ptr_ty}, false);
  auto get_buf_fn = declareExtern(mod, b, "ylc_get_output_buf", get_buf_ty);
  Value out_buf =
      b.create<LLVM::CallOp>(loc, get_buf_fn, ValueRange{ctx.node_ptr})
          .getResult();
  b.create<LLVM::ReturnOp>(loc, ValueRange{out_buf});

  return {std::move(module_ref), state_bytes};
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
    auto fn = declareExtern(mod, r, "ylc_read_inlet", fn_ty);

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
    auto fn = declareExtern(mod, r, "ylc_write_output", fn_ty);

    Value frame = r.create<arith::IndexCastOp>(loc, i64, operands[1]);
    r.replaceOpWithNewOp<LLVM::CallOp>(
        op, fn, ValueRange{operands[0], frame, operands[2]});
    return success();
  }
};

// OscOp: GEP into state at offset, load phase, advance, store, return
// sin(phase*2π).
struct OscOpLowering : public ConversionPattern {
  OscOpLowering(MLIRContext *ctx)
      : ConversionPattern(OscOp::getOperationName(), 1, ctx) {}
  LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                ConversionPatternRewriter &r) const override {
    auto osc = cast<OscOp>(op);
    auto loc = op->getLoc();
    auto ptr = LLVM::LLVMPointerType::get(op->getContext());
    auto f64 = r.getF64Type();
    auto i64 = r.getI64Type();

    // GEP: state_ptr + state_offset → &phase
    Value off_val = r.create<LLVM::ConstantOp>(
        loc, i64, r.getI64IntegerAttr(osc.getStateOffset()));
    Value phase_ptr = r.create<LLVM::GEPOp>(loc, ptr, r.getI8Type(),
                                            operands[0], ValueRange{off_val});

    // Load current phase
    Value phase = r.create<LLVM::LoadOp>(loc, f64, phase_ptr);

    // phase += freq * spf
    Value step = r.create<LLVM::FMulOp>(loc, operands[1], operands[2]);
    Value new_phase = r.create<LLVM::FAddOp>(loc, phase, step);
    r.create<LLVM::StoreOp>(loc, new_phase, phase_ptr);

    // sin(phase * 2π) via llvm.intr.sin
    // TODO: multiply by 2π, call @llvm.sin.f64
    // For now, return phase directly as a placeholder.
    r.replaceOp(op, new_phase);
    return success();
  }
};

// BufplayOp and EnvAslrOp follow the same GEP-load-update-store pattern.
// TODO: implement interpolated table read (bufplay) and phase-state-machine
// (env_aslr).
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
    patterns.add<InletOpLowering, OutletOpLowering, OscOpLowering,
                 BufplayOpLowering, EnvAslrOpLowering>(&getContext());

    if (failed(applyPartialConversion(getOperation(), target,
                                      std::move(patterns))))
      signalPassFailure();
  }
};

// =============================================================================
// Pass pipelines
// =============================================================================

static LogicalResult runMLIRPasses(ModuleOp mod, MLIRContext *ctx) {
  PassManager pm(ctx);
  pm.addPass(std::make_unique<DspToLLVMPass>());    // dsp.* → LLVM dialect
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

extern "C" LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *jit_ctx,
                                              LLVMModuleRef module_ref,
                                              LLVMBuilderRef) {
  Ast *lambda = ast->data.AST_APPLICATION.args;
  if (!lambda || lambda->tag != AST_LAMBDA) {
    fprintf(stderr, "compile_audio_fn: expected fn () -> ...\n");
    return LLVMConstInt(LLVMInt32Type(), 0, 0);
  }

  MLIRContext *mlir_ctx = get_mlir_ctx();
  auto [mlir_mod, state_bytes] = build_dsp_module(lambda, jit_ctx, mlir_ctx);
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

  fprintf(stderr, "compile_audio_fn: OK (state=%d bytes)\n", state_bytes);
  return LLVMGetNamedFunction(module_ref, "audio_synth_perform");
}

// =============================================================================
// Library constructor
// =============================================================================

__attribute__((constructor)) static void ylc_audio_jit_init() {
  if (!ylc_jit_ctx) {
    fprintf(stderr, "libaudio_jit: no JIT context at load time\n");
    return;
  }
  ht *stack = ylc_jit_ctx->frame->table;
  JITSymbol *sym =
      new_symbol(STYPE_GENERIC_FUNCTION, nullptr, nullptr, nullptr);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =
      CompileAudioFnHandler;
  const char *name = "compile_audio_fn";
  ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
  fprintf(stderr, "libaudio_jit: registered compile_audio_fn\n");
}
