#include "backend_llvm/variant.h"
#include "common.h"
#include "match.h"
#include "types.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/Types.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

uint64_t max_datatype_size(LLVMTypeRef data_types[], size_t num_types,
                           LLVMTargetDataRef target_data) {
  uint64_t max_size = 0;

  for (size_t i = 0; i < num_types; i++) {

    if (data_types[i] == NULL) {
      continue;
    }

    uint64_t type_size = LLVMStoreSizeOfType(target_data, data_types[i]);

    if (type_size > max_size) {
      max_size = type_size;
    }
  }

  return max_size;
}

#define BYTE_TYPE LLVMInt8Type()
#define TAG_TYPE LLVMInt8Type()
#define I32_TYPE LLVMInt32Type()
#define UI32(i) LLVMConstInt(I32_TYPE, i, 0)
LLVMTypeRef codegen_union_type(LLVMTypeRef contained_datatypes[],
                               int variant_len, LLVMModuleRef module) {
  LLVMTargetDataRef target_data = LLVMGetModuleDataLayout(module);
  uint64_t largest_size =
      max_datatype_size(contained_datatypes, variant_len, target_data);
  // use a bit of memory equal to largest_size * i8 to represent the union
  // consumers of this variant type will already know the member index of the
  // variant they're dealing with and can then bitcast the union to be the type
  // they expect
  LLVMTypeRef union_type = LLVMArrayType(BYTE_TYPE, largest_size);
  return union_type;
}

LLVMTypeRef codegen_tagged_union_type(LLVMTypeRef contained_datatypes[],
                                      int variant_len, LLVMModuleRef module) {
  LLVMTypeRef union_type =
      codegen_union_type(contained_datatypes, variant_len, module);

  return LLVMStructType(
      (LLVMTypeRef[]){
          TAG_TYPE,
          union_type,
      },
      2, 0);
}

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

LLVMValueRef match_variant_member(LLVMValueRef left, LLVMValueRef right,
                                  int variant_idx, Type *expected_member_type,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {

  LLVMValueRef left_tag = variant_extract_tag(left, builder);
  LLVMValueRef right_tag = variant_extract_tag(right, builder);
  LLVMValueRef tags_match =
      codegen_eq_int(left_tag, right_tag, module, builder);

  if (expected_member_type->data.T_CONS.num_args > 0) {
    LLVMValueRef res = tags_match;

    expected_member_type =
        expected_member_type->data.T_CONS
            .args[0]; // variant member cons should only accept one arg

    LLVMValueRef vals_match = codegen_equality(
        variant_extract_value(
            left, type_to_llvm_type(expected_member_type, ctx->env, module),
            builder),
        expected_member_type,
        variant_extract_value(
            right, type_to_llvm_type(expected_member_type, ctx->env, module),
            builder),
        expected_member_type, ctx, module, builder);

    return LLVMBuildAnd(builder, res, vals_match, "tag match && values match");
  }

  return tags_match;
}

LLVMValueRef match_simple_variant_member(Ast *id, int vidx, Type *variant_type,
                                         LLVMValueRef val, JITLangCtx *ctx,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder) {

  LLVMValueRef left_tag = LLVMConstInt(LLVMInt32Type(), vidx, 0);
  LLVMValueRef right_tag = variant_extract_tag(val, builder);

  return codegen_eq_int(left_tag, right_tag, module, builder);
}

LLVMTypeRef simple_enum_type() {
  // Create a struct type with just the tag
  LLVMTypeRef tag_only_type = LLVMStructType(
      (LLVMTypeRef[]){
          TAG_TYPE,
      },
      1, 0);
  return tag_only_type;
}

LLVMTypeRef variant_member_to_llvm_type(Type *type, TypeEnv *env,
                                        LLVMModuleRef module) {
  if (type->kind != T_CONS) {
    return NULL;
  }

  int vidx;
  char *vname;
  Type *variant_parent =
      variant_member_lookup(env, type->data.T_CONS.name, &vidx, &vname);

  if (!variant_parent) {
    return NULL;
  }

  if (type->data.T_CONS.num_args == 0) {
    return simple_enum_type();
  }

  if (variant_parent) {

    Type *vtype = copy_type(variant_parent);

    TypeEnv *_env = NULL;

    Type *ret = vtype;

    for (int i = 0; i < ret->data.T_CONS.num_args; i++) {
      Type *gen_mem = ret->data.T_CONS.args[i];

      if (strcmp(gen_mem->data.T_CONS.name, type->data.T_CONS.name) == 0) {
        for (int j = 0; j < gen_mem->data.T_CONS.num_args; j++) {
          Type *t = gen_mem->data.T_CONS.args[i];
          Type *v = type->data.T_CONS.args[i];
          if (t->kind == T_VAR) {
            _env = env_extend(_env, t->data.T_VAR, v);
          }
        }
      }
    }
    vtype = resolve_generic_type(vtype, _env);
    return type_to_llvm_type(vtype, env, module);
  }
  return NULL;
}

LLVMValueRef _variant_extract_tag(LLVMValueRef val, LLVMBuilderRef builder) {
  return LLVMBuildExtractValue(builder, val, 0, "extract_tagged_union_tag");
}
LLVMValueRef variant_extract_tag(LLVMValueRef val, LLVMBuilderRef builder) {
  // Get the type of the tagged union
  LLVMTypeRef union_type = LLVMTypeOf(val);

  // Create indices for GEP: [0, 0] to get the first element (tag)
  LLVMValueRef indices[2] = {LLVMConstInt(LLVMInt32Type(), 0, 0),
                             LLVMConstInt(LLVMInt32Type(), 0, 0)};

  // Use GEP to get a pointer to the tag
  LLVMValueRef tag_ptr =
      LLVMBuildGEP2(builder, union_type, val, indices, 2, "tag_ptr");

  // Load the tag value
  return LLVMBuildLoad2(builder, LLVMInt8Type(), tag_ptr,
                        "extract_tagged_union_tag");
}

LLVMValueRef _variant_extract_value(LLVMValueRef val, LLVMTypeRef expected_type,
                                    LLVMBuilderRef builder) {
  LLVMValueRef extracted =
      LLVMBuildExtractValue(builder, val, 1, "extract_tagged_union_value");

  // Check if we need to bitcast
  if (LLVMTypeOf(extracted) != expected_type) {
    return LLVMBuildBitCast(builder, extracted, expected_type,
                            "bitcast_union_value");
  }

  return extracted;
}
LLVMValueRef variant_extract_value(LLVMValueRef val, LLVMTypeRef expected_type,
                                   LLVMBuilderRef builder) {

  LLVMTypeRef specific_type = LLVMStructType(
      (LLVMTypeRef[]){
          TAG_TYPE,
          expected_type,
      },
      2, 0);

  LLVMValueRef specific_variant_bitcast = LLVMBuildBitCast(
      builder, val, LLVMPointerType(specific_type, 0), "union_as_specific");

  LLVMValueRef value_indices[2] = {UI32(0), UI32(1)};

  LLVMValueRef value_ptr =
      LLVMBuildGEP2(builder, specific_type, specific_variant_bitcast,
                    value_indices, 2, "value_ptr");

  return LLVMBuildLoad2(builder, expected_type, value_ptr,
                        "extract_tagged_union_value");
}

LLVMValueRef tagged_union_constructor(Ast *ast, LLVMTypeRef tagged_union_type,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  Type *type = ast->md;

  int vidx;
  char *vname;
  Type *variant_parent =
      variant_member_lookup(ctx->env, type->data.T_CONS.name, &vidx, &vname);

  LLVMValueRef cons_input = codegen(
      ast->data.AST_APPLICATION.args, // only one arg for cons types in variants
      ctx, module, builder);

  LLVMValueRef union_value = LLVMBuildAlloca(builder, tagged_union_type, "");

  LLVMValueRef tag_value = LLVMConstInt(TAG_TYPE, vidx, 0);
  LLVMValueRef tag_ptr =
      LLVMBuildStructGEP2(builder, tagged_union_type, tag_value, 0, "tag_ptr");
  LLVMBuildStore(builder, tag_value, tag_ptr);

  Type *contained_type = type->data.T_CONS.args[0];

  LLVMTypeRef specific_type = LLVMStructType(
      (LLVMTypeRef[]){TAG_TYPE,
                      type_to_llvm_type(contained_type, ctx->env, module)

      },
      2, 0);

  LLVMValueRef specific_variant_bitcast =
      LLVMBuildBitCast(builder, union_value, LLVMPointerType(specific_type, 0),
                       "union_as_specific");

  LLVMValueRef value_indices[2] = {UI32(0), UI32(1)};
  LLVMValueRef value_ptr =
      LLVMBuildGEP2(builder, specific_type, specific_variant_bitcast,
                    value_indices, 2, "value_ptr");
  LLVMBuildStore(builder, cons_input, value_ptr);

  // return LLVMBuildLoad2(builder, tagged_union_type, union_value, "");
  return union_value;
}
