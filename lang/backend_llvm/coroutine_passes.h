#ifndef COROUTINE_PASSES_H
#define COROUTINE_PASSES_H

#include <llvm-c/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run LLVM coroutine lowering passes on a module
 *
 * This must be called BEFORE standard optimization passes.
 * Returns 0 on success, non-zero on failure.
 */
int run_coroutine_passes(LLVMModuleRef module);

#ifdef __cplusplus
}
#endif

#endif // COROUTINE_PASSES_H
