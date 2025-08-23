#include "./coroutine_scheduling.h"
#include "coroutines.h"
#include "function.h"
#include "types.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
static LLVMValueRef build_scheduled_cor_wrapper(LLVMTypeRef promise_type,
                                                LLVMValueRef scheduler,
                                                LLVMTypeRef scheduler_type,
                                                LLVMModuleRef module,
                                                LLVMBuilderRef builder) {
  LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(promise_type);

  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(LLVMVoidType(),
                       (LLVMTypeRef[]){
                           LLVMPointerType(coro_obj_type, 0),
                           LLVMInt64Type(),
                       },
                       2, 0);
  LLVMValueRef func =
      LLVMAddFunction(module, "scheduler_wrapper", wrapper_fn_type);

  LLVMSetLinkage(func, LLVMExternalLinkage);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");

  LLVMBasicBlockRef finished =
      LLVMAppendBasicBlock(func, "coro.is_finished_block");
  LLVMBasicBlockRef not_finished =
      LLVMAppendBasicBlock(func, "coro.resume_block");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMValueRef coro = LLVMGetParam(func, 0);
  LLVMValueRef timestamp = LLVMGetParam(func, 1);
  LLVMTypeRef val_type = LLVMDoubleType();
  CoroutineCtx coro_ctx = {.coro_obj_type = coro_obj_type,
                           .promise_type = promise_type};

  coro = coro_advance(coro, &coro_ctx, builder);

  LLVMValueRef is_finished = coro_is_finished(coro, &coro_ctx, builder);
  LLVMBuildCondBr(builder, is_finished, finished, not_finished);
  LLVMPositionBuilderAtEnd(builder, not_finished);
  LLVMValueRef promise =
      coro_promise(coro, coro_obj_type, promise_type, builder);
  LLVMValueRef val = LLVMBuildExtractValue(builder, promise, 1, "");

  LLVMValueRef scheduler_call =
      LLVMBuildCall2(builder, scheduler_type, scheduler,
                     (LLVMValueRef[]){
                         timestamp,
                         val,
                         func,
                         coro,
                     },
                     4, "schedule_next");

  LLVMBuildRetVoid(builder);

  LLVMPositionBuilderAtEnd(builder, finished);
  LLVMBuildRetVoid(builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  Ast *timestamp_ast = ast->data.AST_APPLICATION.args;
  LLVMValueRef ts_val = codegen(timestamp_ast, ctx, module, builder);

  Ast *scheduler_ast = ast->data.AST_APPLICATION.args + 1;
  Type *scheduler_type = scheduler_ast->term;

  Ast *coro_ast = ast->data.AST_APPLICATION.args + 2;
  Type *coro_type = coro_ast->term;

  LLVMValueRef scheduler = codegen(scheduler_ast, ctx, module, builder);
  LLVMTypeRef llvm_scheduler_type =
      type_to_llvm_type(scheduler_type, ctx, module);

  LLVMValueRef coro = codegen(coro_ast, ctx, module, builder);
  // Type *ptype = create_option_type(&t_num);
  Type *ptype = fn_return_type(coro_type);
  LLVMTypeRef promise_type = type_to_llvm_type(ptype, ctx, module);

  LLVMValueRef wrapper_fn = build_scheduled_cor_wrapper(
      promise_type, scheduler, llvm_scheduler_type, module, builder);
  return LLVMBuildCall2(builder, llvm_scheduler_type, scheduler,
                        (LLVMValueRef[]){
                            ts_val,
                            LLVMConstReal(LLVMDoubleType(), 0.),
                            wrapper_fn,
                            coro,
                        },
                        4, "");
}
