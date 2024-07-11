#ifndef _LANG_BACKEND_LLVM_UTIL_H
#define _LANG_BACKEND_LLVM_UTIL_H

#include "llvm-c/Types.h"

void struct_ptr_set(int item_offset, LLVMValueRef struct_ptr,
                    LLVMTypeRef struct_type, LLVMValueRef val,
                    LLVMBuilderRef builder);

LLVMValueRef struct_ptr_get(int item_offset, LLVMValueRef struct_ptr,
                            LLVMTypeRef struct_type, LLVMBuilderRef builder);

LLVMValueRef increment_ptr(LLVMValueRef ptr, LLVMTypeRef node_type,
                           LLVMValueRef element_size, LLVMBuilderRef builder);

LLVMValueRef null_node(LLVMTypeRef node_type);

LLVMValueRef is_null_node(LLVMValueRef node, LLVMTypeRef node_type,
                          LLVMBuilderRef builder);

LLVMValueRef is_not_null_node(LLVMValueRef node, LLVMTypeRef node_type,
                              LLVMBuilderRef builder);
#endif
