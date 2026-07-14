#include "./lowering.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <string.h>

LLVMValueRef lower_mir_top_level(MirFunction *top, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  printf("lower mir top\n");
  dump_function(stdout, top);
}

LLVMValueRef lower_mir(MirProgram *prog, LLVMModuleRef module,
                       LLVMBuilderRef builder) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef ret = LLVMVoidTypeInContext(llvm_ctx);
  LLVMTypeRef funcType = LLVMFunctionType(ret, NULL, 0, 0);
  LLVMValueRef top = LLVMAddFunction(module, "top", funcType);
  LLVMSetLinkage(top, LLVMExternalLinkage);
  if (top == NULL) {
    return NULL;
  }

  MirFunctionVec v = prog->functions;
  for (v = v; v.len; v = (MirFunctionVec){v.items + 1, v.len - 1, v.cap}) {
    MirFunction *f = v.items[0];
    if (strcmp(f->name, "$top") == 0) {
      LLVMBasicBlockRef block =
          LLVMAppendBasicBlockInContext(llvm_ctx, top, "entry");
      LLVMPositionBuilderAtEnd(builder, block);
      lower_mir_top_level(f, module, builder);
      LLVMBuildRetVoid(builder);
    }
  }

  return top;
}
