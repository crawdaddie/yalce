#ifndef _LANG_BACKEND_LLVM_COMMON_H
#define _LANG_BACKEND_LLVM_COMMON_H

#include "../ht.h"
#include "../parse.h"
#include "../types/type.h"
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
  const char *serialized_type;
  LLVMValueRef func;
  struct SpecificFns *next;
} SpecificFns;

typedef struct FnVariants {
  Type *type;
  LLVMValueRef func;
  struct FnVariants *next;
} FnVariants;

typedef enum symbol_type {
  STYPE_FN_PARAM,
  STYPE_TOP_LEVEL_VAR,
  STYPE_LOCAL_VAR,
  STYPE_FUNCTION,
  STYPE_GENERIC_FUNCTION,
  STYPE_MODULE,
  STYPE_FN_VARIANTS
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
    } STYPE_FUNCTION;

    struct {
      Ast *ast;
      int stack_ptr;
      SpecificFns *specific_fns;
    } STYPE_GENERIC_FUNCTION;

    FnVariants STYPE_FN_VARIANTS;

    struct {
      ht *symbols;
    } STYPE_MODULE;
  } symbol_data;
  Type *symbol_type;
} JITSymbol;

typedef struct {
  int stack_level;
  JITSymbol val;
} JITLookupResult;

JITLangCtx ctx_push(JITLangCtx ctx);

#endif
