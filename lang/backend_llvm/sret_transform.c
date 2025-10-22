#include "backend_llvm/sret_transform.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/Types.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Check if a struct return needs sret transformation (>16 bytes on
// ARM64/x86_64)
static bool needs_sret_transform(LLVMTypeRef ret_type,
                                 LLVMTargetDataRef target_data) {
  if (LLVMGetTypeKind(ret_type) != LLVMStructTypeKind) {
    return false;
  }

  unsigned long long size = LLVMStoreSizeOfType(target_data, ret_type);
  // ARM64 and x86_64 use sret for structs >16 bytes
  return size > 16;
}

// Transform a function signature to use sret
// Original: ret_type func(args...)
// New: void func(ret_type* sret, args...)
static LLVMTypeRef create_sret_function_type(LLVMTypeRef original_fn_type,
                                             LLVMTypeRef ret_type) {
  unsigned param_count = LLVMCountParamTypes(original_fn_type);

  // Create new parameter array with sret pointer as first param
  LLVMTypeRef *new_params = malloc(sizeof(LLVMTypeRef) * (param_count + 1));
  new_params[0] = LLVMPointerType(ret_type, 0); // sret pointer

  // Copy original parameters
  if (param_count > 0) {
    LLVMTypeRef *orig_params = malloc(sizeof(LLVMTypeRef) * param_count);
    LLVMGetParamTypes(original_fn_type, orig_params);
    for (unsigned i = 0; i < param_count; i++) {
      new_params[i + 1] = orig_params[i];
    }
    free(orig_params);
  }

  // Return void since we're using sret
  LLVMTypeRef result =
      LLVMFunctionType(LLVMVoidType(), new_params, param_count + 1,
                       LLVMIsFunctionVarArg(original_fn_type));
  free(new_params);
  return result;
}

// Transform a single call instruction to use sret
void transform_call_to_sret(LLVMValueRef call_inst, LLVMValueRef callee,
                            LLVMTypeRef ret_type, LLVMTypeRef fn_type,
                            LLVMModuleRef module) {

  const char *callee_name = LLVMGetValueName(callee);
  printf("Transforming call to '%s' to use sret\n", callee_name);

  // Create a builder positioned before the call
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderBefore(builder, call_inst);

  // Allocate space for the return value
  LLVMValueRef ret_alloc = LLVMBuildAlloca(builder, ret_type, "sret_alloc");

  // Get original call arguments
  unsigned arg_count = LLVMGetNumArgOperands(call_inst);
  LLVMValueRef *new_args = malloc(sizeof(LLVMValueRef) * (arg_count + 1));
  new_args[0] = ret_alloc; // First arg is sret pointer

  // Copy original arguments
  for (unsigned i = 0; i < arg_count; i++) {
    new_args[i + 1] = LLVMGetOperand(call_inst, i);
  }

  // Get or create the sret version of the function
  LLVMTypeRef sret_fn_type = create_sret_function_type(fn_type, ret_type);

  // The C function already uses sret ABI, so we declare it with void return
  LLVMValueRef sret_func = callee; // Reuse the same function declaration
  LLVMSetValueName2(sret_func, callee_name, strlen(callee_name));

  // Make the new call with sret pointer
  LLVMValueRef new_call = LLVMBuildCall2(builder, sret_fn_type, sret_func,
                                         new_args, arg_count + 1, "sret_call");

  // Load the result from the sret allocation
  LLVMValueRef result =
      LLVMBuildLoad2(builder, ret_type, ret_alloc, "sret_load");

  // Replace all uses of the old call with the loaded result
  LLVMReplaceAllUsesWith(call_inst, result);

  // Delete the old call instruction
  LLVMInstructionEraseFromParent(call_inst);

  free(new_args);
  LLVMDisposeBuilder(builder);
}

// Transform calls to extern functions with large struct returns
void transform_sret_calls(LLVMModuleRef module) {
  LLVMTargetDataRef target_data = LLVMGetModuleDataLayout(module);

  // First, collect all extern functions that need sret transformation
  typedef struct {
    LLVMValueRef func;
    LLVMTypeRef ret_type;
    LLVMTypeRef fn_type;
  } ExternFunc;

  ExternFunc *extern_funcs = NULL;
  int num_extern_funcs = 0;

  // Iterate through all function declarations (externs)
  for (LLVMValueRef func = LLVMGetFirstFunction(module); func;
       func = LLVMGetNextFunction(func)) {

    // Only process extern declarations (not definitions)
    if (!LLVMIsDeclaration(func)) {
      continue;
    }

    LLVMTypeRef fn_type = LLVMGlobalGetValueType(func);
    LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);

    // Check if needs sret transformation
    if (needs_sret_transform(ret_type, target_data)) {
      extern_funcs =
          realloc(extern_funcs, sizeof(ExternFunc) * (num_extern_funcs + 1));
      extern_funcs[num_extern_funcs].func = func;
      extern_funcs[num_extern_funcs].ret_type = ret_type;
      extern_funcs[num_extern_funcs].fn_type = fn_type;
      num_extern_funcs++;
    }
  }

  // Now find and transform all calls to these extern functions
  for (int i = 0; i < num_extern_funcs; i++) {
    LLVMValueRef extern_func = extern_funcs[i].func;
    LLVMTypeRef ret_type = extern_funcs[i].ret_type;
    LLVMTypeRef fn_type = extern_funcs[i].fn_type;

    // Collect all calls to this function
    LLVMValueRef *calls_to_transform = NULL;
    int num_calls = 0;

    // Iterate through all functions in the module to find calls
    for (LLVMValueRef func = LLVMGetFirstFunction(module); func;
         func = LLVMGetNextFunction(func)) {

      if (LLVMIsDeclaration(func)) {
        continue;
      }

      // Iterate through basic blocks
      for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb;
           bb = LLVMGetNextBasicBlock(bb)) {

        // Iterate through instructions
        for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
             inst = LLVMGetNextInstruction(inst)) {

          // Check if this is a call to our extern function
          if (LLVMIsACallInst(inst)) {
            LLVMValueRef callee = LLVMGetCalledValue(inst);
            if (callee == extern_func) {
              calls_to_transform = realloc(
                  calls_to_transform, sizeof(LLVMValueRef) * (num_calls + 1));
              calls_to_transform[num_calls] = inst;
              num_calls++;
            }
          }
        }
      }
    }

    // Transform all calls
    for (int j = 0; j < num_calls; j++) {
      transform_call_to_sret(calls_to_transform[j], extern_func, ret_type,
                             fn_type, module);
    }

    if (calls_to_transform) {
      free(calls_to_transform);
    }
  }

  if (extern_funcs) {
    free(extern_funcs);
  }
}
