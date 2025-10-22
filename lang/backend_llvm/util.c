#include "backend_llvm/util.h"
#include "common.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void struct_ptr_set(int item_offset, LLVMValueRef struct_ptr,
                    LLVMTypeRef struct_type, LLVMValueRef data,
                    LLVMBuilderRef builder) {

  // Set the data
  LLVMValueRef data_ptr = LLVMBuildStructGEP2(builder, struct_type, struct_ptr,
                                              item_offset, "data_ptr");
  LLVMBuildStore(builder, data, data_ptr);
}

LLVMValueRef struct_ptr_get(int item_offset, LLVMValueRef struct_ptr,
                            LLVMTypeRef struct_type, LLVMBuilderRef builder) {
  // Get a pointer to the item
  LLVMValueRef item_ptr = LLVMBuildStructGEP2(builder, struct_type, struct_ptr,
                                              item_offset, "item_ptr");

  // Load the value from the pointer
  LLVMTypeRef item_type = LLVMStructGetTypeAtIndex(struct_type, item_offset);
  return LLVMBuildLoad2(builder, item_type, item_ptr, "loaded_item");
}

LLVMValueRef increment_ptr(LLVMValueRef ptr, LLVMTypeRef node_type,
                           LLVMValueRef element_size, LLVMBuilderRef builder) {

  return LLVMBuildGEP2(builder, node_type, ptr, &element_size, 1,
                       "next_element_ptr");
}

LLVMValueRef and_vals(LLVMValueRef l, LLVMValueRef r, LLVMBuilderRef builder) {
  return LLVMBuildAnd(builder, l, r, "and_vals");
}

LLVMValueRef get_extern_fn(const char *name, LLVMTypeRef fn_type,
                           LLVMModuleRef module, bool needs_sret,
                           LLVMTypeRef sret_type) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, name);

  if (fn == NULL) {
    fn = LLVMAddFunction(module, name, fn_type);
    LLVMSetFunctionCallConv(fn, LLVMCCallConv);

    // If this function uses sret (struct return via pointer), add the attribute
    if (needs_sret) {
      LLVMContextRef context = LLVMGetModuleContext(module);
      unsigned sret_kind_id = LLVMGetEnumAttributeKindForName("sret", 4);
      LLVMAttributeRef sret_attr =
          LLVMCreateTypeAttribute(context, sret_kind_id, sret_type);
      // Add sret attribute to parameter 1 (first parameter, index 0 is return)
      LLVMAddAttributeAtIndex(fn, 1, sret_attr);
    }
  }
  return fn;
}

LLVMValueRef alloc(LLVMTypeRef type, JITLangCtx *ctx, LLVMBuilderRef builder) {
  return ctx->stack_ptr == 0 ? LLVMBuildMalloc(builder, type, "heap_alloc")
                             : LLVMBuildAlloca(builder, type, "stack_alloc");
}

LLVMValueRef heap_alloc(LLVMTypeRef type, LLVMBuilderRef builder) {
  return LLVMBuildMalloc(builder, type, "heap_alloc");
}
