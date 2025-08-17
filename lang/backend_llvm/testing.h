#ifndef _LANG_TEST_MODULE_H
#define _LANG_TEST_MODULE_H
#include "common.h"
#include "parse.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Types.h"

int test_module(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                LLVMBuilderRef builder, LLVMTargetMachineRef target_matchine);
#endif
