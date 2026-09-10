#include "backend_llvm/adt.h"
#include "array.h"
#include "types.h"
#include "types/type_ser.h"
#include "util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/Types.h"
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef _codegen_equality(Type *type, LLVMValueRef l, LLVMValueRef r,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

static LLVMContextRef builder_context(LLVMBuilderRef builder) {
  LLVMBasicBlockRef block = LLVMGetInsertBlock(builder);
  if (!block) {
    return LLVMGetGlobalContext();
  }

  LLVMValueRef function = LLVMGetBasicBlockParent(block);
  if (!function) {
    return LLVMGetGlobalContext();
  }

  LLVMModuleRef module = LLVMGetGlobalParent(function);
  return module ? LLVMGetModuleContext(module) : LLVMGetGlobalContext();
}

static LLVMBasicBlockRef append_block_in_module(LLVMModuleRef module,
                                                LLVMValueRef function,
                                                const char *name) {
  return LLVMAppendBasicBlockInContext(LLVMGetModuleContext(module), function,
                                       name);
}

static LLVMTypeRef tag_type(LLVMContextRef context) {
  return LLVMInt8TypeInContext(context);
}

static LLVMTypeRef byte_ptr_type(LLVMContextRef context) {
  return LLVMPointerType(tag_type(context), 0);
}

static LLVMValueRef bool_const(LLVMContextRef context, int value) {
  return LLVMConstInt(LLVMInt1TypeInContext(context), value, 0);
}

static int sum_variant_count(Type *sum_type) {
  return sum_type->data.T_CONS.num_args;
}

static Type *sum_variant_at(Type *sum_type, int idx) {
  return sum_type->data.T_CONS.args[idx];
}

LLVMValueRef codegen_simple_enum_member(Type *enum_type, const char *mem_name,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  int vidx;
  for (vidx = 0; vidx < sum_variant_count(enum_type); vidx++) {
    if (strcmp(mem_name, sum_variant_at(enum_type, vidx)->data.T_CONS.name) ==
        0) {
      break;
    }
  }
  return LLVMConstInt(tag_type(LLVMGetModuleContext(module)), vidx, 0);
}

LLVMValueRef codegen_adt_member(Type *enum_type, const char *mem_name,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  if (enum_type->kind == T_FN) {
    enum_type = fn_return_type(enum_type);
  }

  if (is_simple_enum(enum_type)) {
    return codegen_simple_enum_member(enum_type, mem_name, ctx, module, builder);
  }

  int vidx;
  for (vidx = 0; vidx < sum_variant_count(enum_type); vidx++) {
    if (strcmp(mem_name, sum_variant_at(enum_type, vidx)->data.T_CONS.name) ==
        0) {
      break;
    }
  }

  LLVMTypeRef llvm_type = type_to_llvm_type(enum_type, ctx, module);
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMValueRef value = LLVMGetUndef(llvm_type);
  return LLVMBuildInsertValue(builder, value,
                              LLVMConstInt(tag_type(llvm_ctx), vidx, 0), 0,
                              "insert variant tag");
}

LLVMValueRef codegen_adt_member_with_args(Type *enum_type, LLVMTypeRef tu_type,
                                          Ast *app, const char *mem_name,
                                          JITLangCtx *ctx, LLVMModuleRef module,
                                          LLVMBuilderRef builder) {

  if (enum_type->kind == T_FN) {
    enum_type = fn_return_type(enum_type);
  }

  int i = 0;
  while (strcmp(mem_name, sum_variant_at(enum_type, i)->data.T_CONS.name) != 0) {
    i++;
  }

  LLVMValueRef some = LLVMGetUndef(tu_type);
  LLVMContextRef llvm_ctx = LLVMGetTypeContext(tu_type);

  some = LLVMBuildInsertValue(builder, some,
                              LLVMConstInt(tag_type(llvm_ctx), i, 0), 0,
                              "insert Some tag");

  LLVMValueRef val;

  if (app->data.AST_APPLICATION.len > 1) {
    Type *member_type = extract_member_from_sum_type(
        enum_type, app->data.AST_APPLICATION.function);
    member_type = member_type->data.T_CONS.args[0];

    LLVMTypeRef union_type = LLVMStructGetTypeAtIndex(tu_type, 1);
    LLVMTypeRef llvm_member_type = type_to_llvm_type(member_type, ctx, module);
    LLVMValueRef union_value = LLVMGetUndef(llvm_member_type);

    for (int i = 0; i < app->data.AST_APPLICATION.len; i++) {

      LLVMValueRef field_val =
          codegen(app->data.AST_APPLICATION.args + i, ctx, module, builder);

      union_value = LLVMBuildInsertValue(builder, union_value, field_val, i,
                                         "insert int arg");
    }

    // Store struct to temp, then load as byte array
    LLVMValueRef struct_temp =
        LLVMBuildAlloca(builder, llvm_member_type, "struct_temp");
    LLVMBuildStore(builder, union_value, struct_temp);

    LLVMValueRef byte_ptr = LLVMBuildBitCast(
        builder, struct_temp, byte_ptr_type(llvm_ctx), "byte_ptr");
    LLVMValueRef union_as_bytes =
        LLVMBuildLoad2(builder, union_type, byte_ptr, "load_as_bytes");

    some = LLVMBuildInsertValue(builder, some, union_as_bytes, 1,
                                "insert variant data");
  } else {

    LLVMTypeRef union_type = LLVMStructGetTypeAtIndex(tu_type, 1);
    val = codegen(app->data.AST_APPLICATION.args, ctx, module, builder);

    // Store value into byte array union storage
    LLVMTypeRef val_type = LLVMTypeOf(val);

    // Allocate temp space for the value
    LLVMValueRef val_temp = LLVMBuildAlloca(builder, val_type, "val_temp");
    LLVMBuildStore(builder, val, val_temp);

    // Cast to byte pointer and load as byte array
    LLVMValueRef byte_ptr = LLVMBuildBitCast(
        builder, val_temp, byte_ptr_type(llvm_ctx), "byte_ptr");
    LLVMValueRef union_val =
        LLVMBuildLoad2(builder, union_type, byte_ptr, "load_as_bytes");

    some = LLVMBuildInsertValue(builder, some, union_val, 1,
                                "insert variant data");
  }

  return some;
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
                             size_t count, LLVMTargetDataRef target_data,
                             unsigned long long *largest_size_bits) {
  if (!types || count == 0 || !target_data) {
    return NULL;
  }

  LLVMTypeRef largest_type = types[0];
  unsigned largest_size = LLVMStoreSizeOfType(target_data, largest_type);
  unsigned largest_align = LLVMABIAlignmentOfType(target_data, largest_type);
  if (largest_size_bits) {
    *largest_size_bits = largest_size;
  }

  for (size_t i = 1; i < count; i++) {
    unsigned current_size = LLVMStoreSizeOfType(target_data, types[i]);
    unsigned current_align = LLVMABIAlignmentOfType(target_data, types[i]);

    if (current_size > largest_size ||
        (current_size == largest_size && current_align > largest_align)) {
      largest_type = types[i];
      largest_size = current_size;
      largest_align = current_align;
      if (largest_size_bits) {
        *largest_size_bits = largest_size;
      }
    }
  }

  return largest_type;
}

unsigned long long get_largest_type_size(LLVMContextRef context,
                                         LLVMTypeRef *types, size_t count,
                                         LLVMTargetDataRef target_data) {
  if (!types || count == 0 || !target_data) {
    return 0;
  }

  LLVMTypeRef largest_type = types[0];
  unsigned largest_size = LLVMStoreSizeOfType(target_data, largest_type);
  unsigned largest_align = LLVMABIAlignmentOfType(target_data, largest_type);

  for (size_t i = 1; i < count; i++) {
    unsigned current_size = LLVMStoreSizeOfType(target_data, types[i]);
    unsigned current_align = LLVMABIAlignmentOfType(target_data, types[i]);

    if (current_size > largest_size ||
        (current_size == largest_size && current_align > largest_align)) {
      largest_type = types[i];
      largest_size = current_size;
      largest_align = current_align;
    }
  }

  return largest_size;
}

LLVMTypeRef codegen_option_struct_type(LLVMTypeRef type) {
  LLVMContextRef llvm_ctx = LLVMGetTypeContext(type);
  LLVMTypeRef tu_types[] = {tag_type(llvm_ctx), type};
  LLVMTypeRef tu_type = LLVMStructTypeInContext(llvm_ctx, tu_types, 2, 0);
  return tu_type;
}

LLVMTypeRef codegen_adt_type(Type *type, JITLangCtx *ctx,
                             LLVMModuleRef module) {

  if (type->alias != NULL && strcmp(type->alias, "Option") == 0) {
    Type *_underlying = type_of_option(type);
    if (is_generic(_underlying)) {
      _underlying = specialize_type_for_codegen(_underlying, ctx);
    }
    LLVMTypeRef underlying;

    if (_underlying->kind == T_FN) {
      underlying = byte_ptr_type(LLVMGetModuleContext(module));
    } else {
      underlying = type_to_llvm_type(_underlying, ctx, module);
    }

    return codegen_option_struct_type(underlying);
  }

  int len = sum_variant_count(type);
  LLVMTypeRef contained_types[len];
  for (int i = 0; i < len; i++) {
    Type *mem = sum_variant_at(type, i);
    contained_types[i] = type_to_llvm_type(mem, ctx, module);
  }

  unsigned long long union_size_bytes =
      get_largest_type_size(LLVMGetModuleContext(module), contained_types, len,
                            LLVMGetModuleDataLayout(module));
  unsigned long long union_words = (union_size_bytes + 7) / 8;
  if (union_words == 0) {
    union_words = 1;
  }

  // Use word-aligned opaque storage so pointer-bearing payloads keep their ABI
  // alignment when packed into tagged unions and arrays of tagged unions.
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef fields[] = {tag_type(llvm_ctx),
                          LLVMArrayType(LLVMInt64TypeInContext(llvm_ctx),
                                        (unsigned)union_words)};
  return LLVMStructTypeInContext(llvm_ctx, fields, 2, 0);
}

LLVMValueRef codegen_some(LLVMValueRef val, LLVMBuilderRef builder) {
  LLVMContextRef llvm_ctx = LLVMGetTypeContext(LLVMTypeOf(val));
  LLVMTypeRef tu_types[] = {tag_type(llvm_ctx), LLVMTypeOf(val)};
  LLVMTypeRef tu_type = LLVMStructTypeInContext(llvm_ctx, tu_types, 2, 0);
  LLVMValueRef some = LLVMGetUndef(tu_type);
  some = LLVMBuildInsertValue(
      builder, some, LLVMConstInt(tag_type(llvm_ctx), 0, 0), 0,
      "insert Some tag");

  some = LLVMBuildInsertValue(builder, some, val, 1, "insert Some Value");
  return some;
}

LLVMValueRef codegen_none(LLVMBuilderRef builder) {
  LLVMContextRef llvm_ctx = builder_context(builder);
  LLVMTypeRef tu_fields[] = {tag_type(llvm_ctx), tag_type(llvm_ctx)};
  LLVMTypeRef tu_type = LLVMStructTypeInContext(llvm_ctx, tu_fields, 2, 0);
  LLVMValueRef none =
      STRUCT(tu_type, builder, 2, LLVMConstInt(tag_type(llvm_ctx), 1, 0),
             LLVMConstInt(tag_type(llvm_ctx), 0, 0));

  return none;
}

LLVMValueRef codegen_none_typed(LLVMBuilderRef builder, LLVMTypeRef type) {
  LLVMContextRef llvm_ctx = LLVMGetTypeContext(type);
  LLVMTypeRef tu_types[] = {tag_type(llvm_ctx), type};
  LLVMTypeRef tu_type = LLVMStructTypeInContext(llvm_ctx, tu_types, 2, 0);
  LLVMValueRef none = LLVMGetUndef(tu_type);

  none = LLVMBuildInsertValue(
      builder, none, LLVMConstInt(tag_type(llvm_ctx), 1, 0), 0,
      "insert None tag");

  none = LLVMBuildInsertValue(builder, none, LLVMGetUndef(type), 1,
                              "insert None dummy val");

  return none;
}

LLVMValueRef extract_tag(LLVMValueRef val, LLVMBuilderRef builder) {

  // Get the type of the tagged union
  LLVMTypeRef union_type = LLVMTypeOf(val);
  LLVMContextRef llvm_ctx = LLVMGetTypeContext(union_type);
  LLVMTypeRef llvm_tag_type = tag_type(llvm_ctx);

  if (union_type == llvm_tag_type) {
    return val;
  }

  if (LLVMGetTypeKind(union_type) == LLVMStructTypeKind &&
      LLVMCountStructElementTypes(union_type) == 1 &&
      LLVMStructGetTypeAtIndex(union_type, 0) == llvm_tag_type) {
    return LLVMBuildExtractValue(builder, val, 0, "struct_element");
  }

  LLVMValueRef tu_alloca = LLVMBuildAlloca(builder, union_type, "tu");
  LLVMBuildStore(builder, val, tu_alloca);
  LLVMValueRef tag_ptr =
      LLVMBuildStructGEP2(builder, union_type, tu_alloca, 0, "tag_ptr");

  LLVMValueRef tag = LLVMBuildLoad2(builder, llvm_tag_type, tag_ptr, "tag");
  return tag;
}

LLVMValueRef codegen_option_is_none(LLVMValueRef opt, LLVMBuilderRef builder) {
  LLVMValueRef tag = extract_tag(opt, builder);
  LLVMContextRef llvm_ctx = LLVMGetTypeContext(LLVMTypeOf(tag));
  return LLVMBuildICmp(builder, LLVMIntEQ, tag,
                       LLVMConstInt(tag_type(llvm_ctx), 1, 0), "");
}

LLVMValueRef codegen_option_is_some(LLVMValueRef opt, LLVMBuilderRef builder) {
  LLVMValueRef tag = extract_tag(opt, builder);
  LLVMContextRef llvm_ctx = LLVMGetTypeContext(LLVMTypeOf(tag));
  return LLVMBuildICmp(builder, LLVMIntEQ, tag,
                       LLVMConstInt(tag_type(llvm_ctx), 0, 0), "");
}

LLVMValueRef _codegen_string(const char *chars, int length, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef stream_string_concat(LLVMValueRef *strings, int num_strings,
                                  LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef llvm_string_serialize(LLVMValueRef val, Type *val_type,
                                   JITLangCtx *ctx, LLVMModuleRef module,
                                   LLVMBuilderRef builder);
LLVMValueRef opt_to_string(LLVMValueRef opt_value, Type *val_type,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  LLVMValueRef tag = LLVMBuildExtractValue(builder, opt_value, 0, "tag_val");
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMValueRef is_none = LLVMBuildICmp(builder, LLVMIntEQ, tag,
                                       LLVMConstInt(tag_type(llvm_ctx), 1, 0),
                                       "");

  LLVMValueRef result = LLVMBuildSelect(
      builder, is_none,

      _codegen_string("None", 4, ctx, module, builder),

      stream_string_concat(
          (LLVMValueRef[]){
              _codegen_string("Some ", 5, ctx, module, builder),

              llvm_string_serialize(
                  LLVMBuildExtractValue(builder, opt_value, 1, ""),
                  type_of_option(val_type), ctx, module, builder),
          },
          2, module, builder),
      "select");

  return result;
}

LLVMValueRef OptMapHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  LLVMValueRef func =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  Type *mapper_type = ast->data.AST_APPLICATION.args->type;

  LLVMTypeRef llvm_mapper_type = type_to_llvm_type(mapper_type, ctx, module);

  Type *mapped_type = mapper_type->data.T_FN.to;
  LLVMTypeRef llvm_mapped_type = type_to_llvm_type(mapped_type, ctx, module);

  LLVMValueRef opt_val =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);

  LLVMBasicBlockRef then_block =
      append_block_in_module(module, function, "then");
  LLVMBasicBlockRef else_block =
      append_block_in_module(module, function, "else");
  LLVMBasicBlockRef merge_block =
      append_block_in_module(module, function, "merge");

  LLVMValueRef is_some = codegen_option_is_some(opt_val, builder);
  LLVMBuildCondBr(builder, is_some, then_block, else_block);

  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMValueRef value_field =
      LLVMBuildExtractValue(builder, opt_val, 1, "value");
  LLVMValueRef mapped_value = LLVMBuildCall2(builder, llvm_mapper_type, func,
                                             &value_field, 1, "mapped");
  LLVMBuildBr(builder, merge_block);

  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMBuildBr(builder, merge_block);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, llvm_mapped_type, "result");

  LLVMValueRef incoming_values[2] = {mapped_value,
                                     LLVMGetUndef(llvm_mapped_type)};
  LLVMBasicBlockRef incoming_blocks[2] = {then_block, else_block};
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);

  return phi;
}
// Helper to check if a type contains a recursive reference
bool type_contains_recursive_ref(Type *type, const char *target_name) {

  if (!type) {
    return false;
  }

  switch (type->kind) {
  case T_VAR:
    return type->is_recursive_type_ref;

  case T_RECURSIVE_REF:
    return type->data.T_RECURSIVE_REF.name &&
           strcmp(type->data.T_RECURSIVE_REF.name, target_name) == 0;

  case T_CONS:
  case T_SUM: {

    if (type->data.T_CONS.name &&
        strcmp(type->data.T_CONS.name, target_name) == 0) {
      return true;
    }

    for (int i = 0; i < type->data.T_CONS.num_args; i++) {

      if (type_contains_recursive_ref(type->data.T_CONS.args[i], target_name)) {
        return true;
      }
    }
    return false;
  }

  case T_FN:
    return type_contains_recursive_ref(type->data.T_FN.from, target_name) ||
           type_contains_recursive_ref(type->data.T_FN.to, target_name);

  default:
    return false;
  }
}

Type *find_recursive_type_container(Type *t, const char *name,
                                    Type *container) {
  if (t->kind == T_VAR && t->is_recursive_type_ref &&
      CHARS_EQ(t->data.T_VAR.name, name)) {
    return container;
  }
  if (t->kind == T_CONS || t->kind == T_SUM) {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      Type *x;
      if ((x = find_recursive_type_container(t->data.T_CONS.args[i], name,
                                             t)) != NULL) {
        return x;
      }
    }
  }
  return NULL;
}

LLVMTypeRef codegen_recursive_datatype(Type *type, Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module) {
  // For recursive datatypes like PatResult/List, we need to:
  // 1. Create an opaque struct type first (forward declaration)
  // 2. Convert member types (which may reference the opaque type)
  // 3. Set the struct body with the actual fields

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);

  Ast *name_ast = ast->data.AST_LET.binding;
  const char *name = name_ast->data.AST_IDENTIFIER.value;

  // Create opaque named struct for the variant type
  LLVMTypeRef variant_struct = LLVMStructCreateNamed(llvm_ctx, name);
  STACK_ALLOC_CTX_PUSH(_ctx, ctx);

  int len = sum_variant_count(type);
  LLVMTypeRef member_types[len];

  for (int i = 0; i < len; i++) {
    Type *member_type = sum_variant_at(type, i);

    // Type *container;
    // if (type_contains_recursive_ref(member_type, name)) {
    //
    //   Type *container = find_recursive_type_container(
    //       member_type->data.T_CONS.args[0], name, type);
    //
    //   if (!(is_list_type(container) || is_array_type(container) ||
    //         is_coroutine_type(container))) {
    //     fprintf(stderr,
    //             "Error: type %s cannot hold a recursive reference without a "
    //             "List or Array container\n",
    //             name);
    //
    //     return NULL;
    //   }
    // }
    member_types[i] = type_to_llvm_type(member_type, ctx, module);
  }

  LLVMTargetDataRef target_data = LLVMGetModuleDataLayout(module);
  unsigned long long union_size_bytes =

      get_largest_type_size(llvm_ctx, member_types, len, target_data);
  unsigned long long union_words = (union_size_bytes + 7) / 8;
  if (union_words == 0) {
    union_words = 1;
  }

  // Use word-aligned opaque storage so pointer-bearing payloads keep their ABI
  // alignment when packed into tagged unions and arrays of tagged unions.
  LLVMTypeRef body_fields[] = {tag_type(llvm_ctx),
                               LLVMArrayType(LLVMInt64TypeInContext(llvm_ctx),
                                             (unsigned)union_words)};
  LLVMStructSetBody(variant_struct, body_fields, 2, 0);

  destroy_ctx(&_ctx);
  return variant_struct;
}

LLVMValueRef cast_union(LLVMValueRef un, Type *desired_type, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMTypeRef target_llvm_type = type_to_llvm_type(desired_type, ctx, module);
  if (!target_llvm_type) {
    fprintf(stderr, "Error: could not get LLVM type for desired type\n");
    return NULL;
  }

  switch (desired_type->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL: {
    // Extract scalar from byte array union storage
    // Store byte array to memory, then load as target type
    LLVMValueRef union_alloca =
        LLVMBuildAlloca(builder, LLVMTypeOf(un), "union_cast_temp");

    LLVMBuildStore(builder, un, union_alloca);

    // Cast pointer to target type and load
    LLVMValueRef typed_ptr = LLVMBuildBitCast(
        builder, union_alloca, LLVMPointerType(target_llvm_type, 0),
        "cast_to_target_ptr");
    return LLVMBuildLoad2(builder, target_llvm_type, typed_ptr,
                          "union_to_scalar");
  }
  case T_FN: {

    LLVMValueRef union_alloca =
        LLVMBuildAlloca(builder, LLVMTypeOf(un), "union_cast_temp");

    LLVMBuildStore(builder, un, union_alloca);

    // Cast pointer to target type and load
    LLVMValueRef typed_ptr =
        LLVMBuildBitCast(builder, union_alloca,
                         byte_ptr_type(LLVMGetModuleContext(module)),
                         "cast_to_target_ptr");
    return LLVMBuildLoad2(builder, target_llvm_type, typed_ptr,
                          "union_to_fn_ptr");
  }

  case T_VOID: {
    // No value needed for void
    return NULL;
  }

  case T_STRING:
  case T_CONS:
  case T_SUM: {
    // Extract from byte array union storage
    // Store byte array to memory, then load as target type
    LLVMValueRef union_alloca =
        LLVMBuildAlloca(builder, LLVMTypeOf(un), "union_cast_temp");
    LLVMBuildStore(builder, un, union_alloca);

    // Cast pointer to target type and load
    // This reinterprets the bytes as the target type
    LLVMValueRef typed_ptr = LLVMBuildBitCast(
        builder, union_alloca, LLVMPointerType(target_llvm_type, 0),
        "cast_to_target_ptr");
    return LLVMBuildLoad2(builder, target_llvm_type, typed_ptr,
                          "union_to_struct");
  }

  default: {
    fprintf(stderr, "Error, could not cast union type (kind: %d)\n",
            desired_type->kind);
    print_type_err(desired_type);
    return NULL;
  }
  }
}

LLVMValueRef sum_type_eq(Type *type, LLVMValueRef val1, LLVMValueRef val2,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);

  LLVMBasicBlockRef tag_mismatch_block =
      append_block_in_module(module, function, "sum_tag_mismatch");
  LLVMBasicBlockRef end_block =
      append_block_in_module(module, function, "sum_eq_end");

  LLVMValueRef tag1 = extract_tag(val1, builder);
  LLVMValueRef tag2 = extract_tag(val2, builder);

  LLVMValueRef tags_equal =
      LLVMBuildICmp(builder, LLVMIntEQ, tag1, tag2, "tags_eq");

  LLVMBasicBlockRef switch_block =
      append_block_in_module(module, function, "sum_tag_switch");
  LLVMBuildCondBr(builder, tags_equal, switch_block, tag_mismatch_block);

  LLVMPositionBuilderAtEnd(builder, tag_mismatch_block);
  LLVMBuildBr(builder, end_block);

  LLVMPositionBuilderAtEnd(builder, switch_block);

  int num_variants = sum_variant_count(type);
  LLVMBasicBlockRef default_block = tag_mismatch_block; // Should never happen
  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, tag1, default_block, num_variants);

  LLVMValueRef phi_values[num_variants + 1];
  LLVMBasicBlockRef phi_blocks[num_variants + 1];

  int phi_count = 0;

  phi_values[phi_count] = bool_const(llvm_ctx, 0);
  phi_blocks[phi_count] = tag_mismatch_block;
  phi_count++;

  for (int vidx = 0; vidx < num_variants; vidx++) {
    LLVMBasicBlockRef variant_block =
        append_block_in_module(module, function, "sum_variant_eq");

    LLVMAddCase(switch_inst, LLVMConstInt(tag_type(llvm_ctx), vidx, 0),
                variant_block);

    LLVMPositionBuilderAtEnd(builder, variant_block);
    Type *variant_type = sum_variant_at(type, vidx);
    Type *payload_type = NULL;

    if (variant_type->data.T_CONS.num_args > 0) {
      payload_type = variant_type->data.T_CONS.args[0];
    }

    // Extract payloads
    LLVMValueRef payload1 = LLVMBuildExtractValue(builder, val1, 1, "payload1");
    payload1 = cast_union(payload1, payload_type, ctx, module, builder);

    LLVMValueRef payload2 = LLVMBuildExtractValue(builder, val2, 1, "payload2");
    payload2 = cast_union(payload2, payload_type, ctx, module, builder);

    if (variant_type->data.T_CONS.num_args > 0) {
      // printf("compare\n");
      // print_type(payload_type);

      LLVMValueRef payloads_equal = _codegen_equality(
          payload_type, payload1, payload2, ctx, module, builder);

      phi_values[phi_count] = payloads_equal;
    } else {
      // No payload, just tags matching means equal
      phi_values[phi_count] = bool_const(llvm_ctx, 1);
    }

    phi_blocks[phi_count] = variant_block;
    phi_count++;

    LLVMBuildBr(builder, end_block);
  }

  // Build phi node to merge results
  LLVMPositionBuilderAtEnd(builder, end_block);
  LLVMValueRef result_phi =
      LLVMBuildPhi(builder, LLVMInt1TypeInContext(llvm_ctx), "eq_result");
  LLVMAddIncoming(result_phi, phi_values, phi_blocks, phi_count);

  return result_phi;
}
