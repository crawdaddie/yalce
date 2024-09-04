#ifndef _LANG_BACKEND_LLVM_STRINGS_H
#define _LANG_BACKEND_LLVM_STRINGS_H

#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef stream_string_concat(LLVMValueRef *strings, int num_strings,
                                  LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen_string(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);
#endif
