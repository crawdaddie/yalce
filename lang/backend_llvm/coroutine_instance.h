#ifndef _LANG_BACKEND_LLVM_COROUTINE_INSTANCE_H
#define _LANG_BACKEND_LLVM_COROUTINE_INSTANCE_H
#include "llvm-c/Types.h"

LLVMTypeRef coroutine_def_fn_type(LLVMTypeRef instance_type,
                                  LLVMTypeRef ret_option_type);

LLVMTypeRef coroutine_instance_type(LLVMTypeRef params_obj_type);

LLVMValueRef coroutine_instance_counter_gep(LLVMValueRef instance_ptr,
                                            LLVMTypeRef instance_type,
                                            LLVMBuilderRef builder);

LLVMValueRef coroutine_instance_fn_gep(LLVMValueRef instance_ptr,
                                       LLVMTypeRef instance_type,
                                       LLVMBuilderRef builder);
LLVMValueRef coroutine_instance_params_gep(LLVMValueRef instance_ptr,
                                           LLVMTypeRef instance_type,
                                           LLVMBuilderRef builder);

LLVMValueRef coroutine_instance_parent_gep(LLVMValueRef instance_ptr,
                                           LLVMTypeRef instance_type,
                                           LLVMBuilderRef builder);

void increment_instance_counter(LLVMValueRef instance_ptr,
                                LLVMTypeRef instance_type,
                                LLVMBuilderRef builder);

LLVMValueRef replace_instance(LLVMValueRef instance, LLVMTypeRef instance_type,
                              LLVMValueRef new_instance,
                              LLVMBuilderRef builder);

#endif
