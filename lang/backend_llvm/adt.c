#include "backend_llvm/adt.h"
#include "types.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_simple_enum_member(Type *enum_type, const char *mem_name,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  int vidx;
  for (vidx = 0; vidx < enum_type->data.T_CONS.num_args; vidx++) {
    if (strcmp(mem_name, enum_type->data.T_CONS.args[vidx]->data.T_CONS.name) ==
        0) {
      break;
    }
  }
  return LLVMConstInt(LLVMInt8Type(), vidx, 0);
}

#define TAG_TYPE LLVMInt8Type()
LLVMValueRef codegen_adt_member(Type *enum_type, const char *mem_name,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  if (is_option_type(enum_type)) {
    if (strcmp(mem_name, "None") == 0) {
      return codegen_none(builder);
    }
  }

  int vidx;
  Type *member_type;
  for (vidx = 0; vidx < enum_type->data.T_CONS.num_args; vidx++) {
    if (strcmp(mem_name, enum_type->data.T_CONS.args[vidx]->data.T_CONS.name) ==
        0) {
      member_type = enum_type->data.T_CONS.args[vidx];
      break;
    }
  }
  return LLVMConstInt(LLVMInt8Type(), vidx, 0);
}

/**
 * Finds the type with the largest size from an array of LLVM types
 *
 * @param context The LLVM context
 * @param types Array of LLVM type references to compare
 * @param count Number of types in the array
 * @param target_data Target data layout for size calculations
 * @return The type with the largest size, or NULL if array is empty or on
 * error
 */
LLVMTypeRef get_largest_type(LLVMContextRef context, LLVMTypeRef *types,
                             size_t count, LLVMTargetDataRef target_data) {
  if (!types || count == 0 || !target_data) {
    return NULL;
  }

  LLVMTypeRef largest_type = types[0];
  unsigned largest_size = LLVMStoreSizeOfType(target_data, largest_type);
  // unsigned largest_align = LLVMABIAlignmentOfType(target_data, largest_type);
  unsigned largest_align = 256;

  for (size_t i = 1; i < count; i++) {
    unsigned current_size = LLVMStoreSizeOfType(target_data, types[i]);
    unsigned current_align = LLVMABIAlignmentOfType(target_data, types[i]);

    // Compare size first, then alignment as a tiebreaker
    if (current_size > largest_size ||
        (current_size == largest_size && current_align > largest_align)) {
      largest_type = types[i];
      largest_size = current_size;
      largest_align = current_align;
    }
  }

  return largest_type;
}

LLVMTypeRef codegen_adt_type(Type *type, TypeEnv *env, LLVMModuleRef module) {

  int len = type->data.T_CONS.num_args;
  LLVMTypeRef contained_types[len];
  for (int i = 0; i < type->data.T_CONS.num_args; i++) {
    Type *mem = type->data.T_CONS.args[i];
    contained_types[i] = type_to_llvm_type(mem, env, module);
  }
  LLVMTypeRef largest_type =
      get_largest_type(LLVMGetModuleContext(module), contained_types, len,
                       LLVMGetModuleDataLayout(module));
  return LLVMStructType((LLVMTypeRef[]){TAG_TYPE, largest_type}, 2, 0);
}

LLVMValueRef codegen_option(LLVMValueRef val, LLVMBuilderRef builder) {
  LLVMTypeRef tu_types[] = {TAG_TYPE,
                            val != NULL ? LLVMTypeOf(val) : LLVMInt8Type()};
  LLVMTypeRef tu_type = LLVMStructType(tu_types, 2, 0);
  if (val != NULL) {
    LLVMValueRef some = LLVMGetUndef(tu_type);
    some =
        LLVMBuildInsertValue(builder, some, LLVMConstInt(LLVMInt8Type(), 0, 0),
                             0, "insert Some tag");

    some = LLVMBuildInsertValue(builder, some, val, 1, "insert Some Value");
    return some;
  }
  LLVMValueRef none = LLVMGetUndef(tu_type);

  none = LLVMBuildInsertValue(builder, none, LLVMConstInt(LLVMInt8Type(), 1, 0),
                              0, "insert None tag");
  return none;
}

LLVMValueRef codegen_none(LLVMBuilderRef builder) {
  LLVMTypeRef tu_types[] = {TAG_TYPE};
  LLVMTypeRef tu_type = LLVMStructType(tu_types, 1, 0);
  LLVMValueRef none = LLVMGetUndef(tu_type);

  none = LLVMBuildInsertValue(builder, none, LLVMConstInt(LLVMInt8Type(), 1, 0),
                              0, "insert None tag");
  return none;
}

LLVMValueRef extract_tag(LLVMValueRef val, LLVMBuilderRef builder) {

  // Get the type of the tagged union
  LLVMTypeRef union_type = LLVMTypeOf(val);

  if (union_type == TAG_TYPE) {
    return val;
  }

  if (union_type == LLVMStructType((LLVMTypeRef[]){TAG_TYPE}, 1, 0)) {
    return LLVMBuildExtractValue(builder, val, 0, "struct_element");
  }

  LLVMValueRef tu_alloca = LLVMBuildAlloca(builder, union_type, "tu");
  LLVMBuildStore(builder, val, tu_alloca);
  LLVMValueRef tag_ptr =
      LLVMBuildStructGEP2(builder, union_type, tu_alloca, 0, "tagPtr");

  LLVMValueRef tag = LLVMBuildLoad2(builder, TAG_TYPE, tag_ptr, "tag");
  return tag;
}

LLVMValueRef codegen_option_is_none(LLVMValueRef opt, LLVMBuilderRef builder) {
  LLVMValueRef tag = extract_tag(opt, builder);
  return LLVMBuildICmp(builder, LLVMIntEQ, tag,
                       LLVMConstInt(LLVMInt8Type(), 1, 0), "");
}

LLVMValueRef codegen_option_is_some(LLVMValueRef opt, LLVMBuilderRef builder) {
  LLVMValueRef tag = extract_tag(opt, builder);
  return LLVMBuildICmp(builder, LLVMIntEQ, tag,
                       LLVMConstInt(LLVMInt8Type(), 0, 0), "");
}
