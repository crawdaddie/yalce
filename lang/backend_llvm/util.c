#include "backend_llvm/util.h"
#include "codegen_binop.h"
#include "types/type.h"
#include "types/util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>
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
LLVMValueRef int_to_string(LLVMValueRef int_value, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  // Declare sprintf if it's not already declared
  LLVMValueRef sprintf_func = LLVMGetNamedFunction(module, "sprintf");
  LLVMTypeRef sprintf_type =
      LLVMFunctionType(LLVMInt32Type(),
                       (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0),
                                       LLVMPointerType(LLVMInt8Type(), 0)},
                       2, true);
  if (!sprintf_func) {
    sprintf_func = LLVMAddFunction(module, "sprintf", sprintf_type);
  }

  // Allocate a buffer for the string
  LLVMValueRef buffer =
      LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8Type(), 20), "str_buffer");

  // Create a constant string for the format specifier
  LLVMValueRef format_string =
      LLVMBuildGlobalStringPtr(builder, "%d", "format_string");

  // Call sprintf
  LLVMValueRef args[] = {buffer, format_string, int_value};
  LLVMBuildCall2(builder, sprintf_type, sprintf_func, args, 3, "");

  // Return the buffer
  return buffer;
}

LLVMValueRef llvm_string_serialize(LLVMValueRef val, Type *val_type,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  if (val_type->kind == T_STRING) {
    return val;
  }

  if (is_string_type(val_type)) {
    return val;
  }

  if (val_type->kind == T_INT) {
    return int_to_string(val, module, builder);
  }

  return LLVMBuildGlobalStringPtr(builder, "dummy", ".str");
}

#define INITIAL_SIZE 32

const char *string_concat(const char **strings, int num_strings) {
  int total_len = 0;
  int lengths[num_strings];
  for (int i = 0; i < num_strings; i++) {
    lengths[i] = strlen(strings[i]);
    total_len += lengths[i];
  }
  const char *concatted = malloc(sizeof(char) * (total_len + 1));
  int offset = 0;
  for (int i = 0; i < num_strings; i++) {
    strncpy(concatted + offset, strings[i], lengths[i]);
    offset += lengths[i];
  }
  return concatted;
}

LLVMValueRef stream_string_concat(LLVMValueRef *strings, int num_strings,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  LLVMValueRef string_concat_func =
      LLVMGetNamedFunction(module, "string_concat");
  LLVMTypeRef string_type = LLVMPointerType(LLVMInt8Type(), 0);
  LLVMTypeRef string_array_type = LLVMArrayType(string_type, num_strings);
  LLVMTypeRef fn_type = LLVMFunctionType(
      string_type,
      (LLVMTypeRef[]){LLVMPointerType(string_type, 0), LLVMInt32Type()}, 2, 0);

  if (!string_concat_func) {
    string_concat_func = LLVMAddFunction(module, "string_concat", fn_type);
  }

  LLVMValueRef array_alloca =
      LLVMBuildAlloca(builder, string_array_type, "string_array");

  for (int i = 0; i < num_strings; i++) {
    LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0),
                              LLVMConstInt(LLVMInt32Type(), i, 0)};
    LLVMValueRef ptr =
        LLVMBuildGEP2(builder, string_array_type, array_alloca, indices, 2, "");
    LLVMBuildStore(builder, strings[i], ptr);
  }

  LLVMValueRef array_ptr = LLVMBuildBitCast(
      builder, array_alloca, LLVMPointerType(string_type, 0), "array_ptr");

  LLVMValueRef args[] = {array_ptr,
                         LLVMConstInt(LLVMInt32Type(), num_strings, 0)};

  return LLVMBuildCall2(builder, fn_type, string_concat_func, args, 2,
                        "concat_result");
}

// Helper function to check if a list is null
LLVMValueRef string_is_empty(LLVMValueRef string, LLVMBuilderRef builder) {
  LLVMValueRef first_char =
      LLVMBuildLoad2(builder, LLVMInt8Type(), string, "first_char_of_string");
  return codegen_int_binop(builder, TOKEN_EQUALITY, first_char,
                           LLVMConstInt(LLVMInt8Type(), 0, 0));
}

LLVMValueRef string_is_not_empty(LLVMValueRef string, LLVMBuilderRef builder) {
  LLVMValueRef first_char =
      LLVMBuildLoad2(builder, LLVMInt8Type(), string, "first_char_of_string");
  return codegen_int_binop(builder, TOKEN_NOT_EQUAL, first_char,
                           LLVMConstInt(LLVMInt8Type(), 0, 0));
}

LLVMValueRef strings_equal(LLVMValueRef left, LLVMValueRef right,
                           LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef string_compare_func = LLVMGetNamedFunction(module, "strcmp");
  LLVMTypeRef string_type = LLVMPointerType(LLVMInt8Type(), 0);

  LLVMTypeRef fn_type = LLVMFunctionType(
      LLVMInt32Type(), (LLVMTypeRef[]){string_type, string_type}, 2, 0);

  if (!string_compare_func) {
    string_compare_func = LLVMAddFunction(module, "strcmp", fn_type);
  }
  LLVMValueRef args[] = {left, right};

  LLVMValueRef comp = LLVMBuildCall2(builder, fn_type, string_compare_func,
                                     args, 2, "str_compare_result");
  return codegen_int_binop(builder, TOKEN_EQUALITY, comp,
                           LLVMConstInt(LLVMInt32Type(), 0, 0));
}
// Assume we have an LLVMValueRef representing a string pointer
LLVMValueRef increment_string(LLVMBuilderRef builder, LLVMValueRef string) {
  // Create a constant integer with value 1
  LLVMValueRef one = LLVMConstInt(LLVMInt32Type(), 1, 0);

  // Create a GEP (GetElementPtr) instruction to increment the pointer
  LLVMValueRef indices[] = {one};
  LLVMValueRef incremented = LLVMBuildGEP2(builder, LLVMInt8Type(), string,
                                           indices, 1, "incrementedPtr");

  return incremented;
}

LLVMValueRef get_extern_fn(const char *name, LLVMTypeRef fn_type,
                           LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, name);

  if (fn == NULL) {
    fn = LLVMAddFunction(module, name, fn_type);
  }
  return fn;
}
