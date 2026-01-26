#include "./coroutines.h"
#include "../../types/type.h"
#include "../../types/type_ser.h"
#include "../adt.h"
#include "../binding.h"
#include "../common.h"
#include "../function.h"
#include "../symbols.h"
#include "../types.h"
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

LLVMValueRef get_coro_destroy_intrinsic(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "llvm.coro.destroy");
  if (!fn) {
    LLVMTypeRef fn_type =
        LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
    fn = LLVMAddFunction(module, "llvm.coro.destroy", fn_type);
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

LLVMValueRef coro_create_with_reset_closure(JITSymbol *sym,
                                            Type *expected_fn_type, Ast *app,
                                            JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {

  LLVMValueRef coro_fn = sym->val;

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    LLVMValueRef coro_fn = coro_create_from_generic(sym, expected_fn_type, app,
                                                    ctx, module, builder);
  }

  bool is_void_arg = is_void_func(expected_fn_type);

  LLVMTypeRef closure_args_struct_type;
  LLVMTypeRef arg_types[app->data.AST_APPLICATION.len];

  LLVMTypeRef closure_type;
  if (is_void_arg) {
    closure_type = LLVMFunctionType(GENERIC_PTR, NULL, 0, 0);
  } else {
    closure_type =
        LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){GENERIC_PTR}, 1,
                         0); // &{cons args...} -> coroutine_handle_type
    for (int i = 0; i < app->data.AST_APPLICATION.len; i++) {
      arg_types[i] = type_to_llvm_type(
          (app->data.AST_APPLICATION.args + i)->type, ctx, module);
    }

    closure_args_struct_type =
        LLVMStructType(arg_types, app->data.AST_APPLICATION.len, 0);
  }
  size_t length;
  const char *coro_cons_name = LLVMGetValueName2(coro_fn, &length);
  char name[length + 6];
  sprintf(name, "%s.reset", coro_cons_name);

  LLVMValueRef closure = LLVMAddFunction(module, name, closure_type);

  LLVMSetLinkage(closure, LLVMExternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(closure, "entry");
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMValueRef inner_args_ptr = LLVMGetParam(closure, 0);

  LLVMValueRef inner_args[app->data.AST_APPLICATION.len];
  if (!is_void_arg) {
    for (int i = 0; i < app->data.AST_APPLICATION.len; i++) {
      LLVMValueRef arg_ptr = LLVMBuildStructGEP2(
          builder, closure_args_struct_type, inner_args_ptr, i, "");
      inner_args[i] = LLVMBuildLoad2(builder, arg_types[i], arg_ptr, "");
    }
    LLVMValueRef inner_handle =
        LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_fn), coro_fn,
                       inner_args, // No arguments
                       app->data.AST_APPLICATION.len, "coro.handle[inner]");
    LLVMBuildRet(builder, inner_handle);
  } else {

    LLVMValueRef inner_handle =
        LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_fn), coro_fn,
                       (LLVMValueRef[]){}, // No arguments
                       0, "coro.handle[inner]");
    LLVMBuildRet(builder, inner_handle);
  }

  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMValueRef args[app->data.AST_APPLICATION.len];
  LLVMValueRef args_ptr;

  if (!is_void_arg) {

    LLVMValueRef args_struct = LLVMGetUndef(closure_args_struct_type);

    for (int i = 0; i < app->data.AST_APPLICATION.len; i++) {
      args[i] =
          codegen(app->data.AST_APPLICATION.args + i, ctx, module, builder);
      args_struct = LLVMBuildInsertValue(builder, args_struct, args[i], i, "");
    }

    // TODO: use escape analysis to determine if this can go on the stack
    // (probably not)
    args_ptr = LLVMBuildMalloc(builder, closure_args_struct_type, "");
    LLVMBuildStore(builder, args_struct, args_ptr);
  } else {
    args_ptr = LLVMConstPointerNull(LLVMInt8Type());
  }

  LLVMValueRef handle =
      LLVMBuildCall2(builder, closure_type, closure, (LLVMValueRef[]){args_ptr},
                     1, "coro.handle");

  // LLVMValueRef handle =
  //     LLVMBuildCall2(builder, closure_type, closure,
  //     (LLVMValueRef[]){args_ptr},
  //                    1, "coro.handle");

  // LLVMValueRef handle =
  //     LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_fn), coro_fn,
  //                    args, // No arguments
  //                    app->data.AST_APPLICATION.len, "coro.handle");

  LLVMTypeRef fat_handle_ty = LLVMStructType(
      (LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR, GENERIC_PTR}, 3, 0);

  LLVMValueRef fat_handle = LLVMGetUndef(fat_handle_ty);
  fat_handle =
      LLVMBuildInsertValue(builder, fat_handle, handle, 0, "insert_handle");
  fat_handle =
      LLVMBuildInsertValue(builder, fat_handle, closure, 1, "insert_closure");

  fat_handle = LLVMBuildInsertValue(builder, fat_handle, args_ptr, 2,
                                    "insert_closure_data");

  return fat_handle;
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

  // Jump back to the start - this effectively "restarts" the coroutine with
  // new parameters
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
  if (is_generic(yield_val_type)) {
    yield_val_type = resolve_type_in_env(yield_val_type, ctx->env);
  }

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

  // Use helper for regular yield point
  coro_emit_yield(ctx, module, builder, coro_ctx, yield_value);
  // Now positioned at resume block, ready for next statement

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
  if (!is_coroutine_constructor_type(fn_type)) {
    fprintf(stderr, "Error: invalid coroutine constructor type\n");
    return NULL;
  }
  fn_type = fn_type->data.T_CONS.args[0];

  Type *yield_type = fn_return_type(fn_type);

  if (!is_coroutine_type(yield_type)) {
    fprintf(stderr, "Error: invalid coroutine constructor type\n");
    return NULL;
  }
  yield_type = yield_type->data.T_CONS.args[0];

  // we support one level of coroutine nesting in the yield type - meaning
  // let f = fn a b -> yield a; yield b;; - if a & b are coroutine instances,
  // then we nest a and then b
  //
  // therefore instead of f having type:
  //
  // Coroutine of T -> Coroutine of T -> Coroutine of Coroutine of T, it is
  // flattened to
  //
  // Coroutine of T -> Coroutine of T -> Coroutine of T
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
  COROUTINE_BASIC_BLOCKS(coro_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // LLVMTypeRef prom_struct_type = llvm_yield_type;
  LLVMTypeRef prom_struct_type =
      LLVMStructType((LLVMTypeRef[]){llvm_yield_type, LLVMInt1Type()}, 2, 0);

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, prom_struct_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, prom_struct_type, promise_alloca, 1, "is_done_ptr");

  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  LLVMValueRef get_coro_id = get_coro_id_intrinsic(module);
  LLVMValueRef id =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(get_coro_id), get_coro_id,
                     (LLVMValueRef[]){
                         LLVMConstInt(LLVMInt32Type(), 0, 0), // align = 0
                         promise_alloca, // promise (owned by coroutine)
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

                    // Create alloca for parameter in entry block (will be
                    // part of coro frame)
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

  LLVMValueRef coro_begin = get_coro_begin_intrinsic(module);
  LLVMValueRef handle =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_begin), coro_begin,
                     (LLVMValueRef[]){id, frame}, 2, "coro.handle");

  CoroutineCtx coro_ctx = {0}; // Zero-initialize all fields
  coro_ctx.coro_id = id;
  coro_ctx.promise_alloca = promise_alloca; // Allocated above
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

  coro_emit_initial_suspend(ctx, module, builder, handle, cleanup_bb,
                            suspend_bb, initial_return_bb, start_bb);

  LLVMValueRef coro_suspend = get_coro_suspend_intrinsic(module);
  LLVMValueRef coro_save = get_coro_save_intrinsic(module);
  LLVMValueRef body_result =
      codegen_lambda_body(expr, &coro_lang_ctx, module, builder);

  coro_emit_final_suspend(ctx, module, builder, handle, coro_fn, cleanup_bb,
                          suspend_bb);

  // Use helper for cleanup and suspend blocks
  coro_emit_cleanup_and_suspend(ctx, module, builder, id, handle, cleanup_bb,
                                suspend_bb);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  destroy_ctx(&coro_lang_ctx);
  // LLVMDumpValue(coro_fn);
  return coro_fn;
}
LLVMValueRef coro_is_done(LLVMValueRef handle, LLVMTypeRef yield_type,
                          LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef full_prom_type =
      LLVMStructType((LLVMTypeRef[]){yield_type, LLVMInt1Type()}, 2, 0);

  LLVMValueRef promise_ptr_raw = GET_PROMISE_PTR_RAW(handle);

  // 1. Check if coroutine is done
  LLVMValueRef full_prom_ptr = LLVMBuildBitCast(
      builder, promise_ptr_raw, LLVMPointerType(full_prom_type, 0), "");

  LLVMValueRef is_done_flag_ptr = LLVMBuildStructGEP2(
      builder, full_prom_type, full_prom_ptr, 1, "get_is_done_flag");
  LLVMValueRef is_done_flag =
      LLVMBuildLoad2(builder, LLVMInt1Type(), is_done_flag_ptr, "");

  LLVMValueRef coro_done = get_coro_done_intrinsic(module);
  LLVMValueRef is_done =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_done), coro_done,
                     (LLVMValueRef[]){handle}, 1, "is_done");
  is_done = LLVMBuildOr(builder, is_done_flag, is_done, "");
  return is_done;
}

LLVMValueRef coro_symbol_resume(JITSymbol *sym, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef handle = sym->val;

  // Extract yield type from coroutine type
  // symbol_type should be something like: () -> Option<T>
  Type *coro_fn_type = sym->symbol_type;
  Type *yield_type = coro_fn_type->data.T_CONS.args[0];

  // print_type(yield_type);
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);
  return codegen_handle_resume(handle, llvm_yield_type, ctx, module, builder);
}

LLVMValueRef codegen_handle_resume(LLVMValueRef fat_handle,
                                   LLVMTypeRef llvm_yield_type, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  LLVMDumpValue(fat_handle);
  printf("\n");
  LLVMValueRef handle =
      LLVMBuildExtractValue(builder, fat_handle, 0, "handle_from_fat");
  // Extract yield type from coroutine type
  // symbol_type should be something like: () -> Option<T>
  LLVMTypeRef llvm_option_type = codegen_option_struct_type(llvm_yield_type);

  // Create basic blocks for control flow
  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef current_fn = LLVMGetBasicBlockParent(current_bb);

  LLVMBasicBlockRef done_bb = LLVMAppendBasicBlock(current_fn, "coro.done");
  LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(current_fn, "coro.resume");
  LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlock(current_fn, "coro.merge");

  LLVMValueRef is_done = coro_is_done(handle, llvm_yield_type, module, builder);

  LLVMBuildCondBr(builder, is_done, done_bb, resume_bb);

  // 2. Done branch - return None
  LLVMPositionBuilderAtEnd(builder, done_bb);
  LLVMValueRef none_value = codegen_none_typed(builder, llvm_yield_type);
  LLVMBuildBr(builder, merge_bb);

  // 3. Resume branch - resume and get value
  LLVMPositionBuilderAtEnd(builder, resume_bb);

  // Resume the coroutine
  LLVMValueRef coro_resume = get_coro_resume_intrinsic(module);
  LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_resume), coro_resume,
                 (LLVMValueRef[]){handle}, 1, "");

  // Check AGAIN if done (might have hit final suspend during resume)
  LLVMBasicBlockRef after_resume_done_bb =
      LLVMAppendBasicBlock(current_fn, "coro.after_resume_done");
  LLVMBasicBlockRef after_resume_not_done_bb =
      LLVMAppendBasicBlock(current_fn, "coro.after_resume_not_done");

  LLVMValueRef coro_done = get_coro_done_intrinsic(module);
  LLVMValueRef is_done_after_resume =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_done), coro_done,
                     (LLVMValueRef[]){handle}, 1, "is_done_after_resume");

  LLVMBuildCondBr(builder, is_done_after_resume, after_resume_done_bb,
                  after_resume_not_done_bb);

  // After resume, if done: return None
  LLVMPositionBuilderAtEnd(builder, after_resume_done_bb);
  LLVMValueRef none_after_resume = codegen_none_typed(builder, llvm_yield_type);
  LLVMBuildBr(builder, merge_bb);

  // After resume, if not done: read promise and return Some(value)
  LLVMPositionBuilderAtEnd(builder, after_resume_not_done_bb);

  LLVMValueRef promise_ptr_raw = GET_PROMISE_PTR_RAW(handle);
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

// ============================================================================
// Helper: Setup
// ============================================================================

CoroSetupResult coro_emit_setup(JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder,
                                LLVMTypeRef promise_type) {
  CoroSetupResult result = {0};

  // Allocate promise
  result.promise_alloca = LLVMBuildAlloca(builder, promise_type, "promise");

  // Call coro.id
  result.coro_id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0),
                       result.promise_alloca, LLVMConstNull(GENERIC_PTR),
                       LLVMConstNull(GENERIC_PTR)},
      4, "coro.id");

  // Call coro.size
  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");

  // Allocate frame
  LLVMValueRef frame =
      LLVMBuildArrayMalloc(builder, LLVMInt8Type(), size, "coro.frame");

  // Call coro.begin
  result.handle = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_begin_intrinsic(module)),
      get_coro_begin_intrinsic(module), (LLVMValueRef[]){result.coro_id, frame},
      2, "coro.handle");

  return result;
}

// ============================================================================
// Helper: Initial Suspend
// ============================================================================

void coro_emit_initial_suspend(JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder, LLVMValueRef handle,
                               LLVMBasicBlockRef cleanup_bb,
                               LLVMBasicBlockRef suspend_bb,
                               LLVMBasicBlockRef initial_return_bb,
                               LLVMBasicBlockRef start_bb) {
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
  // executing body
  // 0 = resumed - continue to start block where body executes
  // 1 = destroy - go to cleanup
  LLVMValueRef init_switch =
      LLVMBuildSwitch(builder, initial_suspend, initial_return_bb, 2);
  LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 0, 0), start_bb);
  LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  // Initial return block - returns to caller during initialization (default
  // case -1)
  LLVMPositionBuilderAtEnd(builder, initial_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // Position builder at start of actual coroutine body (resumed execution)
  LLVMPositionBuilderAtEnd(builder, start_bb);
}

// ============================================================================
// Helper: Yield Point
// ============================================================================

LLVMBasicBlockRef coro_emit_yield(JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder,
                                  CoroutineCtx *coro_ctx, LLVMValueRef value) {
  // If we have a coro_ctx, use its fields. Otherwise, caller must ensure
  // they're using this in a standalone context
  LLVMValueRef handle =
      coro_ctx ? coro_ctx->coro_handle : LLVMConstNull(GENERIC_PTR);
  LLVMValueRef promise_alloca =
      coro_ctx ? coro_ctx->promise_alloca : LLVMConstNull(GENERIC_PTR);
  LLVMBasicBlockRef cleanup_bb = coro_ctx ? coro_ctx->cleanup_bb : NULL;
  LLVMBasicBlockRef suspend_bb = coro_ctx ? coro_ctx->suspend_bb : NULL;

  // Store value in promise (not wrapped in Option)
  // The resume wrapper will construct Some(value) when reading
  LLVMBuildStore(builder, value, promise_alloca);

  // Create suspension point
  LLVMValueRef save_token = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "coro.save");

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "coro.suspend");

  // Switch on suspension result
  //     0 = coroutine RESUMED → continue execution
  //     1 = coroutine destroyed → cleanup
  //     default = coroutine SUSPENDED → return to caller

  // Create blocks for the different suspend outcomes
  LLVMBasicBlockRef current_fn =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMBasicBlockRef return_bb =
      LLVMAppendBasicBlock(current_fn, "yield.return");
  LLVMBasicBlockRef resume_bb =
      LLVMAppendBasicBlock(current_fn, "yield.resume");

  // Build the switch - DEFAULT returns to caller (suspended)
  // case 0: coroutine resumed → continue to next yield
  // case 1: coroutine destroyed → cleanup
  // default: coroutine suspended → return to caller
  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  // Return block - suspend: exits coroutine and returns to caller
  LLVMPositionBuilderAtEnd(builder, return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // Resume block - when resumed, continue execution here
  LLVMPositionBuilderAtEnd(builder, resume_bb);

  // Increment yield count if we have a context
  if (coro_ctx) {
    coro_ctx->yield_count++;
  }

  return resume_bb;
}

// ============================================================================
// Helper: Final Suspend
// ============================================================================

void coro_emit_final_suspend(JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder, LLVMValueRef handle,
                             LLVMValueRef function,
                             LLVMBasicBlockRef cleanup_bb,
                             LLVMBasicBlockRef suspend_bb) {
  // Save state before final suspend
  LLVMValueRef final_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "final.save");

  // Final suspend (true = FINAL)
  LLVMValueRef final_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){final_save, LLVMConstInt(LLVMInt1Type(), 1, 0)}, 2,
      "final.suspend");

  // Switch on final suspend result
  LLVMBasicBlockRef final_return_bb =
      LLVMAppendBasicBlock(function, "final.return");
  LLVMValueRef final_switch =
      LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0),
              final_return_bb);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, final_return_bb);
  LLVMBuildBr(builder, suspend_bb);
}

// ============================================================================
// Helper: Cleanup and Suspend
// ============================================================================

void coro_emit_cleanup_and_suspend(JITLangCtx *ctx, LLVMModuleRef module,
                                   LLVMBuilderRef builder, LLVMValueRef coro_id,
                                   LLVMValueRef handle,
                                   LLVMBasicBlockRef cleanup_bb,
                                   LLVMBasicBlockRef suspend_bb) {
  // Cleanup block
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);

  // Free the coroutine frame
  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){coro_id, handle}, 2,
      "coro.free");

  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // Suspend block - return just the handle
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");

  LLVMBuildRet(builder, handle);
}

// ============================================================================
// Helper: Yield-From Loop
// ============================================================================

LLVMBasicBlockRef coro_emit_yield_from_loop(
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder,
    LLVMValueRef wrapper_handle, LLVMValueRef inner_handle,
    LLVMValueRef promise_alloca, LLVMTypeRef yield_type,
    LLVMBasicBlockRef cleanup_bb, LLVMBasicBlockRef suspend_bb,
    const char *label_prefix) {

  // Get current function for creating blocks
  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef current_fn = LLVMGetBasicBlockParent(current_bb);

  // Create blocks for the yield-from loop
  char block_name[128];

  snprintf(block_name, sizeof(block_name), "%s.check", label_prefix);
  LLVMBasicBlockRef loop_check_bb =
      LLVMAppendBasicBlock(current_fn, block_name);

  snprintf(block_name, sizeof(block_name), "%s.body", label_prefix);
  LLVMBasicBlockRef loop_body_bb = LLVMAppendBasicBlock(current_fn, block_name);

  snprintf(block_name, sizeof(block_name), "%s.resume", label_prefix);
  LLVMBasicBlockRef loop_resume_bb =
      LLVMAppendBasicBlock(current_fn, block_name);

  snprintf(block_name, sizeof(block_name), "%s.exit", label_prefix);
  LLVMBasicBlockRef loop_exit_bb = LLVMAppendBasicBlock(current_fn, block_name);

  // Branch to loop check
  LLVMBuildBr(builder, loop_check_bb);

  // === LOOP CHECK: Is inner coroutine done? ===
  LLVMPositionBuilderAtEnd(builder, loop_check_bb);
  LLVMValueRef is_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle}, 1,
      "inner.is_done_before");
  LLVMBuildCondBr(builder, is_done_before, loop_exit_bb, loop_body_bb);

  // === LOOP BODY: Resume inner and read value ===
  LLVMPositionBuilderAtEnd(builder, loop_body_bb);

  // Resume inner coroutine
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
      get_coro_resume_intrinsic(module), (LLVMValueRef[]){inner_handle}, 1, "");

  // Check if done after resume
  LLVMValueRef is_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle}, 1,
      "inner.is_done_after");
  LLVMBuildCondBr(builder, is_done_after, loop_exit_bb, loop_body_bb);

  // If not done, read promise value from inner coroutine
  LLVMValueRef promise_ptr = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){inner_handle, LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "inner.promise.raw");

  LLVMValueRef inner_promise_type_ptr =
      LLVMBuildBitCast(builder, promise_ptr, LLVMPointerType(yield_type, 0),
                       "inner.promise.typed");
  LLVMValueRef inner_value = LLVMBuildLoad2(
      builder, yield_type, inner_promise_type_ptr, "inner.value");

  // Store to our promise
  LLVMBuildStore(builder, inner_value, promise_alloca);

  // Suspend (yield this value to our caller)
  LLVMValueRef save_token = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){wrapper_handle}, 1,
      "coro.save");

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "coro.suspend");

  // Switch on suspend result
  snprintf(block_name, sizeof(block_name), "%s.suspend_return", label_prefix);
  LLVMBasicBlockRef suspend_return_bb =
      LLVMAppendBasicBlock(current_fn, block_name);

  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), loop_resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  // Suspend return - return to caller
  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // Resume block - when we're resumed, loop back to check for more values
  LLVMPositionBuilderAtEnd(builder, loop_resume_bb);
  LLVMBuildBr(builder, loop_check_bb);

  // === LOOP EXIT: Inner exhausted, continue outer ===
  LLVMPositionBuilderAtEnd(builder, loop_exit_bb);

  return loop_exit_bb;
}

// ============================================================================
// Helper: coroutine reset
// ============================================================================

void coro_emit_reset(LLVMValueRef handle, LLVMTypeRef yield_type,
                     LLVMValueRef resume_fn, JITLangCtx *ctx,
                     LLVMModuleRef module, LLVMBuilderRef builder) {
  // DEPRECATED: This function doesn't work reliably for complex coroutines
  // because the suspension index location and spilled state layout varies.
  // Use coro_emit_memcpy_restore instead.

  // The lowered coroutine frame layout is:
  // Offset 0:  Resume function pointer (ptr, 8 bytes)
  // Offset 8:  Destroy function pointer (ptr, 8 bytes)
  // Offset 16: Promise data {yield_type, i1 is_done}
  // Offset 16 + promise_size: Suspension index (iN where N depends on # of
  // suspends)

  // CRITICAL: Restore the resume function pointer at offset 0
  // When a coroutine finishes, LLVM sets this to null to mark it as done.
  // We must restore it to allow the coroutine to be resumed again.
  LLVMBuildStore(builder, resume_fn, handle);

  // Construct the promise struct type {yield_type, i1}
  LLVMTypeRef promise_struct_type =
      LLVMStructType((LLVMTypeRef[]){yield_type, LLVMInt1Type()}, 2, 0);

  // Get pointer to promise at offset 16
  LLVMValueRef promise_ptr_i8 =
      LLVMBuildGEP2(builder, LLVMInt8Type(), handle,
                    (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 16, 0)}, 1,
                    "promise.offset");

  // Cast to promise struct pointer
  LLVMValueRef promise_ptr =
      LLVMBuildBitCast(builder, promise_ptr_i8,
                       LLVMPointerType(promise_struct_type, 0), "promise.ptr");

  // Reset the is_done flag (field 1) to false
  LLVMValueRef is_done_ptr = LLVMBuildStructGEP2(builder, promise_struct_type,
                                                 promise_ptr, 1, "is_done.ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_ptr);

  // Calculate suspension index offset:
  // It's at offset 16 + sizeof(promise_struct)
  // We'll use LLVM's data layout to get the actual size
  LLVMTypeRef i64_type = LLVMInt64Type();

  // Get the promise struct size using GEP trick:
  // GEP from null pointer gives us the size
  LLVMValueRef size_gep = LLVMBuildGEP2(
      builder, promise_struct_type,
      LLVMConstNull(LLVMPointerType(promise_struct_type, 0)),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt64Type(), 1, 0)}, 1, "promise.size");
  LLVMValueRef promise_size =
      LLVMBuildPtrToInt(builder, size_gep, i64_type, "promise.size.int");

  // Suspension index is at offset 16 + promise_size
  LLVMValueRef base_offset = LLVMConstInt(i64_type, 16, 0);
  LLVMValueRef index_offset =
      LLVMBuildAdd(builder, base_offset, promise_size, "index.offset");

  // Get pointer to suspension index
  LLVMValueRef index_ptr_i8 =
      LLVMBuildGEP2(builder, LLVMInt8Type(), handle,
                    (LLVMValueRef[]){index_offset}, 1, "index.ptr.i8");

  // The suspension index type varies based on number of suspend points.
  // We'll use i32 as a safe bet (can represent up to 2^32 suspend points)
  // If there are fewer suspend points, LLVM will use smaller type (i3, i8,
  // etc) but storing i32 zero will still work
  LLVMValueRef index_ptr = LLVMBuildBitCast(
      builder, index_ptr_i8, LLVMPointerType(LLVMInt32Type(), 0), "index.ptr");

  // Reset suspension index to 0 (initial state)
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), index_ptr);
}

// Maximum coroutine frame size we'll snapshot (512 bytes should be plenty)
#define MAX_CORO_FRAME_SIZE 512

void coro_emit_memcpy_restore(LLVMValueRef dst_handle,
                              LLVMValueRef src_snapshot,
                              LLVMBuilderRef builder) {
  // Restore the entire coroutine frame from a previously saved snapshot.
  // This includes: function pointers, promise data, spilled state, and
  // suspension index - everything needed to reset the coroutine to its
  // initial state.

  // We use a fixed size memcpy. This may copy more bytes than the actual
  // frame size, but that's okay - we're restoring from our own snapshot.
  LLVMValueRef frame_size =
      LLVMConstInt(LLVMInt64Type(), MAX_CORO_FRAME_SIZE, 0);

  // Get memcpy intrinsic
  LLVMTypeRef memcpy_param_types[] = {
      GENERIC_PTR,     // dest
      GENERIC_PTR,     // src
      LLVMInt64Type(), // size
      LLVMInt1Type()   // is_volatile
  };

  LLVMTypeRef memcpy_type =
      LLVMFunctionType(LLVMVoidType(), memcpy_param_types, 4, 0);

  // Declare llvm.memcpy.p0.p0.i64
  LLVMModuleRef module =
      LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
  LLVMValueRef memcpy_fn =
      LLVMGetNamedFunction(module, "llvm.memcpy.p0.p0.i64");

  if (!memcpy_fn) {
    memcpy_fn = LLVMAddFunction(module, "llvm.memcpy.p0.p0.i64", memcpy_type);
  }

  // Call memcpy to restore the frame
  LLVMValueRef memcpy_args[] = {
      dst_handle,                        // dest
      src_snapshot,                      // src
      frame_size,                        // size
      LLVMConstInt(LLVMInt1Type(), 0, 0) // is_volatile = false
  };

  LLVMBuildCall2(builder, memcpy_type, memcpy_fn, memcpy_args, 4, "");
}
