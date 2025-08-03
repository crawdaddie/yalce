#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <memory>

using namespace llvm;

class CoroutineIRGenerator {
private:
  LLVMContext context;
  std::unique_ptr<Module> module;
  IRBuilder<> builder;

public:
  CoroutineIRGenerator() : builder(context) {
    module = std::make_unique<Module>("coroutine_example", context);
  }

  Function *createCoroutineFunction() {
    // Create function signature: void @my_coroutine()
    FunctionType *funcType = FunctionType::get(Type::getVoidTy(context),
                                               false // not vararg
    );

    Function *coroFunc = Function::Create(funcType, Function::ExternalLinkage,
                                          "my_coroutine", module.get());

    // Create basic blocks
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", coroFunc);
    BasicBlock *loopBB = BasicBlock::Create(context, "loop", coroFunc);
    BasicBlock *suspendBB = BasicBlock::Create(context, "suspend", coroFunc);
    BasicBlock *cleanupBB = BasicBlock::Create(context, "cleanup", coroFunc);

    // Entry block
    builder.SetInsertPoint(entryBB);

    // Get coroutine intrinsics
    Function *coroId =
        Intrinsic::getDeclaration(module.get(), Intrinsic::coro_id);
    Function *coroSize = Intrinsic::getDeclaration(
        module.get(), Intrinsic::coro_size, {Type::getInt32Ty(context)});
    Function *coroBegin =
        Intrinsic::getDeclaration(module.get(), Intrinsic::coro_begin);
    Function *coroSuspend =
        Intrinsic::getDeclaration(module.get(), Intrinsic::coro_suspend);
    Function *coroEnd =
        Intrinsic::getDeclaration(module.get(), Intrinsic::coro_end);

    // Call llvm.coro.id
    Value *nullPtr = ConstantPointerNull::get(Type::getInt8PtrTy(context));
    Value *coroIdResult = builder.CreateCall(
        coroId, {
                    ConstantInt::get(Type::getInt32Ty(context), 0), // align
                    nullPtr,                                        // promise
                    nullPtr,                                        // corofn
                    nullPtr                                         // fnaddrs
                });

    // Call llvm.coro.size.i32
    Value *frameSize = builder.CreateCall(coroSize, {});

    // Allocate frame (simplified - normally check llvm.coro.alloc)
    Function *mallocFunc =
        Function::Create(FunctionType::get(Type::getInt8PtrTy(context),
                                           {Type::getInt64Ty(context)}, false),
                         Function::ExternalLinkage, "malloc", module.get());

    Value *frameSizeExt =
        builder.CreateZExt(frameSize, Type::getInt64Ty(context));
    Value *framePtr = builder.CreateCall(mallocFunc, {frameSizeExt});

    // Call llvm.coro.begin
    Value *coroHandle = builder.CreateCall(coroBegin, {coroIdResult, framePtr});

    // Create counter variable
    Value *counter = builder.CreateAlloca(Type::getInt32Ty(context));
    builder.CreateStore(ConstantInt::get(Type::getInt32Ty(context), 0),
                        counter);

    // Branch to loop
    builder.CreateBr(loopBB);

    // Loop block
    builder.SetInsertPoint(loopBB);
    Value *currentCount =
        builder.CreateLoad(Type::getInt32Ty(context), counter);
    Value *shouldContinue = builder.CreateICmpSLT(
        currentCount, ConstantInt::get(Type::getInt32Ty(context), 5));

    // Conditional branch
    builder.CreateCondBr(shouldContinue, suspendBB, cleanupBB);

    // Suspend block
    builder.SetInsertPoint(suspendBB);

    // Increment counter
    Value *nextCount = builder.CreateAdd(
        currentCount, ConstantInt::get(Type::getInt32Ty(context), 1));
    builder.CreateStore(nextCount, counter);

    // Call llvm.coro.suspend
    Value *suspendResult = builder.CreateCall(
        coroSuspend, {ConstantTokenNone::get(context),
                      ConstantInt::get(Type::getInt1Ty(context), false)});

    // Switch on suspend result
    SwitchInst *suspendSwitch = builder.CreateSwitch(suspendResult, cleanupBB);
    suspendSwitch->addCase(ConstantInt::get(Type::getInt8Ty(context), 0),
                           loopBB // resume
    );
    suspendSwitch->addCase(ConstantInt::get(Type::getInt8Ty(context), 1),
                           cleanupBB // destroy
    );

    // Cleanup block
    builder.SetInsertPoint(cleanupBB);

    // Call llvm.coro.end
    builder.CreateCall(
        coroEnd,
        {coroHandle, ConstantInt::get(Type::getInt1Ty(context), false)});

    builder.CreateRetVoid();

    return coroFunc;
  }

  void printIR() { module->print(outs(), nullptr); }

  Module *getModule() { return module.get(); }
};

int main() {
  std::cout << "=== Simple LLVM Execution Example ===" << std::endl;

  // Initialize LLVM
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();

  LLVMContext context;
  auto module = std::make_unique<Module>("simple_example", context);
  IRBuilder<> builder(context);

  // Create a simple function: int add(int a, int b) { return a + b; }
  FunctionType *funcType = FunctionType::get(
      Type::getInt32Ty(context),
      {Type::getInt32Ty(context), Type::getInt32Ty(context)}, false);

  Function *addFunc = Function::Create(funcType, Function::ExternalLinkage,
                                       "add", module.get());

  // Create function body
  BasicBlock *entryBB = BasicBlock::Create(context, "entry", addFunc);
  builder.SetInsertPoint(entryBB);

  // Get function arguments
  auto args = addFunc->arg_begin();
  Value *a = &*args++;
  Value *b = &*args;

  // Add the arguments
  Value *result = builder.CreateAdd(a, b, "result");
  builder.CreateRet(result);

  // Print the IR
  std::cout << "Generated LLVM IR:" << std::endl;
  module->print(outs(), nullptr);

  // Create execution engine
  std::string errorStr;
  std::unique_ptr<ExecutionEngine> engine(EngineBuilder(std::move(module))
                                              .setErrorStr(&errorStr)
                                              .setEngineKind(EngineKind::JIT)
                                              .create());

  if (!engine) {
    std::cerr << "Failed to create execution engine: " << errorStr << std::endl;
    return 1;
  }

  auto _args = addFunc->arg_begin();

  GenericValue res = engine->runFunction(addFunc, _args);

  std::cout << "\nExecution result: 5 + 7 = " << result->IntVal.getSExtValue()
            << std::endl;

  return 0;
}
