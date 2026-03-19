#include "./lib_registry.h"

JITLangCtx *ylc_jit_ctx = NULL;
LLVMModuleRef ylc_jit_module = NULL;
LLVMBuilderRef ylc_jit_builder = NULL;

// lang/backend_llvm/lib_registry.c
YlcRuntimeLoadFn ylc_runtime_load_fn = NULL;
