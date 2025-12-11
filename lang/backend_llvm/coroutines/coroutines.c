#include "./coroutines.h"
#include "../binding.h"
#include "../function.h"
#include "../symbols.h"
#include "./coroutines_private.h"
#include "adt.h"
#include "types.h"
#include "types/type.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdio.h>

// ============================================================================
// Helper Functions for Coroutine Private Interface
// ============================================================================

LLVMValueRef coro_counter_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, coro_obj_type, coro, CORO_COUNTER_SLOT,
                             "counter_gep");
}

LLVMValueRef coro_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                          LLVMBuilderRef builder) {
  LLVMValueRef gep = coro_counter_gep(coro, coro_obj_type, builder);
  return LLVMBuildLoad2(builder, LLVMInt32Type(), gep, "counter");
}

LLVMValueRef coro_promise_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, coro_obj_type, coro, CORO_PROMISE_SLOT,
                             "promise_gep");
}

LLVMValueRef coro_promise(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                          LLVMTypeRef promise_type, LLVMBuilderRef builder) {
  LLVMValueRef gep = coro_promise_gep(coro, coro_obj_type, builder);
  return LLVMBuildLoad2(builder, promise_type, gep, "promise");
}

LLVMValueRef coro_promise_set(LLVMValueRef coro, LLVMValueRef val,
                              LLVMTypeRef coro_obj_type,
                              LLVMTypeRef promise_type,
                              LLVMBuilderRef builder) {
  LLVMValueRef gep = coro_promise_gep(coro, coro_obj_type, builder);
  return LLVMBuildStore(builder, val, gep);
}

LLVMValueRef coro_state_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                            LLVMBuilderRef builder) {
  return LLVMBuildStructGEP2(builder, coro_obj_type, coro, CORO_STATE_SLOT,
                             "state_gep");
}

LLVMValueRef coro_state(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                        LLVMBuilderRef builder) {
  LLVMValueRef gep = coro_state_gep(coro, coro_obj_type, builder);
  return LLVMBuildLoad2(builder, GENERIC_PTR, gep, "state");
}

LLVMValueRef coro_next(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                       LLVMBuilderRef builder) {
  LLVMValueRef gep = LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                         CORO_NEXT_SLOT, "next_gep");
  return LLVMBuildLoad2(builder, GENERIC_PTR, gep, "next");
}

LLVMValueRef coro_next_set(LLVMValueRef coro, LLVMValueRef next,
                           LLVMTypeRef coro_obj_type, LLVMBuilderRef builder) {
  LLVMValueRef gep = LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                         CORO_NEXT_SLOT, "next_gep");
  return LLVMBuildStore(builder, next, gep);
}

LLVMValueRef coro_end_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  LLVMValueRef counter_gep = coro_counter_gep(coro, coro_obj_type, builder);
  return LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), -1, 1),
                        counter_gep);
}

LLVMValueRef coro_promise_set_none(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                                   LLVMTypeRef promise_type,
                                   LLVMBuilderRef builder) {
  LLVMValueRef none = codegen_none_typed(builder, promise_type);
  return coro_promise_set(coro, none, coro_obj_type, promise_type, builder);
}

void coro_terminate_block(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                          LLVMBuilderRef builder) {
  coro_end_counter(coro, coro_ctx->coro_obj_type, builder);
  coro_promise_set_none(coro, coro_ctx->coro_obj_type, coro_ctx->promise_type,
                        builder);
}

LLVMValueRef coro_advance(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                          LLVMBuilderRef builder) {
  LLVMValueRef counter = coro_counter(coro, coro_ctx->coro_obj_type, builder);
  LLVMValueRef incremented =
      LLVMBuildAdd(builder, counter, LLVMConstInt(LLVMInt32Type(), 1, 0), "");
  LLVMValueRef counter_gep =
      coro_counter_gep(coro, coro_ctx->coro_obj_type, builder);
  return LLVMBuildStore(builder, incremented, counter_gep);
}

LLVMValueRef coro_incr(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                       LLVMBuilderRef builder) {
  return coro_advance(coro, coro_ctx, builder);
}

LLVMValueRef coro_is_finished(LLVMValueRef coro, CoroutineCtx *ctx,
                              LLVMBuilderRef builder) {
  LLVMValueRef counter = coro_counter(coro, ctx->coro_obj_type, builder);
  return LLVMBuildICmp(builder, LLVMIntSLT, counter,
                       LLVMConstInt(LLVMInt32Type(), 0, 0), "is_finished");
}

LLVMValueRef coro_replace(LLVMValueRef coro, LLVMValueRef new_coro,
                          CoroutineCtx *coro_ctx, LLVMBuilderRef builder) {
  // Copy promise from new_coro to coro
  LLVMValueRef new_promise = coro_promise(new_coro, coro_ctx->coro_obj_type,
                                          coro_ctx->promise_type, builder);
  coro_promise_set(coro, new_promise, coro_ctx->coro_obj_type,
                   coro_ctx->promise_type, builder);
  return coro;
}

LLVMValueRef coro_jump_to_next_block(LLVMValueRef coro, LLVMValueRef next_coro,
                                     CoroutineCtx *coro_ctx,
                                     LLVMBuilderRef builder) {
  coro_next_set(coro, next_coro, coro_ctx->coro_obj_type, builder);
  return coro;
}

LLVMTypeRef get_coro_state_layout(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module) {
  // For LLVM intrinsics, this is not needed as LLVM handles state layout
  // Return a simple empty struct for now
  return LLVMStructType(NULL, 0, 0);
}

// ============================================================================
// LLVM Intrinsic Declaration Helpers
// ============================================================================

/**
 * Get or declare llvm.coro.id intrinsic
 * Signature: token @llvm.coro.id(i32 align, i8* promise, i8* coroaddr, i8*
 * fnaddr)
 */
static LLVMValueRef get_coro_id_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.id");
  if (!fn) {
    LLVMContextRef c = LLVMGetGlobalContext();
    LLVMTypeRef fn_type = LLVMFunctionType(LLVMTokenTypeInContext(c),
                                           (LLVMTypeRef[]){
                                               LLVMInt32Type(), // align
                                               GENERIC_PTR,     // promise
                                               GENERIC_PTR,     // coroaddr
                                               GENERIC_PTR      // fnaddr
                                           },
                                           4, 0);
    fn = LLVMAddFunction(module, "llvm.coro.id", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.begin intrinsic
 * Signature: i8* @llvm.coro.begin(token id, i8* mem)
 */
static LLVMValueRef get_coro_begin_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.begin");
  if (!fn) {

    LLVMContextRef c = LLVMGetGlobalContext();
    LLVMTypeRef fn_type = LLVMFunctionType(
        GENERIC_PTR, (LLVMTypeRef[]){LLVMTokenTypeInContext(c), GENERIC_PTR}, 2,
        0);
    fn = LLVMAddFunction(module, "llvm.coro.begin", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.size.i64 intrinsic
 * Signature: i64 @llvm.coro.size.i64()
 */
static LLVMValueRef get_coro_size_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.size.i64");
  if (!fn) {
    LLVMTypeRef fn_type = LLVMFunctionType(LLVMInt64Type(), NULL, 0, 0);
    fn = LLVMAddFunction(module, "llvm.coro.size.i64", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.save intrinsic
 * Signature: token @llvm.coro.save(i8* handle)
 */
static LLVMValueRef get_coro_save_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.save");
  if (!fn) {

    LLVMContextRef c = LLVMGetGlobalContext();
    LLVMTypeRef fn_type = LLVMFunctionType(LLVMTokenTypeInContext(c),
                                           (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
    fn = LLVMAddFunction(module, "llvm.coro.save", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.suspend intrinsic
 * Signature: i8 @llvm.coro.suspend(token save, i1 final)
 */
static LLVMValueRef get_coro_suspend_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.suspend");
  if (!fn) {

    LLVMContextRef c = LLVMGetGlobalContext();
    LLVMTypeRef fn_type = LLVMFunctionType(
        LLVMInt8Type(),
        (LLVMTypeRef[]){LLVMTokenTypeInContext(c), LLVMInt1Type()}, 2, 0);
    fn = LLVMAddFunction(module, "llvm.coro.suspend", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.end intrinsic
 * Signature: i1 @llvm.coro.end(i8* handle, i1 unwind)
 */
static LLVMValueRef get_coro_end_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.end");
  if (!fn) {
    LLVMTypeRef fn_type = LLVMFunctionType(
        LLVMInt1Type(), (LLVMTypeRef[]){GENERIC_PTR, LLVMInt1Type()}, 2, 0);
    fn = LLVMAddFunction(module, "llvm.coro.end", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.free intrinsic
 * Signature: i8* @llvm.coro.free(token id, i8* handle)
 */
static LLVMValueRef get_coro_free_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.free");
  if (!fn) {

    LLVMContextRef c = LLVMGetGlobalContext();
    LLVMTypeRef fn_type = LLVMFunctionType(
        GENERIC_PTR, (LLVMTypeRef[]){LLVMTokenTypeInContext(c), GENERIC_PTR}, 2,
        0);
    fn = LLVMAddFunction(module, "llvm.coro.free", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.resume intrinsic
 * Signature: void @llvm.coro.resume(i8* handle)
 */
LLVMValueRef get_coro_resume_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.resume");
  if (!fn) {
    LLVMTypeRef fn_type =
        LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
    fn = LLVMAddFunction(module, "llvm.coro.resume", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.done intrinsic
 * Signature: i1 @llvm.coro.done(i8* handle)
 */
LLVMValueRef get_coro_done_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.done");
  if (!fn) {
    LLVMTypeRef fn_type =
        LLVMFunctionType(LLVMInt1Type(), (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
    fn = LLVMAddFunction(module, "llvm.coro.done", fn_type);
  }
  return fn;
}

/**
 * Get or declare llvm.coro.promise intrinsic
 * Signature: i8* @llvm.coro.promise(i8* handle, i32 align, i1 from_promise)
 */
static LLVMValueRef get_coro_promise_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.promise");
  if (!fn) {
    LLVMTypeRef fn_type = LLVMFunctionType(
        GENERIC_PTR,
        (LLVMTypeRef[]){GENERIC_PTR, LLVMInt32Type(), LLVMInt1Type()}, 3, 0);
    fn = LLVMAddFunction(module, "llvm.coro.promise", fn_type);
  }
  return fn;
}

// ============================================================================
// Coroutine Builtin Handlers (TODO: Implement these)
// ============================================================================

LLVMValueRef CorGetLastValHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorGetLastValHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorLoopHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CorMapHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorMapHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorStopHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CorOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorOfListHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CorOfArrayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorOfArrayHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: PlayRoutineHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CurrentCorHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CurrentCorHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CorUnwrapOrEndHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorUnwrapOrEndHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *app,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  if (sym->type == STYPE_FUNCTION) {
    // The coroutine function takes no parameters and returns just the handle
    // Signature: ptr @coro_fn()
    LLVMValueRef coro_fn = sym->val;
    // LLVMDumpValue(sym->val);

    LLVMValueRef handle =
        LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_fn), coro_fn,
                       NULL, // No arguments
                       0, "coro.handle");

    return handle;
  }
  return NULL;
}

// ============================================================================
// Coroutine Symbol Creation
// ============================================================================

LLVMValueRef create_coroutine_symbol(Ast *binding, Ast *expr, Type *expr_type,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  LLVMValueRef expr_val;

  if (is_generic(expr_type)) {
    expr_val = create_generic_fn_binding(binding, expr, ctx);
  } else {
    expr_val = compile_coroutine(expr, ctx, module, builder);
    expr_val =
        create_fn_binding(binding, expr_type, expr_val, ctx, module, builder);
  }

  return expr_val;
}

// ============================================================================
// Yield Implementation
// ============================================================================

/**
 * Emit LLVM code for a yield statement
 *
 * This creates a suspension point in the coroutine. The yielded value
 * is stored in the promise, and control returns to the caller.
 */
LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  CoroutineCtx *coro_ctx = (CoroutineCtx *)ctx->coro_ctx;

  if (!coro_ctx) {
    fprintf(stderr, "Error: yield outside of coroutine at ");
    print_location(ast);
    return NULL;
  }

  // 1. Codegen the value being yielded
  LLVMValueRef yield_value =
      codegen(ast->data.AST_YIELD.expr, ctx, module, builder);

  if (!yield_value) {
    fprintf(stderr, "Error: failed to codegen yield value\n");
    return NULL;
  }

  // 2. Handle nested coroutine chaining (if yielded value is a coroutine)
  Type *yield_val_type = ast->data.AST_YIELD.expr->type;
  if (is_coroutine_type(yield_val_type)) {
    fprintf(stderr, "Error: yield nested coroutine is not yet implemented\n");
    return NULL;
    // Special handling for: yield nested_coro()
    // This should flatten the coroutine chain
    // For now, we'll treat it as a regular value
    // TODO: Implement full chaining support
  }

  // 3. Store raw value in promise (not wrapped in Option)
  // The resume wrapper will construct Some(value) when reading
  LLVMBuildStore(builder, yield_value, coro_ctx->promise_alloca);

  // 4. Create suspension point
  LLVMValueRef save_token = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){coro_ctx->coro_handle},
      1, "coro.save");

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){
          save_token, LLVMConstInt(LLVMInt1Type(), 0, 0) // not final suspend
      },
      2, "coro.suspend");

  // 5. Switch on suspension result (CORRECTED)
  //     0 = coroutine RESUMED → continue execution
  //     1 = coroutine destroyed → cleanup
  //     default = coroutine SUSPENDED → return to caller

  // Create blocks for the different suspend outcomes
  LLVMBasicBlockRef return_bb = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "yield.return");
  LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "yield.resume");

  // Build the switch - DEFAULT returns to caller (suspended)
  // case 0: coroutine resumed → continue to next yield
  // case 1: coroutine destroyed → cleanup
  // default: coroutine suspended → return to caller
  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0),
              coro_ctx->cleanup_bb);

  // 6. Return block - suspend: exits coroutine and returns to caller
  LLVMPositionBuilderAtEnd(builder, return_bb);
  LLVMBuildBr(builder, coro_ctx->suspend_bb);

  // 7. Resume block - case 0: execution continues after resume
  LLVMPositionBuilderAtEnd(builder, resume_bb);

  coro_ctx->yield_count++;

  // Yield expression has type void in the coroutine body
  return LLVMGetUndef(LLVMVoidType());
}

// ============================================================================
// Coroutine Function Compilation
// ============================================================================

/**
 * Compile a function containing yield statements into an LLVM coroutine
 *
 * This generates the coroutine body function that will be called to
 * create and resume the coroutine.
 */
#define PRESPLIT_COROUTINE_KIND_ID 50
LLVMValueRef compile_coroutine(Ast *expr, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  if (expr->tag != AST_LAMBDA) {
    fprintf(stderr, "Error: compile_coroutine expects lambda\n");
    return NULL;
  }

  // 1. Determine types
  Type *fn_type = expr->type;
  Type *yield_type = fn_return_type(fn_type);
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);

  // Promise stores raw T
  // The resume wrapper will construct Option<T> based on coro.done()
  LLVMTypeRef promise_type = llvm_yield_type;

  // 2. Build function signature
  // Returns just the handle: ptr @coro()
  LLVMTypeRef coro_fn_type =
      LLVMFunctionType(GENERIC_PTR, // Return just the handle
                       NULL,        // No parameters
                       0, 0);

  char coro_name[64];
  snprintf(coro_name, sizeof(coro_name), "coro_%s",
           expr->data.AST_LAMBDA.fn_name.chars != NULL
               ? expr->data.AST_LAMBDA.fn_name.chars
               : "anon");

  LLVMValueRef coro_fn = LLVMAddFunction(module, coro_name, coro_fn_type);
  LLVMSetLinkage(coro_fn, LLVMExternalLinkage);

  // Mark function as a presplit coroutine
  // This tells LLVM that this function contains coroutine intrinsics
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);

  LLVMAttributeRef attr =
      LLVMCreateEnumAttribute(llvm_ctx, PRESPLIT_COROUTINE_KIND_ID, 0);
  LLVMAddAttributeAtIndex(coro_fn, LLVMAttributeFunctionIndex, attr);

  // 3. Create basic blocks
  LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(coro_fn, "entry");
  LLVMBasicBlockRef cleanup_bb = LLVMAppendBasicBlock(coro_fn, "cleanup");
  LLVMBasicBlockRef suspend_bb = LLVMAppendBasicBlock(coro_fn, "suspend");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // 4. Allocate promise inside the coroutine frame
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // 5. Initialize coroutine with intrinsics

  // Call coro.id with promise pointer
  LLVMValueRef id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){
          LLVMConstInt(LLVMInt32Type(), 0, 0), // align = 0
          promise_alloca,                      // promise (owned by coroutine)
          LLVMConstNull(GENERIC_PTR),          // coroaddr
          LLVMConstNull(GENERIC_PTR)           // fnaddr
      },
      4, "coro.id");

  // Allocate coroutine frame
  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");

  LLVMValueRef frame =
      LLVMBuildArrayMalloc(builder, LLVMInt8Type(), size, "coro.frame");

  // Call coro.begin
  LLVMValueRef handle = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_begin_intrinsic(module)),
      get_coro_begin_intrinsic(module), (LLVMValueRef[]){id, frame}, 2,
      "coro.handle");

  // INITIAL SUSPEND: Create basic blocks for suspend flow
  // This ensures the coroutine suspends on creation and only executes when
  // first resumed
  LLVMBasicBlockRef initial_return_bb =
      LLVMAppendBasicBlock(coro_fn, "initial.return");
  LLVMBasicBlockRef start_bb = LLVMAppendBasicBlock(coro_fn, "start");

  // Save coroutine state before initial suspend
  LLVMValueRef initial_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "initial.save");

  // Initial suspend (false = normal suspend, not final)
  LLVMValueRef initial_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){initial_save, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "initial.suspend");

  // Switch on suspend result during coroutine initialization:
  // -1 (default) = first time through during init - return to caller without
  // executing body 0 = resumed - continue to start block where body executes 1
  // = destroy - go to cleanup
  LLVMValueRef init_switch =
      LLVMBuildSwitch(builder, initial_suspend, initial_return_bb, 2);
  LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 0, 0),
              start_bb); // Resume case (0) goes to start!
  LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  // Initial return block - returns to caller during initialization (default
  // case -1)
  LLVMPositionBuilderAtEnd(builder, initial_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // Position builder at start of actual coroutine body (resumed execution)
  LLVMPositionBuilderAtEnd(builder, start_bb);

  // 6. Set up coroutine context
  CoroutineCtx coro_ctx = {0}; // Zero-initialize all fields
  coro_ctx.coro_id = id;
  coro_ctx.coro_handle = handle;
  coro_ctx.promise_alloca = promise_alloca; // Allocated above
  coro_ctx.promise_type = promise_type;     // Raw T type
  coro_ctx.yield_type = yield_type;
  coro_ctx.cleanup_bb = cleanup_bb;
  coro_ctx.suspend_bb = suspend_bb;
  coro_ctx.yield_count = 0;
  coro_ctx.coro_obj_type = CORO_OBJ_TYPE(promise_type);

  JITLangCtx coro_lang_ctx = *ctx;
  coro_lang_ctx.coro_ctx = &coro_ctx;

  // 7. Codegen the function body (will contain yields)
  LLVMValueRef body_result =
      codegen_lambda_body(expr, &coro_lang_ctx, module, builder);

  // 8. Function end - FINAL suspend
  // This tells LLVM the coroutine is finished and sets resume ptr to null
  LLVMValueRef final_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "final.save");

  LLVMValueRef final_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){final_save,
                       LLVMConstInt(LLVMInt1Type(), 1, 0)}, // true = FINAL
      2, "final.suspend");

  // Switch on final suspend result
  LLVMBasicBlockRef final_return_bb =
      LLVMAppendBasicBlock(coro_fn, "final.return");
  LLVMValueRef final_switch =
      LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0),
              final_return_bb);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, final_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // 9. Cleanup block
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);

  // Free the coroutine frame
  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2,
      "coro.free");
  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // 10. Suspend block - return just the handle
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");

  // Return just the handle
  LLVMBuildRet(builder, handle);

  // Restore builder position
  LLVMPositionBuilderAtEnd(builder, prev_block);

  // fprintf(stderr,
  //         "DEBUG: compile_coroutine finished - generated %s with %d
  //         yields\n", coro_name, coro_ctx.yield_count);
  // LLVMDumpValue(coro_fn);

  return coro_fn;
}

LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  LLVMValueRef handle = sym->val;

  // Extract yield type from coroutine type
  // symbol_type should be something like: () -> Option<T>
  Type *coro_fn_type = sym->symbol_type;
  Type *yield_type = fn_return_type(coro_fn_type);

  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);
  LLVMTypeRef llvm_option_type =
      type_to_llvm_type(create_option_type(yield_type), ctx, module);

  // Create basic blocks for control flow
  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef current_fn = LLVMGetBasicBlockParent(current_bb);

  LLVMBasicBlockRef done_bb = LLVMAppendBasicBlock(current_fn, "coro.done");
  LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(current_fn, "coro.resume");
  LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlock(current_fn, "coro.merge");

  // 1. Check if coroutine is done
  LLVMValueRef is_done = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){handle}, 1, "is_done");

  LLVMBuildCondBr(builder, is_done, done_bb, resume_bb);

  // 2. Done branch - return None
  LLVMPositionBuilderAtEnd(builder, done_bb);
  LLVMValueRef none_value = codegen_none_typed(builder, llvm_yield_type);
  LLVMBuildBr(builder, merge_bb);

  // 3. Resume branch - resume and get value
  LLVMPositionBuilderAtEnd(builder, resume_bb);

  // Resume the coroutine
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
      get_coro_resume_intrinsic(module), (LLVMValueRef[]){handle}, 1, "");

  // Check AGAIN if done (might have hit final suspend during resume)
  LLVMBasicBlockRef after_resume_done_bb =
      LLVMAppendBasicBlock(current_fn, "coro.after_resume_done");
  LLVMBasicBlockRef after_resume_not_done_bb =
      LLVMAppendBasicBlock(current_fn, "coro.after_resume_not_done");

  LLVMValueRef is_done_after_resume = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "is_done_after_resume");

  LLVMBuildCondBr(builder, is_done_after_resume, after_resume_done_bb,
                  after_resume_not_done_bb);

  // After resume, if done: return None
  LLVMPositionBuilderAtEnd(builder, after_resume_done_bb);
  LLVMValueRef none_after_resume = codegen_none_typed(builder, llvm_yield_type);
  LLVMBuildBr(builder, merge_bb);

  // After resume, if not done: read promise and return Some(value)
  LLVMPositionBuilderAtEnd(builder, after_resume_not_done_bb);

  // Get promise pointer using coro.promise intrinsic
  // Signature: i8* @llvm.coro.promise(i8* handle, i32 align, i1 from_promise)
  LLVMValueRef promise_ptr_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){
          handle, LLVMConstInt(LLVMInt32Type(), 0, 0), // align = 0
          LLVMConstInt(LLVMInt1Type(), 0, 0)           // from_promise = false
      },
      3, "promise.raw");

  // Cast to correct type
  LLVMValueRef promise_ptr =
      LLVMBuildBitCast(builder, promise_ptr_raw,
                       LLVMPointerType(llvm_yield_type, 0), "promise.ptr");

  // Load the yielded value
  LLVMValueRef yielded_value =
      LLVMBuildLoad2(builder, llvm_yield_type, promise_ptr, "yielded");

  // Wrap in Some
  LLVMValueRef some_value = codegen_some(yielded_value, builder);
  LLVMBuildBr(builder, merge_bb);

  // 4. Merge block - phi node to select result (3 incoming paths)
  LLVMPositionBuilderAtEnd(builder, merge_bb);
  LLVMValueRef result_phi =
      LLVMBuildPhi(builder, llvm_option_type, "coro.result");

  LLVMAddIncoming(result_phi,
                  (LLVMValueRef[]){none_value, none_after_resume, some_value},
                  (LLVMBasicBlockRef[]){done_bb, after_resume_done_bb,
                                        after_resume_not_done_bb},
                  3);

  return result_phi;
}
