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

std::unique_ptr<CompilerState>
create_compiler_state(LLVMModuleRef mod_ref, LLVMBuilderRef builder_ref) {
  auto *module = unwrap_module(mod_ref);
  auto &context = get_context_from_module(mod_ref);

  auto state = std::make_unique<CompilerState>(module, context);

  if (builder_ref) {
    auto *bb = llvm::unwrap(builder_ref)->GetInsertBlock();
    if (bb) {
      state->builder.SetInsertPoint(bb);
    }
  }

  return state;
}
} // namespace
llvm::Type *cpp_type(YLCType *t, JITLangCtx *ctx, llvm::Module *module) {
  LLVMTypeRef tref = type_to_llvm_type(t, ctx, llvm::wrap(module));
  return llvm::unwrap(tref);
}

llvm::Function *
compile_llvm_coroutine(Ast *ast, JITLangCtx *ctx, llvm::Module *module,
                       llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
                       const char *func_name = "generated_coroutine") {
  print_ast(ast);
  YLCType *coroutine_type = (YLCType *)ast->md;
  YLCType *cor_yield_opt_type = fn_return_type(coroutine_type);
  YLCType *cor_yield_type = type_of_option(cor_yield_opt_type);
  print_type(cor_yield_type);

  auto *prev_bb = builder.GetInsertBlock();

  FunctionType *funcType = FunctionType::get(
      llvm::PointerType::getUnqual(context), // Returns coroutine handle
      false                                  // not vararg
  );

  Function *cor_func =
      Function::Create(funcType, Function::ExternalLinkage, func_name, module);

  BasicBlock *entry_bb = BasicBlock::Create(context, "entry", cor_func);
  BasicBlock *cleanup_bb = BasicBlock::Create(context, "cleanup", cor_func);

  Function *coro_id =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_id);

  Function *coro_size = Intrinsic::getOrInsertDeclaration(
      module, Intrinsic::coro_size, {cpp_type(cor_yield_type, ctx, module)});
  Function *coro_begin =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_begin);
  Function *coro_suspend =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_suspend);
  Function *coro_end =
      Intrinsic::getOrInsertDeclaration(module, Intrinsic::coro_end);

  builder.SetInsertPoint(entry_bb);
  builder.SetInsertPoint(prev_bb);
  return cor_func;
}

extern "C" {

LLVMValueRef create_coroutine_constructor_binding(Ast *binding, Ast *ast,
                                                  JITLangCtx *ctx,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder) {

  if (is_generic((YLCType *)ast->md)) {
    fprintf(stderr,
            "Error - generic coroutine compilation not yet implemented\n");
    return NULL;
  }

  auto state = create_compiler_state(module, builder);
  auto *llvm_func = compile_llvm_coroutine(ast, ctx, state->module,
                                           state->context, state->builder);

  LLVMDumpModule(module);
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

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  return NULL;
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
