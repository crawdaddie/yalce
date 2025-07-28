#ifndef _LANG_BACKEND_LLVM_COROUTINE_H
#define _LANG_BACKEND_LLVM_COROUTINE_H
#include "common.h"
#include "parse.h"

LLVMValueRef create_coroutine_constructor_binding(Ast *binding, Ast *ast,
                                                  JITLangCtx *ctx,
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

int get_inner_state_slot(Ast *ast, JITLangCtx *ctx);

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

LLVMTypeRef cor_inst_struct_type();
LLVMTypeRef cor_coroutine_fn_type();
LLVMValueRef get_instance_state_gep(LLVMValueRef instance_ptr,
                                    LLVMBuilderRef builder);
LLVMValueRef _cor_next(LLVMValueRef instance_ptr, LLVMValueRef ret_val_ref,
                       LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef null_cor_inst();

LLVMValueRef _cor_map(LLVMValueRef instance_ptr, LLVMValueRef map_fn,
                      LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef _cor_alloc(LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef _cor_loop(LLVMValueRef instance_ptr, LLVMModuleRef module,
                       LLVMBuilderRef builder);

LLVMValueRef get_instance_counter_gep(LLVMValueRef instance_ptr,
                                      LLVMBuilderRef builder);

LLVMValueRef _cor_replace(LLVMValueRef this, LLVMValueRef other,
                          LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef _cor_stop(LLVMValueRef this, LLVMModuleRef module,
                       LLVMBuilderRef builder);
#endif
