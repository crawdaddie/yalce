#ifndef _LANG_BACKEND_LLVM_COMMON_H
#define _LANG_BACKEND_LLVM_COMMON_H

#include "ht.h"
#include "llvm-c/Types.h"
typedef struct {
  // ht stack[STACK_MAX];
  ht *stack;
  int stack_ptr;
} JITLangCtx;

typedef struct {
  LLVMTypeRef type;
  LLVMValueRef val;
} JITValue;

typedef struct {
  int stack_level;
  JITValue val;
  // LLVMValueRef val;
} JITLookupResult;

#endif
