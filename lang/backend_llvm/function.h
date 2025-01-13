#ifndef _LANG_BACKEND_LLVM_FUNCTION_H
#define _LANG_BACKEND_LLVM_FUNCTION_H
#include "common.h"
#include "types/type.h"
#include "llvm-c/Types.h"

LLVMTypeRef fn_prototype(Type *fn_type, int fn_len, TypeEnv *env,
                         LLVMModuleRef module);

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder);

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder);

LLVMValueRef get_specific_callable(JITSymbol *sym, const char *sym_name,
                                   Type *expected_fn_type, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

void add_recursive_fn_ref(ObjString fn_name, LLVMValueRef func, Type *fn_type,
                          JITLangCtx *fn_ctx);

typedef LLVMValueRef (*LLVMBinopMethod)(LLVMValueRef, LLVMValueRef,
                                        LLVMModuleRef, LLVMBuilderRef);

LLVMValueRef specific_fns_lookup(SpecificFns *list, Type *key);

SpecificFns *specific_fns_extend(SpecificFns *list, Type *arg_types,
                                 LLVMValueRef func);

Ast *get_specific_fn_ast_variant(Ast *original_fn_ast, Type *specific_fn_type);

LLVMValueRef
call_coroutine_generator_symbol(LLVMValueRef _instance, JITSymbol *sym,
                                Ast *args, int args_len, Type *expected_fn_type,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef handle_type_conversions(LLVMValueRef val, Type *from_type,
                                     Type *to_type, LLVMModuleRef module,
                                     LLVMBuilderRef builder);
#endif
