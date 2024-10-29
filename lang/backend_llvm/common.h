#ifndef _LANG_BACKEND_LLVM_COMMON_H
#define _LANG_BACKEND_LLVM_COMMON_H

#include "ht.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"

#define STACK_MAX 256

typedef struct {
  // ht stack[STACK_MAX];
  ht *stack;
  int stack_ptr;
  TypeEnv *env;
  int *num_globals;
  void **global_storage_array;
  int *global_storage_capacity;
} JITLangCtx;

typedef struct SpecificFns {
  Type *arg_types_key;
  LLVMValueRef func;
  struct SpecificFns *next;
} SpecificFns;

typedef struct coroutine_generator_symbol_data_t {
  Ast *ast;
  int stack_ptr;
  Type *ret_option_type;
  LLVMTypeRef llvm_ret_option_type;
  Type *params_obj_type;
  LLVMTypeRef llvm_params_obj_type;
  LLVMTypeRef def_fn_type;
  bool recursive_ref;
} coroutine_generator_symbol_data_t;

typedef enum symbol_type {
  STYPE_FN_PARAM,
  STYPE_TOP_LEVEL_VAR,
  STYPE_LOCAL_VAR,
  STYPE_FUNCTION,
  STYPE_GENERIC_FUNCTION,
  STYPE_COROUTINE_GENERATOR,
  STYPE_COROUTINE_INSTANCE,
  STYPE_GENERIC_COROUTINE_GENERATOR,
} symbol_type;

typedef struct {
  symbol_type type;
  LLVMTypeRef llvm_type;
  LLVMValueRef val;
  union {
    int STYPE_TOP_LEVEL_VAR;
    int STYPE_FN_PARAM;

    struct {
      Type *fn_type;
      int num_args;
      bool recursive_ref;
    } STYPE_FUNCTION;

    struct {
      Ast *ast;
      int stack_ptr;
      SpecificFns *specific_fns;
    } STYPE_GENERIC_FUNCTION;

    struct {
      Ast *ast;
      int stack_ptr;
      SpecificFns *specific_fns;
    } STYPE_GENERIC_COROUTINE_GENERATOR;

    coroutine_generator_symbol_data_t STYPE_COROUTINE_GENERATOR;

    struct {
      LLVMTypeRef def_fn_type;
    } STYPE_COROUTINE_INSTANCE;

  } symbol_data;
  Type *symbol_type;
} JITSymbol;

typedef struct {
  int stack_level;
  JITSymbol val;
} JITLookupResult;

JITLangCtx ctx_push(JITLangCtx ctx);

#endif
