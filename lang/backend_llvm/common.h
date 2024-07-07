#ifndef _LANG_BACKEND_LLVM_COMMON_H
#define _LANG_BACKEND_LLVM_COMMON_H

#include "ht.h"
#include "parse.h"
#include "llvm-c/Types.h"

typedef struct {
  // ht stack[STACK_MAX];
  ht *stack;
  int stack_ptr;
} JITLangCtx;

typedef enum symbol_type {
  STYPE_FN_PARAM,
  STYPE_TOP_LEVEL_VAR,
  STYPE_LOCAL_VAR,
  STYPE_FUNCTION,
  STYPE_GENERIC_FUNCTION
} symbol_type;

typedef struct {
  symbol_type type;
  LLVMTypeRef llvm_type;
  LLVMValueRef val;
  union {
    int STYPE_FN_PARAM;
    struct {
      Ast *ast;
      int stack_ptr;
    } STYPE_GENERIC_FUNCTION;
  } symbol_data;
} JITSymbol;

typedef struct {
  int stack_level;
  JITSymbol val;
} JITLookupResult;

#endif
