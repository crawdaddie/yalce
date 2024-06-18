#ifndef _LANG_BACKEND_LLVM_COMMON_H
#define _LANG_BACKEND_LLVM_COMMON_H

#include "ht.h"
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
  STYPE_FUNCTION
} symbol_type;

typedef struct {
  symbol_type symbol_type;
  LLVMTypeRef llvm_type;
  LLVMValueRef val;
} JITSymbol;

typedef struct {
  int stack_level;
  JITSymbol val;
} JITLookupResult;

#endif
