#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>

LLVMValueRef get_extern_fn(const char *name, LLVMTypeRef fn_type,
                           LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, name);

  if (fn == NULL) {
    fn = LLVMAddFunction(module, name, fn_type);
  }
  return fn;
}

int main() {
  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("coroutine_module", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

  // Create coroutine function type (no parameters, returns double)
  LLVMTypeRef ret_type = LLVMDoubleTypeInContext(context);
  LLVMTypeRef coro_fn_type = LLVMFunctionType(ret_type, NULL, 0, 0);

  // Create coroutine function
  LLVMValueRef coro_fn = LLVMAddFunction(module, "coroutine", coro_fn_type);

  // Create entry basic block
  LLVMBasicBlockRef entry_bb =
      LLVMAppendBasicBlockInContext(context, coro_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Get necessary intrinsic functions
  LLVMTypeRef void_ptr_type =
      LLVMPointerType(LLVMInt8TypeInContext(context), 0);
  LLVMTypeRef size_t_type = LLVMInt64TypeInContext(context);
  LLVMTypeRef i32_type = LLVMInt32TypeInContext(context);
  LLVMTypeRef i1_type = LLVMInt1TypeInContext(context);
  LLVMTypeRef i8_type = LLVMInt8TypeInContext(context);

  // llvm.coro.id
  LLVMTypeRef coro_id_type = LLVMFunctionType(
      void_ptr_type,
      (LLVMTypeRef[]){i32_type, void_ptr_type, void_ptr_type, void_ptr_type}, 4,
      0);
  LLVMValueRef coro_id_fn = get_extern_fn("llvm.coro.id", coro_id_type, module);

  // llvm.coro.size.i64
  LLVMTypeRef coro_size_type = LLVMFunctionType(size_t_type, NULL, 0, 0);
  LLVMValueRef coro_size_fn =
      get_extern_fn("llvm.coro.size.i64", coro_size_type, module);

  // llvm.coro.begin
  LLVMTypeRef coro_begin_type = LLVMFunctionType(
      void_ptr_type, (LLVMTypeRef[]){void_ptr_type, void_ptr_type}, 2, 0);
  LLVMValueRef coro_begin_fn =
      get_extern_fn("llvm.coro.begin", coro_begin_type, module);

  // llvm.coro.suspend
  LLVMTypeRef coro_suspend_type = LLVMFunctionType(
      i8_type, (LLVMTypeRef[]){LLVMTokenTypeInContext(context), i1_type}, 2, 0);
  LLVMValueRef coro_suspend_fn =
      get_extern_fn("llvm.coro.suspend", coro_suspend_type, module);

  // llvm.coro.end
  LLVMTypeRef coro_end_type =
      LLVMFunctionType(i1_type, (LLVMTypeRef[]){void_ptr_type, i1_type}, 2, 0);
  LLVMValueRef coro_end_fn =
      get_extern_fn("llvm.coro.end", coro_end_type, module);

  // Create coroutine ID
  LLVMValueRef zero = LLVMConstInt(i32_type, 0, 0);
  LLVMValueRef null_ptr = LLVMConstNull(void_ptr_type);
  LLVMValueRef coro_id = LLVMBuildCall2(
      builder, coro_id_type, coro_id_fn,
      (LLVMValueRef[]){zero, null_ptr, null_ptr, null_ptr}, 4, "coro.id");

  // Get coroutine frame size
  LLVMValueRef coro_size = LLVMBuildCall2(builder, coro_size_type, coro_size_fn,
                                          NULL, 0, "coro.size");

  // Allocate memory for coroutine frame
  LLVMValueRef malloc_fn = get_extern_fn(
      "malloc",
      LLVMFunctionType(void_ptr_type, (LLVMTypeRef[]){size_t_type}, 1, 0),
      module);
  LLVMValueRef frame_mem = LLVMBuildCall2(
      builder,
      LLVMFunctionType(void_ptr_type, (LLVMTypeRef[]){size_t_type}, 1, 0),
      malloc_fn, &coro_size, 1, "frame.mem");

  // Create coroutine promise
  LLVMValueRef coro_hdl =
      LLVMBuildCall2(builder, coro_begin_type, coro_begin_fn,
                     (LLVMValueRef[]){coro_id, frame_mem}, 2, "coro.hdl");

  // Create basic blocks for yields and final suspend
  LLVMBasicBlockRef yield1_bb =
      LLVMAppendBasicBlockInContext(context, coro_fn, "yield1");
  LLVMBasicBlockRef yield2_bb =
      LLVMAppendBasicBlockInContext(context, coro_fn, "yield2");
  LLVMBasicBlockRef yield3_bb =
      LLVMAppendBasicBlockInContext(context, coro_fn, "yield3");
  LLVMBasicBlockRef yield4_bb =
      LLVMAppendBasicBlockInContext(context, coro_fn, "yield4");
  LLVMBasicBlockRef final_bb =
      LLVMAppendBasicBlockInContext(context, coro_fn, "final");

  // Implement yields
  LLVMValueRef yields[] = {
      LLVMConstReal(ret_type, 1.0), LLVMConstReal(ret_type, 2.0),
      LLVMConstReal(ret_type, 3.0), LLVMConstReal(ret_type, 4.0)};

  LLVMBasicBlockRef yield_bbs[] = {yield1_bb, yield2_bb, yield3_bb, yield4_bb};

  for (int i = 0; i < 4; i++) {
    LLVMPositionBuilderAtEnd(builder, yield_bbs[i]);
    LLVMValueRef token = LLVMBuildCall2(
        builder, coro_suspend_type, coro_suspend_fn,
        (LLVMValueRef[]){LLVMConstNull(LLVMTokenTypeInContext(context)),
                         LLVMConstInt(i1_type, 0, 0)},
        2, "suspend");
    LLVMValueRef switch_val = LLVMBuildSwitch(
        builder, token, (i < 3) ? yield_bbs[i + 1] : final_bb, 2);
    LLVMAddCase(switch_val, LLVMConstInt(i8_type, 0, 0),
                (i < 3) ? yield_bbs[i + 1] : final_bb);
    LLVMAddCase(switch_val, LLVMConstInt(i8_type, 1, 0), final_bb);
    LLVMBuildRet(builder, yields[i]);
  }

  // Implement final suspend
  LLVMPositionBuilderAtEnd(builder, final_bb);
  LLVMBuildCall2(builder, coro_end_type, coro_end_fn,
                 (LLVMValueRef[]){coro_hdl, LLVMConstInt(i1_type, 0, 0)}, 2,
                 "");
  LLVMBuildRet(builder, LLVMConstReal(ret_type, 0.0));

  LLVMPositionBuilderAtEnd(builder, entry_bb);
  LLVMBuildRetVoid(builder);

  LLVMDumpModule(module);
  // Verify the module

  char *error = NULL;
  LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
  LLVMDisposeMessage(error);

  // Print the module IR
  char *ir = LLVMPrintModuleToString(module);
  printf("%s\n", ir);
  LLVMDisposeMessage(ir);

  // Clean up
  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(module);
  LLVMContextDispose(context);

  return 0;
}
