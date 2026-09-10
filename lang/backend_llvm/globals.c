#include "backend_llvm/globals.h"
#include "backend_llvm/common.h"
#include "symbols.h"
#include "types.h"
#include "llvm-c/Core.h"
#include <stdio.h>
#include <string.h>

// Global variables
LLVMValueRef global_storage_array_llvm;
LLVMValueRef global_storage_size_llvm;

#define _GLOBAL_STORAGE_SIZE 1024

static LLVMContextRef module_context(LLVMModuleRef module) {
  return LLVMGetModuleContext(module);
}

static LLVMTypeRef void_ptr_type(LLVMModuleRef module) {
  return LLVMPointerType(LLVMInt8TypeInContext(module_context(module)), 0);
}

static LLVMTypeRef global_storage_type(LLVMModuleRef module) {
  return LLVMArrayType(void_ptr_type(module), _GLOBAL_STORAGE_SIZE);
}

static LLVMValueRef i32_const(LLVMModuleRef module, unsigned value) {
  return LLVMConstInt(LLVMInt32TypeInContext(module_context(module)), value,
                      false);
}

static LLVMTypeRef symbol_storage_type(JITSymbol *sym, JITLangCtx *ctx,
                                       LLVMModuleRef module) {
  if (sym->symbol_type) {
    Type *type = specialize_type_for_codegen(sym->symbol_type, ctx);
    LLVMTypeRef llvm_type = type_to_llvm_type(type, ctx, module);
    if (llvm_type) {
      return llvm_type;
    }
  }

  return sym->llvm_type;
}

LLVMValueRef get_global_storage_array(LLVMModuleRef module) {

  // Look up the global fresh each time instead of using cached reference
  // to avoid stale references after optimization passes
  LLVMValueRef storage_array =
      LLVMGetNamedGlobal(module, "global_storage_array");

  if (storage_array == NULL) {
    // printf("global_storage_array not found, recreating...\n");
    storage_array =
        LLVMAddGlobal(module, global_storage_type(module), "global_storage_array");
    LLVMSetLinkage(storage_array, LLVMExternalLinkage);
  }
  return storage_array;
}

void codegen_set_global(const char *sym_name, JITSymbol *sym,
                        LLVMValueRef value, Type *ttype, LLVMTypeRef llvm_type,
                        JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder) {
  (void)ttype;

  char buf[32];
  snprintf(buf, 32, "%s_malloc", sym_name);
  LLVMValueRef malloced_space = LLVMBuildMalloc(builder, llvm_type, buf);

  LLVMBuildStore(builder, value, malloced_space);

  snprintf(buf, 32, "%s_generic_ptr", sym_name);
  LLVMValueRef generic_ptr =
      LLVMBuildBitCast(builder, malloced_space, void_ptr_type(module), buf);
  int slot = *ctx->num_globals;
  sym->symbol_data.STYPE_TOP_LEVEL_VAR = slot;

  LLVMValueRef slot_index = i32_const(module, slot);

  LLVMValueRef indices[] = {i32_const(module, 0), slot_index};

  snprintf(buf, 32, "%s_slot_ptr", sym_name);
  LLVMValueRef storage_array = get_global_storage_array(module);

  LLVMValueRef slot_ptr = LLVMBuildGEP2(builder, global_storage_type(module),
                                        storage_array, indices, 2, buf);

  LLVMBuildStore(builder, generic_ptr, slot_ptr);

  *(ctx->num_globals) = slot + 1;
}

LLVMValueRef codegen_get_global(const char *sym_name, JITSymbol *sym,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  char buf[32];
  int slot = sym->symbol_data.STYPE_TOP_LEVEL_VAR;
  LLVMTypeRef llvm_type = symbol_storage_type(sym, ctx, module);
  if (!llvm_type) {
    fprintf(stderr, "Error: could not rematerialize global type for %s\n",
            sym_name);
    return NULL;
  }

  LLVMValueRef slot_index = i32_const(module, slot);

  LLVMValueRef indices[] = {i32_const(module, 0), slot_index};
  snprintf(buf, 32, "%s_slot_ptr", sym_name);

  LLVMValueRef storage_array = get_global_storage_array(module);
  LLVMValueRef slot_ptr = LLVMBuildGEP2(builder, global_storage_type(module),
                                        storage_array, indices, 2, buf);

  LLVMValueRef generic_ptr =
      LLVMBuildLoad2(builder, void_ptr_type(module), slot_ptr, "void_ptr");

  LLVMValueRef typed_ptr = LLVMBuildBitCast(
      builder, generic_ptr, LLVMPointerType(llvm_type, 0), "typed_ptr");

  snprintf(buf, 32, "%s_load", sym_name);
  LLVMValueRef load_inst = LLVMBuildLoad2(builder, llvm_type, typed_ptr, buf);
  mark_invariant(load_inst);
  return load_inst;
}

void setup_global_storage(LLVMModuleRef module, LLVMBuilderRef builder) {
  global_storage_array_llvm =
      LLVMAddGlobal(module, global_storage_type(module), "global_storage_array");
  LLVMSetLinkage(global_storage_array_llvm, LLVMExternalLinkage);

  global_storage_size_llvm =
      LLVMAddGlobal(module, LLVMInt32TypeInContext(module_context(module)),
                    "global_storage_size");
}
