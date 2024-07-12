#ifndef _LANG_BACKEND_LLVM_COMMON_H
#define _LANG_BACKEND_LLVM_COMMON_H

#include "ht.h"
#include "parse.h"
#include "types/util.h"
#include "llvm-c/Types.h"

typedef struct {
  // ht stack[STACK_MAX];
  ht *stack;
  int stack_ptr;
  TypeEnv *env;
} JITLangCtx;

typedef struct SpecificFns {
  const char *serialized_type;
  LLVMValueRef func;
  struct SpecificFns *next;
} SpecificFns;

typedef enum symbol_type {
  STYPE_FN_PARAM,
  STYPE_TOP_LEVEL_VAR,
  STYPE_LOCAL_VAR,
  STYPE_FUNCTION,
  STYPE_GENERIC_FUNCTION,
  STYPE_MODULE,
} symbol_type;

typedef struct {
  symbol_type type;
  LLVMTypeRef llvm_type;
  LLVMValueRef val;
  union {
    int STYPE_FN_PARAM;
    struct {
      Type *fn_type;
    } STYPE_FUNCTION;
    struct {
      Ast *ast;
      int stack_ptr;
      SpecificFns *specific_fns;
    } STYPE_GENERIC_FUNCTION;
    struct {
      ht *symbols;
    } STYPE_MODULE;
  } symbol_data;
} JITSymbol;

typedef struct {
  int stack_level;
  JITSymbol val;
} JITLookupResult;

JITLangCtx ctx_push(JITLangCtx ctx);

#endif
