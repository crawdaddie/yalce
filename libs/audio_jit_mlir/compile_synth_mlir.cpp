#include <memory>
#include <string>
#include <vector>

// MLIR
#include <mlir/Dialect/Arith/IR/Arith.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/LLVMIR/LLVMDialect.h>
#include <mlir/IR/Builders.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/MLIRContext.h>
#include <mlir/IR/OwningOpRef.h>
#include <mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h>
#include <mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h>
#include <mlir/Target/LLVMIR/Export.h>

// MLIR conversion passes
#include <mlir/Conversion/ArithToLLVM/ArithToLLVM.h>
#include <mlir/Conversion/FuncToLLVM/ConvertFuncToLLVMPass.h>
#include <mlir/Conversion/Passes.h>
#include <mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h>
#include <mlir/Pass/PassManager.h>

// LLVM ORC / passes
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/TargetSelect.h>

// C lang headers after all C++ headers (avoids stdatomic macro conflicts)
extern "C" {
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/common.h"
#include "../../lang/parse.h"
#include "../../lang/types/type_ser.h"
}

#include "./compile_synth_mlir.hpp"
#include "./dsp_build_expr.hpp"
#include "./dsp_dialect.hpp"
#include "./state_plan.hpp"

#include <cstdio>
#include <cstdlib>

// ---------------------------------------------------------------------------
// MLIR context — shared across all compilations.
// ---------------------------------------------------------------------------

static mlir::MLIRContext *g_mlir_ctx = nullptr;

static void ensure_mlir_ctx() {
  if (g_mlir_ctx)
    return;
  g_mlir_ctx = new mlir::MLIRContext();
  g_mlir_ctx->loadDialect<mlir::func::FuncDialect>();
  g_mlir_ctx->loadDialect<mlir::LLVM::LLVMDialect>();
  g_mlir_ctx->loadDialect<mlir::arith::ArithDialect>();
  g_mlir_ctx->loadDialect<DspDialect>();
  mlir::registerBuiltinDialectTranslation(*g_mlir_ctx);
  mlir::registerLLVMDialectTranslation(*g_mlir_ctx);
}

// ---------------------------------------------------------------------------
// LLJIT — one instance for the library lifetime.
// ---------------------------------------------------------------------------

static llvm::orc::LLJIT *g_jit = nullptr;

static void ensure_jit() {
  if (g_jit)
    return;

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  auto jit = llvm::cantFail(llvm::orc::LLJITBuilder().create());

  // Expose all process symbols (ylc_create_audio_node, etc.) to JIT'd code.
  auto &jd = jit->getMainJITDylib();
  jd.addGenerator(llvm::cantFail(
      llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          jit->getDataLayout().getGlobalPrefix())));

  g_jit = jit.release();
}

// ---------------------------------------------------------------------------
// Synth registry
// ---------------------------------------------------------------------------

struct MlirSynthRegistry {
  std::vector<MlirSynthRecord> records;

  int extend(MlirSynthRecord rec) {
    int id = (int)records.size();
    records.push_back(rec);
    return id;
  }

  MlirSynthRecord &operator[](int id) { return records[id]; }
  int len() const { return (int)records.size(); }
};

static MlirSynthRegistry g_registry;

extern "C" void mlir_registry_init() { g_registry.records.clear(); }

extern "C" MlirSynthRecord mlir_registry_get(int id) { return g_registry[id]; }
extern "C" int mlir_registry_len() { return g_registry.len(); }

extern "C" void mlir_registry_set_ctor_ptr(int id, void *ptr) {
  __atomic_store_n(&g_registry[id].ctor_ptr, ptr, __ATOMIC_RELEASE);
}
extern "C" void *mlir_registry_get_ctor_ptr(int id) {
  return __atomic_load_n(&g_registry[id].ctor_ptr, __ATOMIC_ACQUIRE);
}
extern "C" void mlir_registry_set_frame_ptr(int id, void *ptr) {
  __atomic_store_n(&g_registry[id].frame_ptr, ptr, __ATOMIC_RELEASE);
}
extern "C" void *mlir_registry_get_frame_ptr(int id) {
  return __atomic_load_n(&g_registry[id].frame_ptr, __ATOMIC_ACQUIRE);
}

// ---------------------------------------------------------------------------
// Stub body builders — replaced by DSP dialect lowering later.
// All use func dialect with llvm dialect ops so translateModuleToLLVMIR
// works without conversion passes.
// ---------------------------------------------------------------------------

static void build_ctor_stub(mlir::func::FuncOp fn, mlir::OpBuilder &b,
                            mlir::Location loc,
                            mlir::LLVM::LLVMPointerType ptr_ty) {
  fn.addEntryBlock();
  b.setInsertionPointToStart(&fn.getBody().front());
  auto null = b.create<mlir::LLVM::ZeroOp>(loc, ptr_ty);
  b.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{null});
}

static void build_init_stub(mlir::func::FuncOp fn, mlir::OpBuilder &b,
                            mlir::Location loc) {
  fn.addEntryBlock();
  b.setInsertionPointToStart(&fn.getBody().front());
  b.create<mlir::func::ReturnOp>(loc);
}

// frame stub — this is where the DSP dialect body will eventually live.
static void build_frame_stub(mlir::func::FuncOp fn, mlir::OpBuilder &b,
                             mlir::Location loc) {
  fn.addEntryBlock();
  b.setInsertionPointToStart(&fn.getBody().front());
  auto zero = b.create<mlir::arith::ConstantOp>(loc, b.getF64FloatAttr(0.0));
  b.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{zero});
}

static void build_perform_stub(mlir::func::FuncOp fn, mlir::OpBuilder &b,
                               mlir::Location loc,
                               mlir::LLVM::LLVMPointerType ptr_ty) {
  fn.addEntryBlock();
  b.setInsertionPointToStart(&fn.getBody().front());
  auto null = b.create<mlir::LLVM::ZeroOp>(loc, ptr_ty);
  b.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{null});
}

static llvm::FunctionCallee getOrInsertRuntimeFn(llvm::Module &mod,
                                                 llvm::StringRef name,
                                                 llvm::FunctionType *fnTy) {
  return mod.getOrInsertFunction(name, fnTy);
}

static void build_ctor_body(llvm::Module &mod, llvm::StringRef synthName,
                            int numInputs, int64_t stateBytes) {
  auto &ctx = mod.getContext();
  auto *ctorFn = mod.getFunction((synthName + ".ctor").str());
  auto *initFn = mod.getFunction((synthName + ".init").str());
  auto *performFn = mod.getFunction((synthName + ".perform").str());
  if (!ctorFn || !initFn || !performFn)
    return;

  ctorFn->deleteBody();
  auto *entry = llvm::BasicBlock::Create(ctx, "entry", ctorFn);
  llvm::IRBuilder<> builder(entry);

  auto *ptrTy = builder.getPtrTy();
  auto *i32Ty = builder.getInt32Ty();
  auto *i8Ty = builder.getInt8Ty();
  auto *i64Ty = builder.getInt64Ty();
  auto *f64Ty = builder.getDoubleTy();

  auto createFn = getOrInsertRuntimeFn(
      mod, "ylc_create_audio_node",
      llvm::FunctionType::get(ptrTy, {ptrTy, i32Ty, i32Ty, ptrTy}, false));
  auto constInletFn = getOrInsertRuntimeFn(
      mod, "ylc_const_inlet", llvm::FunctionType::get(ptrTy, {f64Ty}, false));
  auto plugInputFn = getOrInsertRuntimeFn(
      mod, "plug_input_in_graph",
      llvm::FunctionType::get(builder.getVoidTy(), {i32Ty, ptrTy, ptrTy},
                              false));

  llvm::Value *metaStr = builder.CreateGlobalString(synthName, "synth.meta");
  llvm::Value *node = builder.CreateCall(
      createFn,
      {performFn, builder.getInt32(numInputs),
       builder.getInt32(static_cast<int32_t>(stateBytes)), metaStr},
      "node");

  llvm::Value *statePtr = builder.CreateGEP(
      i8Ty, node, llvm::ConstantInt::get(i64Ty, sizeof(Node)), "node.state");
  builder.CreateCall(initFn, {statePtr});

  int inputIndex = 0;
  for (llvm::Argument &arg : ctorFn->args()) {
    llvm::Value *constNode =
        builder.CreateCall(constInletFn, {&arg}, "const_inlet");
    builder.CreateCall(plugInputFn,
                       {builder.getInt32(inputIndex++), node, constNode});
  }

  builder.CreateRet(node);
}

static void build_perform_body(llvm::Module &mod, llvm::StringRef synthName,
                               int numInputs) {
  auto &ctx = mod.getContext();
  auto *performFn = mod.getFunction((synthName + ".perform").str());
  auto *frameFn = mod.getFunction((synthName + ".frame").str());
  if (!performFn || !frameFn)
    return;

  performFn->deleteBody();
  auto *entry = llvm::BasicBlock::Create(ctx, "entry", performFn);
  auto *cond = llvm::BasicBlock::Create(ctx, "frames.cond", performFn);
  auto *body = llvm::BasicBlock::Create(ctx, "frames.body", performFn);
  auto *exit = llvm::BasicBlock::Create(ctx, "frames.exit", performFn);
  llvm::IRBuilder<> builder(entry);

  auto *ptrTy = builder.getPtrTy();
  auto *i32Ty = builder.getInt32Ty();
  auto *i64Ty = builder.getInt64Ty();
  auto *f64Ty = builder.getDoubleTy();

  auto argsIt = performFn->arg_begin();
  llvm::Value *node = &*argsIt++;
  llvm::Value *state = &*argsIt++;
  llvm::Value *inputs = &*argsIt++;
  llvm::Value *nframes = &*argsIt++;
  (void)argsIt++;

  auto readFn = getOrInsertRuntimeFn(
      mod, "ylc_read_inlet_node",
      llvm::FunctionType::get(f64Ty, {ptrTy, i64Ty}, false));
  auto writeFn = getOrInsertRuntimeFn(
      mod, "dsp_write_output",
      llvm::FunctionType::get(builder.getVoidTy(), {ptrTy, i64Ty, f64Ty},
                              false));

  llvm::AllocaInst *frameIdxPtr =
      builder.CreateAlloca(i32Ty, nullptr, "frame_idx.ptr");
  builder.CreateStore(builder.getInt32(0), frameIdxPtr);
  builder.CreateBr(cond);

  builder.SetInsertPoint(cond);
  llvm::Value *frameIdx = builder.CreateLoad(i32Ty, frameIdxPtr, "frame_idx");
  llvm::Value *hasMore =
      builder.CreateICmpSLT(frameIdx, nframes, "frame_idx.lt");
  builder.CreateCondBr(hasMore, body, exit);

  builder.SetInsertPoint(body);
  llvm::Value *frameI64 = builder.CreateSExt(frameIdx, i64Ty, "frame_idx.i64");

  llvm::SmallVector<llvm::Value *> frameArgs;
  frameArgs.reserve(static_cast<size_t>(numInputs) + 2);
  frameArgs.push_back(state);
  frameArgs.push_back(node);
  for (int i = 0; i < numInputs; ++i) {
    llvm::Value *slotPtr =
        builder.CreateGEP(ptrTy, inputs, builder.getInt64(i), "inlet.slot");
    llvm::Value *inletNode = builder.CreateLoad(ptrTy, slotPtr, "inlet.node");
    llvm::Value *sample =
        builder.CreateCall(readFn, {inletNode, frameI64}, "inlet.sample");
    frameArgs.push_back(sample);
  }

  llvm::Value *frameSample =
      builder.CreateCall(frameFn, frameArgs, "frame.call");
  builder.CreateCall(writeFn, {node, frameI64, frameSample});

  llvm::Value *nextIdx =
      builder.CreateAdd(frameIdx, builder.getInt32(1), "frame_idx.next");
  builder.CreateStore(nextIdx, frameIdxPtr);
  builder.CreateBr(cond);

  builder.SetInsertPoint(exit);
  builder.CreateRet(
      llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptrTy)));
}

// ---------------------------------------------------------------------------
// Lower func + arith dialect ops to llvm dialect so translateModuleToLLVMIR
// can handle them. The DSP dialect lowering will slot in here later.
// ---------------------------------------------------------------------------

static bool lower_to_llvm_dialect(mlir::ModuleOp &mod) {
  // Register arith → llvm conversion patterns via dialect interface.
  mlir::DialectRegistry reg;
  mlir::arith::registerConvertArithToLLVMInterface(reg);
  g_mlir_ctx->appendDialectRegistry(reg);

  mlir::PassManager pm(g_mlir_ctx);
  // ConvertToLLVMPass handles arith (via registered interface) and other
  // dialects.
  pm.addPass(mlir::createConvertFuncToLLVMPass());
  pm.addPass(mlir::createConvertToLLVMPass());
  pm.addPass(mlir::createReconcileUnrealizedCastsPass());
  return mlir::succeeded(pm.run(mod));
}

// ---------------------------------------------------------------------------
// Cross-synth dependency linking: find external frame references in an
// llvm::Module, re-translate the stored MLIR module for each dependency, and
// link it in so the normal LLVM optimization pipeline can decide whether to
// inline it.
// ---------------------------------------------------------------------------

static void link_frame_dependencies(llvm::Module &dst, llvm::LLVMContext &ctx) {
  // Collect dependency ids first — don't mutate dst while iterating it.
  llvm::SmallVector<int, 4> deps;
  for (auto &fn : dst) {
    if (!fn.isDeclaration())
      continue;
    llvm::StringRef name = fn.getName();
    for (int i = 0; i < g_registry.len(); i++) {
      if (!g_registry[i].mlir_module)
        continue;
      std::string frame_name = std::string(g_registry[i].name) + ".frame";
      if (name == frame_name) {
        deps.push_back(i);
        break;
      }
    }
  }

  for (int id : deps) {
    auto *owned = static_cast<mlir::OwningOpRef<mlir::ModuleOp> *>(
        g_registry[id].mlir_module);
    std::string synth_prefix = std::string(g_registry[id].name) + ".";
    std::string frame_name = synth_prefix + "frame";

    auto dep_mod =
        mlir::cast<mlir::ModuleOp>((*owned)->getOperation()->clone());
    if (!lower_to_llvm_dialect(dep_mod)) {
      fprintf(stderr,
              "audio_jit_mlir: failed to lower dependency '%s' to llvm "
              "dialect\n",
              g_registry[id].name);
      continue;
    }

    auto dep_llvm = mlir::translateModuleToLLVMIR(dep_mod, ctx, "dep");
    if (!dep_llvm) {
      fprintf(
          stderr,
          "audio_jit_mlir: failed to re-translate lowered dependency '%s'\n",
          g_registry[id].name);
      continue;
    }

    // Only the dependency frame body is needed in the caller module.
    // Keeping ctor/init/perform causes duplicate exported symbols once the
    // dependency has already been added to the JIT as its own synth module.
    llvm::SmallVector<llvm::Function *> to_erase;
    for (auto &dep_fn : *dep_llvm) {
      if (!dep_fn.getName().starts_with(synth_prefix))
        continue;
      if (dep_fn.getName() == frame_name)
        continue;
      to_erase.push_back(&dep_fn);
    }
    for (llvm::Function *fn : to_erase)
      fn->eraseFromParent();

    if (llvm::Linker::linkModules(dst, std::move(dep_llvm))) {
      fprintf(stderr, "audio_jit_mlir: linker error adding dependency '%s'\n",
              g_registry[id].name);
      continue;
    }

    for (auto &linked_fn : dst) {
      if (!linked_fn.getName().starts_with(synth_prefix))
        continue;
      linked_fn.setLinkage(llvm::GlobalValue::PrivateLinkage);
      linked_fn.removeFnAttr(llvm::Attribute::NoInline);
    }
  }
}

static void run_llvm_o3_pipeline(llvm::Module &mod) {
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

  llvm::ModulePassManager mpm =
      pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
  mpm.run(mod, mam);
}

struct SynthBuildSpec {
  Ast *lambda;
  Ast *binding;
  std::string name;
  int numInputs;
};

static int count_lambda_inputs(Ast *lambda) {
  int num_inputs = 0;
  if (!lambda || is_void_func(lambda->type))
    return 0;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    if (p->ast->tag == AST_IDENTIFIER)
      num_inputs++;
    else if (p->ast->tag == AST_TUPLE)
      num_inputs += p->ast->data.AST_LIST.len;
  }
  return num_inputs;
}

static bool collect_synth_specs(Ast *source,
                                llvm::SmallVectorImpl<SynthBuildSpec> &out) {
  if (!source)
    return false;

  auto pushLet = [&](Ast *letAst) {
    if (!letAst || letAst->tag != AST_LET)
      return;
    Ast *binding = letAst->data.AST_LET.binding;
    Ast *lambda = letAst->data.AST_LET.expr;
    if (!binding || binding->tag != AST_IDENTIFIER || !lambda ||
        lambda->tag != AST_LAMBDA)
      return;
    const char *name = binding->data.AST_IDENTIFIER.value;
    if (!name)
      return;
    out.push_back(SynthBuildSpec{
        .lambda = lambda,
        .binding = binding,
        .name = name,
        .numInputs = count_lambda_inputs(lambda),
    });
  };

  if (source->tag == AST_LET) {
    pushLet(source);
    return !out.empty();
  }

  if (source->tag == AST_BODY) {
    AST_LIST_ITER(source->data.AST_BODY.stmts, ({ pushLet(l->ast); }));
    return !out.empty();
  }

  return false;
}

static void build_synth_decls(mlir::ModuleOp mod, mlir::OpBuilder &b,
                              mlir::Location loc, const SynthBuildSpec &spec,
                              JITLangCtx *ctx, mlir::func::FuncOp &ctor_fn,
                              mlir::func::FuncOp &init_fn,
                              mlir::func::FuncOp &frame_fn,
                              mlir::func::FuncOp &perform_fn) {
  auto ptr_ty = mlir::LLVM::LLVMPointerType::get(g_mlir_ctx);
  auto f64_ty = b.getF64Type();
  auto i32_ty = b.getI32Type();

  // Body builders leave the insertion point inside function regions.
  // Re-anchor function declarations at module scope for each synth.
  b.setInsertionPointToEnd(mod.getBody());

  llvm::SmallVector<mlir::Type> ctor_params;
  ctor_params.reserve(spec.numInputs);
  for (int i = 0; i < spec.numInputs; i++)
    ctor_params.push_back(f64_ty);
  ctor_fn = b.create<mlir::func::FuncOp>(
      loc, spec.name + ".ctor",
      mlir::FunctionType::get(g_mlir_ctx, ctor_params, {ptr_ty}));
  ctor_fn.setPublic();

  init_fn = b.create<mlir::func::FuncOp>(
      loc, spec.name + ".init",
      mlir::FunctionType::get(g_mlir_ctx, {ptr_ty}, {}));
  init_fn.setPrivate();

  llvm::SmallVector<mlir::Type> frame_params = {ptr_ty, ptr_ty};
  for (int i = 0; i < spec.numInputs; i++)
    frame_params.push_back(f64_ty);
  frame_fn = b.create<mlir::func::FuncOp>(
      loc, spec.name + ".frame",
      mlir::FunctionType::get(g_mlir_ctx, frame_params, {f64_ty}));
  frame_fn.setPublic();

  perform_fn = b.create<mlir::func::FuncOp>(
      loc, spec.name + ".perform",
      mlir::FunctionType::get(
          g_mlir_ctx, {ptr_ty, ptr_ty, ptr_ty, i32_ty, f64_ty}, {ptr_ty}));
  perform_fn.setPublic();

  build_ctor_stub(ctor_fn, b, loc, ptr_ty);
  build_init_stub(init_fn, b, loc);
  frame_fn.addEntryBlock();
  if (!dsp_build_frame(frame_fn, b, loc, spec.lambda, ctx, ctx_sample_rate())) {
    fprintf(stderr,
            "audio_jit_mlir: dsp_build_frame failed for '%s', using stub\n",
            spec.name.c_str());
    frame_fn.getBody().front().clear();
    b.setInsertionPointToStart(&frame_fn.getBody().front());
    auto zero = b.create<mlir::arith::ConstantOp>(loc, b.getF64FloatAttr(0.0));
    b.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{zero});
  }
  build_perform_stub(perform_fn, b, loc, ptr_ty);
}

static int compile_synth_specs(llvm::ArrayRef<SynthBuildSpec> specs,
                               JITLangCtx *ctx, const char *dumpLabel) {
  if (specs.empty())
    return -1;

  ensure_mlir_ctx();
  ensure_jit();

  mlir::OpBuilder b(g_mlir_ctx);
  auto loc = b.getUnknownLoc();
  auto mod = mlir::ModuleOp::create(loc);
  b.setInsertionPointToEnd(mod.getBody());

  llvm::SmallVector<mlir::func::FuncOp> frameFns;
  llvm::SmallVector<int64_t> stateBytesBySpec;
  frameFns.reserve(specs.size());
  stateBytesBySpec.resize(specs.size(), 0);

  for (const SynthBuildSpec &spec : specs) {
    mlir::func::FuncOp ctor_fn, init_fn, frame_fn, perform_fn;
    build_synth_decls(mod, b, loc, spec, ctx, ctor_fn, init_fn, frame_fn,
                      perform_fn);
    frameFns.push_back(frame_fn);
  }

  llvm::errs() << "\n=== Pre-State MLIR [" << dumpLabel << "] ===\n";
  mod.print(llvm::errs());
  llvm::errs() << "\n";

  if (!run_dsp_state_passes(g_mlir_ctx, mod)) {
    fprintf(stderr, "audio_jit_mlir: state planning failed for '%s'\n",
            dumpLabel);
    return -1;
  }

  for (auto [i, frameFn] : llvm::enumerate(frameFns)) {
    if (auto attr =
            frameFn->getAttrOfType<mlir::IntegerAttr>("dsp.state_bytes"))
      stateBytesBySpec[i] = attr.getInt();
    else
      stateBytesBySpec[i] = dsp::assign_dsp_state_offsets(frameFn);
  }

  llvm::errs() << "\n=== MLIR [" << dumpLabel << "] ===\n";
  mod.print(llvm::errs());
  llvm::errs() << "\n";

  llvm::SmallVector<void *> storedMods;
  storedMods.reserve(specs.size());
  for (size_t i = 0; i < specs.size(); ++i) {
    auto stored = mlir::cast<mlir::ModuleOp>(mod.getOperation()->clone());
    std::string synth_prefix = specs[i].name + ".";
    llvm::SmallVector<mlir::func::FuncOp> funcs_to_erase;
    for (mlir::func::FuncOp fn : stored.getOps<mlir::func::FuncOp>()) {
      llvm::StringRef fn_name = fn.getName();
      if (!fn_name.contains('.'))
        continue;
      if (fn_name.starts_with(synth_prefix))
        continue;
      funcs_to_erase.push_back(fn);
    }
    for (mlir::func::FuncOp fn : funcs_to_erase)
      fn.erase();

    auto *stored_mod = new mlir::OwningOpRef<mlir::ModuleOp>(std::move(stored));
    storedMods.push_back(stored_mod);
  }

  if (!lower_to_llvm_dialect(mod)) {
    fprintf(stderr,
            "audio_jit_mlir: lowering to llvm dialect failed for '%s'\n",
            dumpLabel);
    for (void *stored : storedMods)
      delete static_cast<mlir::OwningOpRef<mlir::ModuleOp> *>(stored);
    return -1;
  }

  auto llvm_ctx = std::make_unique<llvm::LLVMContext>();
  auto llvm_mod = mlir::translateModuleToLLVMIR(mod, *llvm_ctx, dumpLabel);
  if (!llvm_mod) {
    fprintf(stderr, "audio_jit_mlir: translateModuleToLLVMIR failed for '%s'\n",
            dumpLabel);
    for (void *stored : storedMods)
      delete static_cast<mlir::OwningOpRef<mlir::ModuleOp> *>(stored);
    return -1;
  }

  for (auto [i, spec] : llvm::enumerate(specs)) {
    build_ctor_body(*llvm_mod, spec.name, spec.numInputs, stateBytesBySpec[i]);
    build_perform_body(*llvm_mod, spec.name, spec.numInputs);
  }

  link_frame_dependencies(*llvm_mod, *llvm_ctx);
  run_llvm_o3_pipeline(*llvm_mod);

  llvm::errs() << "\n=== LLVM IR O3 [" << dumpLabel << "] ===\n";
  llvm_mod->print(llvm::errs(), nullptr);
  llvm::errs() << "\n";

  auto tsm =
      llvm::orc::ThreadSafeModule(std::move(llvm_mod), std::move(llvm_ctx));
  if (auto err = g_jit->addIRModule(std::move(tsm))) {
    llvm::errs() << "audio_jit_mlir: addIRModule failed: " << err << "\n";
    for (void *stored : storedMods)
      delete static_cast<mlir::OwningOpRef<mlir::ModuleOp> *>(stored);
    return -1;
  }

  int firstId = -1;
  for (auto [i, spec] : llvm::enumerate(specs)) {
    auto ctor_sym = g_jit->lookup(spec.name + ".ctor");
    if (!ctor_sym) {
      llvm::errs() << "audio_jit_mlir: lookup " << spec.name
                   << ".ctor: " << ctor_sym.takeError() << "\n";
      for (void *stored : storedMods)
        delete static_cast<mlir::OwningOpRef<mlir::ModuleOp> *>(stored);
      return -1;
    }
    auto frame_sym = g_jit->lookup(spec.name + ".frame");
    if (!frame_sym) {
      llvm::errs() << "audio_jit_mlir: lookup " << spec.name
                   << ".frame: " << frame_sym.takeError() << "\n";
      for (void *stored : storedMods)
        delete static_cast<mlir::OwningOpRef<mlir::ModuleOp> *>(stored);
      return -1;
    }

    MlirSynthRecord rec{};
    rec.name = strdup(spec.name.c_str());
    rec.ctor_ptr = ctor_sym->toPtr<void *>();
    rec.frame_ptr = frame_sym->toPtr<void *>();
    rec.mlir_module = storedMods[i];
    rec.state_bytes = (int)stateBytesBySpec[i];
    rec.num_inputs = spec.numInputs;
    int id = g_registry.extend(rec);
    if (firstId < 0)
      firstId = id;
    fprintf(stderr, "audio_jit_mlir: compiled '%s' id=%d\n", spec.name.c_str(),
            id);
  }

  return firstId;
}

// ---------------------------------------------------------------------------
// Public C entry point
// ---------------------------------------------------------------------------

extern "C" int mlir_compile_synth(void *ast_raw, const char *name,
                                  void *ctx_raw, int num_inputs) {
  if (!name) {
    fprintf(stderr, "audio_jit_mlir: null name\n");
    return -1;
  }

  Ast *lambda = static_cast<Ast *>(ast_raw);
  print_type(lambda->type);
  JITLangCtx *ctx = static_cast<JITLangCtx *>(ctx_raw);
  llvm::SmallVector<SynthBuildSpec> specs;
  specs.push_back(SynthBuildSpec{
      .lambda = lambda,
      .binding = nullptr,
      .name = name,
      .numInputs = num_inputs,
  });
  return compile_synth_specs(specs, ctx, name);
}

extern "C" int mlir_compile_synth_group(void *source_ast, void *ctx_raw,
                                        int *out_count) {
  Ast *source = static_cast<Ast *>(source_ast);
  JITLangCtx *ctx = static_cast<JITLangCtx *>(ctx_raw);

  llvm::SmallVector<SynthBuildSpec> specs;
  if (!collect_synth_specs(source, specs)) {
    fprintf(stderr, "audio_jit_mlir: no synth lets found in group compile\n");
    if (out_count)
      *out_count = 0;
    return -1;
  }

  if (out_count)
    *out_count = (int)specs.size();

  std::string dump_label = "group";
  for (auto [i, spec] : llvm::enumerate(specs)) {
    dump_label += (i == 0) ? ":" : ",";
    dump_label += spec.name;
  }
  return compile_synth_specs(specs, ctx, dump_label.c_str());
}
