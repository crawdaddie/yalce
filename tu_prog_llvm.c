#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("variant_ctx", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

  // Create types
  LLVMTypeRef i32Type = LLVMInt32TypeInContext(context);
  LLVMTypeRef i64Type = LLVMInt64TypeInContext(context);
  LLVMTypeRef doubleType = LLVMDoubleTypeInContext(context);

  // Create union type
  LLVMTypeRef unionTypes[] = {doubleType}; // We only need the largest type
  LLVMTypeRef unionType = LLVMStructCreateNamed(context, "anon");
  LLVMStructSetBody(unionType, unionTypes, 1, 0);

  // Create TU struct type
  LLVMTypeRef tuTypes[] = {i32Type, unionType};
  LLVMTypeRef tuType = LLVMStructCreateNamed(context, "TU");
  LLVMStructSetBody(tuType, tuTypes, 2, 0);

  // Create proc_tu function
  LLVMTypeRef procTuParamTypes[] = {LLVMArrayType(i64Type, 2)};
  LLVMTypeRef procTuFuncType =
      LLVMFunctionType(i32Type, procTuParamTypes, 1, 0);
  LLVMValueRef procTuFunc = LLVMAddFunction(module, "proc_tu", procTuFuncType);

  // Build proc_tu function body
  LLVMBasicBlockRef entryBlock =
      LLVMAppendBasicBlockInContext(context, procTuFunc, "entry");
  LLVMPositionBuilderAtEnd(builder, entryBlock);

  LLVMValueRef tuParam = LLVMGetParam(procTuFunc, 0);
  LLVMValueRef tuAlloca = LLVMBuildAlloca(builder, tuType, "tu");
  LLVMBuildStore(builder, tuParam, tuAlloca);

  LLVMValueRef tagPtr =
      LLVMBuildStructGEP2(builder, tuType, tuAlloca, 0, "tagPtr");
  LLVMValueRef tag = LLVMBuildLoad2(builder, i32Type, tagPtr, "tag");

  LLVMBasicBlockRef switchDefault =
      LLVMAppendBasicBlockInContext(context, procTuFunc, "switch.default");
  LLVMValueRef switchInst = LLVMBuildSwitch(builder, tag, switchDefault, 3);

  // Case A
  LLVMBasicBlockRef caseA =
      LLVMAppendBasicBlockInContext(context, procTuFunc, "case.A");
  LLVMAddCase(switchInst, LLVMConstInt(i32Type, 0, 0), caseA);
  LLVMPositionBuilderAtEnd(builder, caseA);

  LLVMValueRef valuePtr =
      LLVMBuildStructGEP2(builder, tuType, tuAlloca, 1, "valuePtr");
  LLVMValueRef aValue = LLVMBuildLoad2(builder, i32Type, valuePtr, "aValue");
  LLVMValueRef cmp = LLVMBuildICmp(builder, LLVMIntEQ, aValue,
                                   LLVMConstInt(i32Type, 3, 0), "cmp");
  LLVMBasicBlockRef thenBlock =
      LLVMAppendBasicBlockInContext(context, procTuFunc, "then");
  LLVMBasicBlockRef elseBlock =
      LLVMAppendBasicBlockInContext(context, procTuFunc, "else");
  LLVMBuildCondBr(builder, cmp, thenBlock, elseBlock);

  LLVMPositionBuilderAtEnd(builder, thenBlock);
  LLVMBuildRet(builder, LLVMConstInt(i32Type, 4, 0));

  LLVMPositionBuilderAtEnd(builder, elseBlock);
  LLVMBuildRet(builder, LLVMConstInt(i32Type, 1, 0));

  // Case B
  LLVMBasicBlockRef caseB =
      LLVMAppendBasicBlockInContext(context, procTuFunc, "case.B");
  LLVMAddCase(switchInst, LLVMConstInt(i32Type, 1, 0), caseB);
  LLVMPositionBuilderAtEnd(builder, caseB);
  LLVMBuildRet(builder, LLVMConstInt(i32Type, 2, 0));

  // Case C
  LLVMBasicBlockRef caseC =
      LLVMAppendBasicBlockInContext(context, procTuFunc, "case.C");
  LLVMAddCase(switchInst, LLVMConstInt(i32Type, 2, 0), caseC);
  LLVMPositionBuilderAtEnd(builder, caseC);
  LLVMBuildRet(builder, LLVMConstInt(i32Type, 3, 0));

  // Default case
  LLVMPositionBuilderAtEnd(builder, switchDefault);
  LLVMBuildUnreachable(builder);

  // Create main function
  LLVMTypeRef mainFuncType = LLVMFunctionType(i32Type, NULL, 0, 0);
  LLVMValueRef mainFunc = LLVMAddFunction(module, "main", mainFuncType);

  LLVMBasicBlockRef mainEntry =
      LLVMAppendBasicBlockInContext(context, mainFunc, "entry");
  LLVMPositionBuilderAtEnd(builder, mainEntry);

  // Create t1, t2, t3, t4
  LLVMValueRef t1 = LLVMBuildAlloca(builder, tuType, "t1");
  LLVMValueRef t2 = LLVMBuildAlloca(builder, tuType, "t2");
  LLVMValueRef t3 = LLVMBuildAlloca(builder, tuType, "t3");
  LLVMValueRef t4 = LLVMBuildAlloca(builder, tuType, "t4");

  // Initialize t1
  LLVMBuildStore(builder, LLVMConstInt(i32Type, 0, 0),
                 LLVMBuildStructGEP2(builder, tuType, t1, 0, ""));
  LLVMBuildStore(builder, LLVMConstInt(i32Type, 1, 0),
                 LLVMBuildStructGEP2(builder, tuType, t1, 1, ""));

  // Initialize t2
  LLVMBuildStore(builder, LLVMConstInt(i32Type, 0, 0),
                 LLVMBuildStructGEP2(builder, tuType, t2, 0, ""));
  LLVMBuildStore(builder, LLVMConstInt(i32Type, 3, 0),
                 LLVMBuildStructGEP2(builder, tuType, t2, 1, ""));

  // Initialize t3
  LLVMBuildStore(builder, LLVMConstInt(i32Type, 1, 0),
                 LLVMBuildStructGEP2(builder, tuType, t3, 0, ""));
  LLVMBuildStore(builder, LLVMConstReal(doubleType, 2.0),
                 LLVMBuildStructGEP2(builder, tuType, t3, 1, ""));

  // Initialize t4
  LLVMBuildStore(builder, LLVMConstInt(i32Type, 2, 0),
                 LLVMBuildStructGEP2(builder, tuType, t4, 0, ""));
  LLVMBuildStore(builder, LLVMConstInt(i64Type, 200, 0),
                 LLVMBuildStructGEP2(builder, tuType, t4, 1, ""));

  // Call proc_tu for each t1, t2, t3, t4
  LLVMBuildCall2(builder, procTuFuncType, procTuFunc, &t1, 1, "");
  LLVMBuildCall2(builder, procTuFuncType, procTuFunc, &t2, 1, "");
  LLVMBuildCall2(builder, procTuFuncType, procTuFunc, &t3, 1, "");
  LLVMBuildCall2(builder, procTuFuncType, procTuFunc, &t4, 1, "");

  LLVMBuildRet(builder, LLVMConstInt(i32Type, 0, 0));

  // Verify the module
  char *error = NULL;
  LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
  LLVMDisposeMessage(error);

  // Print the generated IR
  char *ir = LLVMPrintModuleToString(module);
  printf("%s\n", ir);
  LLVMDisposeMessage(ir);

  // Clean up
  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(module);
  LLVMContextDispose(context);

  return 0;
}
