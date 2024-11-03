#ifndef _LANG_BACKEND_LLVM_SYMBOLS_H
#define _LANG_BACKEND_LLVM_SYMBOLS_H

#include "backend_llvm/common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_identifier(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

int codegen_lookup_id(const char *id, int length, JITLangCtx *ctx,
                      JITSymbol **result);

JITSymbol *lookup_id_mutable(const char *id, int length, JITLangCtx *ctx);

LLVMValueRef codegen_assignment(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef codegen_multiple_assignment(Ast *binding, LLVMValueRef expr_val,
                                         Type *expr_type, JITLangCtx *ctx,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         bool is_fn_param, int fn_param_idx);

LLVMValueRef codegen_single_assignment(Ast *id, LLVMValueRef expr_val,
                                       Type *expr_type, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder, bool is_fn_param,
                                       int fn_param_idx);

void bind_symbol_in_scope(const char *id, uint64_t id_hash, LLVMTypeRef type,
                          LLVMValueRef val, symbol_type sym_type,
                          JITLangCtx *ctx);

LLVMValueRef current_func(LLVMBuilderRef builder);

JITSymbol *lookup_id_ast(Ast *id, JITLangCtx *ctx);

JITSymbol *lookup_id_in_current_scope(Ast *ast, JITLangCtx *ctx);

JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type);

int lookup_id_ast_in_place(Ast *ast, JITLangCtx *ctx, JITSymbol *sym);
JITSymbol *sym_lookup_by_name_mut(ObjString key, JITLangCtx *ctx);
TypeEnv *initialize_builtin_funcs(ht *stack, TypeEnv *env);

// TODO: use gperf for better builtin func lookups 
#define SYM_NAME_ARRAY_AT "array_at"
#define SYM_NAME_ARRAY_SIZE "array_size"
#define SYM_NAME_ARRAY_DATA_PTR "array_data_ptr"
#define SYM_NAME_ARRAY_INCR "array_incr"
#define SYM_NAME_ARRAY_SLICE "array_slice"
#define SYM_NAME_ARRAY_NEW "array_new"
#define SYM_NAME_ARRAY_TO_LIST "array_to_list"
#define SYM_NAME_DEREF "deref"
#define SYM_NAME_STRING_AT "string_at"
#define SYM_NAME_ITER_OF_LIST "iter_of_list"
#define SYM_NAME_ITER_OF_ARRAY "iter_of_array"
#define SYM_NAME_ITER_OF_ARRAY_INF "iter_of_array_inf"
#endif
