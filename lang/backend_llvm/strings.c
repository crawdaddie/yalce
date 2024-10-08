#include "backend_llvm/strings.h"
#include "list.h"
#include "types/type.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdlib.h>
#include <string.h>

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

LLVMValueRef num_to_string(LLVMValueRef double_value, LLVMModuleRef module,
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
      LLVMBuildGlobalStringPtr(builder, "%f", "format_string");

  // Call sprintf
  LLVMValueRef args[] = {buffer, format_string, double_value};
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

  if (val_type->kind == T_NUM) {
    return num_to_string(val, module, builder);
  }

  if (val_type->kind == T_BOOL) {
    return int_to_string(val, module, builder);
  }

  return LLVMBuildGlobalStringPtr(builder, "", ".str");
}

#define INITIAL_SIZE 32

const char *string_concat(const char **strings, int num_strings) {
  int total_len = 0;
  int lengths[num_strings];
  for (int i = 0; i < num_strings; i++) {
    lengths[i] = strlen(strings[i]);
    total_len += lengths[i];
  }
  char *concatted = malloc(sizeof(char) * (total_len + 1));
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
  // return LLVMbcodegen_int_binop(builder, TOKEN_EQUALITY, first_char,
  //                          LLVMConstInt(LLVMInt8Type(), 0, 0));
  return LLVMBuildICmp(builder, LLVMIntEQ, first_char,
                       LLVMConstInt(LLVMInt8Type(), 0, 0), "Int8 ==");
}

LLVMValueRef string_is_not_empty(LLVMValueRef string, LLVMBuilderRef builder) {
  LLVMValueRef first_char =
      LLVMBuildLoad2(builder, LLVMInt8Type(), string, "first_char_of_string");
  return LLVMBuildICmp(builder, LLVMIntNE, first_char,
                       LLVMConstInt(LLVMInt8Type(), 0, 0), "Int8 ==");
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

  // return codegen_int_binop(builder, TOKEN_EQUALITY, comp,
  //                          LLVMConstInt(LLVMInt32Type(), 0, 0));

  return LLVMBuildICmp(builder, LLVMIntEQ, comp,
                       LLVMConstInt(LLVMInt32Type(), 0, 0), "Int ==");
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

LLVMValueRef __codegen_string(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  const char *chars = ast->data.AST_STRING.value;
  int length = ast->data.AST_STRING.length;
  ObjString vstr = (ObjString){
      .chars = chars, .length = length, .hash = hash_string(chars, length)};

  LLVMTypeRef data_ptr_type = LLVMPointerType(LLVMInt8Type(), 0);
  LLVMTypeRef struct_type = array_struct_type(data_ptr_type);

  return LLVMBuildGlobalString(builder, chars, ".str");
}

LLVMValueRef char_array(const char *chars, int length, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMTypeRef char_type = LLVMInt8Type();
  LLVMTypeRef array_type = LLVMArrayType(char_type, length + 1);

  LLVMValueRef data_ptr =
      (ctx->stack_ptr == 0)
          ? LLVMBuildMalloc(builder, array_type, "heap_array")
          : LLVMBuildAlloca(builder, array_type, "stack_array");

  for (int i = 0; i < length; i++) {

    // Calculate pointer to array element
    LLVMValueRef element_ptr =
        LLVMBuildGEP2(builder, array_type, data_ptr,
                      (LLVMValueRef[]){
                          LLVMConstInt(LLVMInt32Type(), 0, 0),
                          LLVMConstInt(LLVMInt32Type(), i, 0) // Array index
                      },
                      2, "element_ptr");

    LLVMValueRef value =
        LLVMConstInt(LLVMInt8Type(), (long long)*(chars + i), 0);
    LLVMBuildStore(builder, value, element_ptr);
  }

  // LLVMValueRef value = LLVMConstInt(LLVMInt8Type(), 0, 0);
  // LLVMBuildStore(builder, value, element_ptr);

  // LLVMValueRef const_array =
  //     LLVMConstStringInContext(LLVMGetModuleContext(module), chars, length,
  //     1);
  // LLVMBuildMemCpy(builder, data_ptr, 0, const_array, 0,
  //                 LLVMConstInt(LLVMInt32Type(), length + 1, 0));
  //
  return data_ptr;
}

LLVMValueRef codegen_string(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  const char *chars = ast->data.AST_STRING.value;
  int length = ast->data.AST_STRING.length;

  LLVMValueRef data_ptr = char_array(chars, length, ctx, module, builder);
  LLVMTypeRef data_ptr_type = LLVMTypeOf(data_ptr);
  // Create struct type
  //

  LLVMTypeRef struct_type = array_struct_type(data_ptr_type);

  LLVMValueRef str = LLVMGetUndef(struct_type);
  str = LLVMBuildInsertValue(builder, str, data_ptr, 1, "insert_array_data");
  str = LLVMBuildInsertValue(builder, str,
                             LLVMConstInt(LLVMInt32Type(), length, 0), 0,
                             "insert_array_size");
  return str;
}
