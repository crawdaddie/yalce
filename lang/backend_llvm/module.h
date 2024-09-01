#ifndef _LANG_BACKEND_LLVM_CODEGEN_MODULE_H
#define _LANG_BACKEND_LLVM_CODEGEN_MODULE_H
#include "backend_llvm/common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"
void import_module(char *dirname, Ast *import, TypeEnv **env, JITLangCtx *ctx,
                   LLVMModuleRef main_module, LLVMContextRef llvm_ctx);
#endif
