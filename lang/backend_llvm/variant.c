#include "backend_llvm/variant.h"
#include "common.h"
#include "match.h"
#include "serde.h"
#include "types.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/Types.h"
#include <stdlib.h>
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

typedef struct SizeCache {
  LLVMTypeRef struct_type;
  uint64_t type_size;
  struct SizeCache *next;
} SizeCache;

static SizeCache *size_cache = NULL;

SizeCache *size_cache_extend(SizeCache *env, LLVMTypeRef struct_type,
                             uint64_t size) {
  SizeCache *new_env = malloc(sizeof(SizeCache));
  new_env->struct_type = struct_type;
  new_env->type_size = size;
  new_env->next = env;
  return new_env;
}

bool size_cache_lookup(SizeCache *env, LLVMTypeRef type, uint64_t *size) {
  while (env) {
    if (type == env->struct_type) {
      *size = env->type_size;
      return true;
    }
    env = env->next;
  }
  return false;
}

uint64_t max_datatype_size(LLVMTypeRef data_types[], size_t num_types,
                           LLVMTargetDataRef target_data, int *largest_idx) {
  uint64_t max_size = 0;

  for (size_t i = 0; i < num_types; i++) {

    if (data_types[i] == NULL) {
      continue;
    }

    uint64_t type_size;
    if (!size_cache_lookup(size_cache, data_types[i], &type_size)) {
      type_size = LLVMStoreSizeOfType(target_data, data_types[i]);
      size_cache = size_cache_extend(size_cache, data_types[i], type_size);
    }

    if (type_size > max_size) {

      *largest_idx = i;
      max_size = type_size;
    }
  }

  return max_size;
}

LLVMTypeRef codegen_union_type(LLVMTypeRef contained_datatypes[],
                               int variant_len, LLVMModuleRef module) {

  LLVMTargetDataRef target_data = LLVMGetModuleDataLayout(module);
  int largest_idx = 0;
  uint64_t largest_size = max_datatype_size(contained_datatypes, variant_len,
                                            target_data, &largest_idx);

  // use a bit of memory equal to largest_size * i8 to represent the union
  // consumers of this variant type will already know the member index of the
  // variant they're dealing with and can then bitcast the union to be the type
  // they expect
  LLVMTypeRef largest_type = contained_datatypes[largest_idx];

  if (largest_type == NULL) {
    return NULL;
  }
  return largest_type;
  // if (largest_type

  // printf("\nget module context????\n");
  // LLVMContextRef context = LLVMGetModuleContext(module);
  // Create union type
  // LLVMTypeRef union_types[] = {largest_type}; // We only need the largest
  // type
  // // LLVMTypeRef union_type = LLVMStructCreateNamed(context, "anon");
  // LLVMTypeRef union_type = LLVMStructType(union_types, 1, 0);
  // // LLVMStructSetBody(union_type, union_types, 1, 0);
  // return union_type;
}

LLVMTypeRef codegen_simple_enum_type() { return TAG_TYPE; }

LLVMTypeRef codegen_tagged_union_type(LLVMTypeRef contained_datatypes[],
                                      int variant_len, LLVMModuleRef module) {

  LLVMTypeRef union_type =
      codegen_union_type(contained_datatypes, variant_len, module);

  if (union_type == NULL) {
    printf("union type is null: \n");
    return TAG_TYPE;
  }

  // Create TU struct type
  // LLVMContextRef context = LLVMGetModuleContext(module);
  LLVMTypeRef tu_types[] = {TAG_TYPE, union_type};
  // LLVMTypeRef tu_type = LLVMStructCreateNamed(context, "TU");
  // LLVMStructSetBody(tu_type, tu_types, 2, 0);
  LLVMTypeRef tu_type = LLVMStructType(tu_types, 2, 0);
  return tu_type;
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

  LLVMValueRef left_tag = LLVMConstInt(TAG_TYPE, vidx, 0);
  return codegen_eq_int(left_tag, val, module, builder);
}

LLVMTypeRef simple_enum_type(LLVMModuleRef module) {
  LLVMContextRef context = LLVMGetModuleContext(module);
  // Create TU struct type
  LLVMTypeRef tu_types[] = {TAG_TYPE};
  LLVMTypeRef tu_type = LLVMStructCreateNamed(context, "TU");
  LLVMStructSetBody(tu_type, tu_types, 1, 0);
  return tu_type;
}

LLVMTypeRef variant_member_to_llvm_type(Type *type, TypeEnv *env,
                                        LLVMModuleRef module) {
  if (type == NULL) {
    fprintf(stderr, "Error - variant member type is null %s:%d", __FILE__,
            __LINE__);
    return NULL;
  }
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
    return simple_enum_type(module);
  }

  if (variant_parent != NULL) {

    Type *vtype = copy_type(variant_parent);

    TypeEnv *_env = NULL;

    const Type *ret = vtype;

    for (int i = 0; i < ret->data.T_CONS.num_args; i++) {
      const Type *gen_mem = ret->data.T_CONS.args[i];

      if (strcmp(gen_mem->data.T_CONS.name, type->data.T_CONS.name) == 0) {
        for (int j = 0; j < gen_mem->data.T_CONS.num_args; j++) {
          const Type *t = gen_mem->data.T_CONS.args[i];
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

LLVMValueRef variant_extract_tag(LLVMValueRef val, LLVMBuilderRef builder) {

  // Get the type of the tagged union
  LLVMTypeRef union_type = LLVMTypeOf(val);
  if (union_type == TAG_TYPE) {
    return val;
  }

  LLVMValueRef tu_alloca = LLVMBuildAlloca(builder, union_type, "tu");
  LLVMBuildStore(builder, val, tu_alloca);
  LLVMValueRef tag_ptr =
      LLVMBuildStructGEP2(builder, union_type, tu_alloca, 0, "tagPtr");

  LLVMValueRef tag = LLVMBuildLoad2(builder, TAG_TYPE, tag_ptr, "tag");
  return tag;
}

LLVMValueRef variant_extract_value(LLVMValueRef val, LLVMTypeRef expected_type,
                                   LLVMBuilderRef builder) {

  LLVMTypeRef union_type = LLVMTypeOf(val);
  LLVMValueRef tu_alloca = LLVMBuildAlloca(builder, union_type, "tu");
  LLVMBuildStore(builder, val, tu_alloca);

  LLVMValueRef value_ptr = LLVMBuildStructGEP2(builder, union_type, tu_alloca,
                                               1, "variant_value_ptr");

  LLVMValueRef contained_value =
      LLVMBuildLoad2(builder, expected_type, value_ptr, "contained_value");

  return contained_value;
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

  LLVMValueRef tu = LLVMBuildAlloca(builder, tagged_union_type, "");

  // Initialize t1
  LLVMBuildStore(builder, LLVMConstInt(TAG_TYPE, vidx, 0),
                 LLVMBuildStructGEP2(builder, tagged_union_type, tu, 0, ""));

  LLVMBuildStore(builder, cons_input,
                 LLVMBuildStructGEP2(builder, tagged_union_type, tu, 1, ""));
  return LLVMBuildLoad2(builder, tagged_union_type, tu, "");
}

LLVMValueRef codegen_simple_enum_member(Ast *ast, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  Type *member_type = ast->md;

  if (!member_type) {
    return NULL;
  }

  if (member_type->kind == T_CONS) {
    int vidx;
    // print_type(member_type);
    // printf("codegen simple enum member %d\n", vidx);
    // print_type_env(ctx->env);
    Type *v = variant_lookup(ctx->env, member_type, &vidx);
    if (!v) {
      return NULL;
    }
    return LLVMConstInt(LLVMInt8Type(), vidx, 0);

    //   LLVMTypeRef struct_type =
    //       LLVMStructType((LLVMTypeRef[]){LLVMInt8Type(), LLVMInt1Type()}, 2,
    //       0);
    //
    //   LLVMValueRef none = LLVMGetUndef(struct_type);
    //
    //   none = LLVMBuildInsertValue(
    //       builder, none, LLVMConstInt(LLVMInt8Type(), vidx, 0), 0, "insert
    //       Tag");
    //   return none;
    // }

    //   if (v && is_generic(v)) {
    //     if (member_type->data.T_CONS.num_args == 0) {
    //
    //       LLVMContextRef context = LLVMGetModuleContext(module);
    //       LLVMTypeRef tu_types[] = {TAG_TYPE};
    //       LLVMTypeRef tu_type = LLVMStructCreateNamed(context, "TU");
    //       LLVMStructSetBody(tu_type, tu_types, 1, 0);
    //       LLVMValueRef alloca = LLVMBuildAlloca(builder, tu_type, "");
    //       LLVMBuildStore(builder, LLVMConstInt(TAG_TYPE, vidx, 0),
    //                      LLVMBuildStructGEP2(builder, tu_type, alloca, 0,
    //                      ""));
    //       return LLVMBuildLoad2(builder, tu_type, alloca, "");
    //     }
    //   }
    //
    //   LLVMTypeRef t = type_to_llvm_type(v, ctx->env, module);
    //
    //   if (t == NULL) {
    //     return NULL;
    //   }
    //
    //   if (t == TAG_TYPE) {
    //     return LLVMConstInt(t, vidx, 0);
    //   }
    //
    //   LLVMValueRef tu = LLVMBuildAlloca(builder, t, "");
    //
    //   LLVMBuildStore(builder, LLVMConstInt(TAG_TYPE, vidx, 0),
    //                  LLVMBuildStructGEP2(builder, t, tu, 0, ""));
    //
    //   return LLVMBuildLoad2(builder, t, tu, "");
    // }
    // return NULL;
  }
}
