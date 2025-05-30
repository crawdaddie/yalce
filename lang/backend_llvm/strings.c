#include "backend_llvm/strings.h"
#include "adt.h"
#include "backend_llvm/array.h"
#include "list.h"
#include "types.h"
#include "types/type.h"
#include "util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <string.h>

#define GET_SPRINTF                                                            \
  LLVMValueRef sprintf_func = LLVMGetNamedFunction(module, "sprintf");         \
  LLVMTypeRef sprintf_type =                                                   \
      LLVMFunctionType(LLVMInt32Type(),                                        \
                       (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0),     \
                                       LLVMPointerType(LLVMInt8Type(), 0)},    \
                       2, 1);                                                  \
  if (!sprintf_func) {                                                         \
    sprintf_func = LLVMAddFunction(module, "sprintf", sprintf_type);           \
  };

LLVMValueRef _codegen_string(const char *chars, int length, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen_print_char_matrix(LLVMValueRef array2d,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  LLVMValueRef rows = LLVMBuildExtractValue(builder, array2d, 0, "matrix_rows");
  LLVMValueRef cols = LLVMBuildExtractValue(builder, array2d, 1, "matrix_cols");
  LLVMValueRef string =
      LLVMBuildExtractValue(builder, array2d, 2, "string_data");

  LLVMValueRef char_data = LLVMBuildExtractValue(builder, string, 1, "chars");
  LLVMTypeRef char_m_print_type =
      LLVMFunctionType(LLVMVoidType(),
                       (LLVMTypeRef[]){LLVMInt32Type(), LLVMInt32Type(),
                                       LLVMPointerType(LLVMInt8Type(), 0)},
                       3, 0);
  LLVMValueRef char_m_print =
      get_extern_fn("print_char_matrix", char_m_print_type, module);
  return LLVMBuildCall2(builder, char_m_print_type, char_m_print,
                        (LLVMValueRef[]){rows, cols, char_data}, 3,
                        "print_char_matrix");
}

#define STRLEN_TYPE                                                            \
  LLVMFunctionType(LLVMInt32Type(),                                            \
                   (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)}, 1,     \
                   false)

LLVMValueRef get_strlen_func(LLVMModuleRef module) {
  LLVMValueRef strlen_func = LLVMGetNamedFunction(module, "strlen");
  LLVMTypeRef strlen_type = STRLEN_TYPE;

  if (!strlen_func) {
    strlen_func = LLVMAddFunction(module, "strlen", strlen_type);
  }
  return strlen_func;
}

LLVMValueRef _int_to_string(LLVMValueRef int_value, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  GET_SPRINTF
  LLVMValueRef buffer =
      LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8Type(), 20), "str_buffer");

  LLVMValueRef format_string =
      LLVMBuildGlobalStringPtr(builder, "%d", "format_string");

  LLVMValueRef args[] = {buffer, format_string, int_value};
  LLVMBuildCall2(builder, sprintf_type, sprintf_func, args, 3, "");

  return buffer;
}

LLVMValueRef _uint64_to_string(LLVMValueRef int_value, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  GET_SPRINTF
  LLVMValueRef buffer =
      LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8Type(), 20), "str_buffer");

  LLVMValueRef format_string =
      LLVMBuildGlobalStringPtr(builder, "%llu", "format_string");

  LLVMValueRef args[] = {buffer, format_string, int_value};
  LLVMBuildCall2(builder, sprintf_type, sprintf_func, args, 3, "");

  return buffer;
}

LLVMValueRef _char_to_string(LLVMValueRef int_value, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  GET_SPRINTF
  LLVMValueRef buffer =
      LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8Type(), 1), "str_buffer");

  LLVMValueRef format_string =
      LLVMBuildGlobalStringPtr(builder, "%c", "format_string");

  LLVMValueRef args[] = {buffer, format_string, int_value};
  LLVMBuildCall2(builder, sprintf_type, sprintf_func, args, 3, "");

  return buffer;
}

LLVMValueRef int_to_string(LLVMValueRef int_value, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  LLVMValueRef data_ptr = _int_to_string(int_value, module, builder);
  LLVMValueRef strlen_func = get_strlen_func(module);

  LLVMTypeRef strlen_type = STRLEN_TYPE;
  LLVMValueRef len =
      LLVMBuildCall2(builder, strlen_type, strlen_func,
                     (LLVMValueRef[]){data_ptr}, 1, "strlen_call");

  LLVMTypeRef data_ptr_type = LLVMTypeOf(data_ptr);

  LLVMTypeRef struct_type = codegen_array_type(LLVMInt8Type());

  LLVMValueRef str = LLVMGetUndef(struct_type);
  str = LLVMBuildInsertValue(builder, str, data_ptr, 1, "insert_array_data");
  str = LLVMBuildInsertValue(builder, str, len, 0, "insert_array_size");
  return str;
}

LLVMValueRef uint64_to_string(LLVMValueRef int_value, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  // printf("uint64 to string\n");
  // LLVMDumpValue(int_value);
  LLVMValueRef data_ptr = _uint64_to_string(int_value, module, builder);
  LLVMValueRef strlen_func = get_strlen_func(module);

  LLVMTypeRef strlen_type = STRLEN_TYPE;
  LLVMValueRef len =
      LLVMBuildCall2(builder, strlen_type, strlen_func,
                     (LLVMValueRef[]){data_ptr}, 1, "strlen_call");

  LLVMTypeRef data_ptr_type = LLVMTypeOf(data_ptr);

  LLVMTypeRef struct_type = codegen_array_type(LLVMInt8Type());

  LLVMValueRef str = LLVMGetUndef(struct_type);
  str = LLVMBuildInsertValue(builder, str, data_ptr, 1, "insert_array_data");
  str = LLVMBuildInsertValue(builder, str, len, 0, "insert_array_size");
  return str;
}

LLVMValueRef char_to_string(LLVMValueRef char_value, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  LLVMValueRef data_ptr = LLVMBuildAlloca(builder, LLVMInt8Type(), "char_ptr");

  LLVMBuildStore(builder, char_value, data_ptr);

  LLVMValueRef len = LLVMConstInt(LLVMInt32Type(), 1, 0);

  LLVMTypeRef data_ptr_type = LLVMPointerType(LLVMInt8Type(), 0);

  LLVMTypeRef struct_type = codegen_array_type(LLVMInt8Type());

  LLVMValueRef str = LLVMGetUndef(struct_type);
  str = LLVMBuildInsertValue(builder, str, data_ptr, 1, "insert_array_data");
  str = LLVMBuildInsertValue(builder, str, len, 0, "insert_array_size");
  return str;
}

LLVMValueRef _num_to_string(LLVMValueRef double_value, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  GET_SPRINTF
  LLVMValueRef buffer =
      LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8Type(), 20), "str_buffer");

  // TODO: allow specifying precision
  // LLVMValueRef format_string =
  //     LLVMBuildGlobalStringPtr(builder, "%.16f", "format_string");

  LLVMValueRef format_string =
      LLVMBuildGlobalStringPtr(builder, "%f", "format_string");

  LLVMValueRef args[] = {buffer, format_string, double_value};
  LLVMBuildCall2(builder, sprintf_type, sprintf_func, args, 3, "");

  return buffer;
}

LLVMValueRef num_to_string(LLVMValueRef int_value, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  LLVMValueRef data_ptr = _num_to_string(int_value, module, builder);
  LLVMValueRef strlen_func = get_strlen_func(module);
  LLVMValueRef len = LLVMBuildCall2(builder, STRLEN_TYPE, strlen_func,
                                    (LLVMValueRef[]){data_ptr}, 1, "");

  LLVMTypeRef data_ptr_type = LLVMTypeOf(data_ptr);

  LLVMTypeRef struct_type = codegen_array_type(LLVMInt8Type());

  LLVMValueRef str = LLVMGetUndef(struct_type);
  str = LLVMBuildInsertValue(builder, str, data_ptr, 1, "insert_array_data");
  str = LLVMBuildInsertValue(builder, str, len, 0, "insert_array_size");
  return str;
}

LLVMValueRef tuple_to_string(LLVMValueRef val, Type *tuple_type,
                             JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  int len = tuple_type->data.T_CONS.num_args;
  int num_string_components = 1 + len + (len - 1);

  bool has_names = false;
  if (tuple_type->data.T_CONS.names != NULL) {
    has_names = true;
    num_string_components += len;
  }
  LLVMValueRef strings[num_string_components];

  int tuple_type_name_len = strlen(tuple_type->data.T_CONS.name);

  char n[tuple_type_name_len + 2];

  sprintf(n, "%s ", tuple_type->data.T_CONS.name);

  strings[0] =
      _codegen_string(n, tuple_type_name_len + 1, ctx, module, builder);

  for (int i = 0; i < len; i++) {
    Type *mem_type = tuple_type->data.T_CONS.args[i];
    LLVMValueRef v = LLVMBuildExtractValue(builder, val, i, "");
    if (has_names) {
      char *name = tuple_type->data.T_CONS.names[i];
      int name_len = strlen(name);
      char n[name_len + 3];

      sprintf(n, "%s: ", name);

      strings[1 + (3 * i)] =
          _codegen_string(n, name_len + 2, ctx, module, builder);

      strings[1 + (3 * i) + 1] =
          llvm_string_serialize(v, mem_type, ctx, module, builder);

      strings[1 + (3 * i) + 2] = _codegen_string(", ", 2, ctx, module, builder);
    } else {
      strings[1 + (2 * i)] =
          llvm_string_serialize(v, mem_type, ctx, module, builder);

      strings[1 + (2 * i) + 1] = _codegen_string(", ", 2, ctx, module, builder);
    }
  }
  return stream_string_concat(strings, num_string_components, module, builder);
}

LLVMValueRef llvm_string_serialize(LLVMValueRef val, Type *val_type,
                                   JITLangCtx *ctx, LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  if (val_type == NULL) {
    return _codegen_string("", 0, ctx, module, builder);
  }
  if (val_type->kind == T_STRING) {
    return val;
  }

  if (val_type->kind == T_CHAR) {
    return char_to_string(val, module, builder);
  }

  if (is_string_type(val_type)) {
    return val;
  }

  if (val_type->kind == T_INT) {
    return int_to_string(val, module, builder);
  }

  if (val_type->kind == T_UINT64) {
    return uint64_to_string(val, module, builder);
  }

  if (val_type->kind == T_NUM) {
    return num_to_string(val, module, builder);
  }

  if (val_type->kind == T_BOOL) {
    return int_to_string(val, module, builder);
  }

  if ((strcmp(val_type->data.T_CONS.name, "Variant") == 0) &&
      (val_type->data.T_CONS.num_args == 2) &&
      (strcmp(val_type->data.T_CONS.args[0]->data.T_CONS.name, "Some") == 0) &&
      (strcmp(val_type->data.T_CONS.args[1]->data.T_CONS.name, "None") == 0)) {

    return opt_to_string(val, val_type, ctx, module, builder);
  }

  if (is_option_type(val_type)) {
    return opt_to_string(val, val_type, ctx, module, builder);
  }

  if (is_pointer_type(val_type)) {
    return _codegen_string("Ptr", 3, ctx, module, builder);
  }

  if (is_list_type(val_type)) {
    return codegen_list_to_string(val, val_type, ctx, module, builder);
  }

  if (val_type->kind == T_CONS) {
    return _codegen_string("Tuple", 5, ctx, module, builder);
  }

  return char_to_string(LLVMConstInt(LLVMInt8Type(), 60, 0), module, builder);
}

#define INITIAL_SIZE 32

LLVMValueRef stream_string_concat(LLVMValueRef *strings, int num_strings,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  LLVMValueRef string_concat_func =
      LLVMGetNamedFunction(module, "string_concat");

  LLVMTypeRef string_type = LLVMStructType(
      (LLVMTypeRef[]){LLVMInt32Type(), LLVMPointerType(LLVMInt8Type(), 0)}, 2,
      0);

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

LLVMValueRef string_is_empty(LLVMValueRef string, LLVMBuilderRef builder) {
  LLVMValueRef first_char =
      LLVMBuildLoad2(builder, LLVMInt8Type(), string, "first_char_of_string");

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

LLVMValueRef char_array(const char *chars, int length, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef char_type = LLVMInt8Type();
  LLVMTypeRef array_type = LLVMArrayType(char_type, length + 1);

  LLVMValueRef length_val = LLVMConstInt(LLVMInt32Type(), length + 1, 0);
  LLVMValueRef str_const = LLVMConstString(chars, length, 0);
  LLVMTypeRef str_const_type = LLVMTypeOf(str_const);

  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

  LLVMValueRef data_ptr =
      (ctx->stack_ptr == 0)
          ? LLVMBuildMalloc(builder, str_const_type, "heap_array")
          : LLVMBuildAlloca(builder, str_const_type, "stack_array");
  //
  //
  // LLVMValueRef data_ptr =
  //     LLVMBuildMalloc(builder, str_const_type, "heap_array");

  LLVMBuildStore(builder, str_const, data_ptr);

  // LLVMValueRef null_terminator = LLVMConstInt(char_type, 0, 0);
  // LLVMValueRef last_elem_ptr = LLVMBuildGEP2(builder, char_type, data_ptr,
  //                                            &length_val, 1,
  //                                            "last_elem_ptr");
  // LLVMBuildStore(builder, null_terminator, last_elem_ptr);
  return data_ptr;
}

LLVMTypeRef string_struct_type(LLVMTypeRef data_ptr_type) {

  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(),
          data_ptr_type,
      },
      2, 0);
}

LLVMValueRef _codegen_string(const char *chars, int length, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMValueRef data_ptr = char_array(chars, length, ctx, module, builder);
  LLVMTypeRef data_ptr_type = LLVMTypeOf(data_ptr);

  LLVMTypeRef struct_type = string_struct_type(data_ptr_type);

  LLVMValueRef str = LLVMGetUndef(struct_type);
  str = LLVMBuildInsertValue(builder, str, data_ptr, 1, "insert_array_data");
  str = LLVMBuildInsertValue(builder, str,
                             LLVMConstInt(LLVMInt32Type(), length, 0), 0,
                             "insert_array_size");
  return str;
}

LLVMValueRef codegen_string(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  const char *chars = ast->data.AST_STRING.value;
  int length = ast->data.AST_STRING.length;
  return _codegen_string(chars, length, ctx, module, builder);
}

LLVMValueRef codegen_string_add(LLVMValueRef a, LLVMValueRef b, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  LLVMValueRef a_len = LLVMBuildExtractValue(builder, a, 0, "a_len");
  LLVMValueRef b_len = LLVMBuildExtractValue(builder, b, 0, "b_len");

  LLVMValueRef new_len = LLVMBuildAdd(builder, a_len, b_len, "new_len");

  LLVMValueRef a_data = LLVMBuildExtractValue(builder, a, 1, "a_data");
  LLVMValueRef b_data = LLVMBuildExtractValue(builder, b, 1, "b_data");

  LLVMValueRef new_len_plus_one =
      LLVMBuildAdd(builder, new_len, LLVMConstInt(LLVMInt32Type(), 1, 0),
                   "new_len_plus_one");
  LLVMTypeRef char_type = LLVMInt8Type();
  LLVMValueRef new_data =
      LLVMBuildMalloc(builder, LLVMArrayType(char_type, 0), "new_data");
  new_data = LLVMBuildBitCast(builder, new_data, LLVMPointerType(char_type, 0),
                              "new_data_ptr");

  LLVMBuildMemCpy(builder, new_data, 0, a_data, 0, a_len);

  LLVMValueRef offset =
      LLVMBuildGEP2(builder, char_type, new_data, &a_len, 1, "offset");
  LLVMBuildMemCpy(builder, offset, 0, b_data, 0, b_len);

  LLVMValueRef null_terminator_ptr = LLVMBuildGEP2(
      builder, char_type, new_data, &new_len, 1, "null_terminator_ptr");
  LLVMBuildStore(builder, LLVMConstInt(char_type, 0, 0), null_terminator_ptr);

  LLVMTypeRef struct_type = string_struct_type(LLVMPointerType(char_type, 0));
  LLVMValueRef new_str = LLVMGetUndef(struct_type);
  new_str =
      LLVMBuildInsertValue(builder, new_str, new_len, 0, "insert_new_len");
  new_str =
      LLVMBuildInsertValue(builder, new_str, new_data, 1, "insert_new_data");

  return new_str;
}
