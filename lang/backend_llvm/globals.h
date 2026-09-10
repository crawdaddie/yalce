#ifndef _LANG_BACKEND_LLVM_CODEGEN_GLOBALS_H
#define _LANG_BACKEND_LLVM_CODEGEN_GLOBALS_H

#include "backend_llvm/common.h"

void codegen_set_global(const char *sym_name, JITSymbol *sym,
                        LLVMValueRef value, Type *ttype, LLVMTypeRef llvm_type,
                        JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder);

LLVMValueRef codegen_get_global(const char *sym_name, JITSymbol *sym,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

void setup_global_storage(LLVMModuleRef module, LLVMBuilderRef builder);

// Return the @global_storage_array external LLVM global in `module`,
// creating it (external linkage, [1024 x i8*]) if absent. The array is
// resolved across per-REPL-input LLVM modules to one process symbol
// (mapped to the C global_storage_array), so values stored via a slot
// in one input load in a later input.
LLVMValueRef get_global_storage_array(LLVMModuleRef module);

#endif
