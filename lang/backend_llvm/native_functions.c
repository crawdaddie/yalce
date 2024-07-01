#include "backend_llvm/native_functions.h"
#include "../common.h"
#include "../type_inference.h"
#include "backend_llvm/common.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

#define LLVM_TYPE_int LLVMInt32Type()
#define LLVM_TYPE_float LLVMFloatType()
#define LLVM_TYPE_double LLVMDoubleType()
#define LLVM_TYPE_void LLVMVoidType()
#define LLVM_TYPE_str LLVMPointerType(LLVMInt8Type(), 0)
#define LLVM_TYPE_ptr(type) LLVMPointerType(LLVM_TYPE_##type, 0)

#define GET_LLVM_TYPE(type) LLVM_TYPE_##type
#define ADD_NATIVE_FUNCTION(stack, module, func_name, return_type, num_args,   \
                            ...)                                               \
  do {                                                                         \
    const char *name = #func_name;                                             \
    int name_len = strlen(name);                                               \
    LLVMTypeRef args[] = {MAP_LLVM_TYPES(num_args, __VA_ARGS__)};              \
    LLVMTypeRef func_type =                                                    \
        LLVMFunctionType(GET_LLVM_TYPE(return_type), args, num_args, false);   \
    LLVMValueRef func = LLVMAddFunction(module, name, func_type);              \
    JITSymbol *v = malloc(sizeof(JITSymbol));                                  \
    *v = (JITSymbol){                                                          \
        .llvm_type = func_type, .symbol_type = STYPE_FUNCTION, .val = func};   \
    ht_set_hash(stack, name, hash_string(name, name_len), v);                  \
  } while (0)

#define MAP_LLVM_TYPES(num_args, ...)                                          \
  MAP_LLVM_TYPES_HELPER(num_args, __VA_ARGS__)

#define MAP_LLVM_TYPES_HELPER(num_args, ...)                                   \
  MAP_LLVM_TYPES_##num_args(__VA_ARGS__)

#define MAP_LLVM_TYPES_0()
#define MAP_LLVM_TYPES_1(a) GET_LLVM_TYPE(a)
#define MAP_LLVM_TYPES_2(a, b) GET_LLVM_TYPE(a), GET_LLVM_TYPE(b)

#define MAP_LLVM_TYPES_3(a, b, c)                                              \
  GET_LLVM_TYPE(a), GET_LLVM_TYPE(b), GET_LLVM_TYPE(c)

#define MAP_LLVM_TYPES_4(a, b, c, d)                                           \
  GET_LLVM_TYPE(a), GET_LLVM_TYPE(b), GET_LLVM_TYPE(c), GET_LLVM_TYPE(d)

TypeScheme *create_printf_type_scheme();

void _add_native_functions(ht *stack, LLVMModuleRef module, TypeEnv *env) {
  const char *name = "printf";
  int name_len = strlen(name);
  LLVMTypeRef printf_args[] = {
      LLVMPointerType(LLVMInt8Type(), 0)}; // char* (i8*)
  LLVMTypeRef printf_type = LLVMFunctionType(
      LLVMInt32Type(), printf_args, 1, 1); // return type i32, 1 arg, varargs
  LLVMValueRef printf_func = LLVMAddFunction(module, name, printf_type);

  JITSymbol *v = malloc(sizeof(JITSymbol));
  *v = (JITSymbol){.llvm_type = printf_type,
                   .symbol_type = STYPE_FUNCTION,
                   .val = printf_func};

  ht_set_hash(stack, name, hash_string(name, name_len), v);
  // *env = extend_env(*env, name, create_printf_type_scheme());

  ADD_NATIVE_FUNCTION(stack, module, init_audio, void, 0);
}

// Create the TypeScheme for printf (String -> void)
TypeScheme *create_printf_type_scheme() {
  // Create the String type
  Type *string_type = malloc(sizeof(Type));
  string_type->kind = T_STRING;

  // Create the Void type
  Type *void_type = malloc(sizeof(Type));
  void_type->kind = T_VOID;

  // Create the function type String -> Void
  Type *fn_type = create_function_type(string_type, void_type);

  // Create the TypeScheme
  TypeScheme *scheme = malloc(sizeof(TypeScheme));
  scheme->variables = NULL; // No type variables in this scheme
  scheme->num_variables = 0;
  scheme->type = fn_type;

  return scheme;
}
