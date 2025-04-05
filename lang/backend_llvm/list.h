#ifndef _LANG_BACKEND_LLVM_CODEGEN_LIST_H
#define _LANG_BACKEND_LLVM_CODEGEN_LIST_H
#include "backend_llvm/common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_list(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);

LLVMTypeRef create_llvm_list_type(Type *list_el_type, TypeEnv *env,
                                  LLVMModuleRef module);

LLVMValueRef ll_get_head_val(LLVMValueRef list, LLVMTypeRef list_el_type,
                             LLVMBuilderRef builder);

LLVMValueRef ll_get_next(LLVMValueRef list, LLVMTypeRef list_el_type,
                         LLVMBuilderRef builder);

LLVMValueRef ll_is_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                        LLVMBuilderRef builder);

LLVMValueRef ll_is_not_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                            LLVMBuilderRef builder);

LLVMTypeRef llnode_type(LLVMTypeRef llvm_el_type);

LLVMValueRef null_node(LLVMTypeRef node_type);

LLVMValueRef codegen_list_prepend(LLVMValueRef l, LLVMValueRef list,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef);

LLVMValueRef codegen_list_to_string(LLVMValueRef val, Type *val_type,
                                    JITLangCtx *ctx, LLVMModuleRef module,
                                    LLVMBuilderRef builder);
LLVMValueRef ListConcatHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef ListTailHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder);

LLVMValueRef ListPrependHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef QueueOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef QueueAppendRightHandler(Ast *ast, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder);

LLVMValueRef QueuePopLeftHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef ListRefSetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder);
#endif

