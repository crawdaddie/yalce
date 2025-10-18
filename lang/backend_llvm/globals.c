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
  LLVMValueRef slot_ptr =
      LLVMBuildGEP2(builder, _GLOBAL_STORAGE_TYPE, global_storage_array_llvm,
                    indices, 2, buf);

  LLVMBuildStore(builder, generic_ptr, slot_ptr);

  *(ctx->num_globals) = slot + 1;
}

int codegen_set_global_closure_storage(LLVMValueRef rec_storage,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  LLVMValueRef malloced_space =
      LLVMBuildMalloc(builder, GENERIC_PTR, "closure_storage");

  LLVMBuildStore(builder, rec_storage, malloced_space);

  LLVMValueRef generic_ptr =
      LLVMBuildBitCast(builder, malloced_space, _VOID_PTR_T, "closure_storage");

  int slot = *ctx->num_globals;
  LLVMValueRef slot_index = LLVMConstInt(LLVMInt32Type(), slot, false);

  LLVMValueRef indices[] = {ZERO, slot_index};
  LLVMValueRef slot_ptr =
      LLVMBuildGEP2(builder, _GLOBAL_STORAGE_TYPE, global_storage_array_llvm,
                    indices, 2, "closure_storage_slot_ptr");
  LLVMBuildStore(builder, generic_ptr, slot_ptr);
  *(ctx->num_globals) = slot + 1;
  return slot;
}

LLVMValueRef codegen_get_global_closure_storage(int slot, JITLangCtx *ctx,
                                                LLVMModuleRef module,
                                                LLVMBuilderRef builder) {

  LLVMValueRef slot_index = LLVMConstInt(LLVMInt32Type(), slot, false);

  LLVMValueRef indices[] = {ZERO, slot_index};
  LLVMValueRef slot_ptr =
      LLVMBuildGEP2(builder, _GLOBAL_STORAGE_TYPE, global_storage_array_llvm,
                    indices, 2, "get_closure_slot_ptr");

  LLVMValueRef generic_ptr =
      LLVMBuildLoad2(builder, _VOID_PTR_T, slot_ptr, "void_ptr");

  LLVMValueRef typed_ptr = LLVMBuildBitCast(
      builder, generic_ptr, LLVMPointerType(GENERIC_PTR, 0), "typed_ptr");

  return LLVMBuildLoad2(builder, GENERIC_PTR, typed_ptr, "get_closure_data");
}

LLVMValueRef codegen_get_global(const char *sym_name, JITSymbol *sym,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  char buf[32];
  int slot = sym->symbol_data.STYPE_TOP_LEVEL_VAR;
  LLVMTypeRef llvm_type = sym->llvm_type;

  LLVMValueRef slot_index = LLVMConstInt(LLVMInt32Type(), slot, false);

  LLVMValueRef indices[] = {ZERO, slot_index};
  snprintf(buf, 32, "%s_slot_ptr", sym_name);
  LLVMValueRef slot_ptr =
      LLVMBuildGEP2(builder, _GLOBAL_STORAGE_TYPE, global_storage_array_llvm,
                    indices, 2, buf);

  LLVMValueRef generic_ptr =
      LLVMBuildLoad2(builder, _VOID_PTR_T, slot_ptr, "void_ptr");

  LLVMValueRef typed_ptr = LLVMBuildBitCast(
      builder, generic_ptr, LLVMPointerType(llvm_type, 0), "typed_ptr");

  snprintf(buf, 32, "%s_load", sym_name);
  return LLVMBuildLoad2(builder, llvm_type, typed_ptr, buf);
}

void setup_global_storage(LLVMModuleRef module, LLVMBuilderRef builder) {
  global_storage_array_llvm =
      LLVMAddGlobal(module, _GLOBAL_STORAGE_TYPE, "global_storage_array");
  LLVMSetLinkage(global_storage_array_llvm, LLVMExternalLinkage);

  global_storage_size_llvm =
      LLVMAddGlobal(module, LLVMInt32Type(), "global_storage_size");
}
