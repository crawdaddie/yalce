#ifndef _LANG_BACKEND_LLVM_UTIL_H
#define _LANG_BACKEND_LLVM_UTIL_H

#include "types/type.h"
#include "llvm-c/Types.h"

void struct_ptr_set(int item_offset, LLVMValueRef struct_ptr,
                    LLVMTypeRef struct_type, LLVMValueRef val,
                    LLVMBuilderRef builder);

LLVMValueRef struct_ptr_get(int item_offset, LLVMValueRef struct_ptr,
                            LLVMTypeRef struct_type, LLVMBuilderRef builder);

LLVMValueRef increment_ptr(LLVMValueRef ptr, LLVMTypeRef node_type,
                           LLVMValueRef element_size, LLVMBuilderRef builder);

LLVMValueRef and_vals(LLVMValueRef res, LLVMValueRef res2,
                      LLVMBuilderRef builder);

LLVMValueRef codegen_printf(const char *format, LLVMValueRef *args,
                            int arg_count, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef insert_printf_call(const char *format, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef llvm_string_serialize(LLVMValueRef val, Type *val_type,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

LLVMValueRef get_extern_fn(const char *name, LLVMTypeRef fn_type,
                           LLVMModuleRef module);
#endif
