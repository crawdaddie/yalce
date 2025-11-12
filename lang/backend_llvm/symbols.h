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

LLVMValueRef codegen_let_expr(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder);

void bind_symbol_in_scope(const char *id, uint64_t id_hash, LLVMTypeRef type,
                          LLVMValueRef val, symbol_type sym_type,
                          JITLangCtx *ctx);

LLVMValueRef current_func(LLVMBuilderRef builder);

JITSymbol *lookup_id_ast(Ast *id, JITLangCtx *ctx);

JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type);

JITSymbol *create_generic_fn_symbol(Ast *fn_ast, JITLangCtx *ctx);

LLVMValueRef create_generic_fn_binding(Ast *binding, Ast *fn_ast,
                                       JITLangCtx *ctx);

LLVMValueRef create_fn_binding(Ast *binding, Type *fn_type, LLVMValueRef fn,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);
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
#define SYM_NAME_LOOP "loop"
#define SYM_NAME_MAP_ITER "map_iter"
#define SYM_NAME_ITER_COR "iter_cor"
#endif
