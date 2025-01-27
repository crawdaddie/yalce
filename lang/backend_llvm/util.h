#ifndef _LANG_BACKEND_LLVM_UTIL_H
#define _LANG_BACKEND_LLVM_UTIL_H

#include "common.h"
#include "types/type.h"
#include "llvm-c/Types.h"

void struct_ptr_set(int item_offset, LLVMValueRef struct_ptr,
                    LLVMTypeRef struct_type, LLVMValueRef val,
                    LLVMBuilderRef builder);

LLVMValueRef struct_ptr_get(int item_offset, LLVMValueRef struct_ptr,
                            LLVMTypeRef struct_type, LLVMBuilderRef builder);

LLVMValueRef increment_ptr(LLVMValueRef ptr, LLVMTypeRef node_type,
                           LLVMValueRef element_size, LLVMBuilderRef builder);

LLVMValueRef and_vals(LLVMValueRef res, LLVMValueRef res2,
                      LLVMBuilderRef builder);

LLVMValueRef codegen_printf(const char *format, LLVMValueRef *args,
                            int arg_count, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef insert_printf_call(const char *format, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef get_extern_fn(const char *name, LLVMTypeRef fn_type,
                           LLVMModuleRef module);
LLVMValueRef alloc(LLVMTypeRef type, JITLangCtx *ctx, LLVMBuilderRef builder);

LLVMValueRef heap_alloc(LLVMTypeRef type, LLVMBuilderRef builder);

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)

#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

#define LLVM_IF_ELSE(builder, test_val, then_expr, else_expr)                  \
  ({                                                                           \
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);             \
    LLVMValueRef function = LLVMGetBasicBlockParent(current_block);            \
    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");     \
    LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");     \
    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");   \
                                                                               \
    LLVMBuildCondBr(builder, test_val, then_block, else_block);                \
                                                                               \
    /* Then block */                                                           \
    LLVMPositionBuilderAtEnd(builder, then_block);                             \
    LLVMValueRef then_result = (then_expr);                                    \
    LLVMBuildBr(builder, merge_block);                                         \
    LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(builder);            \
                                                                               \
    /* Else block */                                                           \
    LLVMPositionBuilderAtEnd(builder, else_block);                             \
    LLVMValueRef else_result = (else_expr);                                    \
    LLVMBuildBr(builder, merge_block);                                         \
    LLVMBasicBlockRef else_end_block = LLVMGetInsertBlock(builder);            \
                                                                               \
    /* Merge block */                                                          \
    LLVMPositionBuilderAtEnd(builder, merge_block);                            \
    LLVMValueRef phi =                                                         \
        LLVMBuildPhi(builder, LLVMTypeOf(then_result), "result");              \
    LLVMValueRef incoming_vals[] = {then_result, else_result};                 \
    LLVMBasicBlockRef incoming_blocks[] = {then_end_block, else_end_block};    \
    LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);                   \
    phi; /* Result value */                                                    \
  })
#endif
