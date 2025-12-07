#include "trampoline_closures.h"
#include "types.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

// Get or declare the llvm.init.trampoline intrinsic
// Signature: void @llvm.init.trampoline(ptr <tramp>, ptr <func>, ptr <nval>)
LLVMValueRef get_init_trampoline_intrinsic(LLVMModuleRef module) {
  LLVMValueRef intrinsic = LLVMGetNamedFunction(module, "llvm.init.trampoline");

  if (!intrinsic) {
    // Declare the intrinsic: void (ptr, ptr, ptr)
    LLVMTypeRef param_types[3] = {
        LLVMPointerType(LLVMInt8Type(), 0), // trampoline buffer
        LLVMPointerType(LLVMInt8Type(), 0), // function pointer
        LLVMPointerType(LLVMInt8Type(),
                        0) // nested function value (closure env)
    };

    LLVMTypeRef intrinsic_type =
        LLVMFunctionType(LLVMVoidType(), // returns void
                         param_types,
                         3, // 3 parameters
                         0  // not vararg
        );

    intrinsic = LLVMAddFunction(module, "llvm.init.trampoline", intrinsic_type);
  }

  return intrinsic;
}

// Get or declare the llvm.adjust.trampoline intrinsic
// Signature: ptr @llvm.adjust.trampoline(ptr <tramp>)
LLVMValueRef get_adjust_trampoline_intrinsic(LLVMModuleRef module) {
  LLVMValueRef intrinsic =
      LLVMGetNamedFunction(module, "llvm.adjust.trampoline");

  if (!intrinsic) {
    // Declare the intrinsic: ptr (ptr)
    LLVMTypeRef param_types[1] = {
        LLVMPointerType(LLVMInt8Type(), 0) // trampoline buffer
    };

    LLVMTypeRef intrinsic_type =
        LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), // returns ptr
                         param_types,
                         1, // 1 parameter
                         0  // not vararg
        );

    intrinsic =
        LLVMAddFunction(module, "llvm.adjust.trampoline", intrinsic_type);
  }

  return intrinsic;
}

// Create a trampoline for a closure
// This allocates a trampoline buffer, initializes it, and returns the adjusted
// trampoline
LLVMValueRef create_closure_trampoline(
    LLVMValueRef closure_impl_fn, // The actual implementation function (takes
                                  // env + args)
    LLVMValueRef closure_env,     // The environment/captured values
    LLVMTypeRef
        expected_fn_type, // The expected function signature (without env param)
    bool use_heap, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder) {
  // Trampoline size is target-dependent, but LLVM docs suggest allocating at
  // least 40 bytes We'll allocate 64 bytes to be safe
  LLVMValueRef trampoline_size = LLVMConstInt(LLVMInt32Type(), 64, 0);

  // Allocate space for the trampoline on the stack
  LLVMTypeRef trampoline_type = LLVMArrayType(LLVMInt8Type(), 64);
  LLVMValueRef trampoline_buffer =
      use_heap ? LLVMBuildMalloc(builder, trampoline_type, "trampoline")
               : LLVMBuildAlloca(builder, trampoline_type, "trampoline");

  // Cast to i8* for the intrinsic
  LLVMValueRef trampoline_ptr =
      LLVMBuildBitCast(builder, trampoline_buffer,
                       LLVMPointerType(LLVMInt8Type(), 0), "trampoline_ptr");

  // Cast the implementation function to i8*
  LLVMValueRef impl_fn_ptr =
      LLVMBuildBitCast(builder, closure_impl_fn,
                       LLVMPointerType(LLVMInt8Type(), 0), "impl_fn_ptr");

  // Cast the environment to i8*
  LLVMValueRef env_ptr = LLVMBuildBitCast(
      builder, closure_env, LLVMPointerType(LLVMInt8Type(), 0), "env_ptr");

  // Call llvm.init.trampoline(trampoline_ptr, impl_fn_ptr, env_ptr)
  LLVMValueRef init_intrinsic = get_init_trampoline_intrinsic(module);
  LLVMValueRef init_args[3] = {trampoline_ptr, impl_fn_ptr, env_ptr};

  LLVMTypeRef init_intrinsic_type = ({
    LLVMTypeRef param_types[3] = {
        LLVMPointerType(LLVMInt8Type(), 0), // trampoline buffer
        LLVMPointerType(LLVMInt8Type(), 0), // function pointer
        LLVMPointerType(LLVMInt8Type(),
                        0) // nested function value (closure env)
    };

    LLVMTypeRef intrinsic_type =
        LLVMFunctionType(LLVMVoidType(), // returns void
                         param_types,
                         3, // 3 parameters
                         0  // not vararg
        );
    intrinsic_type;
  });

  LLVMBuildCall2(builder, init_intrinsic_type, init_intrinsic, init_args, 3,
                 "init_intrinsic");

  // Call llvm.adjust.trampoline(trampoline_ptr) to get the callable function
  // pointer
  LLVMValueRef adjust_intrinsic = get_adjust_trampoline_intrinsic(module);
  LLVMValueRef adjust_args[1] = {trampoline_ptr};

  LLVMTypeRef adjust_intrinsic_type = ({
    LLVMTypeRef param_types[1] = {
        LLVMPointerType(LLVMInt8Type(), 0) // trampoline buffer
    };

    LLVMTypeRef intrinsic_type =
        LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), // returns ptr
                         param_types,
                         1, // 1 parameter
                         0  // not vararg
        );
    intrinsic_type;
  });
  LLVMValueRef adjusted_trampoline =
      LLVMBuildCall2(builder, adjust_intrinsic_type, adjust_intrinsic,
                     adjust_args, 1, "adjusted_trampoline");

  // Cast the adjusted trampoline to the expected function type
  LLVMValueRef fn_ptr =
      LLVMBuildBitCast(builder, adjusted_trampoline,
                       LLVMPointerType(expected_fn_type, 0), "closure_fn");

  return fn_ptr;
}

// This is the "nested function" that actually does the work
// Signature: result_type impl_fn(env_type* %env, arg1_type %arg1, ...)
LLVMValueRef create_closure_impl_function(
    const char *name,
    Type *closure_type, // The full closure type (includes environment)
    Type *env_type,     // Type of the captured environment
    LLVMModuleRef module, JITLangCtx *ctx) {
  // Build the parameter types: first param is env pointer, rest are the actual
  // args
  int num_params = fn_type_args_len(closure_type) + 1; // +1 for env
  LLVMTypeRef param_types[num_params];

  // First parameter is the environment pointer
  param_types[0] = LLVMPointerType(type_to_llvm_type(env_type, ctx, module), 0);

  // Add the actual function parameters
  Type *fn_type = closure_type;
  for (int i = 1; i < num_params; i++) {
    param_types[i] = type_to_llvm_type(fn_type->data.T_FN.from, ctx, module);
    fn_type = fn_type->data.T_FN.to;
  }

  // Get the return type
  LLVMTypeRef return_type = type_to_llvm_type(fn_type, ctx, module);

  // Create the function type
  LLVMTypeRef impl_fn_type =
      LLVMFunctionType(return_type, param_types, num_params, 0);

  // Add the function to the module
  LLVMValueRef impl_fn = LLVMAddFunction(module, name, impl_fn_type);

  // Set the 'nest' attribute on the first parameter (the environment)
  // This tells LLVM that this parameter is the nested function context
  LLVMAttributeRef nest_attr = LLVMCreateEnumAttribute(
      LLVMGetGlobalContext(), LLVMGetEnumAttributeKindForName("nest", 4), 0);
  LLVMAddAttributeAtIndex(impl_fn, 1, nest_attr); // Index 1 is first param

  return impl_fn;
}
