#include "./state_plan.hpp"

#include "./dsp_dialect.hpp"

#include <mlir/Dialect/Arith/IR/Arith.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/LLVMIR/LLVMDialect.h>
#include <mlir/IR/Builders.h>
#include <mlir/Pass/Pass.h>
#include <mlir/Pass/PassManager.h>

#include <optional>

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>

namespace {

enum class StateObjectKind {
  Phasor,
  SubSynth,
};

struct StateObject {
  StateObjectKind kind;
  mlir::Operation *owner;
  int32_t size;
  int32_t align;
  double initF64;
  bool needsInit;
  bool live;
  std::string synthName;
  int32_t numInputs = 0;
};

struct StatePlan {
  llvm::SmallVector<StateObject> objects;
  int64_t totalBytes = 0;
};

static int64_t alignTo(int64_t value, int32_t align) {
  return (value + align - 1) & ~static_cast<int64_t>(align - 1);
}

static bool hasLiveObjects(const StatePlan &plan) {
  for (const StateObject &obj : plan.objects) {
    if (obj.live)
      return true;
  }
  return false;
}

static std::optional<StatePlan> buildStatePlan(mlir::func::FuncOp frameFn) {
  StatePlan plan;
  frameFn.walk([&](dsp::PhasorOp op) {
    plan.objects.push_back(StateObject{
        .kind = StateObjectKind::Phasor,
        .owner = op.getOperation(),
        .size = 8,
        .align = 8,
        .initF64 = 0.0,
        .needsInit = true,
        .live = !op.getResult().use_empty(),
    });
  });
  frameFn.walk([&](dsp::SubSynthOp op) {
    plan.objects.push_back(StateObject{
        .kind = StateObjectKind::SubSynth,
        .owner = op.getOperation(),
        .size = static_cast<int32_t>(op.getStateBytes()),
        .align = 8,
        .initF64 = 0.0,
        .needsInit = op.getStateBytes() > 0,
        .live = !op.getResult().use_empty(),
        .synthName = op.getSynthName().str(),
        .numInputs = static_cast<int32_t>(op.getNumInputs()),
    });
  });

  if (plan.objects.empty())
    return std::nullopt;

  int64_t offset = 0;
  for (const StateObject &obj : plan.objects) {
    if (!obj.live)
      continue;
    offset = alignTo(offset, obj.align);
    offset += obj.size;
  }
  plan.totalBytes = alignTo(offset, 8);
  return plan;
}

static mlir::func::FuncOp
getOrCreatePrivateDecl(mlir::ModuleOp module, mlir::Location loc,
                       llvm::StringRef name,
                       mlir::FunctionType fnType) {
  if (auto fn = module.lookupSymbol<mlir::func::FuncOp>(name))
    return fn;

  mlir::OpBuilder b(module.getContext());
  b.setInsertionPointToStart(module.getBody());
  auto fn = b.create<mlir::func::FuncOp>(loc, name, fnType);
  fn.setPrivate();
  return fn;
}

static mlir::LogicalResult rewriteInitFromPlan(mlir::func::FuncOp initFn,
                                               const StatePlan &plan) {
  if (initFn.empty())
    return mlir::failure();

  auto &block = initFn.getBody().front();
  block.clear();

  mlir::OpBuilder b(initFn.getContext());
  auto loc = initFn.getLoc();
  b.setInsertionPointToStart(&block);
  auto ptrTy = mlir::LLVM::LLVMPointerType::get(initFn.getContext());
  auto module = initFn->getParentOfType<mlir::ModuleOp>();

  if (hasLiveObjects(plan)) {
    mlir::Value cursor =
        b.create<dsp::StateCursorOp>(loc, ptrTy, block.getArgument(0)).getCursor();
    for (const StateObject &obj : plan.objects) {
      if (!obj.live)
        continue;
      auto consume =
          b.create<dsp::StateConsumeOp>(loc, cursor, obj.size, obj.align);
      cursor = consume.getNextCursor();

      if (obj.needsInit) {
        if (obj.kind == StateObjectKind::Phasor) {
          auto init = b.create<mlir::arith::ConstantOp>(
              loc, b.getF64FloatAttr(obj.initF64));
          b.create<mlir::LLVM::StoreOp>(loc, init.getResult(), consume.getPtr());
        } else if (obj.kind == StateObjectKind::SubSynth) {
          auto callee = getOrCreatePrivateDecl(
              module, loc, obj.synthName + ".init",
              b.getFunctionType({ptrTy}, {}));
          b.create<mlir::func::CallOp>(loc, callee,
                                       mlir::ValueRange{consume.getPtr()});
        }
      }
    }
  }

  b.create<mlir::func::ReturnOp>(loc);
  return mlir::success();
}

static mlir::LogicalResult rewriteFrameFromPlan(mlir::func::FuncOp frameFn,
                                                const StatePlan &plan) {
  if (frameFn.empty())
    return mlir::failure();

  auto &block = frameFn.getBody().front();
  mlir::OpBuilder b(frameFn.getContext());
  auto loc = frameFn.getLoc();
  auto ptrTy = mlir::LLVM::LLVMPointerType::get(frameFn.getContext());
  auto f64Ty = b.getF64Type();
  auto module = frameFn->getParentOfType<mlir::ModuleOp>();

  llvm::SmallVector<mlir::Value> statePtrs;
  statePtrs.reserve(plan.objects.size());
  if (hasLiveObjects(plan)) {
    b.setInsertionPointToStart(&block);
    mlir::Value cursor =
        b.create<dsp::StateCursorOp>(loc, ptrTy, block.getArgument(0)).getCursor();
    for (const StateObject &obj : plan.objects) {
      if (!obj.live) {
        statePtrs.push_back({});
        continue;
      }
      auto consume =
          b.create<dsp::StateConsumeOp>(loc, cursor, obj.size, obj.align);
      statePtrs.push_back(consume.getPtr());
      cursor = consume.getNextCursor();
    }
  } else {
    statePtrs.resize(plan.objects.size());
  }

  for (auto [index, obj] : llvm::enumerate(plan.objects)) {
    if (obj.kind == StateObjectKind::Phasor) {
      auto phasor = mlir::dyn_cast<dsp::PhasorOp>(obj.owner);
      if (!phasor)
        continue;
      if (!obj.live) {
        phasor.erase();
        continue;
      }

      b.setInsertionPoint(phasor);
      auto stateful = b.create<dsp::PhasorStatefulOp>(
          phasor.getLoc(), statePtrs[index], phasor.getFreq(),
          phasor.getSpf().convertToDouble());
      phasor.replaceAllUsesWith(stateful.getResult());
      phasor.erase();
      continue;
    }

    if (obj.kind == StateObjectKind::SubSynth) {
      auto subSynth = mlir::dyn_cast<dsp::SubSynthOp>(obj.owner);
      if (!subSynth)
        continue;
      if (!obj.live) {
        subSynth.erase();
        continue;
      }

      llvm::SmallVector<mlir::Type> inputs = {ptrTy, ptrTy};
      for (int32_t i = 0; i < obj.numInputs; ++i)
        inputs.push_back(f64Ty);
      auto callee = getOrCreatePrivateDecl(
          module, loc, obj.synthName + ".frame",
          b.getFunctionType(inputs, {f64Ty}));

      b.setInsertionPoint(subSynth);
      llvm::SmallVector<mlir::Value> args = {statePtrs[index],
                                             block.getArgument(1)};
      for (mlir::Value input : subSynth.getInputs())
        args.push_back(input);
      while ((int32_t)args.size() < obj.numInputs + 2) {
        auto zero = b.create<mlir::arith::ConstantOp>(loc, b.getF64FloatAttr(0.0));
        args.push_back(zero);
      }

      auto call = b.create<mlir::func::CallOp>(loc, callee, args);
      subSynth.replaceAllUsesWith(call.getResult(0));
      subSynth.erase();
    }
  }

  return mlir::success();
}

struct MaterializeDspStatePass
    : public mlir::PassWrapper<MaterializeDspStatePass,
                               mlir::OperationPass<mlir::ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(MaterializeDspStatePass)

  llvm::StringRef getArgument() const final { return "materialize-dsp-state"; }
  llvm::StringRef getDescription() const final {
    return "Build a deterministic state plan for DSP ops and materialize it";
  }

  void runOnOperation() override {
    auto module = getOperation();
    for (auto frameFn : module.getOps<mlir::func::FuncOp>()) {
      llvm::StringRef name = frameFn.getName();
      if (!name.ends_with(".frame"))
        continue;

      auto maybePlan = buildStatePlan(frameFn);
      if (!maybePlan)
        continue;
      const StatePlan &plan = *maybePlan;

      std::string initName = (name.drop_back(6) + ".init").str();
      auto initFn = module.lookupSymbol<mlir::func::FuncOp>(initName);
      if (!initFn) {
        frameFn.emitError("missing matching init function for state plan pass");
        signalPassFailure();
        return;
      }

      auto stateBytesAttr = mlir::IntegerAttr::get(
          mlir::IntegerType::get(&getContext(), 64), plan.totalBytes);
      frameFn->setAttr("dsp.state_bytes", stateBytesAttr);
      initFn->setAttr("dsp.state_bytes", stateBytesAttr);

      if (mlir::failed(rewriteInitFromPlan(initFn, plan)) ||
          mlir::failed(rewriteFrameFromPlan(frameFn, plan))) {
        signalPassFailure();
        return;
      }
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createMaterializeDspStatePass() {
  return std::make_unique<MaterializeDspStatePass>();
}

bool run_dsp_state_passes(mlir::MLIRContext *ctx, mlir::ModuleOp &mod) {
  mlir::PassManager pm(ctx);
  pm.addPass(createMaterializeDspStatePass());
  return mlir::succeeded(pm.run(mod));
}
