#include "./lib_registry.h"
#include <llvm-c/BitReader.h>
#include <llvm-c/Core.h>
#include <llvm-c/Linker.h>
#include <stddef.h>
#include <stdio.h>

JITLangCtx *ylc_jit_ctx = NULL;
LLVMModuleRef ylc_jit_module = NULL;
LLVMBuilderRef ylc_jit_builder = NULL;
MirProgram *ylc_mir_program = NULL;
MirCtx *ylc_mir_ctx = NULL;

// lang/backend_llvm/lib_registry.c
YlcRuntimeLoadFn ylc_runtime_load_fn = NULL;

bool ylc_link_llvm_bitcode_file(LLVMModuleRef module, const char *path) {
  if (!module || !path || path[0] == '\0') {
    return false;
  }

  LLVMMemoryBufferRef buffer = NULL;
  char *error = NULL;
  if (LLVMCreateMemoryBufferWithContentsOfFile(path, &buffer, &error)) {
    fprintf(stderr, "failed to read LLVM bitcode '%s': %s\n", path,
            error ? error : "unknown error");
    if (error) {
      LLVMDisposeMessage(error);
    }
    return false;
  }

  LLVMModuleRef bitcode_module = NULL;
  if (LLVMParseBitcodeInContext2(LLVMGetModuleContext(module), buffer,
                                 &bitcode_module)) {
    fprintf(stderr, "failed to parse LLVM bitcode '%s'\n", path);
    LLVMDisposeMemoryBuffer(buffer);
    return false;
  }
  LLVMDisposeMemoryBuffer(buffer);

  const char *target = LLVMGetTarget(module);
  if (target && target[0] != '\0') {
    LLVMSetTarget(bitcode_module, target);
  }

  const char *data_layout = LLVMGetDataLayoutStr(module);
  if (data_layout && data_layout[0] != '\0') {
    LLVMSetDataLayout(bitcode_module, data_layout);
  }

  if (LLVMLinkModules2(module, bitcode_module)) {
    fprintf(stderr, "failed to link LLVM bitcode '%s'\n", path);
    return false;
  }

  return true;
}
