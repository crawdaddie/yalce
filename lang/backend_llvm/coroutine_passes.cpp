/**
 * C++ wrapper for LLVM coroutine passes
 *
 * The C API doesn't properly expose coroutine lowering, so we use
 * the C++ API here with a C-compatible interface.
 */

#include "llvm/Transforms/Coroutines/CoroCleanup.h"
#include "llvm/Transforms/Coroutines/CoroEarly.h"
#include "llvm/Transforms/Coroutines/CoroSplit.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>

extern "C" {

/**
 * Run LLVM coroutine lowering passes on a module
 *
 * This must be called BEFORE standard optimization passes, as those
 * will remove coro.id/coro.begin as "dead code" before lowering can run.
 *
 * Returns 0 on success, non-zero on failure.
 */
int run_coroutine_passes(LLVMModuleRef module_ref) {
  // Convert C API types to C++ API types
  llvm::Module *module = llvm::unwrap(module_ref);

  // Create analysis managers
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  // Create pass builder and register all analysis managers
  llvm::PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  // Create module pass manager for coroutine lowering
  llvm::ModulePassManager MPM;

  // CoroEarly is a module-level pass - run first to prepare coroutines
  MPM.addPass(llvm::CoroEarlyPass());

  // CoroSplit is a CGSCC pass - needs to run on each SCC
  llvm::CGSCCPassManager CGPM;
  CGPM.addPass(llvm::CoroSplitPass());

  // Wrap CGSCC passes in module-to-CGSCC adapter
  MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(std::move(CGPM)));

  // CoroCleanup is a module-level pass - run last to clean up
  MPM.addPass(llvm::CoroCleanupPass());

  // Run the passes
  MPM.run(*module, MAM);

  return 0;
}

} // extern "C"
