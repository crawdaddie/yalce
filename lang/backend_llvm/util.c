#include "backend_llvm/util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdio.h>

void struct_ptr_set(int item_offset, LLVMValueRef struct_ptr,
                    LLVMTypeRef struct_type, LLVMValueRef data,
                    LLVMBuilderRef builder) {

  // Set the data
  LLVMValueRef data_ptr = LLVMBuildStructGEP2(builder, struct_type, struct_ptr,
                                              item_offset, "data_ptr");
  LLVMBuildStore(builder, data, data_ptr);
}

LLVMValueRef struct_ptr_get(int item_offset, LLVMValueRef struct_ptr,
                            LLVMTypeRef struct_type, LLVMBuilderRef builder) {
  // Get a pointer to the item
  LLVMValueRef item_ptr = LLVMBuildStructGEP2(builder, struct_type, struct_ptr,
                                              item_offset, "item_ptr");

  // Load the value from the pointer
  LLVMTypeRef item_type = LLVMStructGetTypeAtIndex(struct_type, item_offset);
  return LLVMBuildLoad2(builder, item_type, item_ptr, "loaded_item");
}

LLVMValueRef increment_ptr(LLVMValueRef ptr, LLVMTypeRef node_type,
                           LLVMValueRef element_size, LLVMBuilderRef builder) {

  return LLVMBuildGEP2(builder, node_type, ptr, &element_size, 1,
                       "next_element_ptr");
}

LLVMValueRef and_vals(LLVMValueRef l, LLVMValueRef r, LLVMBuilderRef builder) {
  return LLVMBuildAnd(builder, l, r, "and_vals");
}
