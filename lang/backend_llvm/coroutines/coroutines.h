#ifndef _LANG_BACKEND_LLVM_COROUTINE_HPP
#define _LANG_BACKEND_LLVM_COROUTINE_HPP

#include "../../parse.h"
#include "../common.h"
#include "llvm-c/Core.h"

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible function declarations
//
LLVMValueRef compile_coroutine(LLVMModuleRef module, LLVMContextRef context,
                               LLVMBuilderRef builder);

LLVMTypeRef cor_inst_struct_type();
LLVMValueRef compile_coroutine_expression(Ast *ast, JITLangCtx *ctx,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder);

LLVMValueRef create_coroutine_instance_from_constructor(JITSymbol *sym,
                                                        Ast *args, int args_len,
                                                        JITLangCtx *ctx,
                                                        LLVMModuleRef module,
                                                        LLVMBuilderRef builder);

LLVMValueRef create_coroutine_instance_from_generic_constructor(
    JITSymbol *sym, Type *expected_type, Ast *args, int args_len,
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef yield_from_coroutine_instance(JITSymbol *sym, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef WrapCoroutineWithEffectHandler(Ast *ast, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder);

LLVMValueRef MapCoroutineHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef IterOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef IterOfArrayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef RunInSchedulerHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

int get_inner_state_slot(Ast *ast);

LLVMValueRef get_inner_state_slot_gep(int slot, Ast *ast,
                                      LLVMBuilderRef builder);

LLVMValueRef CorReplaceHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef IterHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef CoroutineEndHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef UseOrFinishHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

void run_coroutine_passes_on_function(LLVMValueRef func_ref);

void run_coroutine_passes_on_module(LLVMModuleRef module_ref);

#ifdef __cplusplus
}

namespace coroutines {}

#endif

#endif // _LANG_BACKEND_LLVM_COROUTINE_HPP
