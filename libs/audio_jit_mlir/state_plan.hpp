#pragma once

#include <memory>

#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/MLIRContext.h>
#include <mlir/Pass/Pass.h>

std::unique_ptr<mlir::Pass> createMaterializeDspStatePass();
bool run_dsp_state_passes(mlir::MLIRContext *ctx, mlir::ModuleOp &mod);
