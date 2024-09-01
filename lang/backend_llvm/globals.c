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

void codegen_set_global(JITSymbol *sym, LLVMValueRef value, Type *ttype,
                        LLVMTypeRef llvm_type, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef malloced_space = LLVMBuildMalloc(builder, llvm_type, "");

  // Store the value in the malloced space
  LLVMBuildStore(builder, value, malloced_space);

  // Cast the malloced space to a generic void ptr for storage in global array
  LLVMValueRef generic_ptr =
      LLVMBuildBitCast(builder, malloced_space, _VOID_PTR_T, "generic_ptr");
  // Calculate the offset for the specific slot
  int slot = *ctx->num_globals;
  LLVMValueRef slot_index = LLVMConstInt(LLVMInt32Type(), slot, false);

  // Get the pointer to the correct slot in the global storage
  LLVMValueRef indices[] = {ZERO, slot_index};
  LLVMValueRef slot_ptr =
      LLVMBuildGEP2(builder, _GLOBAL_STORAGE_TYPE, global_storage_array_llvm,
                    indices, 2, "slot_ptr");

  // Store the generic_ptr into the slot
  LLVMBuildStore(builder, generic_ptr, slot_ptr);

  *sym = (JITSymbol){STYPE_TOP_LEVEL_VAR, llvm_type, value,
                     .symbol_data = {.STYPE_TOP_LEVEL_VAR = slot},
                     .symbol_type = ttype};

  *(ctx->num_globals) = slot + 1;
}

LLVMValueRef codegen_get_global(JITSymbol *sym, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  int slot = sym->symbol_data.STYPE_TOP_LEVEL_VAR;
  LLVMTypeRef llvm_type = sym->llvm_type;

  LLVMValueRef slot_index = LLVMConstInt(LLVMInt32Type(), slot, false);

  // Get the pointer to the correct slot in the global storage
  LLVMValueRef indices[] = {ZERO, slot_index};
  LLVMValueRef slot_ptr =
      LLVMBuildGEP2(builder, _GLOBAL_STORAGE_TYPE, global_storage_array_llvm,
                    indices, 2, "slot_ptr");

  // Load the void* from the slot
  LLVMValueRef generic_ptr =
      LLVMBuildLoad2(builder, _VOID_PTR_T, slot_ptr, "void_ptr");

  LLVMValueRef typed_ptr = LLVMBuildBitCast(
      builder, generic_ptr, LLVMPointerType(llvm_type, 0), "typed_ptr");
  return LLVMBuildLoad2(builder, llvm_type, typed_ptr, "loaded_value");
}

void setup_global_storage(LLVMModuleRef module, LLVMBuilderRef builder) {
  global_storage_array_llvm =
      LLVMAddGlobal(module, _GLOBAL_STORAGE_TYPE, "global_storage_array");
  LLVMSetLinkage(global_storage_array_llvm, LLVMExternalLinkage);

  // Create global variable for the size
  global_storage_size_llvm =
      LLVMAddGlobal(module, LLVMInt32Type(), "global_storage_size");
}
