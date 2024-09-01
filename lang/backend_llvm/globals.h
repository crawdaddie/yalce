#ifndef _LANG_BACKEND_LLVM_CODEGEN_GLOBALS_H
#define _LANG_BACKEND_LLVM_CODEGEN_GLOBALS_H

#include "backend_llvm/common.h"

void codegen_set_global(JITSymbol *sym, LLVMValueRef value, Type *ttype,
                        LLVMTypeRef llvm_type, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen_get_global(JITSymbol *sym, LLVMModuleRef module,
                                LLVMBuilderRef builder);

void setup_global_storage(LLVMModuleRef module, LLVMBuilderRef builder);

#endif
