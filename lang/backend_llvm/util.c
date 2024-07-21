#include "backend_llvm/util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdio.h>
#include <string.h>

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

LLVMValueRef codegen_printf(const char *format, LLVMValueRef *args,
                            int arg_count, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  LLVMValueRef printf_func = LLVMGetNamedFunction(module, "printf");

  if (!printf_func) {
    LLVMTypeRef printf_type = LLVMFunctionType(
        LLVMInt32Type(), (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)}, 1,
        1);

    printf_func = LLVMAddFunction(module, "printf", printf_type);
  }
  // Create a global string constant for the format string
  LLVMValueRef format_const =
      LLVMBuildGlobalStringPtr(builder, format, "format");

  // Prepare the arguments for the printf call
  LLVMValueRef call_args[arg_count + 1];
  call_args[0] = format_const;

  memcpy(call_args + 1, args, arg_count * sizeof(LLVMValueRef));

  // Insert the call to printf
  LLVMValueRef call =
      LLVMBuildCall2(builder, LLVMInt32Type(), printf_func,
                     (LLVMValueRef[]){format_const}, arg_count + 1, "printf");

  // Clean up
  return call;
}
LLVMValueRef insert_printf_call(const char *format, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  // Declare printf if it hasn't been declared yet
  LLVMValueRef printf_func = LLVMGetNamedFunction(module, "printf");
  if (!printf_func) {
    LLVMTypeRef param_types[] = {LLVMPointerType(LLVMInt8Type(), 0)};
    LLVMTypeRef printf_type =
        LLVMFunctionType(LLVMInt32Type(), param_types, 1, 0);
    printf_func = LLVMAddFunction(module, "printf", printf_type);
  }

  // Get the type of the printf function
  LLVMTypeRef printf_type = LLVMTypeOf(printf_func);

  // Create a global string constant for the format string
  LLVMValueRef format_const =
      LLVMBuildGlobalStringPtr(builder, format, "format");

  // Insert the call to printf
  LLVMValueRef args[] = {format_const};
  LLVMValueRef call =
      LLVMBuildCall2(builder, printf_type, printf_func, args, 1, "printf_call");

  return call;
}
