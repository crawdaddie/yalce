extern "C" {

#include "coroutines.h"
#include "../../types/type.h"
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
#include "llvm/Transforms/Coroutines/CoroSplit.h"
#include <cstdlib>
#include <cstring>

using namespace llvm;

// C++ Helper functions for LLVM coroutines
namespace {
// Compiler state for C++ LLVM operations
struct CompilerState {
  llvm::Module *module;
  llvm::LLVMContext &context;
  llvm::IRBuilder<> builder;

  CompilerState(llvm::Module *mod, llvm::LLVMContext &ctx)
      : module(mod), context(ctx), builder(ctx) {}
};

// Convert C LLVM types to C++ LLVM types
llvm::Module *unwrap_module(LLVMModuleRef mod) { return llvm::unwrap(mod); }

llvm::LLVMContext &get_context_from_module(LLVMModuleRef mod) {
  return llvm::unwrap(mod)->getContext();
}

llvm::IRBuilder<> *create_builder(LLVMBuilderRef builder_ref) {
  // Get the context from the builder's current insertion point
  auto *bb = llvm::unwrap(builder_ref)->GetInsertBlock();
  if (!bb)
    return nullptr;

  return new llvm::IRBuilder<>(bb->getContext());
}

// Create a CompilerState from C LLVM references
std::unique_ptr<CompilerState>
create_compiler_state(LLVMModuleRef mod_ref, LLVMBuilderRef builder_ref) {
  auto *module = unwrap_module(mod_ref);
  auto &context = get_context_from_module(mod_ref);

  auto state = std::make_unique<CompilerState>(module, context);

  // If we have a valid builder reference, position our builder at the same
  // location
  if (builder_ref) {
    auto *bb = llvm::unwrap(builder_ref)->GetInsertBlock();
    if (bb) {
      state->builder.SetInsertPoint(bb);
    }
  }

  return state;
}
} // namespace

// C++ implementation of LLVM-based coroutines
llvm::Function *
compile_llvm_coroutine_basic(llvm::Module *module, llvm::LLVMContext &context,
                             llvm::IRBuilder<> &builder,
                             const char *func_name = "generated_coroutine") {
  // Create function signature: void @coroutine()
  FunctionType *funcType =
      FunctionType::get(llvm::Type::getVoidTy(context), false);

  Function *coroFunc =
      Function::Create(funcType, Function::ExternalLinkage, func_name, module);

  // Create basic blocks
  BasicBlock *entryBB = BasicBlock::Create(context, "entry", coroFunc);
  BasicBlock *loopBB = BasicBlock::Create(context, "loop", coroFunc);
  BasicBlock *suspendBB = BasicBlock::Create(context, "suspend", coroFunc);
  BasicBlock *cleanupBB = BasicBlock::Create(context, "cleanup", coroFunc);

  // Entry block
  builder.SetInsertPoint(entryBB);

  // Get coroutine intrinsics
  Function *coroId = Intrinsic::getDeclaration(module, Intrinsic::coro_id);
  Function *coroSize = Intrinsic::getDeclaration(
      module, Intrinsic::coro_size, {llvm::Type::getInt32Ty(context)});
  Function *coroBegin =
      Intrinsic::getDeclaration(module, Intrinsic::coro_begin);
  Function *coroSuspend =
      Intrinsic::getDeclaration(module, Intrinsic::coro_suspend);
  Function *coroEnd = Intrinsic::getDeclaration(module, Intrinsic::coro_end);

  // Call llvm.coro.id
  Value *nullPtr = ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context));
  Value *coroIdResult = builder.CreateCall(
      coroId, {
                  ConstantInt::get(llvm::Type::getInt32Ty(context), 0), // align
                  nullPtr, // promise
                  nullPtr, // corofn
                  nullPtr  // fnaddrs
              });

  // Call llvm.coro.size.i32
  Value *frameSize = builder.CreateCall(coroSize, {});

  // Allocate frame (simplified - normally check llvm.coro.alloc)
  Function *mallocFunc = Function::Create(
      FunctionType::get(llvm::Type::getInt8PtrTy(context),
                        {llvm::Type::getInt64Ty(context)}, false),
      Function::ExternalLinkage, "malloc", module);

  Value *frameSizeExt =
      builder.CreateZExt(frameSize, llvm::Type::getInt64Ty(context));
  Value *framePtr = builder.CreateCall(mallocFunc, {frameSizeExt});

  // Call llvm.coro.begin
  Value *coroHandle = builder.CreateCall(coroBegin, {coroIdResult, framePtr});

  // Create counter variable
  Value *counter = builder.CreateAlloca(llvm::Type::getInt32Ty(context));
  builder.CreateStore(ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
                      counter);

  // Branch to loop
  builder.CreateBr(loopBB);

  // Loop block
  builder.SetInsertPoint(loopBB);
  Value *currentCount =
      builder.CreateLoad(llvm::Type::getInt32Ty(context), counter);
  Value *shouldContinue = builder.CreateICmpSLT(
      currentCount, ConstantInt::get(llvm::Type::getInt32Ty(context), 5));

  // Conditional branch
  builder.CreateCondBr(shouldContinue, suspendBB, cleanupBB);

  // Suspend block
  builder.SetInsertPoint(suspendBB);

  // Increment counter
  Value *nextCount = builder.CreateAdd(
      currentCount, ConstantInt::get(llvm::Type::getInt32Ty(context), 1));
  builder.CreateStore(nextCount, counter);

  // Call llvm.coro.suspend
  Value *suspendResult = builder.CreateCall(
      coroSuspend, {ConstantTokenNone::get(context),
                    ConstantInt::get(llvm::Type::getInt1Ty(context), false)});

  // Switch on suspend result
  SwitchInst *suspendSwitch = builder.CreateSwitch(suspendResult, cleanupBB);
  suspendSwitch->addCase(ConstantInt::get(llvm::Type::getInt8Ty(context), 0),
                         loopBB); // resume
  suspendSwitch->addCase(ConstantInt::get(llvm::Type::getInt8Ty(context), 1),
                         cleanupBB); // destroy

  // Cleanup block
  builder.SetInsertPoint(cleanupBB);

  // Call llvm.coro.end
  builder.CreateCall(
      coroEnd,
      {coroHandle, ConstantInt::get(llvm::Type::getInt1Ty(context), false)});

  builder.CreateRetVoid();

  return coroFunc;
}

llvm::Function *
compile_llvm_coroutine(Ast *ast, JITLangCtx *ctx, llvm::Module *module,
                       llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
                       const char *func_name = "generated_coroutine") {

  auto *prev_bb = builder.GetInsertBlock();

  FunctionType *funcType = FunctionType::get(
      llvm::Type::getInt8PtrTy(context), // Returns coroutine handle
      false                              // not vararg
  );

  Function *cor_func =
      Function::Create(funcType, Function::ExternalLinkage, func_name, module);

  BasicBlock *entry_bb = BasicBlock::Create(context, "entry", cor_func);
  BasicBlock *cleanup_bb = BasicBlock::Create(context, "cleanup", cor_func);

  Function *coro_id = Intrinsic::getDeclaration(module, Intrinsic::coro_id);
  Function *coro_size = Intrinsic::getDeclaration(
      module, Intrinsic::coro_size, {llvm::Type::getInt32Ty(context)});
  Function *coro_begin =
      Intrinsic::getDeclaration(module, Intrinsic::coro_begin);
  Function *coro_suspend =
      Intrinsic::getDeclaration(module, Intrinsic::coro_suspend);
  Function *coro_end = Intrinsic::getDeclaration(module, Intrinsic::coro_end);

  builder.SetInsertPoint(entry_bb);
  builder.SetInsertPoint(prev_bb);
  return cor_func;
}

extern "C" {
// C wrapper function to run coroutine passes on a module
void run_coroutine_passes_on_module(LLVMModuleRef module_ref) {
  auto *module = llvm::unwrap(module_ref);

  // For LLVM 16, we need to create pass instances directly
  llvm::legacy::PassManager PM;

  // Add target library info (required for some passes)
  llvm::TargetLibraryInfoImpl TLII;
  PM.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

  // Create coroutine pass instances directly
  PM.add(llvm::CoroEarlyPass());
  PM.add(llvm::CoroSplitPass());
  PM.add(llvm::CoroElidePass());
  PM.add(llvm::CoroCleanupPass());

  // Run the passes
  PM.run(*module);

  printf("Coroutine passes completed using legacy pass manager\n");
}

// Alternative: run passes on a specific function
// void run_coroutine_passes_on_function(LLVMValueRef func_ref) {
//   auto *func = llvm::unwrap<llvm::Function>(func_ref);
//   auto *module = func->getParent();
//
//   // Create function pass manager
//   llvm::legacy::FunctionPassManager fpm(module);
//
//   // Add coroutine passes that work on functions
//   fpm.add(llvm::createCoroEarlyLegacyPass());
//
//   // Initialize and run
//   fpm.doInitialization();
//   fpm.run(*func);
//   fpm.doFinalization();
//
//   // Then run module-level passes
//   llvm::legacy::PassManager pm;
//   pm.add(llvm::createCoroSplitLegacyPass());
//   pm.add(llvm::createCoroElidePass());
//   pm.add(llvm::createCoroCleanupLegacyPass());
//   pm.run(*module);
//
//   printf("Coroutine passes completed on function: %s\n",
//          func->getName().str().c_str());
// }
LLVMTypeRef cor_inst_struct_type() {
  // typedef struct cor {
  //   int counter;
  //   CoroutineFn fn_ptr;
  //   struct cor *next;
  //   void *argv;
  // } cor;
  LLVMTypeRef types[] = {
      LLVMInt32Type(), // counter @ 0
      GENERIC_PTR,     // fn ptr @ 1
      GENERIC_PTR,     // next ptr
      GENERIC_PTR,     // void *argv - state
      LLVMInt8Type(),  // SIGNAL enum
  };
  LLVMTypeRef instance_struct_type = LLVMStructType(types, 5, 0);
  return instance_struct_type;
}

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
