#include "function.h"
#include "types.h"
extern "C" {

#include "../../types/type.h"
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
  coro_ctx.cleanup_bb = BasicBlock::Create(context, "cleanup", cor_func);

  fn_ctx.coroutine_ctx = &coro_ctx;
  builder.SetInsertPoint(entry_bb);

  codegen_lambda_body(ast, &fn_ctx, llvm::wrap(module), llvm::wrap(&builder));

  builder.SetInsertPoint(prev_bb);
  destroy_ctx(&fn_ctx);

  return cor_func;
}

llvm::Value *compile_yield(Ast *ast, JITLangCtx *ctx, llvm::Module *module,
                           llvm::LLVMContext &context,
                           llvm::IRBuilder<> &builder) {

  printf("coroutine yield\n");
  print_ast(ast);
  return nullptr;
}

extern "C" {

LLVMValueRef create_coroutine_constructor_binding(Ast *binding, Ast *ast,
                                                  JITLangCtx *ctx,
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

  LLVMDumpModule(mod_ref);
  return NULL;
}

// Stub implementations for the rest of the C interface
LLVMValueRef create_coroutine_instance_from_constructor(
    YLCSym *sym, Ast *args, int args_len, JITLangCtx *ctx, LLVMModuleRef module,
    LLVMBuilderRef builder) {
  printf("hi - compiling coroutine\n");
  return NULL;
}

LLVMValueRef create_coroutine_instance_from_generic_constructor(
    YLCSym *sym, YLCType *expected_type, Ast *args, int args_len,
    JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef yield_from_coroutine_instance(YLCSym *sym, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  return NULL;
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
