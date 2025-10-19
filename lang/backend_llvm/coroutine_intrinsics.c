#include "./coroutines.h"
#include "adt.h"
#include "application.h"
#include "function.h"
#include "serde.h"
#include "types.h"
#include "types/builtins.h"
#include "types/type.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations for helper functions
static LLVMValueRef get_or_declare_intrinsic(LLVMModuleRef module,
                                             const char *name,
                                             LLVMTypeRef type);
static LLVMValueRef compile_coroutine_body(Ast *ast, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder,
                                           LLVMTypeRef promise_type,
                                           const char *name);
static LLVMValueRef
compile_coroutine_ramp(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                       LLVMBuilderRef builder, LLVMTypeRef promise_type,
                       const char *name, LLVMValueRef coro_func);

// Helper to get or declare an intrinsic function
static LLVMValueRef get_or_declare_intrinsic(LLVMModuleRef module,
                                             const char *name,
                                             LLVMTypeRef type) {
  LLVMValueRef func = LLVMGetNamedFunction(module, name);
  if (!func) {
    func = LLVMAddFunction(module, name, type);

    // Mark coroutine intrinsics with special attributes to prevent removal
    if (strncmp(name, "llvm.coro.", 10) == 0) {
      LLVMContextRef ctx = LLVMGetModuleContext(module);

      // Add nounwind attribute - coroutine intrinsics don't throw
      LLVMAttributeRef nounwind_attr = LLVMCreateEnumAttribute(
          ctx, LLVMGetEnumAttributeKindForName("nounwind", 8), 0);
      LLVMAddAttributeAtIndex(func, LLVMAttributeFunctionIndex, nounwind_attr);

      // Add willreturn attribute - intrinsics always return
      LLVMAttributeRef willreturn_attr = LLVMCreateEnumAttribute(
          ctx, LLVMGetEnumAttributeKindForName("willreturn", 10), 0);
      LLVMAddAttributeAtIndex(func, LLVMAttributeFunctionIndex, willreturn_attr);
    }
  }
  return func;
}

LLVMValueRef compile_coroutine(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  Type *t = ast->md;
  Type *coro_cons_fn_type = t->data.T_CONS.args[0];
  Type *ret_type = fn_return_type(coro_cons_fn_type)->data.T_CONS.args[0];

  Type return_opt_type = TOPT(ret_type);
  LLVMTypeRef promise_type = type_to_llvm_type(&return_opt_type, ctx, module);

  // Get or generate coroutine name
  const char *name;
  if (ast->data.AST_LAMBDA.fn_name.chars) {
    name = ast->data.AST_LAMBDA.fn_name.chars;
  } else {
    name = "anon_coro";
  }

  printf("Compiling coroutine: %s\n", name);

  // Compile the actual coroutine body function (which will be split by LLVM)
  LLVMValueRef coro_func =
      compile_coroutine_body(ast, ctx, module, builder, promise_type, name);

  // Compile the ramp function (initialization/constructor)
  LLVMValueRef ramp_func = compile_coroutine_ramp(
      ast, ctx, module, builder, promise_type, name, coro_func);
  printf("RAMP: \n");
  LLVMDumpValue(ramp_func);
  printf("\n\n");

  printf("COR FN: \n");
  LLVMDumpValue(coro_func);
  printf("\n\n");

  return ramp_func;
}

static LLVMValueRef compile_coroutine_body(Ast *ast, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder,
                                           LLVMTypeRef promise_type,
                                           const char *name) {
  Type *t = ast->md;
  Type *coro_cons_fn_type = t->data.T_CONS.args[0];

  // Build parameter types for the coroutine function
  int num_params = ast->data.AST_LAMBDA.len;
  if (num_params == 1 && ast->data.AST_LAMBDA.params->ast->tag == AST_VOID) {
    num_params = 0;
  }

  LLVMTypeRef param_types[num_params > 0 ? num_params : 1];
  Type *ftype = coro_cons_fn_type;

  for (int i = 0; i < num_params; i++) {
    param_types[i] = type_to_llvm_type(ftype->data.T_FN.from, ctx, module);
    ftype = ftype->data.T_FN.to;
  }

  // The coroutine function returns a pointer (to the coroutine frame)
  LLVMTypeRef func_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                                           param_types, num_params, 0);

  LLVMValueRef coro_func = LLVMAddFunction(module, name, func_type);
  LLVMSetLinkage(coro_func, LLVMInternalLinkage);

  // Mark as a presplit coroutine
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMAttributeRef presplit_attr =
      LLVMCreateStringAttribute(llvm_ctx, "coroutine.presplit", 19, "", 0);
  LLVMAddAttributeAtIndex(coro_func, LLVMAttributeFunctionIndex, presplit_attr);

  // Create basic blocks
  LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(coro_func, "entry");
  LLVMBasicBlockRef loop_bb = LLVMAppendBasicBlock(coro_func, "loop");

  LLVMBasicBlockRef cleanup_bb = LLVMAppendBasicBlock(coro_func, "cleanup");
  LLVMBasicBlockRef suspend_bb = LLVMAppendBasicBlock(coro_func, "suspend");

  LLVMBasicBlockRef saved_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Declare intrinsics
  LLVMTypeRef ptr_type = LLVMPointerType(LLVMInt8Type(), 0);
  LLVMValueRef null_ptr = LLVMConstNull(ptr_type);

  // Create a null token for intrinsics that need token parameters
  LLVMValueRef token_none = LLVMConstNull(LLVMTokenTypeInContext(llvm_ctx));

  // llvm.coro.id
  LLVMTypeRef coro_id_type = LLVMFunctionType(
      LLVMTokenTypeInContext(llvm_ctx),
      (LLVMTypeRef[]){LLVMInt32Type(), ptr_type, ptr_type, ptr_type}, 4, 0);
  LLVMValueRef coro_id_fn =
      get_or_declare_intrinsic(module, "llvm.coro.id", coro_id_type);

  LLVMValueRef id =
      LLVMBuildCall2(builder, coro_id_type, coro_id_fn,
                     (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0),
                                      null_ptr, null_ptr, null_ptr},
                     4, "id");

  // Mark the call as convergent to prevent certain optimizations from removing it
  LLVMAttributeRef convergent_attr = LLVMCreateEnumAttribute(
      llvm_ctx, LLVMGetEnumAttributeKindForName("convergent", 10), 0);
  LLVMAddCallSiteAttribute(id, LLVMAttributeFunctionIndex, convergent_attr);

  // llvm.coro.size.i32
  LLVMTypeRef coro_size_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
  LLVMValueRef coro_size_fn =
      get_or_declare_intrinsic(module, "llvm.coro.size.i32", coro_size_type);
  LLVMValueRef size =
      LLVMBuildCall2(builder, coro_size_type, coro_size_fn, NULL, 0, "size");

  // Allocate memory for coroutine frame using malloc
  LLVMTypeRef malloc_type =
      LLVMFunctionType(ptr_type, (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
  LLVMValueRef malloc_fn =
      get_or_declare_intrinsic(module, "malloc", malloc_type);
  LLVMValueRef alloc = LLVMBuildCall2(builder, malloc_type, malloc_fn,
                                      (LLVMValueRef[]){size}, 1, "alloc");

  // llvm.coro.begin
  LLVMTypeRef coro_begin_type = LLVMFunctionType(
      ptr_type, (LLVMTypeRef[]){LLVMTokenTypeInContext(llvm_ctx), ptr_type}, 2,
      0);
  LLVMValueRef coro_begin_fn =
      get_or_declare_intrinsic(module, "llvm.coro.begin", coro_begin_type);
  LLVMValueRef hdl = LLVMBuildCall2(builder, coro_begin_type, coro_begin_fn,
                                    (LLVMValueRef[]){id, alloc}, 2, "hdl");

  // Mark llvm.coro.begin as convergent too
  LLVMAttributeRef convergent_attr2 = LLVMCreateEnumAttribute(
      llvm_ctx, LLVMGetEnumAttributeKindForName("convergent", 10), 0);
  LLVMAddCallSiteAttribute(hdl, LLVMAttributeFunctionIndex, convergent_attr2);

  LLVMBuildBr(builder, loop_bb);

  // === LOOP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, loop_bb);

  // Create coroutine object type for promise storage
  LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(promise_type);

  // Set up coroutine context for codegen_yield to access
  CoroutineCtx coro_ctx = {
      .cons_type = coro_cons_fn_type,
      .coro_obj_type = coro_obj_type,
      .promise_type = promise_type,
      .current_yield = 0,
      .num_coroutine_yields = ast->data.AST_LAMBDA.num_yields,
      .num_yield_boundary_xs = ast->data.AST_LAMBDA.num_yield_boundary_crossers,
      .yield_boundary_xs = ast->data.AST_LAMBDA.yield_boundary_crossers,
      .func = coro_func,
      .state_layout = NULL, // Will be set if needed
      .name = name,
      .cleanup_bb = cleanup_bb,
      .suspend_bb = suspend_bb,
      .coro_id = id,  // The token from llvm.coro.id
      .coro_hdl = hdl // The handle from llvm.coro.begin
  };

  // Create a new context with the coroutine context
  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx);
  fn_ctx.coro_ctx = &coro_ctx;

  // Bind function parameters to the context
  // The parameters are available in the coroutine function
  int param_idx = 0;
  for (AstList *param_list = ast->data.AST_LAMBDA.params; param_list != NULL;
       param_list = param_list->next) {
    if (param_list->ast->tag == AST_VOID) {
      continue;
    }
    LLVMValueRef param_val = LLVMGetParam(coro_func, param_idx);
    Type *param_type = coro_cons_fn_type->data.T_FN.from;
    bind_fn_param(param_val, param_type, param_list->ast, ctx, &fn_ctx, module,
                  builder);
    coro_cons_fn_type = coro_cons_fn_type->data.T_FN.to;
    param_idx++;
  }

  // Generate the actual coroutine body
  // This will call codegen_yield when it encounters yield expressions
  printf("Generating coroutine body for %s\n", name);
  LLVMValueRef body_result = codegen_lambda_body(ast, &fn_ctx, module, builder);

  // After body completes (falls through without explicit return/yield)
  // we need to end the coroutine properly
  // Branch to cleanup to properly destroy the coroutine
  LLVMBuildBr(builder, cleanup_bb);

  // === CLEANUP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);

  // llvm.coro.free
  LLVMTypeRef coro_free_type = LLVMFunctionType(
      ptr_type, (LLVMTypeRef[]){LLVMTokenTypeInContext(llvm_ctx), ptr_type}, 2,
      0);
  LLVMValueRef coro_free_fn =
      get_or_declare_intrinsic(module, "llvm.coro.free", coro_free_type);
  LLVMValueRef mem = LLVMBuildCall2(builder, coro_free_type, coro_free_fn,
                                    (LLVMValueRef[]){id, hdl}, 2, "mem");

  // Free the memory
  LLVMTypeRef free_type =
      LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){ptr_type}, 1, 0);
  LLVMValueRef free_fn = get_or_declare_intrinsic(module, "free", free_type);
  LLVMBuildCall2(builder, free_type, free_fn, (LLVMValueRef[]){mem}, 1, "");

  LLVMBuildBr(builder, suspend_bb);

  // === SUSPEND BLOCK ===
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  // llvm.coro.end
  LLVMTypeRef coro_end_type =
      LLVMFunctionType(LLVMInt1Type(),
                       (LLVMTypeRef[]){ptr_type, LLVMInt1Type(),
                                       LLVMTokenTypeInContext(llvm_ctx)},
                       3, 0);
  LLVMValueRef coro_end_fn =
      get_or_declare_intrinsic(module, "llvm.coro.end", coro_end_type);

  LLVMBuildCall2(
      builder, coro_end_type, coro_end_fn,
      (LLVMValueRef[]){hdl, LLVMConstInt(LLVMInt1Type(), 0, 0), token_none}, 3,
      "");

  LLVMBuildRet(builder, hdl);

  // Restore builder position
  if (saved_block) {
    LLVMPositionBuilderAtEnd(builder, saved_block);
  }

  printf("Coroutine body function created: %s\n", name);
  return coro_func;
}

static LLVMValueRef
compile_coroutine_ramp(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                       LLVMBuilderRef builder, LLVMTypeRef promise_type,
                       const char *name, LLVMValueRef coro_func) {
  // The ramp function is the constructor that calls the coroutine
  // and returns the coroutine handle

  Type *t = ast->md;
  Type *coro_cons_fn_type = t->data.T_CONS.args[0];

  // Build parameter types
  int num_params = ast->data.AST_LAMBDA.len;
  if (num_params == 1 && ast->data.AST_LAMBDA.params->ast->tag == AST_VOID) {
    num_params = 0;
  }

  LLVMTypeRef param_types[num_params > 0 ? num_params : 1];
  Type *ftype = coro_cons_fn_type;

  for (int i = 0; i < num_params; i++) {
    param_types[i] = type_to_llvm_type(ftype->data.T_FN.from, ctx, module);
    ftype = ftype->data.T_FN.to;
  }

  // Create the init function name
  char *init_name = calloc(strlen(name) + 6, sizeof(char));
  sprintf(init_name, "%s.init", name);

  // Returns a pointer to the coroutine handle
  LLVMTypeRef ptr_type = LLVMPointerType(LLVMInt8Type(), 0);
  LLVMTypeRef ramp_type =
      LLVMFunctionType(ptr_type, param_types, num_params, 0);

  LLVMValueRef ramp_func = LLVMAddFunction(module, init_name, ramp_type);

  // Create entry block
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(ramp_func, "entry");
  LLVMBasicBlockRef saved_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, entry);

  // Call the coroutine function with the parameters
  LLVMValueRef args[num_params > 0 ? num_params : 1];
  for (int i = 0; i < num_params; i++) {
    args[i] = LLVMGetParam(ramp_func, i);
  }

  LLVMTypeRef coro_func_type =
      LLVMFunctionType(ptr_type, param_types, num_params, 0);
  LLVMValueRef handle = LLVMBuildCall2(builder, coro_func_type, coro_func, args,
                                       num_params, "coro_handle");

  LLVMBuildRet(builder, handle);

  // Restore builder position
  if (saved_block) {
    LLVMPositionBuilderAtEnd(builder, saved_block);
  }

  printf("Coroutine ramp function created: %s\n", init_name);
  free(init_name);
  return ramp_func;
}

LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *app,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  printf("coro create\n");
  print_ast(app);

  // Get the callable (the ramp function)
  // If it's a generic function, we need to instantiate it first
  LLVMValueRef callable;
  if (sym->type == STYPE_GENERIC_FUNCTION) {
    // TODO: Handle generic coroutines - need to instantiate with specific types
    fprintf(stderr, "Generic coroutines not yet supported with intrinsics\n");
    return NULL;
  } else {
    // The symbol value is the ramp function (e.g., "coroutine_name.init")
    callable = sym->val;
  }

  // The expected_fn_type is something like: Coroutine<T>
  // We need to modify the type to replace the Coroutine<T> return with ptr
  // This is because the ramp function returns a coroutine handle (ptr)

  print_type(expected_fn_type);
  Type *ctype = deep_copy_type(expected_fn_type);
  Type *c = ctype;
  int i = 0;

  // Walk through the function type to find where the coroutine type is
  // For example: i32 -> i32 -> Coroutine<f64>
  // We want to change it to: i32 -> i32 -> ptr
  while (!is_coroutine_type(c->data.T_FN.to)) {
    c = c->data.T_FN.to;
    i++;
  }

  // Replace the Coroutine<T> with ptr
  c->data.T_FN.to = &t_ptr;

  // Call the ramp function with the application's arguments
  // This will:
  // 1. Call the ramp function (e.g., "my_coro.init")
  // 2. Which calls the coroutine body function
  // 3. Which executes until the first yield (or completion)
  // 4. Returns the coroutine handle (pointer)
  LLVMValueRef coro_handle =
      call_callable(app, ctype, callable, ctx, module, builder);

  printf("Coroutine instance created, handle: %p\n", (void *)coro_handle);

  return coro_handle;
}

LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  // Get the coroutine handle from the symbol
  LLVMValueRef coro_handle = sym->val;

  // Get the coroutine type to extract the promise type
  Type *coro_type = sym->symbol_type;
  // coro_type is Coroutine<T>, we need to get T
  Type *item_type = coro_type->data.T_CONS.args[0];
  // The promise is Option<T>
  Type *ret_opt_type = create_option_type(item_type);
  LLVMTypeRef promise_type = type_to_llvm_type(ret_opt_type, ctx, module);

  // Get current function for creating basic blocks
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef func = LLVMGetBasicBlockParent(current_block);

  // Create basic blocks for control flow
  LLVMBasicBlockRef check_done_block =
      LLVMAppendBasicBlock(func, "coro.check_done");
  LLVMBasicBlockRef done_block = LLVMAppendBasicBlock(func, "coro.is_done");
  LLVMBasicBlockRef resume_block = LLVMAppendBasicBlock(func, "coro.resume");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "coro.merge");

  // Branch to check block
  LLVMBuildBr(builder, check_done_block);

  // === CHECK IF DONE BLOCK ===
  LLVMPositionBuilderAtEnd(builder, check_done_block);

  // Check if coroutine is done using llvm.coro.done
  LLVMTypeRef ptr_type = LLVMPointerType(LLVMInt8Type(), 0);
  LLVMTypeRef coro_done_type =
      LLVMFunctionType(LLVMInt1Type(), (LLVMTypeRef[]){ptr_type}, 1, 0);
  LLVMValueRef coro_done_fn =
      get_or_declare_intrinsic(module, "llvm.coro.done", coro_done_type);

  LLVMValueRef is_done =
      LLVMBuildCall2(builder, coro_done_type, coro_done_fn,
                     (LLVMValueRef[]){coro_handle}, 1, "is_done");

  LLVMBuildCondBr(builder, is_done, done_block, resume_block);

  // === DONE BLOCK (coroutine completed) ===
  LLVMPositionBuilderAtEnd(builder, done_block);

  // Return None since coroutine is finished
  LLVMValueRef none_promise = codegen_none_typed(builder, promise_type);
  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef done_end_block = LLVMGetInsertBlock(builder);

  // === RESUME BLOCK (coroutine still running) ===
  LLVMPositionBuilderAtEnd(builder, resume_block);

  // Resume the coroutine using llvm.coro.resume
  LLVMTypeRef coro_resume_type =
      LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){ptr_type}, 1, 0);
  LLVMValueRef coro_resume_fn =
      get_or_declare_intrinsic(module, "llvm.coro.resume", coro_resume_type);

  LLVMBuildCall2(builder, coro_resume_type, coro_resume_fn,
                 (LLVMValueRef[]){coro_handle}, 1, "");

  // Get the promise (yielded value) using llvm.coro.promise
  // llvm.coro.promise(ptr <handle>, i32 <alignment>, i1 <from_front>)
  LLVMTypeRef coro_promise_type = LLVMFunctionType(
      ptr_type, (LLVMTypeRef[]){ptr_type, LLVMInt32Type(), LLVMInt1Type()}, 3,
      0);
  LLVMValueRef coro_promise_fn =
      get_or_declare_intrinsic(module, "llvm.coro.promise", coro_promise_type);

  // Get promise pointer with alignment and from_front=false
  LLVMValueRef promise_ptr = LLVMBuildCall2(
      builder, coro_promise_type, coro_promise_fn,
      (LLVMValueRef[]){coro_handle,
                       LLVMConstInt(LLVMInt32Type(), 8, 0), // alignment
                       LLVMConstInt(LLVMInt1Type(), 0, 0)}, // from_front
      3, "promise_ptr");

  // Cast the promise pointer to the correct type and load it
  LLVMTypeRef promise_ptr_typed = LLVMPointerType(promise_type, 0);
  LLVMValueRef promise_ptr_cast = LLVMBuildBitCast(
      builder, promise_ptr, promise_ptr_typed, "promise_ptr_typed");
  LLVMValueRef promise_value =
      LLVMBuildLoad2(builder, promise_type, promise_ptr_cast, "promise");

  LLVMBuildBr(builder, merge_block);
  LLVMBasicBlockRef resume_end_block = LLVMGetInsertBlock(builder);

  // === MERGE BLOCK ===
  LLVMPositionBuilderAtEnd(builder, merge_block);

  // Create PHI node to select between None (done) and promise value (resumed)
  LLVMValueRef result_phi = LLVMBuildPhi(builder, promise_type, "result");
  LLVMAddIncoming(result_phi, (LLVMValueRef[]){none_promise, promise_value},
                  (LLVMBasicBlockRef[]){done_end_block, resume_end_block}, 2);

  return result_phi;
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  CoroutineCtx *coro_ctx = ctx->coro_ctx;
  if (!coro_ctx) {
    fprintf(stderr, "Error - yield must only be used in a coroutine context\n");
    return NULL;
  }

  LLVMValueRef coro_func = coro_ctx->func;
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);

  Ast *expr = ast->data.AST_YIELD.expr;

  // TODO: Handle coroutine-yielding-coroutine cases (recursive, tail call,
  // chaining) For now, implement the simple case: yielding a value

  // 1. Evaluate the yielded expression
  LLVMValueRef yield_val = codegen(expr, ctx, module, builder);

  // 2. Store the value in the promise
  // TODO: Implement promise storage using llvm.coro.promise intrinsic
  // For now, we'll skip this and just suspend

  // 3. Get the coroutine handle from the context
  // This is the handle returned by llvm.coro.begin in the entry block
  LLVMValueRef coro_handle = coro_ctx->coro_hdl;

  // Call llvm.coro.save to get the suspend point token
  // This is used when you might have multiple suspend points that need to be
  // distinguished
  LLVMTypeRef ptr_type = LLVMPointerType(LLVMInt8Type(), 0);
  LLVMTypeRef coro_save_type = LLVMFunctionType(
      LLVMTokenTypeInContext(llvm_ctx), (LLVMTypeRef[]){ptr_type}, 1, 0);
  LLVMValueRef coro_save_fn =
      get_or_declare_intrinsic(module, "llvm.coro.save", coro_save_type);

  LLVMValueRef save_token =
      LLVMBuildCall2(builder, coro_save_type, coro_save_fn,
                     (LLVMValueRef[]){coro_handle}, 1, "save");

  // 4. Call llvm.coro.suspend to suspend the coroutine
  LLVMTypeRef coro_suspend_type = LLVMFunctionType(
      LLVMInt8Type(),
      (LLVMTypeRef[]){LLVMTokenTypeInContext(llvm_ctx), LLVMInt1Type()}, 2, 0);
  LLVMValueRef coro_suspend_fn =
      get_or_declare_intrinsic(module, "llvm.coro.suspend", coro_suspend_type);

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, coro_suspend_type, coro_suspend_fn,
      (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");

  // 5. Create basic blocks for the switch
  LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(coro_func, "resume");
  LLVMBasicBlockRef cleanup_bb = coro_ctx->cleanup_bb;
  LLVMBasicBlockRef suspend_bb = coro_ctx->suspend_bb;

  // If cleanup_bb doesn't exist, we need to handle this differently
  // For now, assume it exists from compile_coroutine_body
  if (!cleanup_bb) {
    fprintf(stderr, "Error: cleanup block not found in coroutine function\n");
    cleanup_bb = LLVMAppendBasicBlock(coro_func, "cleanup_fallback");
  }
  if (!suspend_bb) {
    suspend_bb = LLVMAppendBasicBlock(coro_func, "suspend_fallback");
  }

  // 6. Switch on suspend result
  // 0 = resume (continue execution after yield)
  // 1 = destroy (cleanup and exit)
  // default = suspend (return to caller)
  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  // 7. Position builder at resume block for code after the yield
  LLVMPositionBuilderAtEnd(builder, resume_bb);

  // Increment yield counter
  coro_ctx->current_yield++;

  return yield_val;
}

LLVMValueRef coro_promise_set(LLVMValueRef coro, LLVMValueRef val,
                              LLVMTypeRef coro_obj_type,
                              LLVMTypeRef promise_type,
                              LLVMBuilderRef builder) {
  // TODO: Use llvm.coro.promise to get promise storage and set the value
  return NULL;
}

LLVMValueRef coro_counter_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  // Placeholder - may not be needed with LLVM intrinsics
  return NULL;
}

LLVMValueRef coro_end_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  // Placeholder - may not be needed with LLVM intrinsics
  return NULL;
}

LLVMValueRef coro_promise_set_none(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                                   LLVMTypeRef promise_type,
                                   LLVMBuilderRef builder) {
  // TODO: Set promise to None/null value
  return NULL;
}

void coro_terminate_block(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                          LLVMBuilderRef builder) {
  // TODO: Generate termination code for coroutine
  // Should call llvm.coro.end and possibly cleanup
}

LLVMValueRef coro_jump_to_next_block(LLVMValueRef coro, LLVMValueRef next_coro,
                                     CoroutineCtx *coro_ctx,
                                     LLVMBuilderRef builder) {
  // TODO: Handle chaining of coroutines
  return NULL;
}

LLVMValueRef coro_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                          LLVMBuilderRef builder) {
  // Placeholder - may not be needed with LLVM intrinsics
  return NULL;
}

LLVMValueRef coro_next_set(LLVMValueRef coro, LLVMValueRef next,
                           LLVMTypeRef coro_obj_type, LLVMBuilderRef builder) {
  // Placeholder - for custom coroutine chaining if needed
  return NULL;
}

LLVMValueRef coro_next(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                       LLVMBuilderRef builder) {
  // Placeholder - for custom coroutine chaining if needed
  return NULL;
}

LLVMValueRef coro_advance(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                          LLVMBuilderRef builder) {
  // TODO: Advance coroutine using llvm.coro.resume
  return NULL;
}

LLVMValueRef coro_replace(LLVMValueRef coro, LLVMValueRef new_coro,
                          CoroutineCtx *coro_ctx, LLVMBuilderRef builder) {
  // Placeholder - for custom coroutine chaining if needed
  return NULL;
}

LLVMValueRef coro_promise_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder) {
  // TODO: Get promise pointer using llvm.coro.promise intrinsic
  return NULL;
}

LLVMTypeRef get_coro_state_layout(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module) {
  // TODO: Calculate state layout for values that cross yield boundaries
  // With LLVM intrinsics, this is handled automatically by the frame
  return NULL;
}

LLVMValueRef coro_state_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                            LLVMBuilderRef builder) {
  // Placeholder - state is managed by LLVM coroutine frame
  return NULL;
}

LLVMValueRef coro_is_finished(LLVMValueRef coro, CoroutineCtx *ctx,
                              LLVMBuilderRef builder) {
  // TODO: Check if coroutine is finished using llvm.coro.done
  return NULL;
}

LLVMValueRef coro_state(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                        LLVMBuilderRef builder) {
  // Placeholder - state is managed by LLVM coroutine frame
  return NULL;
}

LLVMValueRef coro_promise(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                          LLVMTypeRef promise_type, LLVMBuilderRef builder) {
  // TODO: Get promise value using llvm.coro.promise intrinsic
  return NULL;
}

LLVMValueRef coro_incr(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                       LLVMBuilderRef builder) {
  // Placeholder - increment operations may not be needed with LLVM intrinsics
  return NULL;
}
