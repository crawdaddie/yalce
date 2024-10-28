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
                            0), // coroutine generator function type (generic -
                                // go with void *)
            LLVMInt32Type(),    // coroutine counter
        },
        2, 0);
  }
  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMPointerType(LLVMInt8Type(),
                          0), // coroutine generator function type (generic - go
                              // with void *)
          LLVMInt32Type(),    // coroutine counter
          params_obj_type,    // params tuple
      },
      3, 0);
}
