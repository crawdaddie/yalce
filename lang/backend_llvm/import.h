#ifndef _LANG_BACKEND_LLVM_CODEGEN_IMPORT_H
#define _LANG_BACKEND_LLVM_CODEGEN_IMPORT_H
#include "common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"

// void import_module(char *dirname, Ast *import, TypeEnv **env, JITLangCtx
// *ctx,
//                    LLVMModuleRef main_module, LLVMContextRef llvm_ctx);

void import_module(Ast *binding, Ast *import_name, const char *dirname,
                   JITLangCtx *ctx, LLVMModuleRef llvm_module,
                   LLVMBuilderRef builder, LLVMContextRef llvm_ctx);
extern ht modules;
#endif
