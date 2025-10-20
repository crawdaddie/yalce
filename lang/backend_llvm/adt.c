#include "backend_llvm/adt.h"
#include "types.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/Types.h"
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

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

  if (CHARS_EQ(enum_type->data.T_CONS.name, TYPE_NAME_VARIANT) &&
      (enum_type->data.T_CONS.num_args == 2) &&
      (strcmp(enum_type->data.T_CONS.args[0]->data.T_CONS.name, "Some") == 0) &&
      (strcmp(enum_type->data.T_CONS.args[1]->data.T_CONS.name, "None") == 0)) {

    if (strcmp(mem_name, "None") == 0) {
      return codegen_none(builder);
    }
  }
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

LLVMValueRef codegen_adt_member_with_args(Type *enum_type, LLVMTypeRef tu_type,
                                          Ast *app, const char *mem_name,
                                          JITLangCtx *ctx, LLVMModuleRef module,
                                          LLVMBuilderRef builder) {

  int i = 0;
  while (strcmp(mem_name, enum_type->data.T_CONS.args[i]->data.T_CONS.name) !=
         0) {
    i++;
  }

  LLVMValueRef some = LLVMGetUndef(tu_type);
  int num = 0;
  some = LLVMBuildInsertValue(builder, some, LLVMConstInt(LLVMInt8Type(), i, 0),
                              0, "insert Some tag");

  // Get the union field type (the second field of your struct)
  LLVMTypeRef union_type = LLVMStructGetTypeAtIndex(tu_type, 1);
  LLVMValueRef union_value = LLVMGetUndef(union_type);

  // For "Accept of Int" - insert the integer into the first field of the union
  // Codegen the integer argument
  LLVMValueRef val =
      codegen(app->data.AST_APPLICATION.args, ctx, module, builder);

  // Insert the integer into the first position of the union struct
  union_value =
      LLVMBuildInsertValue(builder, union_value, val, 0, "insert int arg");

  // Insert the populated union into the main struct
  some = LLVMBuildInsertValue(builder, some, union_value, 1,
                              "insert variant data");

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
                             size_t count, LLVMTargetDataRef target_data) {
  if (!types || count == 0 || !target_data) {
    return NULL;
  }

  LLVMTypeRef largest_type = types[0];
  unsigned largest_size = LLVMStoreSizeOfType(target_data, largest_type);
  unsigned largest_align = 256;

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

  return largest_type;
}

LLVMTypeRef codegen_option_struct_type(LLVMTypeRef type) {
  LLVMTypeRef tu_types[] = {OPTION_TAG_TYPE, type};
  LLVMTypeRef tu_type = LLVMStructType(tu_types, 2, 0);
  return tu_type;
}

LLVMTypeRef codegen_adt_type(Type *type, JITLangCtx *ctx,
                             LLVMModuleRef module) {
  if (type->alias != NULL && strcmp(type->alias, "Option") == 0) {
    Type *_underlying = type_of_option(type);
    if (is_generic(_underlying)) {
      _underlying = resolve_type_in_env(_underlying, ctx->env);
    }
    LLVMTypeRef underlying;

    if (_underlying->kind == T_FN) {
      underlying = GENERIC_PTR;
    } else {
      underlying = type_to_llvm_type(_underlying, ctx, module);
    }

    return codegen_option_struct_type(underlying);
  }

  int len = type->data.T_CONS.num_args;
  LLVMTypeRef contained_types[len];
  for (int i = 0; i < type->data.T_CONS.num_args; i++) {
    Type *mem = type->data.T_CONS.args[i];
    contained_types[i] = type_to_llvm_type(mem, ctx, module);
  }

  LLVMTypeRef largest_type =
      get_largest_type(LLVMGetModuleContext(module), contained_types, len,
                       LLVMGetModuleDataLayout(module));

  return STRUCT_TY(2, TAG_TYPE, largest_type);
}

LLVMValueRef codegen_some(LLVMValueRef val, LLVMBuilderRef builder) {
  LLVMTypeRef tu_types[] = {OPTION_TAG_TYPE, LLVMTypeOf(val)};
  LLVMTypeRef tu_type = LLVMStructType(tu_types, 2, 0);
  LLVMValueRef some = LLVMGetUndef(tu_type);
  some = LLVMBuildInsertValue(
      builder, some, LLVMConstInt(OPTION_TAG_TYPE, 0, 0), 0, "insert Some tag");

  some = LLVMBuildInsertValue(builder, some, val, 1, "insert Some Value");
  return some;
}

LLVMValueRef codegen_none(LLVMBuilderRef builder) {
  LLVMTypeRef tu_type = STRUCT_TY(2, OPTION_TAG_TYPE, LLVMInt8Type());
  LLVMValueRef none =
      STRUCT(tu_type, builder, 2, LLVMConstInt(OPTION_TAG_TYPE, 1, 0),
             LLVMConstInt(LLVMInt32Type(), 0, 0));

  return none;
}

LLVMValueRef codegen_none_typed(LLVMBuilderRef builder, LLVMTypeRef type) {
  LLVMTypeRef tu_types[] = {OPTION_TAG_TYPE, type};
  LLVMTypeRef tu_type = LLVMStructType(tu_types, 2, 0);
  LLVMValueRef none = LLVMGetUndef(tu_type);

  none = LLVMBuildInsertValue(
      builder, none, LLVMConstInt(OPTION_TAG_TYPE, 1, 0), 0, "insert None tag");

  none = LLVMBuildInsertValue(builder, none, LLVMGetUndef(type), 1,
                              "insert None dummy val");

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
      LLVMBuildStructGEP2(builder, union_type, tu_alloca, 0, "tag_ptr");

  LLVMValueRef tag = LLVMBuildLoad2(builder, TAG_TYPE, tag_ptr, "tag");
  return tag;
}

LLVMValueRef codegen_option_is_none(LLVMValueRef opt, LLVMBuilderRef builder) {
  LLVMValueRef tag = extract_tag(opt, builder);
  return LLVMBuildICmp(builder, LLVMIntEQ, tag,
                       LLVMConstInt(OPTION_TAG_TYPE, 1, 0), "");
}

LLVMValueRef codegen_option_is_some(LLVMValueRef opt, LLVMBuilderRef builder) {
  LLVMValueRef tag = extract_tag(opt, builder);
  return LLVMBuildICmp(builder, LLVMIntEQ, tag,
                       LLVMConstInt(OPTION_TAG_TYPE, 0, 0), "");
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
  LLVMValueRef is_none = LLVMBuildICmp(builder, LLVMIntEQ, tag,
                                       LLVMConstInt(OPTION_TAG_TYPE, 1, 0), "");

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

  Type *mapper_type = ast->data.AST_APPLICATION.args->md;

  LLVMTypeRef llvm_mapper_type = type_to_llvm_type(mapper_type, ctx, module);

  Type *mapped_type = mapper_type->data.T_FN.to;
  LLVMTypeRef llvm_mapped_type = type_to_llvm_type(mapped_type, ctx, module);

  LLVMValueRef opt_val =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(current_block);

  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");

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
