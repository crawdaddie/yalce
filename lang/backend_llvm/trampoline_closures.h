#ifndef _LANG_BACKEND_LLVM_TRAMPOLINE_CLOSURES_H
#define _LANG_BACKEND_LLVM_TRAMPOLINE_CLOSURES_H

#include "../parse.h"
#include "../types/type.h"
#include "./common.h"
#include "llvm-c/Types.h"

// Get the llvm.init.trampoline intrinsic
LLVMValueRef get_init_trampoline_intrinsic(LLVMModuleRef module);

// Get the llvm.adjust.trampoline intrinsic
LLVMValueRef get_adjust_trampoline_intrinsic(LLVMModuleRef module);

// Create a trampoline for a closure
// Returns a function pointer that can be called with the standard signature
LLVMValueRef create_closure_trampoline(
    LLVMValueRef
        closure_impl_fn,      // The implementation function (takes env + args)
    LLVMValueRef closure_env, // The environment/captured values
    LLVMTypeRef expected_fn_type, // The expected function signature
    bool use_heap, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder);

// Create a closure implementation function with 'nest' attribute
LLVMValueRef create_closure_impl_function(const char *name, Type *closure_type,
                                          Type *env_type, LLVMModuleRef module,
                                          JITLangCtx *ctx);

#endif
