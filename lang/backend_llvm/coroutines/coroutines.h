#ifndef _LANG_BACKEND_LLVM_COROUTINES_H
#define _LANG_BACKEND_LLVM_COROUTINES_H

#define PRESPLIT_COROUTINE_KIND_ID 50
#include "../common.h"

typedef struct {
  // Common fields
  Type *cons_type;
  LLVMTypeRef coro_obj_type;
  LLVMTypeRef promise_type;

  // Fields for old switch-based implementation
  // int num_coroutine_yields;
  // int current_yield;
  // LLVMBasicBlockRef *branches;
  // LLVMBasicBlockRef switch_default;
  // LLVMValueRef switch_ref;
  // LLVMValueRef func;
  // LLVMTypeRef state_layout;
  // const char *name;

  // Fields for LLVM intrinsics implementation
  LLVMValueRef coro_id;         // Result of coro.id
  LLVMValueRef coro_handle;     // Result of coro.begin
  LLVMValueRef promise_alloca;  // Alloca for promise storage
  LLVMBasicBlockRef cleanup_bb; // Basic block for cleanup
  LLVMBasicBlockRef suspend_bb; // Basic block for final suspend
  LLVMBasicBlockRef start_bb;   // Basic block for start
  Type *yield_type;             // Type of yielded values
  LLVMTypeRef llvm_yield_type;  // Type of yielded values
  int yield_count;              // Number of yields encountered
  const char *coro_name;
  int num_param_allocas;
  LLVMValueRef *param_allocas;

  AstList *yield_boundary_xs;
  int num_yield_boundary_xs;
} CoroutineCtx;

LLVMValueRef compile_coroutine(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *app,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

#define PTR_ID_FUNC_TYPE(obj)                                                  \
  LLVMFunctionType(LLVMPointerType(obj, 0),                                    \
                   (LLVMTypeRef[]){LLVMPointerType(obj, 0)}, 1, 0)

LLVMValueRef create_coroutine_symbol(Ast *binding, Ast *expr, Type *expr_type,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder);
// intrinsic declarations
LLVMValueRef get_coro_id_intrinsic(LLVMModuleRef module);
/**
 * Get or declare llvm.coro.begin intrinsic
 * Signature: i8* @llvm.coro.begin(token id, i8* mem)
 */
LLVMValueRef get_coro_begin_intrinsic(LLVMModuleRef module);
/**
 * Get or declare llvm.coro.size.i64 intrinsic
 * Signature: i64 @llvm.coro.size.i64()
 */
LLVMValueRef get_coro_size_intrinsic(LLVMModuleRef module);

/**
 * Get or declare llvm.coro.save intrinsic
 * Signature: token @llvm.coro.save(i8* handle)
 */
LLVMValueRef get_coro_save_intrinsic(LLVMModuleRef module);
/**
 * Get or declare llvm.coro.suspend intrinsic
 * Signature: i8 @llvm.coro.suspend(token save, i1 final)
 */
LLVMValueRef get_coro_suspend_intrinsic(LLVMModuleRef module);

/**
 * Get or declare llvm.coro.end intrinsic
 * Signature: i1 @llvm.coro.end(i8* handle, i1 unwind)
 */
LLVMValueRef get_coro_end_intrinsic(LLVMModuleRef module);

/**
 * Get or declare llvm.coro.free intrinsic
 * Signature: i8* @llvm.coro.free(token id, i8* handle)
 */
LLVMValueRef get_coro_free_intrinsic(LLVMModuleRef module);

/**
 * Get or declare llvm.coro.resume intrinsic
 * Signature: void @llvm.coro.resume(i8* handle)
 */
LLVMValueRef get_coro_resume_intrinsic(LLVMModuleRef module);
/**
 * Get or declare llvm.coro.done intrinsic
 * Signature: i1 @llvm.coro.done(i8* handle)
 */
LLVMValueRef get_coro_done_intrinsic(LLVMModuleRef module);
/**
 * Get or declare llvm.coro.promise intrinsic
 * Signature: i8* @llvm.coro.promise(i8* handle, i32 align, i1 from_promise)
 */
LLVMValueRef get_coro_promise_intrinsic(LLVMModuleRef module);

#define COROUTINE_BASIC_BLOCKS(coro_fn)                                        \
  LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(coro_fn, "entry");         \
  LLVMBasicBlockRef cleanup_bb = LLVMAppendBasicBlock(coro_fn, "cleanup");     \
  LLVMBasicBlockRef suspend_bb = LLVMAppendBasicBlock(coro_fn, "suspend");     \
  LLVMBasicBlockRef initial_return_bb =                                        \
      LLVMAppendBasicBlock(coro_fn, "initial.return");                         \
  LLVMBasicBlockRef start_bb = LLVMAppendBasicBlock(coro_fn, "start");

#define COROUTINE_ATTR_MARKING(coro_fn)                                        \
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);                      \
  LLVMAttributeRef attr =                                                      \
      LLVMCreateEnumAttribute(llvm_ctx, PRESPLIT_COROUTINE_KIND_ID, 0);        \
  LLVMAddAttributeAtIndex(coro_fn, LLVMAttributeFunctionIndex, attr);

#endif
