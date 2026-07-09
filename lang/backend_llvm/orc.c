#include "./orc.h"
#include "globals.h"
#include "modules.h"
#include "types/builtins.h"
#include <llvm-c/Core.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Target.h>
#include <stdint.h>
#include <stdio.h>

#define GLOBAL_STORAGE_CAPACITY 1024

int orcjit(int argc, char **argv) {

  printf("start orcjit\n");

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  LLVMOrcLLJITRef jit = NULL;
  LLVMErrorRef err = LLVMOrcCreateLLJIT(&jit, NULL);
  if (err) {
    return 1;
  }
  LLVMOrcJITDylibRef jd = LLVMOrcLLJITGetMainJITDylib(jit);

  LLVMOrcDefinitionGeneratorRef gen = NULL;
  err = LLVMOrcCreateDynamicLibrarySearchGeneratorForProcess(
      &gen, LLVMOrcLLJITGetGlobalPrefix(jit), NULL, NULL);

  if (err) {
    return 1;
  }

  LLVMOrcJITDylibAddGenerator(jd, gen);

  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("ylc.top-level", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

  LLVMSetTarget(module, LLVMOrcLLJITGetTripleString(jit));
  LLVMSetDataLayout(module, LLVMOrcLLJITGetDataLayoutStr(jit));

  void *global_storage_array[GLOBAL_STORAGE_CAPACITY];
  int global_storage_capacity = GLOBAL_STORAGE_CAPACITY;
  int num_globals = 0;

  init_module_registry();
  setup_global_storage(module, builder);

  TypeEnv *env = NULL;
  initialize_builtin_types();

  ht table;
  ht_init(&table);
  StackFrame initial_stack_frame = {.table = &table, .next = NULL};

  JITLangCtx ctx = {.stack_ptr = 0,
                    .env = env,
                    .num_globals = &num_globals,
                    .global_storage_array = global_storage_array,
                    .global_storage_capacity = &global_storage_capacity,
                    .frame = &initial_stack_frame};
  LLVMTypeRef top_type =
      LLVMFunctionType(LLVMVoidTypeInContext(context), NULL, 0, false);
  LLVMValueRef top_fn = LLVMAddFunction(module, "top", top_type);
  LLVMSetLinkage(top_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(context, top_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMBuildRetVoid(builder);

  LLVMDisposeBuilder(builder);
  LLVMDumpModule(module);
  LLVMOrcThreadSafeContextRef tsc =
      LLVMOrcCreateNewThreadSafeContextFromLLVMContext(context);

  LLVMOrcThreadSafeModuleRef tsm =
      LLVMOrcCreateNewThreadSafeModule(module, tsc);

  err = LLVMOrcLLJITAddLLVMIRModule(jit, jd, tsm);
  if (err) {
    return 1;
  }
  LLVMOrcDisposeThreadSafeContext(tsc);
  LLVMOrcExecutorAddress addr = 0;
  err = LLVMOrcLLJITLookup(jit, &addr, "top");
  if (err) {
    fprintf(stderr, "Error: top-level function not found\n");
    return 1;
  }

  typedef void (*top_fn_t)(void);
  ((top_fn_t)(uintptr_t)addr)();

  LLVMOrcDisposeLLJIT(jit);
  return 0;
}
