#include "function.h"
#include "types.h"
extern "C" {

#include "../../types/type.h"
#include "../adt.h"
#include "coroutines.h"
#include "serde.h"
#include "llvm-c/Types.h"
// Create an alias for the C JITSymbol to avoid conflicts
typedef struct JITSymbol YLCSym;
typedef struct Type YLCType;
}

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Coroutines/CoroCleanup.h"
#include "llvm/Transforms/Coroutines/CoroConditionalWrapper.h"
#include "llvm/Transforms/Coroutines/CoroEarly.h"
#include "llvm/Transforms/Coroutines/CoroElide.h"
#include <cstdlib>
#include <cstring>

using namespace llvm;

// C++ Helper functions for LLVM coroutines
namespace {

struct CompilerState {
  llvm::Module *module;
  llvm::LLVMContext &context;
  llvm::IRBuilder<> builder;

  CompilerState(llvm::Module *mod, llvm::LLVMContext &ctx)
      : module(mod), context(ctx), builder(ctx) {}
};

llvm::Module *unwrap_module(LLVMModuleRef mod) { return llvm::unwrap(mod); }

llvm::LLVMContext &get_context_from_module(LLVMModuleRef mod) {
  return llvm::unwrap(mod)->getContext();
}

llvm::IRBuilder<> *create_builder(LLVMBuilderRef builder_ref) {
  auto *bb = llvm::unwrap(builder_ref)->GetInsertBlock();
  if (!bb)
    return nullptr;

  return new llvm::IRBuilder<>(bb->getContext());
}
} // namespace
//
typedef struct CoroutineCompilationCtx {
  llvm::Function *current_function;
  llvm::BasicBlock *cleanup_bb;
  llvm::BasicBlock *final_suspend_bb;
  int current_yield_index;
  int total_yields;
  llvm::AllocaInst *frame_ptr;
  llvm::AllocaInst *promise_ptr;
  llvm::Value *coro_handle;
  llvm::Value *coro_id_val;
  std::vector<llvm::BasicBlock *> yield_blocks;
  std::vector<llvm::Value *> yielded_values;
  llvm::Type *yield_type;

  // LLVM coroutine intrinsics
  llvm::Function *coro_id;
  llvm::Function *coro_size;
  llvm::Function *coro_begin;
  llvm::Function *coro_suspend;
  llvm::Function *coro_end;
  llvm::Function *coro_free;
  llvm::Function *coro_destroy;
  llvm::Function *coro_done;
  llvm::Function *coro_promise;
  llvm::Function *coro_resume;
} CoroutineCompilationCtx;

llvm::Type *cpp_type(YLCType *t, JITLangCtx *ctx, llvm::Module *module) {
  LLVMTypeRef tref = type_to_llvm_type(t, ctx, llvm::wrap(module));
  return llvm::unwrap(tref);
}

void setup_coroutine_intrinsics(CoroutineCompilationCtx *ctx,
                                llvm::Module *module, llvm::Type *yield_type) {
  using namespace llvm;

  ctx->coro_id = Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_id);
  ctx->coro_size = Intrinsic::getOrInsertDeclaration(
      module, Intrinsic::coro_size, {yield_type});
  ctx->coro_begin =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_begin);
  ctx->coro_suspend =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_suspend);
  ctx->coro_end =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_end);
  ctx->coro_free =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_free);
  ctx->coro_destroy =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_destroy);
  ctx->coro_done =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_done);
  ctx->coro_promise =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_promise);
  ctx->coro_resume =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_resume);
}
#define LLVM_OPQ_PTR_TYPE(context) llvm::PointerType::getUnqual(context)
#define LLVM_NULLPTR(context)                                                  \
  ConstantPointerNull::get(LLVM_OPQ_PTR_TYPE(context))

// Create the coroutine frame structure
void setup_coroutine_frame(CoroutineCompilationCtx *ctx,
                           llvm::IRBuilder<> &builder, llvm::Type *yield_type,
                           llvm::LLVMContext &context) {}
llvm::Function *
compile_llvm_coroutine(Ast *ast, JITLangCtx *ctx, llvm::Module *module,
                       llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
                       const char *func_name = "generated_coroutine") {

  YLCType *coroutine_type = (YLCType *)ast->md;
  YLCType *cor_yield_opt_type = fn_return_type(coroutine_type);
  YLCType *cor_yield_type = type_of_option(cor_yield_opt_type);

  auto *prev_bb = builder.GetInsertBlock();

  FunctionType *func_type = FunctionType::get(
      llvm::PointerType::getUnqual(context), // Returns coroutine handle
      false                                  // not vararg
  );

  Function *cor_func =
      Function::Create(func_type, Function::ExternalLinkage, func_name, module);

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  BasicBlock *entry_bb = BasicBlock::Create(context, "entry", cor_func);

  auto yield_t = cpp_type(cor_yield_type, ctx, module);
  CoroutineCompilationCtx coro_ctx = {
      .current_function = cor_func,
      .current_yield_index = 0,
      .total_yields = ast->data.AST_LAMBDA.num_yields,
      .yield_type = yield_t,
  };

  setup_coroutine_intrinsics(&coro_ctx, module, yield_t);
  setup_coroutine_frame(&coro_ctx, builder, yield_t, context);

  for (int i = 0; i < coro_ctx.total_yields; i++) {
    BasicBlock *yield_bb =
        BasicBlock::Create(context, "yield." + std::to_string(i), cor_func);

    coro_ctx.yield_blocks.push_back(yield_bb);
  }
  coro_ctx.final_suspend_bb =
      BasicBlock::Create(context, "final.suspend", cor_func);
  coro_ctx.cleanup_bb = BasicBlock::Create(context, "cleanup", cor_func);

  fn_ctx.coroutine_ctx = &coro_ctx;
  builder.SetInsertPoint(entry_bb);

  Value *null_ptr = LLVM_NULLPTR(context);
  Value *null_func = LLVM_NULLPTR(context);

  std::vector<Value *> id_args = {
      ConstantInt::get(llvm::Type::getInt32Ty(context), 0), // align
      null_ptr,                                             // promise
      null_func,                                            // corofunc
      null_ptr                                              // funcs
  };

  coro_ctx.coro_id_val =
      builder.CreateCall(coro_ctx.coro_id, id_args, "coro.id");
  Value *coro_size_val =
      builder.CreateCall(coro_ctx.coro_size, {}, "coro.size");

  Function *malloc_func = cast<Function>(
      module
          ->getOrInsertFunction("malloc", LLVM_OPQ_PTR_TYPE(context),
                                llvm::Type::getInt64Ty(context))
          .getCallee());

  Value *frame_mem =
      builder.CreateCall(malloc_func, {coro_size_val}, "coro.mem");

  coro_ctx.coro_handle = builder.CreateCall(
      coro_ctx.coro_begin, {coro_ctx.coro_id_val, frame_mem}, "coro.handle");

  coro_ctx.promise_ptr = builder.CreateAlloca(yield_t, nullptr, "coro.promise");

  Value *initial_suspend = builder.CreateCall(
      coro_ctx.coro_suspend,
      {ConstantTokenNone::get(context),
       ConstantInt::get(llvm::Type::getInt1Ty(context), false)},
      "initial.suspend");

  SwitchInst *initial_switch =
      builder.CreateSwitch(initial_suspend, coro_ctx.cleanup_bb, 2);
  initial_switch->addCase(ConstantInt::get(llvm::Type::getInt8Ty(context), 0),
                          coro_ctx.yield_blocks[0]);
  initial_switch->addCase(ConstantInt::get(llvm::Type::getInt8Ty(context), 1),
                          coro_ctx.cleanup_bb);
  builder.SetInsertPoint(coro_ctx.yield_blocks[0]);

  codegen_lambda_body(ast, &fn_ctx, llvm::wrap(module), llvm::wrap(&builder));
  builder.SetInsertPoint(coro_ctx.final_suspend_bb);

  // Final suspend (with suspend=true for permanent suspension)
  Value *final_suspend = builder.CreateCall(
      coro_ctx.coro_suspend,
      {coro_ctx.coro_id_val,
       ConstantInt::get(llvm::Type::getInt1Ty(context), true)},
      "final.suspend");

  builder.CreateUnreachable();

  builder.SetInsertPoint(coro_ctx.cleanup_bb);

  Value *coro_free_mem = builder.CreateCall(
      coro_ctx.coro_free, {coro_ctx.coro_id_val, coro_ctx.coro_handle},
      "coro.free.mem");

  Function *free_func = cast<Function>(
      module
          ->getOrInsertFunction("free", llvm::Type::getVoidTy(context),
                                LLVM_OPQ_PTR_TYPE(context))
          .getCallee());

  builder.CreateCall(free_func, {coro_free_mem});

  // End coroutine
  Value *coro_end = builder.CreateCall(
      coro_ctx.coro_end,
      {coro_ctx.coro_handle,
       ConstantInt::get(llvm::Type::getInt1Ty(context), false),
       coro_ctx.coro_id_val},
      "coro.end");

  builder.CreateRet(coro_ctx.coro_handle);

  builder.SetInsertPoint(prev_bb);
  destroy_ctx(&fn_ctx);

  return cor_func;
}

llvm::Value *compile_yield(Ast *ast, JITLangCtx *ctx, llvm::Module *module,
                           llvm::LLVMContext &context,
                           llvm::IRBuilder<> &builder) {

  auto val = codegen(ast->data.AST_YIELD.expr, ctx, llvm::wrap(module),
                     llvm::wrap(&builder));
  llvm::Value *yield_value = llvm::unwrap(val);

  CoroutineCompilationCtx *coro_ctx =
      (CoroutineCompilationCtx *)ctx->coroutine_ctx;

  builder.CreateStore(yield_value, coro_ctx->promise_ptr);

  Value *suspend_result = builder.CreateCall(
      coro_ctx->coro_suspend,
      {coro_ctx->coro_id_val,
       ConstantInt::get(llvm::Type::getInt1Ty(context), false)},
      "suspend");

  int next_yield_index = coro_ctx->current_yield_index + 1;
  BasicBlock *next_block;

  if (next_yield_index < coro_ctx->total_yields) {
    next_block = coro_ctx->yield_blocks[next_yield_index];
  } else {
    next_block = coro_ctx->final_suspend_bb;
  }

  SwitchInst *suspend_switch =
      builder.CreateSwitch(suspend_result, coro_ctx->cleanup_bb, 2);
  suspend_switch->addCase(ConstantInt::get(llvm::Type::getInt8Ty(context), 0),
                          next_block);
  suspend_switch->addCase(ConstantInt::get(llvm::Type::getInt8Ty(context), 1),
                          coro_ctx->cleanup_bb);

  coro_ctx->current_yield_index++;

  if (next_yield_index < coro_ctx->total_yields) {
    builder.SetInsertPoint(next_block);
  }

  return yield_value;
}

extern "C" {

LLVMValueRef compile_coroutine_expression(Ast *ast, JITLangCtx *ctx,
                                          LLVMModuleRef mod_ref,
                                          LLVMBuilderRef builder) {

  if (is_generic((YLCType *)ast->md)) {
    fprintf(stderr,
            "Error - generic coroutine compilation not yet implemented\n");
    return NULL;
  }

  auto *module = unwrap_module(mod_ref);
  auto &context = get_context_from_module(mod_ref);

  auto *llvm_func =
      compile_llvm_coroutine(ast, ctx, module, context, *llvm::unwrap(builder),
                             (ast->data.AST_LAMBDA.fn_name.chars != NULL)
                                 ? ast->data.AST_LAMBDA.fn_name.chars
                                 : "anonymous_coroutine");

  return llvm::wrap(llvm_func);
}

LLVMValueRef create_coroutine_instance_from_constructor(
    YLCSym *sym, Ast *args, int args_len, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder) {

  LLVMValueRef coroutine_func = sym->val;

  LLVMTypeRef llvm_func_type;

  if (args_len == 0 || (args_len == 1 && args->tag == AST_VOID)) {
    llvm_func_type = LLVMFunctionType(GENERIC_PTR, NULL, 0, 0);
    // Simple case: no arguments to coroutine
    //
    LLVMValueRef coro_instance =
        LLVMBuildCall2(builder, llvm_func_type,
                       coroutine_func, // function to call
                       NULL,           // no arguments
                       0,              // arg count
                       "coro.instance");
    return coro_instance;
  }
  LLVMValueRef compiled_args[args_len];
  for (int i = 0; i < args_len; i++) {
    compiled_args[i] = codegen(&args[i], ctx, module, builder);
  }

  LLVMValueRef coro_instance =
      LLVMBuildCall2(builder, LLVMGetElementType(LLVMTypeOf(coroutine_func)),
                     coroutine_func, compiled_args, args_len, "coro.instance");

  return coro_instance;
}

LLVMValueRef create_coroutine_instance_from_generic_constructor(
    YLCSym *sym, YLCType *expected_type, Ast *args, int args_len,
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder) {
  return NULL;
}

static LLVMValueRef wrap_yield_result_in_option(LLVMValueRef coro_handle,
                                                LLVMTypeRef ret_val_type,
                                                LLVMBuilderRef builder,
                                                LLVMModuleRef module) {

  LLVMContextRef context = LLVMGetModuleContext(module);

  // 1. Get or declare llvm.coro.resume
  LLVMValueRef coro_resume_func =
      LLVMGetNamedFunction(module, "llvm.coro.resume");
  if (!coro_resume_func) {
    LLVMTypeRef resume_func_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context),
        (LLVMTypeRef[]){LLVMPointerTypeInContext(context, 0)}, 1, 0);
    coro_resume_func =
        LLVMAddFunction(module, "llvm.coro.resume", resume_func_type);
  }

  // Resume the coroutine
  LLVMBuildCall2(builder, LLVMGetElementType(LLVMTypeOf(coro_resume_func)),
                 coro_resume_func, (LLVMValueRef[]){coro_handle}, 1, "");

  // 2. Get or declare llvm.coro.done
  LLVMValueRef coro_done_func = LLVMGetNamedFunction(module, "llvm.coro.done");
  if (!coro_done_func) {
    LLVMTypeRef done_func_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context),
        (LLVMTypeRef[]){LLVMPointerTypeInContext(context, 0)}, 1, 0);
    coro_done_func = LLVMAddFunction(module, "llvm.coro.done", done_func_type);
  }

  // Check if finished
  LLVMValueRef is_finished = LLVMBuildCall2(
      builder, LLVMGetElementType(LLVMTypeOf(coro_done_func)), coro_done_func,
      (LLVMValueRef[]){coro_handle}, 1, "is_finished");

  // 3. Get or declare llvm.coro.promise
  LLVMValueRef coro_promise_func =
      LLVMGetNamedFunction(module, "llvm.coro.promise");
  if (!coro_promise_func) {
    LLVMTypeRef promise_func_type =
        LLVMFunctionType(LLVMPointerTypeInContext(context, 0),
                         (LLVMTypeRef[]){
                             LLVMPointerTypeInContext(context, 0), // handle
                             LLVMInt32TypeInContext(context),      // alignment
                             LLVMInt1TypeInContext(context) // from_untyped
                         },
                         3, 0);
    coro_promise_func =
        LLVMAddFunction(module, "llvm.coro.promise", promise_func_type);
  }

  // Get promise value
  LLVMValueRef promise_ptr = LLVMBuildCall2(
      builder, LLVMGetElementType(LLVMTypeOf(coro_promise_func)),
      coro_promise_func,
      (LLVMValueRef[]){coro_handle,
                       LLVMConstInt(LLVMInt32TypeInContext(context), 4, 0),
                       LLVMConstInt(LLVMInt1TypeInContext(context), 0, 0)},
      3, "promise_ptr");

  // 4. Load the value from promise
  LLVMValueRef typed_promise = LLVMBuildBitCast(
      builder, promise_ptr, LLVMPointerType(ret_val_type, 0), "typed_promise");
  LLVMValueRef promise_value =
      LLVMBuildLoad2(builder, ret_val_type, typed_promise, "promise_value");

  // 5. Simple select: finished ? None : Some(promise_value)
  LLVMValueRef result = LLVMBuildSelect(
      builder, is_finished,
      codegen_none_typed(builder, ret_val_type), // None if finished
      codegen_some(promise_value, builder),      // Some(value) if not finished
      "result_option");

  return result;
}

LLVMValueRef yield_from_coroutine_instance(YLCSym *sym, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {

  YLCType *inst_type = sym->symbol_data.STYPE_FUNCTION.fn_type;
  YLCType *yield_opt_type = fn_return_type(inst_type);
  YLCType *yield_type = type_of_option(yield_opt_type);
  LLVMTypeRef ytype = type_to_llvm_type(yield_type, ctx, module);

  // Get the coroutine handle from the symbol
  LLVMValueRef coro_handle = sym->val;
  LLVMContextRef context = LLVMGetModuleContext(module);

  // 1. Get or declare llvm.coro.resume
  LLVMTypeRef resume_func_type = LLVMFunctionType(
      LLVMVoidTypeInContext(context),
      (LLVMTypeRef[]){LLVMPointerTypeInContext(context, 0)}, 1, 0);
  LLVMValueRef coro_resume_func =
      LLVMGetNamedFunction(module, "llvm.coro.resume");
  if (!coro_resume_func) {
    coro_resume_func =
        LLVMAddFunction(module, "llvm.coro.resume", resume_func_type);
  }

  // Resume the coroutine
  LLVMBuildCall2(builder, resume_func_type, coro_resume_func,
                 (LLVMValueRef[]){coro_handle}, 1, "");

  // 2. Get or declare llvm.coro.done
  LLVMTypeRef done_func_type = LLVMFunctionType(
      LLVMInt1TypeInContext(context),
      (LLVMTypeRef[]){LLVMPointerTypeInContext(context, 0)}, 1, 0);
  LLVMValueRef coro_done_func = LLVMGetNamedFunction(module, "llvm.coro.done");
  if (!coro_done_func) {
    coro_done_func = LLVMAddFunction(module, "llvm.coro.done", done_func_type);
  }

  // Check if finished
  LLVMValueRef is_finished =
      LLVMBuildCall2(builder, done_func_type, coro_done_func,
                     (LLVMValueRef[]){coro_handle}, 1, "is_finished");

  // 3. Get or declare llvm.coro.promise
  LLVMTypeRef promise_func_type =
      LLVMFunctionType(LLVMPointerTypeInContext(context, 0),
                       (LLVMTypeRef[]){
                           LLVMPointerTypeInContext(context, 0), // handle
                           LLVMInt32TypeInContext(context),      // alignment
                           LLVMInt1TypeInContext(context)        // from_untyped
                       },
                       3, 0);
  LLVMValueRef coro_promise_func =
      LLVMGetNamedFunction(module, "llvm.coro.promise");

  if (!coro_promise_func) {
    coro_promise_func =
        LLVMAddFunction(module, "llvm.coro.promise", promise_func_type);
  }

  // Get promise value
  LLVMValueRef promise_ptr = LLVMBuildCall2(
      builder, promise_func_type, coro_promise_func,
      (LLVMValueRef[]){coro_handle,
                       LLVMConstInt(LLVMInt32TypeInContext(context), 4, 0),
                       LLVMConstInt(LLVMInt1TypeInContext(context), 0, 0)},
      3, "promise_ptr");

  // 4. Load the value from promise
  LLVMValueRef typed_promise = LLVMBuildBitCast(
      builder, promise_ptr, LLVMPointerType(ytype, 0), "typed_promise");
  LLVMValueRef promise_value =
      LLVMBuildLoad2(builder, ytype, typed_promise, "promise_value");

  // 5. Simple select: finished ? None : Some(promise_value)
  LLVMValueRef result = LLVMBuildSelect(
      builder, is_finished,
      codegen_none_typed(builder, ytype),   // None if finished
      codegen_some(promise_value, builder), // Some(value) if not finished
      "result_option");

  return result;
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef mod_ref,
                           LLVMBuilderRef builder) {

  auto *module = unwrap_module(mod_ref);
  auto &context = get_context_from_module(mod_ref);

  return llvm::wrap(
      compile_yield(ast, ctx, module, context, *llvm::unwrap(builder)));
}

LLVMValueRef WrapCoroutineWithEffectHandler(Ast *ast, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef MapCoroutineHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef IterOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef IterOfArrayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef RunInSchedulerHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  return NULL;
}

int get_inner_state_slot(Ast *ast) { return -1; }

LLVMValueRef get_inner_state_slot_gep(int slot, Ast *ast,
                                      LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef CorReplaceHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef IterHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef CoroutineEndHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef UseOrFinishHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  return NULL;
}

} // extern "C"
