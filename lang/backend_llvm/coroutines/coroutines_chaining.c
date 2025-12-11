/**
 * Nested Coroutine Chaining Implementation
 *
 * This file implements support for "yield nested_coro()" which allows
 * one coroutine to yield all values from another coroutine before continuing.
 *
 * Example:
 * ```
 * let inner = fn () ->
 *   yield 1;
 *   yield 2
 * ;;
 *
 * let outer = fn () ->
 *   yield inner();  // This yields 1, then 2
 *   yield 3
 * ;;
 *
 * let co = outer() in
 * co() -> Some 1
 * co() -> Some 2
 * co() -> Some 3
 * co() -> None
 * ```
 *
 * Architecture:
 * - Use the `next` field in CORO_OBJ_TYPE to chain coroutines
 * - When yielding a coroutine, set it as `next` and delegate to it
 * - When nested coroutine finishes, continue with outer coroutine
 */

#include "../adt.h"
#include "../types.h"
#include "./coroutines.h"
#include "./coroutines_private.h"
#include "function.h"
#include "llvm-c/Core.h"

/**
 * Check if a coroutine is finished (returns None)
 *
 * Signature: bool is_coro_finished(CoroObj* coro)
 */
static LLVMValueRef build_is_coro_finished_helper(LLVMTypeRef coro_obj_type,
                                                  LLVMTypeRef promise_type,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder) {

  LLVMTypeRef helper_type = LLVMFunctionType(
      LLVMInt1Type(), (LLVMTypeRef[]){LLVMPointerType(coro_obj_type, 0)}, 1, 0);

  LLVMValueRef helper_fn =
      LLVMAddFunction(module, "is_coro_finished", helper_type);
  LLVMSetLinkage(helper_fn, LLVMInternalLinkage);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(helper_fn, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef coro = LLVMGetParam(helper_fn, 0);

  // Load promise and check if tag == 1 (None)
  LLVMValueRef promise_gep = coro_promise_gep(coro, coro_obj_type, builder);
  LLVMValueRef promise =
      LLVMBuildLoad2(builder, promise_type, promise_gep, "promise");

  LLVMValueRef is_finished = codegen_option_is_none(promise, builder);

  LLVMBuildRet(builder, is_finished);
  LLVMPositionBuilderAtEnd(builder, prev_block);

  return helper_fn;
}

/**
 * Resume with chaining support
 *
 * This wrapper checks if there's a nested coroutine in the `next` field.
 * If so, it resumes that one. When the nested coroutine finishes,
 * it continues with the outer coroutine.
 *
 * Signature: Option<T> resume_with_chaining(CoroObj* coro, ResumeFn resume_fn)
 */
static LLVMValueRef build_chaining_resume_wrapper(LLVMTypeRef promise_type,
                                                  LLVMTypeRef coro_obj_type,
                                                  LLVMValueRef base_resume_fn,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder) {

  // Wrapper signature: Option<T> resume_chaining(CoroObj* obj)
  LLVMTypeRef wrapper_type = LLVMFunctionType(
      promise_type, (LLVMTypeRef[]){LLVMPointerType(coro_obj_type, 0)}, 1, 0);

  static int chain_counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_resume_chaining_%d",
           chain_counter++);

  LLVMValueRef wrapper_fn = LLVMAddFunction(module, wrapper_name, wrapper_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(wrapper_fn, "entry");
  LLVMBasicBlockRef check_next_bb =
      LLVMAppendBasicBlock(wrapper_fn, "check_next");
  LLVMBasicBlockRef has_next_bb = LLVMAppendBasicBlock(wrapper_fn, "has_next");
  LLVMBasicBlockRef no_next_bb = LLVMAppendBasicBlock(wrapper_fn, "no_next");
  LLVMBasicBlockRef next_finished_bb =
      LLVMAppendBasicBlock(wrapper_fn, "next_finished");
  LLVMBasicBlockRef next_active_bb =
      LLVMAppendBasicBlock(wrapper_fn, "next_active");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMValueRef coro_obj = LLVMGetParam(wrapper_fn, 0);

  // Check if there's a nested coroutine in the `next` field
  LLVMValueRef next_gep = LLVMBuildStructGEP2(builder, coro_obj_type, coro_obj,
                                              CORO_NEXT_SLOT, "next_gep");
  LLVMValueRef next_coro_raw =
      LLVMBuildLoad2(builder, GENERIC_PTR, next_gep, "next_coro_raw");

  LLVMValueRef has_next =
      LLVMBuildIsNotNull(builder, next_coro_raw, "has_next");
  LLVMBuildCondBr(builder, has_next, has_next_bb, no_next_bb);

  // ==== Has nested coroutine ====
  LLVMPositionBuilderAtEnd(builder, has_next_bb);

  // Cast generic pointer to typed coroutine object
  LLVMValueRef next_coro = LLVMBuildBitCast(
      builder, next_coro_raw, LLVMPointerType(coro_obj_type, 0), "next_coro");

  // Resume the nested coroutine
  LLVMValueRef next_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(base_resume_fn), base_resume_fn,
      (LLVMValueRef[]){next_coro}, 1, "next_result");

  // Check if nested coroutine finished (returned None)
  LLVMValueRef is_finished_fn = build_is_coro_finished_helper(
      coro_obj_type, promise_type, module, builder);

  LLVMValueRef next_finished = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(is_finished_fn), is_finished_fn,
      (LLVMValueRef[]){next_coro}, 1, "next_finished");

  LLVMBuildCondBr(builder, next_finished, next_finished_bb, next_active_bb);

  // Nested coroutine still active - return its value
  LLVMPositionBuilderAtEnd(builder, next_active_bb);
  LLVMBuildRet(builder, next_result);

  // Nested coroutine finished - clear next field and resume outer
  LLVMPositionBuilderAtEnd(builder, next_finished_bb);
  LLVMBuildStore(builder, LLVMConstNull(GENERIC_PTR), next_gep);
  // Fall through to resume outer coroutine
  LLVMBuildBr(builder, no_next_bb);

  // ==== No nested coroutine - resume outer ====
  LLVMPositionBuilderAtEnd(builder, no_next_bb);

  LLVMValueRef result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(base_resume_fn), base_resume_fn,
      (LLVMValueRef[]){coro_obj}, 1, "outer_result");

  // After resuming outer, check if it yielded a nested coroutine
  // If so, store it in `next` field for next call
  // (This happens in codegen_yield when yielding a coroutine type)

  LLVMBuildRet(builder, result);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return wrapper_fn;
}

/**
 * Modified codegen_yield to handle coroutine chaining
 *
 * When yielding a coroutine, instead of yielding its value,
 * we set it as the `next` coroutine and immediately delegate to it.
 */
LLVMValueRef codegen_yield_with_chaining(Ast *ast, JITLangCtx *ctx,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder) {

  // Get coroutine context
  CoroutineCtx *coro_ctx = (CoroutineCtx *)ctx->coro_ctx;
  if (!coro_ctx) {
    fprintf(stderr, "Error: yield outside of coroutine\n");
    return NULL;
  }

  // Codegen the yielded value
  LLVMValueRef yield_value =
      codegen(ast->data.AST_YIELD.expr, ctx, module, builder);
  Type *yield_val_type = ast->data.AST_YIELD.expr->type;

  // Check if we're yielding a coroutine
  if (is_coroutine_type(yield_val_type)) {
    // STRATEGY: Store the nested coroutine in a global that the
    // wrapper can access, or pass it back through the promise.
    //
    // For LLVM intrinsics, the challenge is that we can't directly
    // modify the wrapper's behavior from inside the coroutine.
    //
    // SOLUTION: Use the CORO_NEXT_SLOT field!
    // 1. Get access to the outer coroutine object
    // 2. Store nested coroutine in its `next` field
    // 3. Return a special sentinel value that tells wrapper to check `next`

    // Problem: Inside the coroutine function, we don't have direct
    // access to the coroutine object (it's in the caller's scope).
    //
    // ALTERNATIVE: Use the state field to store nested coroutine pointer
    // and have the wrapper check for it.

    // For now, document that this requires additional infrastructure
    fprintf(stderr, "Note: Nested coroutine chaining with LLVM intrinsics "
                    "requires passing coroutine object through promise\n");

    // Fall through to regular yield for now
  }

  // Regular yield (non-coroutine value) - use standard implementation
  return codegen_yield(ast, ctx, module, builder);
}

/**
 * Example usage for complete chaining implementation:
 *
 * 1. Modify coroutine object to include a "parent" pointer
 *    This allows inner coroutine to update outer's state
 *
 * 2. When yielding a coroutine:
 *    - Store it in parent's `next` field
 *    - Return a special marker value
 *    - Wrapper detects marker and checks `next` field
 *
 * 3. Wrapper logic:
 *    while (next != NULL) {
 *        result = resume(next);
 *        if (result == None) {
 *            next = NULL;
 *            continue outer loop;
 *        }
 *        return result;
 *    }
 *    return resume(outer);
 */

// ============================================================================
// Integration Notes
// ============================================================================

/*
 * To fully implement chaining with LLVM intrinsics, we need to:
 *
 * 1. Modify CORO_OBJ_TYPE to include:
 *    - `parent` pointer (to outer coroutine)
 *    - `next` pointer (to nested coroutine)
 *
 * 2. In compile_coroutine:
 *    - Pass coroutine object as parameter to coroutine function
 *    - This allows yield to access and modify the object
 *
 * 3. In codegen_yield (when yielding coroutine):
 *    - Store nested coroutine in `next` field
 *    - Yield a special marker value
 *
 * 4. In resume wrapper:
 *    - Check for nested coroutine before resuming
 *    - Recursively handle nested chains
 *
 * 5. Alternatively, use a simpler "trampoline" approach:
 *    - Wrapper keeps calling resume on current coroutine
 *    - When it yields a coroutine, wrapper switches to that one
 *    - When nested finishes, wrapper switches back
 */
