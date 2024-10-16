#ifndef _LANG_BACKEND_LLVM_YIELD_H
#define _LANG_BACKEND_LLVM_YIELD_H
#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_yield(Ast *, JITLangCtx *, LLVMModuleRef, LLVMBuilderRef);
#endif
