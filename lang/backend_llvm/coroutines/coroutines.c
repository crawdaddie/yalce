#include "./coroutines.h"
#include "../binding.h"
#include "../function.h"
#include "../symbols.h"
#include "adt.h"
#include "common.h"
#include "types.h"
#include "types/type.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// LLVM Intrinsic Declaration Helpers
// ============================================================================

/**
 * Get or declare llvm.coro.id intrinsic
 * Signature: token @llvm.coro.id(i32 align, i8* promise, i8* coroaddr, i8*
 * fnaddr)
 */
LLVMValueRef get_coro_id_intrinsic(LLVMModuleRef module) {
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
LLVMValueRef get_coro_begin_intrinsic(LLVMModuleRef module) {
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
LLVMValueRef get_coro_size_intrinsic(LLVMModuleRef module) {
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
LLVMValueRef get_coro_save_intrinsic(LLVMModuleRef module) {
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
LLVMValueRef get_coro_suspend_intrinsic(LLVMModuleRef module) {
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
LLVMValueRef get_coro_end_intrinsic(LLVMModuleRef module) {
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
LLVMValueRef get_coro_free_intrinsic(LLVMModuleRef module) {
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
LLVMValueRef get_coro_promise_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.promise");
  if (!fn) {
    LLVMTypeRef fn_type = LLVMFunctionType(
        GENERIC_PTR,
        (LLVMTypeRef[]){GENERIC_PTR, LLVMInt32Type(), LLVMInt1Type()}, 3, 0);
    fn = LLVMAddFunction(module, "llvm.coro.promise", fn_type);
  }
  return fn;
}

// #define GET_STRUCTURED_PROMISE(yield_type) \
//   LLVMStructType( \
//       (LLVMTypeRef[]){ \
//           yield_type,  /* yield type */ \
//           GENERIC_PTR, /*holds the next handle */ \
//       }, \ 2, 0)
//
#define GET_STRUCTURED_PROMISE(yield_type) yield_type

#define GET_PROMISE_PTR_RAW(handle)                                            \
  LLVMBuildCall2(builder,                                                      \
                 LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),   \
                 get_coro_promise_intrinsic(module),                           \
                 (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt32Type(), 0, 0), \
                                  LLVMConstInt(LLVMInt1Type(), 0, 0)},         \
                 3, "promise.raw");

static LLVMValueRef coro_create_from_generic(JITSymbol *sym,
                                             Type *expected_fn_type, Ast *ast,
                                             JITLangCtx *ctx,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  // TODO: figure out coroutines that accept other coroutines as args, eg:
  //
  // let cor_zip = fn c1 c2 ->
  //   let x1 = cor_unwrap_or_end @@ c1 ();
  //   let x2 = cor_unwrap_or_end @@ c2 ();
  //   yield (x1, x2);
  //   yield cor_zip c1 c2
  // ;;
  // let c1 = fn () -> yield 1; yield 2;;
  // let c2 = fn () -> yield 2; yield 1;;
  //
  // let x = cor_zip (c1 ()) (c2 ());
  //
  // -- or --
  //
  // let get_head_opt = fn x ->
  //   match x with
  //   | x::rest -> Some (x, rest)
  //   | [] -> None
  // ;;
  //
  // let seq = fn cors ->
  //   let (h, rest) = cor_unwrap_or_end @@ get_head_opt cors;
  //   yield h;
  //   yield combine rest
  // ;;
  //
  // let x = seq [iter_of_list [1,2,3], iter_of_list [3,2,1]];

  LLVMValueRef func = specific_fns_lookup(
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_fn_type);

  if (!func) {

    JITLangCtx compilation_ctx = *ctx;

    Type *generic_type = sym->symbol_type;
    generic_type = generic_type->data.T_CONS.args[0];

    compilation_ctx.stack_ptr =
        sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
    compilation_ctx.frame = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame;

    compilation_ctx.env = create_env_for_generic_fn(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env, generic_type,
        expected_fn_type);

    Ast fn_ast = *sym->symbol_data.STYPE_GENERIC_FUNCTION.ast;

    Type exp = TCONS(TYPE_NAME_COROUTINE_CONSTRUCTOR, 1, expected_fn_type);
    fn_ast.type = &exp;

    LLVMValueRef specific_fn =
        compile_coroutine(&fn_ast, &compilation_ctx, module, builder);

    sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns = specific_fns_extend(
        sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns, expected_fn_type,
        specific_fn);

    func = specific_fn;
  }

  return func;
}

LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *app,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  if (sym->type == STYPE_FUNCTION) {
    LLVMValueRef coro_fn = sym->val;
    LLVMValueRef args[app->data.AST_APPLICATION.len];
    for (int i = 0; i < app->data.AST_APPLICATION.len; i++) {
      args[i] =
          codegen(app->data.AST_APPLICATION.args + i, ctx, module, builder);
    }

    LLVMValueRef handle =
        LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_fn), coro_fn,
                       args, // No arguments
                       app->data.AST_APPLICATION.len, "coro.handle");

    return handle;
  }

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    printf("generic coroutine???\n");
    print_type(expected_fn_type);
    LLVMValueRef coro_fn = coro_create_from_generic(sym, expected_fn_type, app,
                                                    ctx, module, builder);
    LLVMValueRef args[app->data.AST_APPLICATION.len];
    for (int i = 0; i < app->data.AST_APPLICATION.len; i++) {
      args[i] =
          codegen(app->data.AST_APPLICATION.args + i, ctx, module, builder);
    }

    LLVMValueRef handle =
        LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_fn), coro_fn,
                       args, // No arguments
                       app->data.AST_APPLICATION.len, "coro.handle");

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
bool is_recursive_yield(Ast *expr, CoroutineCtx *coro_ctx) {
  if (expr->tag == AST_APPLICATION &&
      expr->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
      coro_ctx->coro_name &&
      CHARS_EQ(expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
               coro_ctx->coro_name)) {
    return true;
  }
  return false;
}

LLVMValueRef handle_recursive_yield(Ast *expr, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  CoroutineCtx *coro_ctx = (CoroutineCtx *)ctx->coro_ctx;

  // expr should be an AST_APPLICATION like: rec_coro(new_arg1, new_arg2, ...)
  if (expr->tag != AST_APPLICATION) {
    fprintf(stderr, "Error: recursive yield expects function application\n");
    return NULL;
  }
  LLVMValueRef arg_vals[expr->data.AST_APPLICATION.len];
  if (coro_ctx->param_allocas) {

    // 1. Extract arguments from the application
    Ast *args = expr->data.AST_APPLICATION.args;

    // 2. Codegen each argument and store into the corresponding param_alloca
    for (int i = 0; i < expr->data.AST_APPLICATION.len; i++) {
      LLVMValueRef arg_val = codegen(args + i, ctx, module, builder);
      LLVMValueRef palloca = coro_ctx->param_allocas[i];
      if (!arg_val) {
        fprintf(stderr,
                "Error: failed to codegen recursive yield argument %d\n", i);
        return NULL;
      }
      arg_vals[i] = arg_val;
    }

    // Store into the parameter spill slot
    for (int j = 0; j < expr->data.AST_APPLICATION.len; j++) {
      LLVMBuildStore(builder, arg_vals[j], coro_ctx->param_allocas[j]);
    }
  }

  // 3. Branch back to the start of the coroutine (tail-call optimization)
  // We need a basic block to jump to - this should be the first block after
  // initial suspend For now, we'll need to store this in the CoroutineCtx
  if (!coro_ctx->start_bb) {
    fprintf(stderr,
            "Error: start_bb not set in CoroutineCtx for recursive yield\n");
    return NULL;
  }

  // Jump back to the start - this effectively "restarts" the coroutine with new
  // parameters
  LLVMBuildBr(builder, coro_ctx->start_bb);
  // Create a new unreachable block for any code after this
  // (the code generator might try to add more code)
  LLVMBasicBlockRef unreachable_bb =
      LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)),
                           "after_recursive_yield");
  LLVMPositionBuilderAtEnd(builder, unreachable_bb);

  // Return undef - this code path doesn't actually return normally
  return LLVMGetUndef(LLVMVoidType());
}

#define IN_BLOCK(block, code)                                                  \
  LLVMPositionBuilderAtEnd(builder, block);                                    \
  code

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

  if (is_recursive_yield(ast->data.AST_YIELD.expr, ctx->coro_ctx)) {
    return handle_recursive_yield(ast->data.AST_YIELD.expr, ctx, module,
                                  builder);
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
    // ===== YIELD FROM: Drain the entire inner coroutine =====

    LLVMValueRef inner_handle = yield_value;

    // Get the inner coroutine's yield type
    Type *inner_yield_type =
        yield_val_type->data.T_CONS.args[0]; // Coroutine<T> -> T
    LLVMTypeRef llvm_inner_yield_type =
        type_to_llvm_type(inner_yield_type, ctx, module);

    // Create loop blocks
    LLVMValueRef parent_fn =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

    LLVMBasicBlockRef loop_check_bb =
        LLVMAppendBasicBlock(parent_fn, "inner_cor_loop.check");
    LLVMBasicBlockRef loop_body_bb =
        LLVMAppendBasicBlock(parent_fn, "inner_cor_loop.body");
    LLVMBasicBlockRef get_value_bb =
        LLVMAppendBasicBlock(parent_fn, "inner_cor.get_value");
    LLVMBasicBlockRef loop_resume_bb =
        LLVMAppendBasicBlock(parent_fn, "inner_cor.resume");
    LLVMBasicBlockRef loop_exit_bb =
        LLVMAppendBasicBlock(parent_fn, "inner_cor.exit");

    // Jump to loop check
    LLVMBuildBr(builder, loop_check_bb);

    // === LOOP CHECK: Check if inner is done ===
    LLVMPositionBuilderAtEnd(builder, loop_check_bb);

    // Check if inner is done BEFORE resuming
    LLVMValueRef is_done_before = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
        get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle}, 1,
        "inner.is_done_before");

    LLVMBuildCondBr(builder, is_done_before, loop_exit_bb, loop_body_bb);

    // === LOOP BODY: Resume inner ===
    LLVMPositionBuilderAtEnd(builder, loop_body_bb);

    // Resume the inner coroutine
    LLVMBuildCall2(builder,
                   LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                   get_coro_resume_intrinsic(module),
                   (LLVMValueRef[]){inner_handle}, 1, "");

    // Check if done AFTER resume
    LLVMValueRef is_done_after = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
        get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle}, 1,
        "inner.is_done_after");

    // If done after resume, exit the loop
    LLVMBuildCondBr(builder, is_done_after, loop_exit_bb, get_value_bb);

    // === GET VALUE: Read inner's promise and yield it ===
    LLVMPositionBuilderAtEnd(builder, get_value_bb);

    // Get inner coroutine's promise
    LLVMValueRef inner_promise_raw = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
        get_coro_promise_intrinsic(module),
        (LLVMValueRef[]){inner_handle, LLVMConstInt(LLVMInt32Type(), 0, 0),
                         LLVMConstInt(LLVMInt1Type(), 0, 0)},
        3, "inner.promise.raw");

    // Cast to correct type
    LLVMValueRef inner_promise_ptr = LLVMBuildBitCast(
        builder, inner_promise_raw, LLVMPointerType(llvm_inner_yield_type, 0),
        "inner.promise.ptr");

    // Load the value from inner's promise
    LLVMValueRef inner_value = LLVMBuildLoad2(builder, llvm_inner_yield_type,
                                              inner_promise_ptr, "inner.value");

    // Store it in OUR promise (we're yielding this value)
    LLVMBuildStore(builder, inner_value, coro_ctx->promise_alloca);

    // Suspend (yield this value to our caller)
    LLVMValueRef save_token = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
        get_coro_save_intrinsic(module),
        (LLVMValueRef[]){coro_ctx->coro_handle}, 1, "coro.save");

    LLVMValueRef suspend_result = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
        get_coro_suspend_intrinsic(module),
        (LLVMValueRef[]){
            save_token, LLVMConstInt(LLVMInt1Type(), 0, 0) // not final
        },
        2, "coro.suspend");

    // Switch on suspend result
    LLVMBasicBlockRef suspend_return_bb =
        LLVMAppendBasicBlock(parent_fn, "yield_from.suspend_return");

    LLVMValueRef switch_inst =
        LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
    LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0),
                loop_resume_bb);
    LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0),
                coro_ctx->cleanup_bb);

    // Suspend return - return to caller
    LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
    LLVMBuildBr(builder, coro_ctx->suspend_bb);

    // Resume block - when we're resumed, loop back to check for more values
    LLVMPositionBuilderAtEnd(builder, loop_resume_bb);
    LLVMBuildBr(builder, loop_check_bb); // Loop back!

    // === LOOP EXIT: Inner exhausted, continue outer ===
    LLVMPositionBuilderAtEnd(builder, loop_exit_bb);

    coro_ctx->yield_count++;

    // Continue execution in outer coroutine
    return LLVMGetUndef(LLVMVoidType());
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
LLVMValueRef compile_coroutine(Ast *expr, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  if (expr->tag != AST_LAMBDA) {
    fprintf(stderr, "Error: compile_coroutine expects lambda\n");
    return NULL;
  }

  // 1. Determine types
  Type *fn_type = expr->type;
  if (is_coroutine_constructor_type(fn_type)) {
    fn_type = fn_type->data.T_CONS.args[0];
  }

  Type *yield_type = fn_return_type(fn_type);
  if (is_coroutine_type(yield_type)) {
    yield_type = yield_type->data.T_CONS.args[0];
  }

  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);

  // Promise stores raw T
  // The resume wrapper will construct Option<T> based on coro.done()
  LLVMTypeRef promise_type = llvm_yield_type;

  // 2. Build function signature
  // Returns just the handle: ptr @coro()
  LLVMTypeRef coro_fn_type = codegen_fn_type(
      GENERIC_PTR, fn_type, expr->data.AST_LAMBDA.len, ctx, module);

  char coro_name[64];
  snprintf(coro_name, sizeof(coro_name), "coro_%s",
           expr->data.AST_LAMBDA.fn_name.chars != NULL
               ? expr->data.AST_LAMBDA.fn_name.chars
               : "anon");

  LLVMValueRef coro_fn = LLVMAddFunction(module, coro_name, coro_fn_type);
  LLVMSetLinkage(coro_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(coro_fn)

  // 3. Create basic blocks
  COROUTINE_BASIC_BLOCKS(coro_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // 4. Allocate promise inside the coroutine frame
  LLVMTypeRef prom_struct_type = llvm_yield_type;

  LLVMValueRef frame_alloca =
      LLVMBuildAlloca(builder, llvm_yield_type, "promise");

  // Call coro.id with promise pointer
  LLVMValueRef get_coro_id = get_coro_id_intrinsic(module);
  LLVMValueRef id =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(get_coro_id), get_coro_id,
                     (LLVMValueRef[]){
                         LLVMConstInt(LLVMInt32Type(), 0, 0), // align = 0
                         frame_alloca, // promise (owned by coroutine)
                         LLVMConstNull(GENERIC_PTR), // coroaddr
                         LLVMConstNull(GENERIC_PTR)  // fnaddr
                     },
                     4, "coro.id");

  // Allocate coroutine frame
  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");

  LLVMValueRef frame =
      LLVMBuildArrayMalloc(builder, LLVMInt8Type(), size, "coro.frame");

  STACK_ALLOC_CTX_PUSH(coro_lang_ctx, ctx)

  int num_param_allocas = expr->data.AST_LAMBDA.len;

  if (is_void_func(fn_type)) {
    num_param_allocas = 0;
  }

  LLVMValueRef param_allocas[num_param_allocas];

  if (!is_void_func(fn_type)) {
    AST_LIST_ITER(expr->data.AST_LAMBDA.params, ({
                    LLVMValueRef param_val = LLVMGetParam(coro_fn, i);
                    Ast *param_ast = l->ast;
                    Type *param_type = fn_type->data.T_FN.from;

                    // Create alloca for parameter in entry block (will be part
                    // of coro frame)
                    LLVMTypeRef param_llvm_type =
                        type_to_llvm_type(param_type, ctx, module);

                    LLVMValueRef param_alloca = LLVMBuildAlloca(
                        builder, param_llvm_type, "param.spill");
                    param_allocas[i] = param_alloca;

                    LLVMBuildStore(builder, param_val, param_alloca);
                    // TODO: need to handle destructured non-simple args
                    LLVMValueRef load_val = NULL;
                    bind_local_value_with_storage(
                        param_ast, load_val, param_alloca, param_type,
                        &coro_lang_ctx, module, builder);

                    fn_type = fn_type->data.T_FN.to;
                  }));
  }

  if (expr->data.AST_LAMBDA.num_yield_boundary_crossers > 0) {
    AST_LIST_ITER(expr->data.AST_LAMBDA.yield_boundary_crossers, ({
                    Ast *bx = l->ast;
                    Type *bxt = bx->type;

                    if (is_generic(bxt)) {
                      bxt = resolve_type_in_env(bxt, ctx->env);
                    }

                    LLVMTypeRef item_type = type_to_llvm_type(bxt, ctx, module);
                    LLVMValueRef state_storage =
                        LLVMBuildAlloca(builder, item_type, "");

                    // LLVMBuildStore(builder, LLVMConstNull(item_type),
                    // state_storage);

                    JITSymbol *sym =
                        new_symbol(STYPE_LOCAL_VAR, bxt, NULL, item_type);
                    sym->storage = state_storage;
                    const char *chars = bx->data.AST_IDENTIFIER.value;
                    int chars_len = bx->data.AST_IDENTIFIER.length;
                    ht_set_hash(coro_lang_ctx.frame->table, chars,
                                hash_string(chars, chars_len), sym);
                  }));
  }

  // INITIAL SUSPEND: Create basic blocks for suspend flow
  // This ensures the coroutine suspends on creation and only executes when
  // first resumed
  // Call coro.begin
  LLVMValueRef coro_begin = get_coro_begin_intrinsic(module);
  LLVMValueRef handle =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_begin), coro_begin,
                     (LLVMValueRef[]){id, frame}, 2, "coro.handle");

  // 6. Set up coroutine context
  CoroutineCtx coro_ctx = {0}; // Zero-initialize all fields
  coro_ctx.coro_id = id;
  coro_ctx.promise_alloca = frame_alloca;   // Allocated above
  coro_ctx.promise_type = prom_struct_type; // Raw T type
  coro_ctx.yield_type = yield_type;
  coro_ctx.llvm_yield_type = llvm_yield_type; // Raw T type
  coro_ctx.cleanup_bb = cleanup_bb;
  coro_ctx.suspend_bb = suspend_bb;
  coro_ctx.start_bb = start_bb;
  coro_ctx.yield_count = 0;
  coro_ctx.coro_handle = handle;
  coro_ctx.coro_name = expr->data.AST_LAMBDA.fn_name.chars;
  coro_ctx.param_allocas = num_param_allocas == 0 ? NULL : param_allocas;
  coro_ctx.num_param_allocas = num_param_allocas;
  coro_lang_ctx.coro_ctx = &coro_ctx;

  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Save coroutine state before initial suspend
  LLVMValueRef coro_save = get_coro_save_intrinsic(module);
  LLVMValueRef initial_save =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_save), coro_save,
                     (LLVMValueRef[]){handle}, 1, "initial.save");

  // Initial suspend (false = normal suspend, not final)
  LLVMValueRef coro_suspend = get_coro_suspend_intrinsic(module);
  LLVMValueRef initial_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(coro_suspend), coro_suspend,
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

  LLVMValueRef body_result =
      codegen_lambda_body(expr, &coro_lang_ctx, module, builder);

  // 8. Function end - FINAL suspend
  LLVMValueRef final_save =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_save), coro_save,
                     (LLVMValueRef[]){handle}, 1, "final.save");

  LLVMValueRef final_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(coro_suspend), coro_suspend,
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
  LLVMValueRef coro_free = get_coro_free_intrinsic(module);
  LLVMValueRef mem =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_free), coro_free,
                     (LLVMValueRef[]){id, handle}, 2, "coro.free");

  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // 10. Suspend block - return just the handle
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  LLVMValueRef coro_end = get_coro_end_intrinsic(module);
  LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_end), coro_end,
                 (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)},
                 2, "");

  LLVMBuildRet(builder, handle);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  // LLVMDumpValue(coro_fn);

  destroy_ctx(&coro_lang_ctx);
  return coro_fn;
}

LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  LLVMValueRef handle = sym->val;

  // Extract yield type from coroutine type
  // symbol_type should be something like: () -> Option<T>
  Type *coro_fn_type = sym->symbol_type;
  Type *yield_type = coro_fn_type->data.T_CONS.args[0];

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
  LLVMValueRef promise_ptr_raw = GET_PROMISE_PTR_RAW(handle);
  // LLVMBuildCall2(
  //     builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
  //     get_coro_promise_intrinsic(module),
  //     (LLVMValueRef[]){
  //         handle, LLVMConstInt(LLVMInt32Type(), 0, 0), // align = 0
  //         LLVMConstInt(LLVMInt1Type(), 0, 0)           // from_promise =
  //         false
  //     },
  //     3, "promise.raw");

  // Cast to correct type
  LLVMValueRef yield_ptr =
      LLVMBuildBitCast(builder, promise_ptr_raw,
                       LLVMPointerType(llvm_yield_type, 0), "promise.ptr");

  // Load the yielded value
  LLVMValueRef yielded_value =
      LLVMBuildLoad2(builder, llvm_yield_type, yield_ptr, "yielded");

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
