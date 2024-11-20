#ifndef _LANG_BACKEND_LLVM_COROUTINES_H #define _LANG_BACKEND_LLVM_COROUTINES_H
#include "common.h"
#include "llvm-c/Types.h"

LLVMTypeRef coroutine_def_fn_type(LLVMTypeRef instance_type,
                                  LLVMTypeRef ret_option_type);

LLVMTypeRef coroutine_instance_type(LLVMTypeRef params_obj_type);

LLVMValueRef coroutine_instance_counter_gep(LLVMValueRef instance_ptr,
                                            LLVMTypeRef instance_type,
                                            LLVMBuilderRef builder);

LLVMValueRef coroutine_instance_fn_gep(LLVMValueRef instance_ptr,
                                       LLVMTypeRef instance_type,
                                       LLVMBuilderRef builder);
LLVMValueRef coroutine_instance_params_gep(LLVMValueRef instance_ptr,
                                           LLVMTypeRef instance_type,
                                           LLVMBuilderRef builder);

LLVMValueRef coroutine_instance_parent_gep(LLVMValueRef instance_ptr,
                                           LLVMTypeRef instance_type,
                                           LLVMBuilderRef builder);

void increment_instance_counter(LLVMValueRef instance_ptr,
                                LLVMTypeRef instance_type,
                                LLVMBuilderRef builder);

void reset_instance_counter(LLVMValueRef instance_ptr,
                            LLVMTypeRef instance_type, LLVMBuilderRef builder);

void set_instance_counter(LLVMValueRef instance_ptr, LLVMTypeRef instance_type,
                          LLVMValueRef counter, LLVMBuilderRef builder);
LLVMValueRef replace_instance(LLVMValueRef instance, LLVMTypeRef instance_type,
                              LLVMValueRef new_instance,
                              LLVMBuilderRef builder);

LLVMValueRef codegen_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);

LLVMValueRef set_instance_params(LLVMValueRef instance,
                                 LLVMTypeRef llvm_instance_type,
                                 LLVMTypeRef llvm_params_obj_type,
                                 LLVMValueRef *params, int params_len,
                                 LLVMBuilderRef builder);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef coroutine_next(LLVMValueRef instance, LLVMTypeRef instance_type,
                            LLVMTypeRef def_fn_type, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen_generic_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder);

LLVMValueRef list_iter_instance(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef array_iter_instance(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef coroutine_array_iter_generator_fn(Type *expected_type, bool inf,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder);

LLVMValueRef coroutine_list_iter_generator_fn(Type *expected_type,
                                              JITLangCtx *ctx,
                                              LLVMModuleRef module,
                                              LLVMBuilderRef builder);

LLVMValueRef compile_generic_coroutine(JITSymbol *sym, Type *expected_fn_type,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder);

LLVMValueRef generic_coroutine_instance(Ast *application_args, int args_len,
                                        Type *def_type, LLVMValueRef func,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder);

LLVMValueRef coroutine_loop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef coroutine_map(Ast *ast, JITSymbol *sym, JITLangCtx *ctx,
                           LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef coroutine_def(Ast *fn_ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder, LLVMTypeRef *_llvm_def_type);

LLVMValueRef coroutine_instance_from_def_symbol(
    LLVMValueRef _instance, JITSymbol *sym, Ast *args, int args_len,
    Type *expected_fn_type, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder);

LLVMValueRef coroutine_def_from_generic(JITSymbol *sym, Type *expected_fn_type,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder);

LLVMTypeRef llvm_def_type_of_instance(Type *instance_type, JITLangCtx *ctx,
                                      LLVMModuleRef module);

LLVMValueRef codegen_coroutine_instance(LLVMValueRef _inst, Type *instance_type,
                                        LLVMValueRef func, LLVMValueRef *params,
                                        int num_params, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder);
#endif
