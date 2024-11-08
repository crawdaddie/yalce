#ifndef _LANG_BACKEND_LLVM_COROUTINES_H #define _LANG_BACKEND_LLVM_COROUTINES_H
#include "common.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);

// LLVMValueRef codegen_coroutine_instance(Ast *args, int args_len,
//                                         JITSymbol *symbol, JITLangCtx *ctx,
//                                         LLVMModuleRef module,
//                                         LLVMBuilderRef builder);
//
LLVMValueRef codegen_coroutine_instance(LLVMValueRef instance, Ast *args,
                                        int args_len, Type *instance_type,
                                        LLVMValueRef def_fn, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef codegen_coroutine_next(LLVMValueRef instance,
                                    LLVMTypeRef instance_type,
                                    LLVMTypeRef def_fn_type, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder);

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

LLVMValueRef coroutine_loop(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef coroutine_map(Ast *ast, JITSymbol *sym, JITLangCtx *ctx,
                           LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef coroutine_def(Ast *fn_ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);
#endif
