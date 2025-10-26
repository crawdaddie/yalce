#include "backend_llvm/globals.h"
#include "backend_llvm/common.h"
#include "llvm-c/Core.h"

// Global variables
LLVMValueRef global_storage_array_llvm;
LLVMValueRef global_storage_size_llvm;

#define _GLOBAL_STORAGE_SIZE 1024
#define _VOID_PTR_T LLVMPointerType(LLVMInt8Type(), 0)
#define _GLOBAL_STORAGE_TYPE LLVMArrayType(_VOID_PTR_T, _GLOBAL_STORAGE_SIZE)
#define ZERO LLVMConstInt(LLVMInt32Type(), 0, 0)

LLVMValueRef get_global_storage_array(LLVMModuleRef module) {

  // Look up the global fresh each time instead of using cached reference
  // to avoid stale references after optimization passes
  LLVMValueRef storage_array =
      LLVMGetNamedGlobal(module, "global_storage_array");

  if (storage_array == NULL) {
    // printf("global_storage_array not found, recreating...\n");
    storage_array =
        LLVMAddGlobal(module, _GLOBAL_STORAGE_TYPE, "global_storage_array");
    LLVMSetLinkage(storage_array, LLVMExternalLinkage);
  }
  return storage_array;
}

void codegen_set_global(const char *sym_name, JITSymbol *sym,
                        LLVMValueRef value, Type *ttype, LLVMTypeRef llvm_type,
                        JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {

  char buf[32];
  snprintf(buf, 32, "%s_malloc", sym_name);
  LLVMValueRef malloced_space = LLVMBuildMalloc(builder, llvm_type, buf);

  LLVMBuildStore(builder, value, malloced_space);

  snprintf(buf, 32, "%s_generic_ptr", sym_name);
  LLVMValueRef generic_ptr =
      LLVMBuildBitCast(builder, malloced_space, _VOID_PTR_T, buf);
  int slot = *ctx->num_globals;
  sym->symbol_data.STYPE_TOP_LEVEL_VAR = slot;
  LLVMValueRef slot_index = LLVMConstInt(LLVMInt32Type(), slot, false);

  LLVMValueRef indices[] = {ZERO, slot_index};

  snprintf(buf, 32, "%s_slot_ptr", sym_name);
  LLVMValueRef storage_array = get_global_storage_array(module);

  LLVMValueRef slot_ptr = LLVMBuildGEP2(builder, _GLOBAL_STORAGE_TYPE,
                                        storage_array, indices, 2, buf);

  LLVMBuildStore(builder, generic_ptr, slot_ptr);

  *(ctx->num_globals) = slot + 1;
}

LLVMValueRef codegen_get_global(const char *sym_name, JITSymbol *sym,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  char buf[32];
  int slot = sym->symbol_data.STYPE_TOP_LEVEL_VAR;
  LLVMTypeRef llvm_type = sym->llvm_type;

  LLVMValueRef slot_index = LLVMConstInt(LLVMInt32Type(), slot, false);

  LLVMValueRef indices[] = {ZERO, slot_index};
  snprintf(buf, 32, "%s_slot_ptr", sym_name);

  LLVMValueRef storage_array = get_global_storage_array(module);
  LLVMValueRef slot_ptr = LLVMBuildGEP2(builder, _GLOBAL_STORAGE_TYPE,
                                        storage_array, indices, 2, buf);

  LLVMValueRef generic_ptr =
      LLVMBuildLoad2(builder, _VOID_PTR_T, slot_ptr, "void_ptr");

  LLVMValueRef typed_ptr = LLVMBuildBitCast(
      builder, generic_ptr, LLVMPointerType(llvm_type, 0), "typed_ptr");

  snprintf(buf, 32, "%s_load", sym_name);
  return LLVMBuildLoad2(builder, llvm_type, typed_ptr, buf);
}

void setup_global_storage(LLVMModuleRef module, LLVMBuilderRef builder) {
  printf("setup global storage\n");
  global_storage_array_llvm =
      LLVMAddGlobal(module, _GLOBAL_STORAGE_TYPE, "global_storage_array");
  LLVMSetLinkage(global_storage_array_llvm, LLVMExternalLinkage);

  global_storage_size_llvm =
      LLVMAddGlobal(module, LLVMInt32Type(), "global_storage_size");
}
