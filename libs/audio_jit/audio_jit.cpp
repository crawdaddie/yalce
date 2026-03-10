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

#include "codegen.h"
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

// =============================================================================
// Wavetable data — embedded at compile time from pre-generated CSV files.
// Avoids any runtime symbol lookup; the address is a true compile-time
// constant.
// =============================================================================
//
#define GRAIN_WINDOW_TABSIZE (1 << 9)
extern const double sin_table[SIN_TABSIZE] = {
#include "../../engine/assets/sin_table.csv"
};
extern const double sq_table[SQ_TABSIZE] = {
#include "../../engine/assets/sq_table.csv"
};
extern const double saw_table[SAW_TABSIZE] = {
#include "../../engine/assets/saw_table.csv"
};
static const double grain_win[GRAIN_WINDOW_TABSIZE] = {
#include "../../engine/assets/grain_win.csv"
};

int STYPE_AUDIO_JIT_SYM;
int STYPE_AUDIO_JIT_INLINE_SYM;
int STYPE_AUDIO_JIT_BUILTIN_HANDLER;

using namespace mlir;

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

Value build_dsp_expr(Ast *ast, DspBuildCtx &ctx, JITLangCtx *jit_ctx) {
  if (!ast)
    return {};
  auto &b = ctx.b;
  auto loc = ctx.loc;

  switch (ast->tag) {

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
  std::vector<HoistableCall> hoistable_calls;
  std::vector<ArrayInitSpec> array_inits;
  std::vector<DynamicDelayAllocSpec> dynamic_delay_allocs;
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

  // Count real (identifier) inputs so hidden inlets start after them.
  int num_real_inputs = 0;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {

    if (p->ast->tag == AST_IDENTIFIER) {
      num_real_inputs++;
    } else if (p->ast->tag == AST_TUPLE) {
      num_real_inputs += p->ast->data.AST_LIST.len;
    }
  }

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
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    Ast *param = p->ast;
    if (param->tag == AST_TUPLE) {
      for (int i = 0; i < param->data.AST_LIST.len; i++) {
        Ast *p = param->data.AST_LIST.items + i;
        printf("tuple param\n");
        print_ast(p);
        if (p->tag == AST_LET) {
          Ast *b = p->data.AST_LET.binding;

          std::string pname(b->data.AST_IDENTIFIER.value,
                            b->data.AST_IDENTIFIER.length);
          ctx.locals[pname] =
              ctx.b
                  .create<InletOp>(loc, ctx.inputs_ptr,
                                   for_op.getInductionVar(), inlet_idx)
                  ->getResult(0);
        } else if (p->tag == AST_IDENTIFIER) {
          std::string pname(p->data.AST_IDENTIFIER.value,
                            p->data.AST_IDENTIFIER.length);
          ctx.locals[pname] =
              ctx.b
                  .create<InletOp>(loc, ctx.inputs_ptr,
                                   for_op.getInductionVar(), inlet_idx)
                  ->getResult(0);
        }

        inlet_idx++;
      }
      continue;
    }
    if (param->tag != AST_IDENTIFIER) {
      continue;
    }
    std::string pname(param->data.AST_IDENTIFIER.value,
                      param->data.AST_IDENTIFIER.length);
    ctx.locals[pname] =
        ctx.b
            .create<InletOp>(loc, ctx.inputs_ptr, for_op.getInductionVar(),
                             inlet_idx)
            ->getResult(0);
    inlet_idx++;
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

  return {std::move(module_ref),    state_bytes,
          ctx.hoistable_calls,      ctx.array_inits,
          ctx.dynamic_delay_allocs, ctx.buf_ref_inputs};
}

// =============================================================================
// Pass pipelines
// =============================================================================

static LogicalResult runMLIRPasses(ModuleOp mod, MLIRContext *ctx) {
  PassManager pm(ctx);
  pm.addPass(createDspToLLVMPass());        // dsp.* → LLVM dialect
  pm.addPass(createCanonicalizerPass());    // fold/canonicalize early
  pm.addPass(createCSEPass());              // dedup repeated constants/ops
  pm.addPass(createSCFToControlFlowPass()); // scf.for → cf
  pm.addPass(createConvertControlFlowToLLVMPass()); // cf → llvm
  pm.addPass(createArithToLLVMConversionPass());    // arith.* → llvm
  pm.addPass(createCanonicalizerPass());            // cleanup after lowering
  pm.addPass(createCSEPass());                      // remove new duplicates
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

LLVMValueRef build_ctor_fn(int synth_id, std::string perform_name,
                           DspModuleResult &result, int num_inputs,
                           JITLangCtx *jit_ctx, LLVMModuleRef module_ref,
                           LLVMBuilderRef builder) {

  auto &mlir_mod = result.mod;
  int state_bytes = result.state_bytes;

  std::string ctor_name = "synth_ctor_" + std::to_string(synth_id);

  int num_total_inputs = num_inputs + (int)result.buf_ref_inputs.size();

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef i8_ty = LLVMInt8TypeInContext(llvm_ctx);
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef void_ty = LLVMVoidType();

  // YLC Array of Doubles: {i32 size, ptr data}
  LLVMTypeRef arr_fields[] = {i32_ty, ptr_ty};
  LLVMTypeRef ylc_arr_ty = LLVMStructTypeInContext(llvm_ctx, arr_fields, 2, 0);

  // --- Emit a named constructor function: Node* synth_ctor_N({i32, ptr}
  // params) All node-creation logic lives inside so callers can instantiate the
  // synth multiple times with different parameter arrays.
  LLVMTypeRef ctor_param_tys[] = {ylc_arr_ty};
  LLVMTypeRef ctor_fn_ty = LLVMFunctionType(ptr_ty, ctor_param_tys, 1, 0);
  LLVMValueRef ctor_fn =
      LLVMAddFunction(module_ref, ctor_name.c_str(), ctor_fn_ty);

  LLVMSetLinkage(ctor_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef ctor_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, ctor_fn, "entry");
  LLVMBuilderRef ctor_b = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMPositionBuilderAtEnd(ctor_b, ctor_bb);

  LLVMValueRef ctor_arg0 = LLVMGetParam(ctor_fn, 0);
  LLVMValueRef params_size =
      LLVMBuildExtractValue(ctor_b, ctor_arg0, 0, "params_size");
  LLVMValueRef params_data =
      LLVMBuildExtractValue(ctor_b, ctor_arg0, 1, "params_data");

  LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
  LLVMValueRef params_data_f64 =
      LLVMBuildBitCast(ctor_b, params_data, f64_ptr_ty, "params_data_f64");

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

  LLVMValueRef dsp_fn = LLVMGetNamedFunction(module_ref, perform_name.c_str());
  // LLVMDumpValue(dsp_fn);
  // printf("\n");
  LLVMValueRef state_bytes_val = LLVMConstInt(i32_ty, state_bytes, 0);

  std::vector<LLVMValueRef> dyn_base_vals;
  std::vector<LLVMValueRef> dyn_len_vals;

  if (!result.dynamic_delay_allocs.empty()) {
    LLVMTypeRef spf_fn_ty = LLVMFunctionType(f64_ty, nullptr, 0, 0);
    LLVMValueRef spf_fn = LLVMGetNamedFunction(module_ref, "ctx_spf");
    if (!spf_fn) {
      spf_fn = LLVMAddFunction(module_ref, "ctx_spf", spf_fn_ty);
      LLVMSetLinkage(spf_fn, LLVMExternalLinkage);
    }
    LLVMValueRef spf_raw =
        LLVMBuildCall2(ctor_b, spf_fn_ty, spf_fn, nullptr, 0, "ctx_spf");
    LLVMValueRef zero_f = LLVMConstReal(f64_ty, 0.0);
    LLVMValueRef spf_ok =
        LLVMBuildFCmp(ctor_b, LLVMRealOGT, spf_raw, zero_f, "spf_ok");
    LLVMValueRef spf_val = LLVMBuildSelect(
        ctor_b, spf_ok, spf_raw, LLVMConstReal(f64_ty, 1.0 / 48000.0), "spf");

    LLVMTypeRef floor_param_tys[] = {f64_ty};
    LLVMTypeRef floor_fn_ty = LLVMFunctionType(f64_ty, floor_param_tys, 1, 0);
    LLVMValueRef floor_fn = LLVMGetNamedFunction(module_ref, "llvm.floor.f64");
    if (!floor_fn) {
      floor_fn = LLVMAddFunction(module_ref, "llvm.floor.f64", floor_fn_ty);
      LLVMSetLinkage(floor_fn, LLVMExternalLinkage);
    }

    LLVMValueRef cur_base = LLVMConstInt(i64_ty, (uint64_t)state_bytes, 0);
    LLVMValueRef dyn_total = LLVMConstInt(i64_ty, 0, 0);
    LLVMValueRef two_i64 = LLVMConstInt(i64_ty, 2, 0);
    LLVMValueRef eight_i64 = LLVMConstInt(i64_ty, 8, 0);

    for (auto &d : result.dynamic_delay_allocs) {
      LLVMValueRef idx_i32 = LLVMConstInt(i32_ty, d.inlet_idx, 0);
      LLVMValueRef has_param =
          LLVMBuildICmp(ctor_b, LLVMIntSGT, params_size, idx_i32, "has_param");
      LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)d.inlet_idx, 0);
      LLVMValueRef param_ptr = LLVMBuildGEP2(ctor_b, f64_ty, params_data_f64,
                                             &idx_i64, 1, "param_ptr");
      LLVMValueRef param_sec =
          LLVMBuildLoad2(ctor_b, f64_ty, param_ptr, "param");
      LLVMValueRef sec =
          LLVMBuildSelect(ctor_b, has_param, param_sec,
                          LLVMConstReal(f64_ty, 0.0), "delay_sec");

      LLVMValueRef samples_f = LLVMBuildFDiv(ctor_b, sec, spf_val, "samples_f");
      LLVMValueRef samples_floor = LLVMBuildCall2(
          ctor_b, floor_fn_ty, floor_fn, &samples_f, 1, "samples_floor");
      LLVMValueRef len_raw =
          LLVMBuildFPToSI(ctor_b, samples_floor, i64_ty, "delay_len_raw");
      LLVMValueRef len_lt_2 =
          LLVMBuildICmp(ctor_b, LLVMIntSLT, len_raw, two_i64, "len_lt_2");
      LLVMValueRef len =
          LLVMBuildSelect(ctor_b, len_lt_2, two_i64, len_raw, "delay_len");
      LLVMValueRef bytes = LLVMBuildMul(ctor_b, len, eight_i64, "delay_bytes");

      dyn_base_vals.push_back(cur_base);
      dyn_len_vals.push_back(len);
      dyn_total = LLVMBuildAdd(ctor_b, dyn_total, bytes, "dyn_total");
      cur_base = LLVMBuildAdd(ctor_b, cur_base, bytes, "cur_base");
    }

    LLVMValueRef total_i64 =
        LLVMBuildAdd(ctor_b, LLVMConstInt(i64_ty, (uint64_t)state_bytes, 0),
                     dyn_total, "state_bytes_i64");
    state_bytes_val = LLVMBuildTrunc(ctor_b, total_i64, i32_ty, "state_bytes");
  }

  LLVMValueRef create_args[] = {
      dsp_fn,
      LLVMConstInt(i32_ty, num_total_inputs, 0),
      state_bytes_val,
  };
  LLVMValueRef node_val =
      LLVMBuildCall2(ctor_b, create_fn_ty, create_fn, create_args, 3, "node");

  if (!result.dynamic_delay_allocs.empty()) {
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
        ctor_b, get_state_fn_ty, get_state_fn, &node_val, 1, "state_ptr");

    LLVMTypeRef zero_fn_tys[] = {ptr_ty, i64_ty, i64_ty};
    LLVMTypeRef zero_fn_ty = LLVMFunctionType(void_ty, zero_fn_tys, 3, 0);
    LLVMValueRef zero_fn =
        LLVMGetNamedFunction(module_ref, "ylc_zero_state_range");
    if (!zero_fn) {
      zero_fn = LLVMAddFunction(module_ref, "ylc_zero_state_range", zero_fn_ty);
      LLVMSetLinkage(zero_fn, LLVMExternalLinkage);
    }
  }

  // Wire real (lambda) inlets from ctor params via ylc_const_inlet.
  if (num_inputs > 0) {
    LLVMTypeRef const_inlet_params[] = {f64_ty};
    LLVMTypeRef const_inlet_fn_ty =
        LLVMFunctionType(ptr_ty, const_inlet_params, 1, 0);
    LLVMValueRef const_inlet_fn =
        LLVMGetNamedFunction(module_ref, "ylc_const_inlet");
    if (!const_inlet_fn) {
      const_inlet_fn =
          LLVMAddFunction(module_ref, "ylc_const_inlet", const_inlet_fn_ty);
      LLVMSetLinkage(const_inlet_fn, LLVMExternalLinkage);
    }

    LLVMTypeRef attach_params[] = {i32_ty, ptr_ty, ptr_ty};
    LLVMTypeRef attach_fn_ty = LLVMFunctionType(void_ty, attach_params, 3, 0);
    LLVMValueRef attach_fn =
        LLVMGetNamedFunction(module_ref, "plug_input_in_graph");
    if (!attach_fn) {
      attach_fn =
          LLVMAddFunction(module_ref, "plug_input_in_graph", attach_fn_ty);
      LLVMSetLinkage(attach_fn, LLVMExternalLinkage);
    }

    for (int i = 0; i < num_inputs; i++) {
      LLVMValueRef idx_i32 = LLVMConstInt(i32_ty, i, 0);
      LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, i, 0);
      LLVMValueRef elem_ptr = LLVMBuildGEP2(ctor_b, f64_ty, params_data_f64,
                                            &idx_i64, 1, "param_ptr");
      LLVMValueRef param_val =
          LLVMBuildLoad2(ctor_b, f64_ty, elem_ptr, "param");
      LLVMValueRef const_node =
          LLVMBuildCall2(ctor_b, const_inlet_fn_ty, const_inlet_fn, &param_val,
                         1, "const_node");
      LLVMValueRef args[] = {idx_i32, node_val, const_node};
      LLVMBuildCall2(ctor_b, attach_fn_ty, attach_fn, args, 3, "");
    }
  }

  // Wire external buffer-ref inlets (existing nodes plugged as hidden inputs).
  // Top-level vars have globally-allocated storage accessible from any
  // function. For locals captured at definition time, spill to a module-level
  // global so the ctor can load them without referencing outer-function SSA
  // values.
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
      LLVMValueRef source;
      if (bri.sym->type == STYPE_TOP_LEVEL_VAR && bri.sym->storage) {
        // Global storage — directly loadable from inside the ctor.
        source = LLVMBuildLoad2(ctor_b, ptr_ty, bri.sym->storage, "buf_node");
      } else {
        // Local SSA value — spill to a module global at definition time so the
        // ctor can load it without crossing function boundaries.
        std::string cap_name =
            ctor_name + "_cap_" + std::to_string(bri.inlet_idx);
        LLVMValueRef cap_global =
            LLVMAddGlobal(module_ref, ptr_ty, cap_name.c_str());
        LLVMSetLinkage(cap_global, LLVMInternalLinkage);
        LLVMSetInitializer(cap_global, LLVMConstNull(ptr_ty));
        LLVMBuildStore(builder, bri.sym->val, cap_global);
        source = LLVMBuildLoad2(ctor_b, ptr_ty, cap_global, "buf_node");
      }
      LLVMValueRef args[] = {LLVMConstInt(i32_ty, bri.inlet_idx, 0), node_val,
                             source};
      LLVMBuildCall2(ctor_b, attach_fn_ty, attach_fn, args, 3, "");
    }
  }

  LLVMBuildRet(ctor_b, node_val);
  LLVMDisposeBuilder(ctor_b);
  // LLVMDumpValue(ctor_fn);

  // Return the constructor function — callers invoke it with a YLC Array of
  // Doubles to create a Node*.
  return ctor_fn;
}

// Build a JIT function: i32 synth_inlet_index_N({i32 len, ptr data} str)
// Returns the inlet index for a named real (lambda) parameter, or -1 if not
// found. The name→index mapping is baked in at compile time via a memcmp chain.
static LLVMValueRef
build_inlet_index_fn(int synth_id, const std::vector<std::string> &param_names,
                     LLVMModuleRef module_ref) {
  std::string fn_name = "synth_inlet_index_" + std::to_string(synth_id);

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef i8_ty = LLVMInt8TypeInContext(llvm_ctx);
  LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef i64_ty = LLVMInt64TypeInContext(llvm_ctx);

  // YLC string: {i32 len, ptr data}
  LLVMTypeRef str_fields[] = {i32_ty, ptr_ty};
  LLVMTypeRef ylc_str_ty = LLVMStructTypeInContext(llvm_ctx, str_fields, 2, 0);

  LLVMTypeRef param_tys[] = {ylc_str_ty};
  LLVMTypeRef fn_ty = LLVMFunctionType(i32_ty, param_tys, 1, 0);
  LLVMValueRef fn = LLVMAddFunction(module_ref, fn_name.c_str(), fn_ty);
  LLVMSetLinkage(fn, LLVMExternalLinkage);

  LLVMBuilderRef b = LLVMCreateBuilderInContext(llvm_ctx);

  // entry: unpack the {i32, ptr} string argument
  LLVMBasicBlockRef entry_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, fn, "entry");
  LLVMPositionBuilderAtEnd(b, entry_bb);

  LLVMValueRef str_arg = LLVMGetParam(fn, 0);
  LLVMValueRef len_val = LLVMBuildExtractValue(b, str_arg, 0, "len");
  LLVMValueRef data_ptr = LLVMBuildExtractValue(b, str_arg, 1, "data");

  // declare memcmp: i32 memcmp(ptr, ptr, i64)
  LLVMTypeRef memcmp_param_tys[] = {ptr_ty, ptr_ty, i64_ty};
  LLVMTypeRef memcmp_ty = LLVMFunctionType(i32_ty, memcmp_param_tys, 3, 0);
  LLVMValueRef memcmp_fn = LLVMGetNamedFunction(module_ref, "memcmp");
  if (!memcmp_fn) {
    memcmp_fn = LLVMAddFunction(module_ref, "memcmp", memcmp_ty);
    LLVMSetLinkage(memcmp_fn, LLVMExternalLinkage);
  }

  // not_found: return -1
  LLVMBasicBlockRef not_found_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, fn, "not_found");
  LLVMPositionBuilderAtEnd(b, not_found_bb);
  LLVMBuildRet(b, LLVMConstInt(i32_ty, (unsigned long long)-1, 0));

  // build check chain starting from entry_bb
  LLVMPositionBuilderAtEnd(b, entry_bb);

  for (int i = 0; i < (int)param_names.size(); i++) {
    const std::string &name = param_names[i];

    // private global holding the param name bytes (no null terminator needed)
    std::string gname =
        "__pname_" + std::to_string(synth_id) + "_" + std::to_string(i);
    LLVMTypeRef arr_ty = LLVMArrayType(i8_ty, (unsigned)name.size());
    LLVMValueRef str_global = LLVMAddGlobal(module_ref, arr_ty, gname.c_str());
    LLVMSetInitializer(str_global,
                       LLVMConstString(name.c_str(), (unsigned)name.size(), 1));
    LLVMSetLinkage(str_global, LLVMPrivateLinkage);
    LLVMSetGlobalConstant(str_global, 1);

    // check length first
    LLVMValueRef expected_len = LLVMConstInt(i32_ty, name.size(), 0);
    LLVMValueRef len_ok =
        LLVMBuildICmp(b, LLVMIntEQ, len_val, expected_len, "len_ok");

    LLVMBasicBlockRef cmp_bb = LLVMAppendBasicBlockInContext(
        llvm_ctx, fn, ("cmp_" + std::to_string(i)).c_str());
    LLVMBasicBlockRef next_bb =
        (i + 1 < (int)param_names.size())
            ? LLVMAppendBasicBlockInContext(
                  llvm_ctx, fn, ("next_" + std::to_string(i)).c_str())
            : not_found_bb;

    LLVMBuildCondBr(b, len_ok, cmp_bb, next_bb);

    // cmp: memcmp the data bytes
    LLVMPositionBuilderAtEnd(b, cmp_bb);
    LLVMValueRef memcmp_args[] = {data_ptr, str_global,
                                  LLVMConstInt(i64_ty, name.size(), 0)};
    LLVMValueRef cmp_result =
        LLVMBuildCall2(b, memcmp_ty, memcmp_fn, memcmp_args, 3, "cmp");
    LLVMValueRef is_match = LLVMBuildICmp(b, LLVMIntEQ, cmp_result,
                                          LLVMConstInt(i32_ty, 0, 0), "match");

    LLVMBasicBlockRef found_bb = LLVMAppendBasicBlockInContext(
        llvm_ctx, fn, ("found_" + std::to_string(i)).c_str());
    LLVMBuildCondBr(b, is_match, found_bb, next_bb);

    LLVMPositionBuilderAtEnd(b, found_bb);
    LLVMBuildRet(b, LLVMConstInt(i32_ty, i, 0));

    if (next_bb != not_found_bb)
      LLVMPositionBuilderAtEnd(b, next_bb);
  }

  if (param_names.empty())
    LLVMBuildBr(b, not_found_bb);

  LLVMDisposeBuilder(b);
  return fn;
}

// Build: ptr synth_set_input_scalar_N(ptr synth, {i32,ptr} name, i64 fo, f64
// val) Resolves name→index via inlet_index_fn, then calls
// set_input_scalar_offset.
static LLVMValueRef build_set_input_scalar_fn(int synth_id,
                                              LLVMValueRef inlet_index_fn,
                                              LLVMModuleRef module_ref) {
  std::string fn_name = "synth_set_input_scalar_" + std::to_string(synth_id);

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef i64_ty = LLVMInt64TypeInContext(llvm_ctx);
  LLVMTypeRef f64_ty = LLVMDoubleTypeInContext(llvm_ctx);

  LLVMTypeRef str_fields[] = {i32_ty, ptr_ty};
  LLVMTypeRef ylc_str_ty = LLVMStructTypeInContext(llvm_ctx, str_fields, 2, 0);

  LLVMTypeRef param_tys[] = {ylc_str_ty, i64_ty, f64_ty, ptr_ty};
  LLVMTypeRef fn_ty = LLVMFunctionType(ptr_ty, param_tys, 4, 0);
  LLVMValueRef fn = LLVMAddFunction(module_ref, fn_name.c_str(), fn_ty);
  LLVMSetLinkage(fn, LLVMExternalLinkage);

  LLVMBuilderRef b = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMBasicBlockRef entry_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, fn, "entry");
  LLVMPositionBuilderAtEnd(b, entry_bb);

  LLVMValueRef name_arg = LLVMGetParam(fn, 0);
  LLVMValueRef fo_arg = LLVMGetParam(fn, 1);
  LLVMValueRef val_arg = LLVMGetParam(fn, 2);
  LLVMValueRef synth_arg = LLVMGetParam(fn, 3);

  // resolve name → inlet index
  LLVMTypeRef idx_fn_param_tys[] = {ylc_str_ty};
  LLVMTypeRef idx_fn_ty = LLVMFunctionType(i32_ty, idx_fn_param_tys, 1, 0);
  LLVMValueRef idx_args[] = {name_arg};
  LLVMValueRef idx =
      LLVMBuildCall2(b, idx_fn_ty, inlet_index_fn, idx_args, 1, "idx");

  // declare set_input_scalar_offset: ptr (ptr, i32, i64, f64)
  LLVMTypeRef siso_param_tys[] = {ptr_ty, i32_ty, i64_ty, f64_ty};
  LLVMTypeRef siso_ty = LLVMFunctionType(ptr_ty, siso_param_tys, 4, 0);
  LLVMValueRef siso_fn =
      LLVMGetNamedFunction(module_ref, "set_input_scalar_offset");
  if (!siso_fn) {
    siso_fn = LLVMAddFunction(module_ref, "set_input_scalar_offset", siso_ty);
    LLVMSetLinkage(siso_fn, LLVMExternalLinkage);
  }

  LLVMValueRef siso_args[] = {synth_arg, idx, fo_arg, val_arg};
  LLVMValueRef result =
      LLVMBuildCall2(b, siso_ty, siso_fn, siso_args, 4, "result");
  LLVMBuildRet(b, result);

  LLVMDisposeBuilder(b);
  return fn;
}

// Build: ptr synth_set_input_trig_N(ptr synth, {i32,ptr} name, i64 fo)
// Resolves name→index via inlet_index_fn, then calls set_input_trig_offset.
static LLVMValueRef build_set_input_trig_fn(int synth_id,
                                            LLVMValueRef inlet_index_fn,
                                            LLVMModuleRef module_ref) {
  std::string fn_name = "synth_set_input_trig_" + std::to_string(synth_id);

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef i64_ty = LLVMInt64TypeInContext(llvm_ctx);

  LLVMTypeRef str_fields[] = {i32_ty, ptr_ty};
  LLVMTypeRef ylc_str_ty = LLVMStructTypeInContext(llvm_ctx, str_fields, 2, 0);

  LLVMTypeRef param_tys[] = {ylc_str_ty, i64_ty, ptr_ty};
  LLVMTypeRef fn_ty = LLVMFunctionType(ptr_ty, param_tys, 3, 0);
  LLVMValueRef fn = LLVMAddFunction(module_ref, fn_name.c_str(), fn_ty);
  LLVMSetLinkage(fn, LLVMExternalLinkage);

  LLVMBuilderRef b = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMBasicBlockRef entry_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, fn, "entry");
  LLVMPositionBuilderAtEnd(b, entry_bb);

  LLVMValueRef name_arg = LLVMGetParam(fn, 0);
  LLVMValueRef fo_arg = LLVMGetParam(fn, 1);
  LLVMValueRef synth_arg = LLVMGetParam(fn, 2);

  // resolve name → inlet index
  LLVMTypeRef idx_fn_param_tys[] = {ylc_str_ty};
  LLVMTypeRef idx_fn_ty = LLVMFunctionType(i32_ty, idx_fn_param_tys, 1, 0);
  LLVMValueRef idx_args[] = {name_arg};
  LLVMValueRef idx =
      LLVMBuildCall2(b, idx_fn_ty, inlet_index_fn, idx_args, 1, "idx");

  // declare set_input_trig_offset: ptr (ptr, i32, i64)
  LLVMTypeRef sito_param_tys[] = {ptr_ty, i32_ty, i64_ty};
  LLVMTypeRef sito_ty = LLVMFunctionType(ptr_ty, sito_param_tys, 3, 0);
  LLVMValueRef sito_fn =
      LLVMGetNamedFunction(module_ref, "set_input_trig_offset");
  if (!sito_fn) {
    sito_fn = LLVMAddFunction(module_ref, "set_input_trig_offset", sito_ty);
    LLVMSetLinkage(sito_fn, LLVMExternalLinkage);
  }

  LLVMValueRef sito_args[] = {synth_arg, idx, fo_arg};
  LLVMValueRef result =
      LLVMBuildCall2(b, sito_ty, sito_fn, sito_args, 3, "result");
  LLVMBuildRet(b, result);

  LLVMDisposeBuilder(b);
  return fn;
}

extern "C" LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *jit_ctx,
                                              LLVMModuleRef module_ref,
                                              LLVMBuilderRef builder) {
  Ast *lambda = ast->data.AST_APPLICATION.args;
  if (!lambda || lambda->tag != AST_LAMBDA) {
    fprintf(stderr, "compile_audio_fn: expected fn () -> ...\n");
    return LLVMConstInt(LLVMInt32Type(), 0, 0);
  }
  // printf("compile %s\n", lambda->data.AST_LAMBDA.fn_name.chars);

  // Count real (identifier) params and collect their names in inlet order.
  int num_inputs = 0;
  std::vector<std::string> param_names;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    if (p->ast->tag == AST_IDENTIFIER) {
      param_names.push_back(std::string(p->ast->data.AST_IDENTIFIER.value,
                                        p->ast->data.AST_IDENTIFIER.length));
      num_inputs++;
    } else if (p->ast->tag == AST_TUPLE) {
      for (int i = 0; i < p->ast->data.AST_LIST.len; i++) {
        num_inputs++;
      }
    }
  }

  int synth_id = g_synth_id++;
  std::string fn_name = "synth_perform_" + std::to_string(synth_id);

  MLIRContext *mlir_ctx = get_mlir_ctx();
  auto result = build_dsp_module(lambda, fn_name, jit_ctx, mlir_ctx);
  auto &mlir_mod = result.mod;
  int state_bytes = result.state_bytes;

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

  fprintf(stderr,
          "compile_audio_fn: OK name=%s real_inputs=%d "
          "buf_refs=%d state=%d bytes\n",
          fn_name.c_str(), num_inputs, (int)result.buf_ref_inputs.size(),
          state_bytes);
  fprintf(stderr, "compile_audio_fn: dynamic_delay_allocs=%zu\n",
          result.dynamic_delay_allocs.size());
  for (size_t i = 0; i < result.dynamic_delay_allocs.size(); i++) {
    auto &d = result.dynamic_delay_allocs[i];
    fprintf(stderr, "  dyn[%zu]: inlet=%d ptr_slot=%d len_slot=%d\n", i,
            d.inlet_idx, d.ptr_slot_offset, d.len_slot_offset);
  }

  LLVMValueRef ctor_fn = build_ctor_fn(synth_id, fn_name, result, num_inputs,
                                       jit_ctx, module_ref, builder);

  LLVMValueRef inlet_index_fn =
      build_inlet_index_fn(synth_id, param_names, module_ref);

  LLVMValueRef set_scalar_fn =
      build_set_input_scalar_fn(synth_id, inlet_index_fn, module_ref);

  LLVMValueRef set_trig_fn =
      build_set_input_trig_fn(synth_id, inlet_index_fn, module_ref);

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module_ref);
  LLVMTypeRef ptr_ty = LLVMPointerTypeInContext(llvm_ctx, 0);
  LLVMTypeRef synth_fns_fields[] = {ptr_ty, ptr_ty, ptr_ty, ptr_ty};
  LLVMTypeRef synth_fns_ty =
      LLVMStructTypeInContext(llvm_ctx, synth_fns_fields, 4, 0);
  LLVMValueRef synth_fns[] = {ctor_fn, inlet_index_fn, set_scalar_fn,
                              set_trig_fn};

  return LLVMConstStructInContext(llvm_ctx, synth_fns, 4, 0);
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
  STYPE_AUDIO_JIT_INLINE_SYM = REGISTERED_JIT_SYMBOL_TYPE++;
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

  // ({
  //   JITSymbol *sym =
  //       new_symbol(STYPE_GENERIC_FUNCTION, nullptr, nullptr, nullptr);
  //   sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =
  //       RegisterAudioOpHandler;
  //   const char *name = "register_audio_op";
  //   ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
  //   fprintf(stderr, "libaudio_jit: registered register_audio_op\n");
  // });

#define DSP_BUILTIN(name, handler, fn_type)                                    \
  ({                                                                           \
    JITSymbol *sym = new_symbol((symbol_type)STYPE_AUDIO_JIT_BUILTIN_HANDLER,  \
                                fn_type, nullptr, nullptr);                    \
    sym->symbol_data._USER_DEFINED_SYMBOL = (void *)handler;                   \
    ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);            \
    add_builtin(name, fn_type);                                                \
    fprintf(stderr, "libaudio_jit: registered " name "\n");                    \
  })
}
