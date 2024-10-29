#include "./coroutine_types.h"
#include "llvm-c/Core.h"
#include <stdio.h>
LLVMTypeRef coroutine_def_fn_type(LLVMTypeRef instance_type,
                                  LLVMTypeRef ret_option_type) {

  return LLVMFunctionType(ret_option_type,
                          (LLVMTypeRef[]){LLVMPointerType(instance_type, 0)}, 1,
                          0);
}

LLVMTypeRef coroutine_instance_type(LLVMTypeRef params_obj_type) {
  if (LLVMGetTypeKind(params_obj_type) == LLVMVoidTypeKind) {

    return LLVMStructType(
        (LLVMTypeRef[]){
            LLVMPointerType(LLVMInt8Type(),
                            0), // coroutine generator function type
                                // (generic - go with void *)
            LLVMInt32Type(),    // coroutine counter
            LLVMPointerType(
                LLVMInt8Type(),
                0), // pointer to 'parent instance' ie previous top of stack
        },
        3, 0);
  }
  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMPointerType(LLVMInt8Type(),
                          0), // coroutine generator function type (generic - go
                              // with void *)
          LLVMInt32Type(),    // coroutine counter
          LLVMPointerType(
              LLVMInt8Type(),
              0), // pointer to 'parent instance' ie previous top of stack
          params_obj_type, // params tuple always last
      },
      4, 0);
}

LLVMValueRef coroutine_instance_counter_gep(LLVMValueRef instance_ptr,
                                            LLVMTypeRef instance_type,
                                            LLVMBuilderRef builder) {

  return LLVMBuildStructGEP2(builder, instance_type, instance_ptr, 1,
                             "instance_counter_ptr");
}

LLVMValueRef coroutine_instance_fn_gep(LLVMValueRef instance_ptr,
                                       LLVMTypeRef instance_type,
                                       LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, instance_type, instance_ptr, 0,
                             "instance_fn_ptr");
}
LLVMValueRef coroutine_instance_params_gep(LLVMValueRef instance_ptr,
                                           LLVMTypeRef instance_type,
                                           LLVMBuilderRef builder) {
  // Get number of elements
  unsigned num_fields = LLVMCountStructElementTypes(instance_type);
  if (num_fields == 4) {
    return LLVMBuildStructGEP2(builder, instance_type, instance_ptr, 2,
                               "instance_params_ptr");
  }

  return NULL;
}

LLVMValueRef coroutine_instance_parent_gep(LLVMValueRef instance_ptr,
                                           LLVMTypeRef instance_type,
                                           LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, instance_type, instance_ptr, 2,
                             "instance_parent_ptr");
}

void increment_instance_counter(LLVMValueRef instance_ptr,
                                LLVMTypeRef instance_type,
                                LLVMBuilderRef builder) {

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance_ptr, instance_type, builder);

  LLVMValueRef counter =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter_gep, "instance_counter");

  counter = LLVMBuildAdd(builder, counter, LLVMConstInt(LLVMInt32Type(), 1, 0),
                         "instance_counter++");
  LLVMBuildStore(builder, counter, counter_gep);
}
