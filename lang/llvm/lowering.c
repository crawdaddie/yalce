#include "./lowering.h"
#include "backend_llvm/adt.h"
#include "backend_llvm/array.h"
#include "backend_llvm/list.h"
#include "backend_llvm/types.h"
#include "escape_analysis.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  LLVMValueRef *items;
  LLVMValueRef *slots;
  LLVMTypeRef *slot_types;
  size_t len;
} MirLlvmValueMap;

typedef struct {
  LLVMBasicBlockRef *items;
  size_t len;
} MirLlvmBlockMap;

typedef struct {
  MirProgram *program;
  JITLangCtx jit_ctx;
  LLVMValueRef *functions;
  LLVMTypeRef *function_types;
  size_t functions_len;
} MirLlvmCtx;

static LLVMValueRef lower_mir_void_stub(LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef ret = LLVMVoidTypeInContext(llvm_ctx);
  LLVMTypeRef func_type = LLVMFunctionType(ret, NULL, 0, 0);
  LLVMValueRef top = LLVMAddFunction(module, "top", func_type);
  if (!top) {
    return NULL;
  }

  LLVMSetLinkage(top, LLVMExternalLinkage);
  LLVMBasicBlockRef block =
      LLVMAppendBasicBlockInContext(llvm_ctx, top, "entry");
  LLVMPositionBuilderAtEnd(builder, block);
  LLVMBuildRetVoid(builder);
  return top;
}

static Type *mir_function_return_type(MirFunction *fn) {
  if (!fn) {
    return NULL;
  }

  Type *type = fn->type;
  for (size_t i = 0; type && type->kind == T_FN && i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (param && param->name && strcmp(param->name, "$env") == 0 &&
        !(type->data.T_FN.from && param->type &&
          types_equal(type->data.T_FN.from, param->type))) {
      continue;
    }
    type = type->data.T_FN.to;
  }

  return type;
}

static LLVMTypeRef lower_mir_type(Type *type, JITLangCtx *ctx,
                                  LLVMModuleRef module, LLVMTypeRef fallback) {
  LLVMTypeRef llvm_type = type_to_llvm_type(type, ctx, module);
  return llvm_type ? llvm_type : fallback;
}

static LLVMTypeRef lower_mir_closure_value_type(LLVMModuleRef module) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef closure_type = LLVMGetTypeByName2(llvm_ctx, "Closure");
  if (!closure_type) {
    closure_type = LLVMStructCreateNamed(llvm_ctx, "Closure");
    LLVMTypeRef generic_ptr =
        LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0);
    LLVMTypeRef fields[] = {generic_ptr, generic_ptr};
    LLVMStructSetBody(closure_type, fields, 2, 0);
  }
  return closure_type;
}

static LLVMTypeRef lower_mir_abi_value_type(Type *type, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMTypeRef fallback) {
  if (type && type->kind == T_FN && !is_closure(type)) {
    return LLVMPointerType(LLVMInt8TypeInContext(LLVMGetModuleContext(module)),
                           0);
  }
  if (type && is_closure(type)) {
    return lower_mir_closure_value_type(module);
  }
  return lower_mir_type(type, ctx, module, fallback);
}

static bool lower_mir_type_has_unresolved_vars(Type *type) {
  if (!type) {
    return false;
  }

  switch (type->kind) {
  case T_VAR:
    return !type->is_recursive_type_ref;
  case T_FN:
    return lower_mir_type_has_unresolved_vars(type->closure_meta) ||
           lower_mir_type_has_unresolved_vars(type->data.T_FN.from) ||
           lower_mir_type_has_unresolved_vars(type->data.T_FN.to);
  case T_CONS:
  case T_SUM:
    if (lower_mir_type_has_unresolved_vars(type->closure_meta)) {
      return true;
    }
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (lower_mir_type_has_unresolved_vars(type->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  default:
    return false;
  }
}

static LLVMValueRef mir_llvm_value_get_rvalue(MirLlvmValueMap *values,
                                              MirValueId value,
                                              LLVMBuilderRef builder) {
  if (!values || value == MIR_NO_VALUE || value >= values->len) {
    return NULL;
  }

  if (values->slots && values->slots[value]) {
    LLVMTypeRef type = values->slot_types ? values->slot_types[value] : NULL;
    if (!type) {
      return NULL;
    }
    return LLVMBuildLoad2(builder, type, values->slots[value], "slot.load");
  }

  return values->items[value];
}

static bool mir_llvm_value_set(MirLlvmValueMap *values, MirValueId value,
                               LLVMValueRef llvm_value) {
  if (!values || value == MIR_NO_VALUE || value >= values->len || !llvm_value) {
    return false;
  }
  values->items[value] = llvm_value;
  return true;
}

static bool mir_llvm_value_set_slot(MirLlvmValueMap *values, MirValueId value,
                                    LLVMValueRef slot, LLVMTypeRef slot_type) {
  if (!values || !values->slots || !values->slot_types ||
      value == MIR_NO_VALUE || value >= values->len || !slot || !slot_type) {
    return false;
  }

  values->slots[value] = slot;
  values->slot_types[value] = slot_type;
  values->items[value] = slot;
  return true;
}

static LLVMBasicBlockRef mir_llvm_block_get(MirLlvmBlockMap *blocks,
                                            MirBlockId block_id) {
  if (!blocks || block_id == MIR_NO_BLOCK || block_id >= blocks->len) {
    return NULL;
  }
  return blocks->items[block_id];
}

static unsigned mir_llvm_int_width(Type *type, JITLangCtx *ctx,
                                   LLVMModuleRef module) {
  if (!type) {
    return 0;
  }

  LLVMTypeRef llvm_type = lower_mir_type(type, ctx, module, NULL);
  if (!llvm_type || LLVMGetTypeKind(llvm_type) != LLVMIntegerTypeKind) {
    return 0;
  }

  return LLVMGetIntTypeWidth(llvm_type);
}

static bool mir_llvm_is_integral_type(Type *type, JITLangCtx *ctx,
                                      LLVMModuleRef module) {
  return mir_llvm_int_width(type, ctx, module) != 0;
}

static bool mir_llvm_is_signed_integral_type(Type *type) {
  return type && type->kind == T_INT;
}

static LLVMTypeRef lower_mir_generic_ptr_type(LLVMModuleRef module) {
  return LLVMPointerType(LLVMInt8TypeInContext(LLVMGetModuleContext(module)),
                         0);
}

static LLVMTypeRef lower_mir_closure_env_record_type(Type *env_type,
                                                     JITLangCtx *ctx,
                                                     LLVMModuleRef module) {
  size_t num_fields = 0;
  if (env_type && (env_type->kind == T_CONS || env_type->kind == T_SUM) &&
      env_type->data.T_CONS.num_args > 0) {
    num_fields = (size_t)env_type->data.T_CONS.num_args;
  }

  LLVMTypeRef *field_types = NULL;
  if (num_fields > 0) {
    field_types = calloc(num_fields, sizeof(LLVMTypeRef));
    if (!field_types) {
      return NULL;
    }
  }

  for (size_t i = 0; i < num_fields; i++) {
    Type *field_type = env_type->data.T_CONS.args
                           ? env_type->data.T_CONS.args[i]
                           : NULL;
    if (field_type && field_type->kind == T_FN && !is_closure(field_type)) {
      field_types[i] = lower_mir_generic_ptr_type(module);
    } else {
      field_types[i] = lower_mir_type(field_type, ctx, module,
                                      lower_mir_generic_ptr_type(module));
    }
  }

  LLVMTypeRef record = LLVMStructTypeInContext(
      LLVMGetModuleContext(module), field_types, (unsigned)num_fields, 0);
  free(field_types);
  return record;
}

static LLVMTypeRef lower_mir_closure_env_ptr_type(Type *env_type,
                                                  JITLangCtx *ctx,
                                                  LLVMModuleRef module) {
  LLVMTypeRef record = lower_mir_closure_env_record_type(env_type, ctx, module);
  return record ? LLVMPointerType(record, 0) : NULL;
}

static LLVMTypeRef lower_mir_value_storage_type(Type *type, JITLangCtx *ctx,
                                                LLVMModuleRef module) {
  if (!type) {
    return NULL;
  }

  if (type->kind == T_FN && !is_closure(type)) {
    return lower_mir_generic_ptr_type(module);
  }
  if (is_closure(type)) {
    return lower_mir_closure_value_type(module);
  }
  return lower_mir_type(type, ctx, module, NULL);
}

static LLVMValueRef lower_mir_cast_value_to_storage(LLVMValueRef value,
                                                    Type *type,
                                                    JITLangCtx *ctx,
                                                    LLVMModuleRef module,
                                                    LLVMBuilderRef builder,
                                                    const char *name) {
  if (!value || !type) {
    return value;
  }

  LLVMTypeRef storage_type = lower_mir_value_storage_type(type, ctx, module);
  if (!storage_type || LLVMTypeOf(value) == storage_type) {
    return value;
  }

  if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMPointerTypeKind &&
      LLVMGetTypeKind(storage_type) == LLVMPointerTypeKind) {
    return LLVMBuildBitCast(builder, value, storage_type,
                            name ? name : "storage.cast");
  }

  return value;
}

static bool lower_mir_param_is_lowered(MirParam *param) {
  return param && param->type && param->type->kind != T_VOID;
}

static bool lower_mir_param_is_env(MirParam *param) {
  return param && param->name && strcmp(param->name, "$env") == 0;
}

static LLVMTypeRef lower_mir_param_abi_type(MirParam *param, JITLangCtx *ctx,
                                            LLVMModuleRef module) {
  if (lower_mir_param_is_env(param)) {
    return lower_mir_closure_env_ptr_type(param->type, ctx, module);
  }
  return lower_mir_abi_value_type(param ? param->type : NULL, ctx, module,
                                  NULL);
}

static const char *lower_mir_function_symbol_name(MirFunction *fn) {
  if (!fn || !fn->name) {
    return "<anonymous>";
  }
  return strcmp(fn->name, "$top") == 0 ? "top" : fn->name;
}

static LLVMTypeRef lower_mir_function_type(MirFunction *fn, JITLangCtx *ctx,
                                           LLVMModuleRef module) {
  if (!fn) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  Type *ret_type = mir_function_return_type(fn);
  LLVMTypeRef llvm_ret_type = lower_mir_abi_value_type(
      ret_type, ctx, module, LLVMVoidTypeInContext(llvm_ctx));
  if (!llvm_ret_type) {
    return NULL;
  }

  size_t lowered_params = 0;
  for (size_t i = 0; i < fn->params.len; i++) {
    if (lower_mir_param_is_lowered(&fn->params.items[i])) {
      lowered_params++;
    }
  }

  LLVMTypeRef *param_types = NULL;
  if (lowered_params > 0) {
    param_types = calloc(lowered_params, sizeof(LLVMTypeRef));
    if (!param_types) {
      return NULL;
    }
  }

  size_t llvm_param = 0;
  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (!lower_mir_param_is_lowered(param)) {
      continue;
    }

    param_types[llvm_param] = lower_mir_param_abi_type(param, ctx, module);
    if (!param_types[llvm_param]) {
      free(param_types);
      return NULL;
    }
    llvm_param++;
  }

  LLVMTypeRef fn_type =
      LLVMFunctionType(llvm_ret_type, param_types, (unsigned)lowered_params, 0);
  free(param_types);
  return fn_type;
}

static LLVMValueRef lower_mir_declare_function(MirLlvmCtx *lctx,
                                               MirFunction *fn,
                                               LLVMModuleRef module) {
  if (!lctx || !fn || fn->id >= lctx->functions_len) {
    return NULL;
  }
  if (lower_mir_type_has_unresolved_vars(fn->type)) {
    return NULL;
  }

  LLVMTypeRef fn_type = lower_mir_function_type(fn, &lctx->jit_ctx, module);
  if (!fn_type) {
    return NULL;
  }

  const char *name = lower_mir_function_symbol_name(fn);
  LLVMValueRef llvm_fn = LLVMGetNamedFunction(module, name);
  if (!llvm_fn) {
    llvm_fn = LLVMAddFunction(module, name, fn_type);
  }
  if (!llvm_fn) {
    return NULL;
  }

  LLVMSetLinkage(llvm_fn, LLVMExternalLinkage);
  lctx->functions[fn->id] = llvm_fn;
  lctx->function_types[fn->id] = fn_type;
  return llvm_fn;
}

static MirFunction *lower_mir_find_function_by_name(MirProgram *program,
                                                    const char *name) {
  if (!program || !name) {
    return NULL;
  }

  const char *needle = name[0] == '$' ? name + 1 : name;
  for (size_t i = 0; i < program->functions.len; i++) {
    MirFunction *fn = program->functions.items ? program->functions.items[i]
                                               : NULL;
    if (!fn || !fn->name) {
      continue;
    }

    const char *candidate = fn->name[0] == '$' ? fn->name + 1 : fn->name;
    if (strcmp(candidate, needle) == 0) {
      return fn;
    }
  }

  return NULL;
}

static bool lower_mir_value_allocates_on_stack(MirFunction *fn,
                                               MirValueId value) {
  EscapeMeta *meta = mir_value_escape_meta(fn, value);
  return meta && meta->status == EA_STACK_ALLOC;
}

static LLVMValueRef lower_mir_fn_ref(MirInstr *instr, MirLlvmCtx *lctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  if (!instr || !lctx || instr->data.fn_ref.fn >= lctx->functions_len) {
    return NULL;
  }

  LLVMValueRef fn = lctx->functions[instr->data.fn_ref.fn];
  if (!fn) {
    MirFunction *target =
        lctx->program && instr->data.fn_ref.fn < lctx->program->functions.len
            ? lctx->program->functions.items[instr->data.fn_ref.fn]
            : NULL;
    if (!target || !lower_mir_type_has_unresolved_vars(target->type)) {
      return NULL;
    }

    LLVMTypeRef storage_type =
        lower_mir_value_storage_type(instr->type, &lctx->jit_ctx, module);
    if (!storage_type) {
      storage_type = lower_mir_generic_ptr_type(module);
    }
    return LLVMConstNull(storage_type);
  }

  LLVMTypeRef storage_type =
      lower_mir_value_storage_type(instr->type, &lctx->jit_ctx, module);
  if (storage_type && LLVMTypeOf(fn) != storage_type) {
    return LLVMBuildBitCast(builder, fn, storage_type, "fn.ref");
  }
  return fn;
}

static LLVMValueRef lower_mir_const(MirInstr *instr, LLVMModuleRef module,
                                    JITLangCtx *ctx) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);

  if (!instr || instr->kind != MIR_CONST) {
    return NULL;
  }

  switch (instr->data.const_value.kind) {
  case MIR_CONST_KIND_INT: {
    LLVMTypeRef type = lower_mir_type(instr->type, ctx, module,
                                      LLVMInt32TypeInContext(llvm_ctx));
    return LLVMConstInt(
        type, (uint64_t)(int64_t)instr->data.const_value.as.int_value, true);
  }
  case MIR_CONST_KIND_UINT64: {
    LLVMTypeRef type = lower_mir_type(instr->type, ctx, module,
                                      LLVMInt64TypeInContext(llvm_ctx));
    return LLVMConstInt(type, instr->data.const_value.as.uint64_value, false);
  }
  case MIR_CONST_KIND_FLOAT: {
    LLVMTypeRef type = lower_mir_type(instr->type, ctx, module,
                                      LLVMFloatTypeInContext(llvm_ctx));
    return LLVMConstReal(type, instr->data.const_value.as.float_value);
  }
  case MIR_CONST_KIND_DOUBLE: {
    LLVMTypeRef type = lower_mir_type(instr->type, ctx, module,
                                      LLVMDoubleTypeInContext(llvm_ctx));
    return LLVMConstReal(type, instr->data.const_value.as.double_value);
  }
  case MIR_CONST_KIND_CHAR: {
    LLVMTypeRef type = lower_mir_type(instr->type, ctx, module,
                                      LLVMInt8TypeInContext(llvm_ctx));
    return LLVMConstInt(
        type, (uint64_t)(uint8_t)instr->data.const_value.as.char_value, false);
  }
  case MIR_CONST_KIND_BOOL: {
    LLVMTypeRef type = lower_mir_type(instr->type, ctx, module,
                                      LLVMInt1TypeInContext(llvm_ctx));
    return LLVMConstInt(type,
                        instr->data.const_value.as.bool_value ? 1 : 0, false);
  }
  case MIR_CONST_KIND_VOID:
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  case MIR_CONST_KIND_STRING:
    return NULL;
  default:
    return NULL;
  }
}

static LLVMValueRef lower_mir_array_literal(MirFunction *fn, MirInstr *instr,
                                            MirLlvmValueMap *values,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder,
                                            JITLangCtx *ctx) {
  if (!instr || !instr->type || !is_array_type(instr->type) ||
      !instr->type->data.T_CONS.args || instr->type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  Type *element_type_ref = instr->type->data.T_CONS.args[0];
  LLVMTypeRef element_type =
      lower_mir_value_storage_type(element_type_ref, ctx, module);
  if (!element_type) {
    return NULL;
  }

  LLVMTypeRef array_type = codegen_array_type(element_type);
  LLVMValueRef array_struct = LLVMGetUndef(array_type);
  LLVMValueRef size = LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx),
                                   instr->data.construct.items.len, false);
  array_struct =
      LLVMBuildInsertValue(builder, array_struct, size, 0, "array.size");

  if (instr->data.construct.items.len == 0) {
    LLVMValueRef null_data = LLVMConstNull(LLVMPointerType(element_type, 0));
    return LLVMBuildInsertValue(builder, array_struct, null_data, 1,
                                "array.data");
  }

  LLVMTypeRef backing_type = LLVMArrayType(
      element_type, (unsigned)instr->data.construct.items.len);
  LLVMValueRef data =
      lower_mir_value_allocates_on_stack(fn, instr->result)
          ? LLVMBuildAlloca(builder, backing_type, "array.data.stack")
          : LLVMBuildMalloc(builder, backing_type, "array.data.heap");
  if (!data) {
    return NULL;
  }

  for (size_t i = 0; i < instr->data.construct.items.len; i++) {
    MirValueId item_id = instr->data.construct.items.items[i];
    LLVMValueRef item = mir_llvm_value_get_rvalue(values, item_id, builder);
    if (!item) {
      return NULL;
    }

    if (element_type_ref && element_type_ref->kind == T_FN &&
        LLVMTypeOf(item) != element_type) {
      item = LLVMBuildBitCast(builder, item, element_type, "array.fn.cast");
    }

    LLVMValueRef index =
        LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), (uint64_t)i, false);
    LLVMValueRef item_ptr =
        LLVMBuildGEP2(builder, element_type, data, (LLVMValueRef[]){index}, 1,
                      "array.item.ptr");
    LLVMBuildStore(builder, item, item_ptr);
  }

  return LLVMBuildInsertValue(builder, array_struct, data, 1, "array.data");
}

static LLVMValueRef lower_mir_array_size(MirFunction *fn, MirInstr *instr,
                                         MirLlvmValueMap *values,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         JITLangCtx *ctx) {
  if (!fn || !instr || !values) {
    return NULL;
  }

  MirValueId array_id = instr->data.op.operands[0];
  Type *array_type = mir_function_value_type(fn, array_id);
  if (!array_type || !is_array_type(array_type) ||
      !array_type->data.T_CONS.args || array_type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  LLVMValueRef array = mir_llvm_value_get_rvalue(values, array_id, builder);
  LLVMTypeRef element_type =
      lower_mir_value_storage_type(array_type->data.T_CONS.args[0], ctx,
                                   module);
  if (!array || !element_type) {
    return NULL;
  }

  return codegen_get_array_size(builder, array, element_type);
}

static LLVMTypeRef lower_mir_array_element_storage_type(MirFunction *fn,
                                                        MirValueId array_id,
                                                        LLVMModuleRef module,
                                                        JITLangCtx *ctx) {
  Type *array_type = mir_function_value_type(fn, array_id);
  if (!array_type || !is_array_type(array_type) ||
      !array_type->data.T_CONS.args || array_type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  return lower_mir_value_storage_type(array_type->data.T_CONS.args[0], ctx,
                                      module);
}

static LLVMValueRef lower_mir_array_at(MirFunction *fn, MirInstr *instr,
                                       MirLlvmValueMap *values,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder,
                                       JITLangCtx *ctx) {
  if (!fn || !instr || !values) {
    return NULL;
  }

  LLVMTypeRef element_type = lower_mir_array_element_storage_type(
      fn, instr->data.extract.value, module, ctx);
  LLVMValueRef array =
      mir_llvm_value_get_rvalue(values, instr->data.extract.value, builder);
  LLVMValueRef index = mir_llvm_value_get_rvalue(
      values, instr->data.extract.index_value, builder);
  if (!element_type || !array || !index) {
    return NULL;
  }

  return get_array_element(builder, array, index, element_type);
}

static LLVMValueRef lower_mir_array_set(MirFunction *fn, MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx) {
  if (!fn || !instr || !values) {
    return NULL;
  }

  LLVMTypeRef element_type = lower_mir_array_element_storage_type(
      fn, instr->data.op.operands[0], module, ctx);
  LLVMValueRef array =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
  LLVMValueRef index =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[1], builder);
  LLVMValueRef value =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[2], builder);
  Type *value_type = mir_function_value_type(fn, instr->data.op.operands[2]);
  value = lower_mir_cast_value_to_storage(value, value_type, ctx, module,
                                          builder, "array.set.cast");
  if (!element_type || !array || !index || !value) {
    return NULL;
  }

  return set_array_element(builder, array, index, value, element_type);
}

static LLVMValueRef lower_mir_list_empty(MirInstr *instr, LLVMModuleRef module,
                                         JITLangCtx *ctx) {
  LLVMTypeRef list_type = lower_mir_type(instr->type, ctx, module, NULL);
  return list_type ? LLVMConstNull(list_type) : NULL;
}

static LLVMValueRef lower_mir_list_cons(MirFunction *fn, MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx) {
  if (!instr || !instr->type || !is_list_type(instr->type)) {
    return NULL;
  }

  Type *element_type_ref = type_of_list(instr->type);
  LLVMTypeRef element_type =
      lower_mir_value_storage_type(element_type_ref, ctx, module);
  if (!element_type) {
    return NULL;
  }

  LLVMValueRef head =
      mir_llvm_value_get_rvalue(values, instr->data.construct.operands[0], builder);
  LLVMValueRef tail =
      mir_llvm_value_get_rvalue(values, instr->data.construct.operands[1], builder);
  if (!head || !tail) {
    return NULL;
  }

  if (element_type_ref && element_type_ref->kind == T_FN &&
      LLVMTypeOf(head) != element_type) {
    head = LLVMBuildBitCast(builder, head, element_type, "list.fn.cast");
  }
  LLVMTypeRef generic_ptr = lower_mir_generic_ptr_type(module);
  if (LLVMTypeOf(tail) != generic_ptr) {
    tail = LLVMBuildBitCast(builder, tail, generic_ptr, "list.tail.cast");
  }

  LLVMTypeRef node_type = llnode_type(element_type);
  LLVMValueRef node =
      lower_mir_value_allocates_on_stack(fn, instr->result)
          ? LLVMBuildAlloca(builder, node_type, "list.node.stack")
          : LLVMBuildMalloc(builder, node_type, "list.node.heap");
  if (!node) {
    return NULL;
  }

  LLVMValueRef head_ptr =
      LLVMBuildStructGEP2(builder, node_type, node, 0, "list.head.ptr");
  LLVMBuildStore(builder, head, head_ptr);

  LLVMValueRef tail_ptr =
      LLVMBuildStructGEP2(builder, node_type, node, 1, "list.tail.ptr");
  LLVMBuildStore(builder, tail, tail_ptr);
  return node;
}

static LLVMValueRef lower_mir_tuple(MirFunction *fn, MirInstr *instr,
                                    MirLlvmValueMap *values,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder,
                                    JITLangCtx *ctx) {
  if (!instr || !instr->type) {
    return NULL;
  }

  LLVMTypeRef tuple_type = lower_mir_type(instr->type, ctx, module, NULL);
  if (!tuple_type) {
    return NULL;
  }

  LLVMValueRef tuple = LLVMGetUndef(tuple_type);
  for (size_t i = 0; i < instr->data.construct.items.len; i++) {
    MirValueId item_id = instr->data.construct.items.items[i];
    LLVMValueRef item = mir_llvm_value_get_rvalue(values, item_id, builder);
    if (!item) {
      return NULL;
    }

    Type *item_type = mir_function_value_type(fn, item_id);
    if (item_type && item_type->kind == T_FN) {
      LLVMTypeRef storage_type =
          lower_mir_value_storage_type(item_type, ctx, module);
      if (storage_type && LLVMTypeOf(item) != storage_type) {
        item = LLVMBuildBitCast(builder, item, storage_type, "tuple.fn.cast");
      }
    }

    tuple = LLVMBuildInsertValue(builder, tuple, item, (unsigned)i, "tuple");
  }

  return tuple;
}

static LLVMValueRef lower_mir_constructor_payload_value(
    MirFunction *fn, MirInstr *instr, MirLlvmValueMap *values,
    LLVMModuleRef module, LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!instr || !instr->data.construct.constructor_type) {
    return NULL;
  }

  Type *constructor_type = instr->data.construct.constructor_type;
  LLVMTypeRef payload_type =
      lower_mir_value_storage_type(constructor_type, ctx, module);
  if (!payload_type) {
    return NULL;
  }

  if (instr->data.construct.items.len == 1 &&
      LLVMGetTypeKind(payload_type) != LLVMStructTypeKind) {
    MirValueId field_id = instr->data.construct.items.items[0];
    LLVMValueRef field = mir_llvm_value_get_rvalue(values, field_id, builder);
    Type *field_type = mir_function_value_type(fn, field_id);
    return lower_mir_cast_value_to_storage(field, field_type, ctx, module,
                                           builder, "variant.field.cast");
  }

  LLVMValueRef payload = LLVMGetUndef(payload_type);
  for (size_t i = 0; i < instr->data.construct.items.len; i++) {
    MirValueId field_id = instr->data.construct.items.items[i];
    LLVMValueRef field = mir_llvm_value_get_rvalue(values, field_id, builder);
    if (!field) {
      return NULL;
    }

    Type *field_type = NULL;
    if (constructor_type->kind == T_CONS && constructor_type->data.T_CONS.args &&
        i < (size_t)constructor_type->data.T_CONS.num_args) {
      field_type = constructor_type->data.T_CONS.args[i];
    }
    if (!field_type) {
      field_type = mir_function_value_type(fn, field_id);
    }

    field = lower_mir_cast_value_to_storage(
        field, field_type, ctx, module, builder, "variant.field.cast");
    payload =
        LLVMBuildInsertValue(builder, payload, field, (unsigned)i, "variant");
  }

  return payload;
}

static LLVMValueRef lower_mir_pack_union_payload(LLVMValueRef payload,
                                                 LLVMTypeRef union_type,
                                                 LLVMBuilderRef builder) {
  if (!payload || !union_type) {
    return NULL;
  }

  LLVMValueRef storage =
      LLVMBuildAlloca(builder, union_type, "variant.payload.storage");
  LLVMValueRef payload_ptr = LLVMBuildBitCast(
      builder, storage, LLVMPointerType(LLVMTypeOf(payload), 0),
      "variant.payload.ptr");
  LLVMBuildStore(builder, payload, payload_ptr);
  return LLVMBuildLoad2(builder, union_type, storage, "variant.payload.bytes");
}

static LLVMValueRef lower_mir_wrap_single_field_payload(
    LLVMValueRef payload, LLVMTypeRef desired_type, LLVMBuilderRef builder) {
  if (!payload || !desired_type ||
      LLVMGetTypeKind(desired_type) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(desired_type) != 1) {
    return NULL;
  }

  LLVMTypeRef field_type = LLVMStructGetTypeAtIndex(desired_type, 0);
  if (LLVMTypeOf(payload) != field_type) {
    if (LLVMGetTypeKind(LLVMTypeOf(payload)) != LLVMPointerTypeKind ||
        LLVMGetTypeKind(field_type) != LLVMPointerTypeKind) {
      return NULL;
    }
    payload = LLVMBuildBitCast(builder, payload, field_type,
                               "variant.payload.field.cast");
  }

  LLVMValueRef wrapped = LLVMGetUndef(desired_type);
  return LLVMBuildInsertValue(builder, wrapped, payload, 0,
                              "variant.payload");
}

static LLVMValueRef lower_mir_variant(MirFunction *fn, MirInstr *instr,
                                      MirLlvmValueMap *values,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder,
                                      JITLangCtx *ctx) {
  if (!instr || !instr->type || instr->data.construct.constructor_index < 0) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  if (is_simple_enum(instr->type)) {
    return LLVMConstInt(LLVMInt8TypeInContext(llvm_ctx),
                        (uint64_t)instr->data.construct.constructor_index, false);
  }

  LLVMTypeRef variant_type = lower_mir_type(instr->type, ctx, module, NULL);
  if (!variant_type ||
      LLVMGetTypeKind(variant_type) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(variant_type) < 1) {
    return NULL;
  }

  LLVMValueRef variant = LLVMGetUndef(variant_type);
  LLVMTypeRef tag_type = LLVMStructGetTypeAtIndex(variant_type, 0);
  variant = LLVMBuildInsertValue(
      builder, variant,
      LLVMConstInt(tag_type, (uint64_t)instr->data.construct.constructor_index,
                   false),
      0, "variant.tag");

  if (LLVMCountStructElementTypes(variant_type) < 2) {
    return variant;
  }

  LLVMTypeRef storage_type = LLVMStructGetTypeAtIndex(variant_type, 1);
  if (is_option_type(instr->type)) {
    LLVMValueRef payload = LLVMGetUndef(storage_type);
    if (instr->data.construct.items.len > 0) {
      MirValueId field_id = instr->data.construct.items.items[0];
      payload = mir_llvm_value_get_rvalue(values, field_id, builder);
      if (!payload) {
        return NULL;
      }

      Type *field_type = NULL;
      Type *constructor_type = instr->data.construct.constructor_type;
      if (constructor_type && constructor_type->kind == T_CONS &&
          constructor_type->data.T_CONS.args &&
          constructor_type->data.T_CONS.num_args > 0) {
        field_type = constructor_type->data.T_CONS.args[0];
      }
      if (!field_type) {
        field_type = mir_function_value_type(fn, field_id);
      }
      payload = lower_mir_cast_value_to_storage(
          payload, field_type, ctx, module, builder, "option.payload.cast");
    }
    return LLVMBuildInsertValue(builder, variant, payload, 1,
                                "variant.payload");
  }

  if (instr->data.construct.items.len == 0) {
    return LLVMBuildInsertValue(builder, variant, LLVMGetUndef(storage_type), 1,
                                "variant.payload");
  }

  LLVMValueRef payload = lower_mir_constructor_payload_value(
      fn, instr, values, module, builder, ctx);
  LLVMValueRef payload_bytes =
      lower_mir_pack_union_payload(payload, storage_type, builder);
  if (!payload_bytes) {
    return NULL;
  }

  return LLVMBuildInsertValue(builder, variant, payload_bytes, 1,
                              "variant.payload");
}

static LLVMTypeRef lower_mir_list_element_type(Type *list_type, JITLangCtx *ctx,
                                               LLVMModuleRef module) {
  Type *element_type = type_of_list(list_type);
  if (!element_type) {
    return NULL;
  }
  return lower_mir_value_storage_type(element_type, ctx, module);
}

static LLVMValueRef lower_mir_list_is_empty(MirFunction *fn, MirInstr *instr,
                                            MirLlvmValueMap *values,
                                            LLVMBuilderRef builder) {
  if (!fn || !instr) {
    return NULL;
  }

  LLVMValueRef list =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
  if (!list) {
    return NULL;
  }

  return LLVMBuildICmp(builder, LLVMIntEQ, list, LLVMConstNull(LLVMTypeOf(list)),
                       "list.is_empty");
}

static LLVMValueRef lower_mir_list_head(MirFunction *fn, MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx) {
  if (!fn || !instr) {
    return NULL;
  }

  LLVMValueRef list =
      mir_llvm_value_get_rvalue(values, instr->data.extract.value, builder);
  Type *list_type = mir_function_value_type(fn, instr->data.extract.value);
  LLVMTypeRef element_type =
      lower_mir_list_element_type(list_type, ctx, module);
  if (!list || !element_type) {
    return NULL;
  }

  LLVMTypeRef node_type = llnode_type(element_type);
  LLVMValueRef head_ptr =
      LLVMBuildStructGEP2(builder, node_type, list, 0, "list.head.ptr");
  return LLVMBuildLoad2(builder, element_type, head_ptr, "list.head");
}

static LLVMValueRef lower_mir_list_tail(MirFunction *fn, MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx) {
  if (!fn || !instr) {
    return NULL;
  }

  LLVMValueRef list =
      mir_llvm_value_get_rvalue(values, instr->data.extract.value, builder);
  Type *list_type = mir_function_value_type(fn, instr->data.extract.value);
  LLVMTypeRef element_type =
      lower_mir_list_element_type(list_type, ctx, module);
  if (!list || !element_type) {
    return NULL;
  }

  LLVMTypeRef node_type = llnode_type(element_type);
  LLVMValueRef tail_ptr =
      LLVMBuildStructGEP2(builder, node_type, list, 1, "list.tail.ptr");
  LLVMValueRef tail = LLVMBuildLoad2(
      builder, lower_mir_generic_ptr_type(module), tail_ptr, "list.tail");
  LLVMTypeRef list_llvm_type = lower_mir_type(instr->type, ctx, module, NULL);
  if (list_llvm_type && LLVMTypeOf(tail) != list_llvm_type) {
    tail = LLVMBuildBitCast(builder, tail, list_llvm_type, "list.tail.cast");
  }
  return tail;
}

static LLVMValueRef lower_mir_variant_tag(MirInstr *instr,
                                          MirLlvmValueMap *values,
                                          LLVMBuilderRef builder) {
  if (!instr) {
    return NULL;
  }

  LLVMValueRef value =
      mir_llvm_value_get_rvalue(values, instr->data.extract.value, builder);
  if (!value) {
    return NULL;
  }

  LLVMTypeRef value_type = LLVMTypeOf(value);
  if (LLVMGetTypeKind(value_type) == LLVMIntegerTypeKind) {
    return value;
  }
  return LLVMBuildExtractValue(builder, value, 0, "variant.tag");
}

static LLVMValueRef lower_mir_tag_eq(MirInstr *instr, MirLlvmValueMap *values,
                                     LLVMBuilderRef builder) {
  if (!instr) {
    return NULL;
  }

  LLVMValueRef tag =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
  if (!tag) {
    return NULL;
  }

  LLVMValueRef expected =
      LLVMConstInt(LLVMTypeOf(tag),
                   (uint64_t)instr->data.op.constructor_index, false);
  return LLVMBuildICmp(builder, LLVMIntEQ, tag, expected, "tag.eq");
}

static LLVMValueRef lower_mir_variant_payload(MirInstr *instr,
                                              MirLlvmValueMap *values,
                                              LLVMModuleRef module,
                                              LLVMBuilderRef builder,
                                              JITLangCtx *ctx) {
  if (!instr || !instr->type) {
    return NULL;
  }

  LLVMValueRef value = mir_llvm_value_get_rvalue(
      values, instr->data.extract.value, builder);
  if (!value) {
    return NULL;
  }

  LLVMTypeRef value_type = LLVMTypeOf(value);
  if (LLVMGetTypeKind(value_type) == LLVMStructTypeKind &&
      LLVMCountStructElementTypes(value_type) > 1) {
    LLVMValueRef payload = LLVMBuildExtractValue(builder, value, 1,
                                                 "variant.payload.raw");
    Type *payload_type = instr->type;

    LLVMTypeRef payload_llvm_type =
        lower_mir_value_storage_type(payload_type, ctx, module);
    if (!payload_llvm_type) {
      return NULL;
    }

    if (LLVMTypeOf(payload) == payload_llvm_type) {
      return payload;
    }

    LLVMValueRef wrapped = lower_mir_wrap_single_field_payload(
        payload, payload_llvm_type, builder);
    if (wrapped) {
      return wrapped;
    }

    return cast_union(payload, payload_type, ctx, module, builder);
  }

  return NULL;
}

static LLVMValueRef lower_mir_primitive_cast(MirInstr *instr,
                                             MirLlvmValueMap *values,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder,
                                             JITLangCtx *ctx) {
  LLVMValueRef value = mir_llvm_value_get_rvalue(
      values, instr->data.op.operands[0], builder);
  Type *from_type = instr->data.op.from_type;
  Type *to_type = instr->data.op.to_type;
  if (!value || !from_type || !to_type) {
    return NULL;
  }

  LLVMTypeRef to_llvm_type = lower_mir_type(to_type, ctx, module, NULL);
  if (!to_llvm_type) {
    return NULL;
  }

  if (from_type->kind == to_type->kind) {
    return value;
  }

  if (from_type->kind == T_NUM &&
      mir_llvm_is_integral_type(to_type, ctx, module)) {
    if (mir_llvm_is_signed_integral_type(to_type)) {
      return LLVMBuildFPToSI(builder, value, to_llvm_type, "cast");
    }
    return LLVMBuildFPToUI(builder, value, to_llvm_type, "cast");
  }

  if (mir_llvm_is_integral_type(from_type, ctx, module) &&
      to_type->kind == T_NUM) {
    if (mir_llvm_is_signed_integral_type(from_type)) {
      return LLVMBuildSIToFP(builder, value, to_llvm_type, "cast");
    }
    return LLVMBuildUIToFP(builder, value, to_llvm_type, "cast");
  }

  if (mir_llvm_is_integral_type(from_type, ctx, module) &&
      mir_llvm_is_integral_type(to_type, ctx, module)) {
    unsigned from_width = mir_llvm_int_width(from_type, ctx, module);
    unsigned to_width = mir_llvm_int_width(to_type, ctx, module);

    if (to_width == 1) {
      LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(value), 0, false);
      return LLVMBuildICmp(builder, LLVMIntNE, value, zero, "cast");
    }

    if (from_width < to_width) {
      if (mir_llvm_is_signed_integral_type(from_type)) {
        return LLVMBuildSExt(builder, value, to_llvm_type, "cast");
      }
      return LLVMBuildZExt(builder, value, to_llvm_type, "cast");
    }

    if (from_width > to_width) {
      return LLVMBuildTrunc(builder, value, to_llvm_type, "cast");
    }

    return LLVMBuildBitCast(builder, value, to_llvm_type, "cast");
  }

  return NULL;
}

static LLVMValueRef lower_mir_primitive(MirInstr *instr, MirLlvmValueMap *values,
                                        LLVMBuilderRef builder) {
  if (!instr || instr->kind != MIR_OP ||
      instr->data.op.kind != MIR_OP_KIND_PRIMITIVE) {
    return NULL;
  }

  if (instr->data.op.primitive == MIR_OP_LNOT) {
    if (instr->data.op.argc != 1) {
      return NULL;
    }
    LLVMValueRef value = mir_llvm_value_get_rvalue(
        values, instr->data.op.operands[0], builder);
    return value ? LLVMBuildNot(builder, value, "not") : NULL;
  }

  if (instr->data.op.argc != 2) {
    return NULL;
  }
  LLVMValueRef lhs = mir_llvm_value_get_rvalue(
      values, instr->data.op.operands[0], builder);
  LLVMValueRef rhs = mir_llvm_value_get_rvalue(
      values, instr->data.op.operands[1], builder);
  if (!lhs || !rhs) {
    return NULL;
  }

  switch (instr->data.op.primitive) {
  case MIR_OP_IADD:
  case MIR_OP_UADD:
    return LLVMBuildAdd(builder, lhs, rhs, "add");
  case MIR_OP_FADD:
    return LLVMBuildFAdd(builder, lhs, rhs, "add");
  case MIR_OP_ISUB:
  case MIR_OP_USUB:
    return LLVMBuildSub(builder, lhs, rhs, "sub");
  case MIR_OP_FSUB:
    return LLVMBuildFSub(builder, lhs, rhs, "sub");
  case MIR_OP_IMUL:
  case MIR_OP_UMUL:
    return LLVMBuildMul(builder, lhs, rhs, "mul");
  case MIR_OP_FMUL:
    return LLVMBuildFMul(builder, lhs, rhs, "mul");
  case MIR_OP_IDIV:
    return LLVMBuildSDiv(builder, lhs, rhs, "div");
  case MIR_OP_UDIV:
    return LLVMBuildUDiv(builder, lhs, rhs, "div");
  case MIR_OP_FDIV:
    return LLVMBuildFDiv(builder, lhs, rhs, "div");
  case MIR_OP_IMOD:
    return LLVMBuildSRem(builder, lhs, rhs, "mod");
  case MIR_OP_UMOD:
    return LLVMBuildURem(builder, lhs, rhs, "mod");
  case MIR_OP_FMOD:
    return LLVMBuildFRem(builder, lhs, rhs, "mod");

  case MIR_OP_IEQ:
  case MIR_OP_UEQ:
  case MIR_OP_CEQ:
  case MIR_OP_BEQ:
    return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eq");
  case MIR_OP_FEQ:
    return LLVMBuildFCmp(builder, LLVMRealOEQ, lhs, rhs, "eq");
  case MIR_OP_IGT:
  case MIR_OP_CGT:
    return LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "gt");
  case MIR_OP_UGT:
    return LLVMBuildICmp(builder, LLVMIntUGT, lhs, rhs, "gt");
  case MIR_OP_FGT:
    return LLVMBuildFCmp(builder, LLVMRealOGT, lhs, rhs, "gt");
  case MIR_OP_IGTE:
  case MIR_OP_CGTE:
    return LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "gte");
  case MIR_OP_UGTE:
    return LLVMBuildICmp(builder, LLVMIntUGE, lhs, rhs, "gte");
  case MIR_OP_FGTE:
    return LLVMBuildFCmp(builder, LLVMRealOGE, lhs, rhs, "gte");
  case MIR_OP_ILT:
  case MIR_OP_CLT:
    return LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "lt");
  case MIR_OP_ULT:
    return LLVMBuildICmp(builder, LLVMIntULT, lhs, rhs, "lt");
  case MIR_OP_FLT:
    return LLVMBuildFCmp(builder, LLVMRealOLT, lhs, rhs, "lt");
  case MIR_OP_ILTE:
  case MIR_OP_CLTE:
    return LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "lte");
  case MIR_OP_ULTE:
    return LLVMBuildICmp(builder, LLVMIntULE, lhs, rhs, "lte");
  case MIR_OP_FLTE:
    return LLVMBuildFCmp(builder, LLVMRealOLE, lhs, rhs, "lte");
  default:
    return NULL;
  }
}

static bool lower_mir_value_is_env(MirFunction *fn, MirValueId value) {
  if (!fn || value == MIR_NO_VALUE) {
    return false;
  }

  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (param->value == value && lower_mir_param_is_env(param)) {
      return true;
    }
  }

  MirInstr *def = mir_function_find_def_instr(fn, value);
  return def && ((def->kind == MIR_CONSTRUCT &&
                  def->data.construct.kind == MIR_CONSTRUCT_CLOSURE_ENV) ||
                 (def->kind == MIR_EXTRACT &&
                  def->data.extract.kind == MIR_EXTRACT_CLOSURE_ENV));
}

static LLVMTypeRef lower_mir_call_operand_abi_type(MirFunction *fn,
                                                   MirValueId operand,
                                                   Type *logical_type,
                                                   JITLangCtx *ctx,
                                                   LLVMModuleRef module) {
  if (lower_mir_value_is_env(fn, operand)) {
    return lower_mir_closure_env_ptr_type(logical_type, ctx, module);
  }
  return lower_mir_abi_value_type(logical_type, ctx, module, NULL);
}

static LLVMTypeRef lower_mir_extract_record_type(MirFunction *fn,
                                                 MirValueId value,
                                                 JITLangCtx *ctx,
                                                 LLVMModuleRef module) {
  Type *logical_type = mir_function_value_type(fn, value);
  if (lower_mir_value_is_env(fn, value)) {
    return lower_mir_closure_env_record_type(logical_type, ctx, module);
  }
  return lower_mir_type(logical_type, ctx, module, NULL);
}

static LLVMValueRef lower_mir_extract_field(MirFunction *fn, MirInstr *instr,
                                            MirLlvmValueMap *values,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder,
                                            JITLangCtx *ctx) {
  if (!fn || !instr || instr->kind != MIR_EXTRACT || !values ||
      instr->data.extract.kind != MIR_EXTRACT_FIELD) {
    return NULL;
  }

  MirValueId source_id = instr->data.extract.value;
  LLVMValueRef source = mir_llvm_value_get_rvalue(values, source_id, builder);
  if (!source) {
    return NULL;
  }

  LLVMTypeRef source_type = LLVMTypeOf(source);
  if (LLVMGetTypeKind(source_type) == LLVMStructTypeKind) {
    return LLVMBuildExtractValue(builder, source,
                                 (unsigned)instr->data.extract.index,
                                 "extract.field");
  }

  if (LLVMGetTypeKind(source_type) != LLVMPointerTypeKind) {
    return NULL;
  }

  LLVMTypeRef record_type =
      lower_mir_extract_record_type(fn, source_id, ctx, module);
  LLVMTypeRef field_type =
      lower_mir_value_storage_type(instr->type, ctx, module);
  if (!record_type || !field_type) {
    return NULL;
  }

  LLVMTypeRef record_ptr_type = LLVMPointerType(record_type, 0);
  if (LLVMTypeOf(source) != record_ptr_type) {
    source = LLVMBuildBitCast(builder, source, record_ptr_type,
                              "extract.field.container.cast");
  }

  LLVMValueRef field_ptr =
      LLVMBuildStructGEP2(builder, record_type, source,
                          (unsigned)instr->data.extract.index,
                          "extract.field.ptr");
  return LLVMBuildLoad2(builder, field_type, field_ptr, "extract.field");
}

static LLVMTypeRef lower_mir_indirect_call_type(MirFunction *fn,
                                                MirInstr *instr,
                                                Type *callee_type,
                                                Type *result_type,
                                                JITLangCtx *ctx,
                                                LLVMModuleRef module) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef ret_type = lower_mir_abi_value_type(
      result_type, ctx, module, LLVMVoidTypeInContext(llvm_ctx));
  if (!ret_type) {
    return NULL;
  }

  size_t operand_count = instr ? instr->data.call.operands.len : 0;
  LLVMTypeRef *param_types = NULL;
  if (operand_count > 0) {
    param_types = calloc(operand_count, sizeof(LLVMTypeRef));
    if (!param_types) {
      return NULL;
    }
  }

  Type *cursor = callee_type;
  size_t param_count = 0;
  size_t operand_index = 0;
  while (cursor && cursor->kind == T_FN && operand_index < operand_count) {
    Type *param_type = cursor->data.T_FN.from;
    MirValueId operand = instr->data.call.operands.items[operand_index++];
    if (param_type && param_type->kind != T_VOID) {
      param_types[param_count] = lower_mir_call_operand_abi_type(
          fn, operand, param_type, ctx, module);
      if (!param_types[param_count]) {
        free(param_types);
        return NULL;
      }
      param_count++;
    }
    cursor = cursor->data.T_FN.to;
  }

  if (operand_index < operand_count) {
    free(param_types);
    return NULL;
  }

  LLVMTypeRef fn_type =
      LLVMFunctionType(ret_type, param_types, (unsigned)param_count, 0);
  free(param_types);
  return fn_type;
}

static LLVMValueRef lower_mir_call(MirFunction *fn, MirInstr *instr,
                                   MirLlvmValueMap *values, MirLlvmCtx *lctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  if (!fn || !instr || !lctx) {
    return NULL;
  }

  LLVMValueRef callee = NULL;
  LLVMTypeRef callee_type = NULL;

  if (instr->data.call.specialized_fn != MIR_NO_FUNCTION &&
      instr->data.call.specialized_fn < lctx->functions_len) {
    callee = lctx->functions[instr->data.call.specialized_fn];
    callee_type = lctx->function_types[instr->data.call.specialized_fn];
  }

  if (!callee && instr->data.call.specialized_name) {
    MirFunction *specialized =
        lower_mir_find_function_by_name(lctx->program,
                                        instr->data.call.specialized_name);
    if (specialized && specialized->id < lctx->functions_len) {
      callee = lctx->functions[specialized->id];
      callee_type = lctx->function_types[specialized->id];
    }
  }

  if (!callee && instr->data.call.callee != MIR_NO_VALUE) {
    MirInstr *callee_def =
        mir_function_find_def_instr(fn, instr->data.call.callee);
    if (callee_def && callee_def->kind == MIR_FN_REF &&
        callee_def->data.fn_ref.fn < lctx->functions_len) {
      MirFunction *target =
          lctx->program &&
                  callee_def->data.fn_ref.fn < lctx->program->functions.len
              ? lctx->program->functions.items[callee_def->data.fn_ref.fn]
              : NULL;
      if (target && lower_mir_type_has_unresolved_vars(target->type)) {
        fprintf(stderr,
                "MIR to LLVM lowering cannot lower unspecialized generic call "
                "$%s in %s\n",
                target->name ? target->name : "<anonymous>",
                fn && fn->name ? fn->name : "<anonymous>");
        return NULL;
      }
      callee = lctx->functions[callee_def->data.fn_ref.fn];
      callee_type = lctx->function_types[callee_def->data.fn_ref.fn];
    }
  }

  if (!callee && instr->data.call.callee != MIR_NO_VALUE) {
    callee =
        mir_llvm_value_get_rvalue(values, instr->data.call.callee, builder);
  }

  if (!callee) {
    fprintf(stderr, "MIR to LLVM lowering could not resolve call callee in %s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return NULL;
  }

  LLVMValueRef *args = NULL;
  if (instr->data.call.operands.len > 0) {
    args = calloc(instr->data.call.operands.len, sizeof(LLVMValueRef));
    if (!args) {
      return NULL;
    }
  }

  size_t arg_count = 0;
  for (size_t i = 0; i < instr->data.call.operands.len; i++) {
    MirValueId operand_id = instr->data.call.operands.items[i];
    if (operand_id == MIR_NO_VALUE || operand_id >= values->len) {
      fprintf(stderr,
              "MIR to LLVM lowering found invalid call operand %zu in %s\n", i,
              fn && fn->name ? fn->name : "<anonymous>");
      free(args);
      return NULL;
    }

    Type *operand_type = mir_function_value_type(fn, operand_id);
    if (operand_type && operand_type->kind == T_VOID) {
      continue;
    }

    LLVMValueRef arg = mir_llvm_value_get_rvalue(values, operand_id, builder);
    if (!arg) {
      free(args);
      return NULL;
    }
    args[arg_count++] = arg;
  }

  if (!callee_type) {
    callee_type = lower_mir_indirect_call_type(
        fn, instr, instr->data.call.callee_type, instr->type, &lctx->jit_ctx,
        module);
  }

  if (!callee_type) {
    free(args);
    return NULL;
  }

  LLVMValueRef result =
      LLVMBuildCall2(builder, callee_type, callee, args, (unsigned)arg_count,
                     instr->type && instr->type->kind == T_VOID ? "" : "call");
  free(args);
  return result;
}

static LLVMValueRef lower_mir_phi(MirInstr *instr, LLVMModuleRef module,
                                  JITLangCtx *ctx,
                                  LLVMBuilderRef builder) {
  if (!instr || !instr->type) {
    return NULL;
  }

  LLVMTypeRef type = lower_mir_abi_value_type(instr->type, ctx, module, NULL);
  if (!type || LLVMGetTypeKind(type) == LLVMVoidTypeKind) {
    return NULL;
  }

  return LLVMBuildPhi(builder, type, "phi");
}

static LLVMValueRef lower_mir_closure_env(MirFunction *fn, MirInstr *instr,
                                          MirLlvmValueMap *values,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder,
                                          JITLangCtx *ctx) {
  if (!instr || !values) {
    return NULL;
  }

  LLVMTypeRef record_type =
      lower_mir_closure_env_record_type(instr->type, ctx, module);
  if (!record_type) {
    return NULL;
  }

  LLVMValueRef env =
      lower_mir_value_allocates_on_stack(fn, instr->result)
          ? LLVMBuildAlloca(builder, record_type, "closure.env.stack")
          : LLVMBuildMalloc(builder, record_type, "closure.env");
  if (!env) {
    return NULL;
  }

  for (size_t i = 0; i < instr->data.construct.items.len; i++) {
    MirValueId field_id = instr->data.construct.items.items[i];
    LLVMValueRef field = mir_llvm_value_get_rvalue(values, field_id, builder);
    if (!field) {
      return NULL;
    }

    Type *field_type = NULL;
    if (instr->type && (instr->type->kind == T_CONS ||
                        instr->type->kind == T_SUM) &&
        instr->type->data.T_CONS.args &&
        i < (size_t)instr->type->data.T_CONS.num_args) {
      field_type = instr->type->data.T_CONS.args[i];
    }
    LLVMTypeRef storage_type =
        lower_mir_value_storage_type(field_type, ctx, module);
    if (storage_type && LLVMTypeOf(field) != storage_type &&
        LLVMGetTypeKind(LLVMTypeOf(field)) == LLVMPointerTypeKind &&
        LLVMGetTypeKind(storage_type) == LLVMPointerTypeKind) {
      field = LLVMBuildBitCast(builder, field, storage_type,
                               "closure.env.field.cast");
    }

    LLVMValueRef field_ptr = LLVMBuildStructGEP2(
        builder, record_type, env, (unsigned)i, "closure.env.field.ptr");
    LLVMBuildStore(builder, field, field_ptr);
  }

  return env;
}

static LLVMValueRef lower_mir_closure(MirInstr *instr, MirLlvmValueMap *values,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder,
                                      JITLangCtx *ctx) {
  if (!instr || !values) {
    return NULL;
  }

  LLVMTypeRef closure_type = lower_mir_closure_value_type(module);
  if (!closure_type) {
    return NULL;
  }

  LLVMValueRef fn =
      mir_llvm_value_get_rvalue(values, instr->data.construct.operands[0], builder);
  LLVMValueRef env =
      mir_llvm_value_get_rvalue(values, instr->data.construct.operands[1], builder);
  if (!fn || !env) {
    return NULL;
  }

  LLVMTypeRef generic_ptr = lower_mir_generic_ptr_type(module);
  if (LLVMTypeOf(fn) != generic_ptr) {
    fn = LLVMBuildBitCast(builder, fn, generic_ptr, "closure.fn.ptr");
  }
  if (LLVMTypeOf(env) != generic_ptr) {
    env = LLVMBuildBitCast(builder, env, generic_ptr, "closure.env.ptr");
  }

  LLVMValueRef closure = LLVMGetUndef(closure_type);
  closure = LLVMBuildInsertValue(builder, closure, env, 1, "closure.env");
  closure = LLVMBuildInsertValue(builder, closure, fn, 0, "closure.fn");
  return closure;
}

static LLVMValueRef lower_mir_closure_fn_part(MirInstr *instr,
                                              MirLlvmValueMap *values,
                                              LLVMBuilderRef builder) {
  if (!instr || !values) {
    return NULL;
  }

  LLVMValueRef closure =
      mir_llvm_value_get_rvalue(values, instr->data.extract.value, builder);
  return closure ? LLVMBuildExtractValue(builder, closure, 0, "closure.fn")
                 : NULL;
}

static LLVMValueRef lower_mir_closure_env_part(MirInstr *instr,
                                               MirLlvmValueMap *values,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder,
                                               JITLangCtx *ctx) {
  if (!instr || !values) {
    return NULL;
  }

  LLVMValueRef closure =
      mir_llvm_value_get_rvalue(values, instr->data.extract.value, builder);
  if (!closure) {
    return NULL;
  }

  LLVMValueRef env =
      LLVMBuildExtractValue(builder, closure, 1, "closure.env");
  LLVMTypeRef env_ptr_type =
      lower_mir_closure_env_ptr_type(instr->type, ctx, module);
  if (env_ptr_type && LLVMTypeOf(env) != env_ptr_type) {
    env = LLVMBuildBitCast(builder, env, env_ptr_type, "closure.env.cast");
  }
  return env;
}

static LLVMValueRef lower_mir_extract(MirFunction *fn, MirInstr *instr,
                                      MirLlvmValueMap *values,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder,
                                      JITLangCtx *ctx) {
  if (!instr || instr->kind != MIR_EXTRACT) {
    return NULL;
  }

  switch (instr->data.extract.kind) {
  case MIR_EXTRACT_FIELD:
    return lower_mir_extract_field(fn, instr, values, module, builder, ctx);
  case MIR_EXTRACT_ARRAY_AT:
    return lower_mir_array_at(fn, instr, values, module, builder, ctx);
  case MIR_EXTRACT_LIST_HEAD:
    return lower_mir_list_head(fn, instr, values, module, builder, ctx);
  case MIR_EXTRACT_LIST_TAIL:
    return lower_mir_list_tail(fn, instr, values, module, builder, ctx);
  case MIR_EXTRACT_VARIANT_TAG:
    return lower_mir_variant_tag(instr, values, builder);
  case MIR_EXTRACT_VARIANT_PAYLOAD:
    return lower_mir_variant_payload(instr, values, module, builder, ctx);
  case MIR_EXTRACT_CLOSURE_FN:
    return lower_mir_closure_fn_part(instr, values, builder);
  case MIR_EXTRACT_CLOSURE_ENV:
    return lower_mir_closure_env_part(instr, values, module, builder, ctx);
  case MIR_EXTRACT_ARRAY_SUCC:
  case MIR_EXTRACT_ARRAY_OFFSET:
    return NULL;
  }

  return NULL;
}

static LLVMValueRef lower_mir_construct(MirFunction *fn, MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx) {
  if (!instr || instr->kind != MIR_CONSTRUCT) {
    return NULL;
  }

  switch (instr->data.construct.kind) {
  case MIR_CONSTRUCT_TUPLE:
    return lower_mir_tuple(fn, instr, values, module, builder, ctx);
  case MIR_CONSTRUCT_VARIANT:
    return lower_mir_variant(fn, instr, values, module, builder, ctx);
  case MIR_CONSTRUCT_LIST_EMPTY:
    return lower_mir_list_empty(instr, module, ctx);
  case MIR_CONSTRUCT_LIST_CONS:
    return lower_mir_list_cons(fn, instr, values, module, builder, ctx);
  case MIR_CONSTRUCT_ARRAY_LITERAL:
    return lower_mir_array_literal(fn, instr, values, module, builder, ctx);
  case MIR_CONSTRUCT_CLOSURE_ENV:
    return lower_mir_closure_env(fn, instr, values, module, builder, ctx);
  case MIR_CONSTRUCT_CLOSURE:
    return lower_mir_closure(instr, values, module, builder, ctx);
  case MIR_CONSTRUCT_ARRAY_FILL_CONST:
  case MIR_CONSTRUCT_ARRAY_FILL:
  case MIR_CONSTRUCT_ARRAY_RANGE:
    return NULL;
  }

  return NULL;
}

static LLVMValueRef lower_mir_rc_hook(LLVMModuleRef module, const char *name) {
  if (!module || !name) {
    return NULL;
  }

  LLVMValueRef fn = LLVMGetNamedFunction(module, name);
  if (fn) {
    return fn;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  LLVMTypeRef fn_type =
      LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
  return LLVMAddFunction(module, name, fn_type);
}

static LLVMValueRef lower_mir_rc_managed_ptr(MirFunction *fn, MirInstr *instr,
                                             MirLlvmValueMap *values,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  if (!fn || !instr || !values) {
    return NULL;
  }

  MirValueId value_id = instr->data.op.operands[0];
  LLVMValueRef value = mir_llvm_value_get_rvalue(values, value_id, builder);
  if (!value) {
    return NULL;
  }

  Type *type = mir_function_value_type(fn, value_id);
  if (type && (is_array_type(type) || is_string_type(type) ||
               is_closure(type))) {
    value = LLVMBuildExtractValue(builder, value, 1, "rc.payload");
  }

  if (!value) {
    return NULL;
  }

  LLVMTypeRef generic_ptr = lower_mir_generic_ptr_type(module);
  if (LLVMTypeOf(value) != generic_ptr &&
      LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMPointerTypeKind) {
    value = LLVMBuildBitCast(builder, value, generic_ptr, "rc.ptr");
  }

  return LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMPointerTypeKind ? value
                                                                   : NULL;
}

static LLVMValueRef lower_mir_rc_marker(MirFunction *fn, MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  if (!instr) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  const char *hook_name =
      instr->data.op.kind == MIR_OP_KIND_DUP ? "__ylc_dup" : "__ylc_drop";
  LLVMValueRef hook = lower_mir_rc_hook(module, hook_name);
  LLVMValueRef ptr =
      lower_mir_rc_managed_ptr(fn, instr, values, module, builder);
  if (!hook || !ptr) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  LLVMTypeRef fn_type =
      LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
  return LLVMBuildCall2(builder, fn_type, hook, &ptr, 1, "");
}

static LLVMValueRef lower_mir_op(MirFunction *fn, MirInstr *instr,
                                 MirLlvmValueMap *values, MirLlvmCtx *lctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder,
                                 JITLangCtx *ctx) {
  (void)lctx;
  if (!instr || instr->kind != MIR_OP) {
    return NULL;
  }

  switch (instr->data.op.kind) {
  case MIR_OP_KIND_CAST:
    return lower_mir_primitive_cast(instr, values, module, builder, ctx);
  case MIR_OP_KIND_PRIMITIVE:
    return lower_mir_primitive(instr, values, builder);
  case MIR_OP_KIND_ARRAY_SIZE:
    return lower_mir_array_size(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_ARRAY_SET:
    return lower_mir_array_set(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_LIST_IS_EMPTY:
    return lower_mir_list_is_empty(fn, instr, values, builder);
  case MIR_OP_KIND_TAG_EQ:
    return lower_mir_tag_eq(instr, values, builder);
  case MIR_OP_KIND_DUP:
  case MIR_OP_KIND_DROP:
    return lower_mir_rc_marker(fn, instr, values, module, builder);
  default:
    return NULL;
  }
}

static LLVMValueRef lower_mir_instr(MirInstr *instr, MirLlvmValueMap *values,
                                    MirFunction *fn, MirLlvmCtx *lctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder, JITLangCtx *ctx) {
  switch (instr->kind) {
  case MIR_CONST:
    return lower_mir_const(instr, module, ctx);

  case MIR_OP:
    return lower_mir_op(fn, instr, values, lctx, module, builder, ctx);

  case MIR_PHI:
    return lower_mir_phi(instr, module, ctx, builder);

  case MIR_EXTRACT:
    return lower_mir_extract(fn, instr, values, module, builder, ctx);

  case MIR_CONSTRUCT:
    return lower_mir_construct(fn, instr, values, module, builder, ctx);

  case MIR_FN_REF:
    return lower_mir_fn_ref(instr, lctx, module, builder);

  case MIR_CALL:
    return lower_mir_call(fn, instr, values, lctx, module, builder);

  default:
    fprintf(stderr,
            "MIR to LLVM lowering does not support instruction kind %d\n",
            instr->kind);
    return NULL;
  }
}

static bool lower_mir_terminator(MirFunction *fn, MirTerminator *term,
                                 MirLlvmValueMap *values,
                                 MirLlvmBlockMap *blocks, LLVMTypeRef ret_type,
                                 LLVMBuilderRef builder) {
  switch (term->kind) {
  case MIR_TERM_RETURN: {
    if (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind) {
      LLVMBuildRetVoid(builder);
      return true;
    }

    LLVMValueRef value =
        mir_llvm_value_get_rvalue(values, term->value, builder);
    if (!value) {
      fprintf(stderr,
              "MIR to LLVM lowering could not find return value %u in %s\n",
              term->value, fn && fn->name ? fn->name : "<anonymous>");
      return false;
    }

    LLVMBuildRet(builder, value);
    return true;
  }
  case MIR_TERM_BR: {
    LLVMBasicBlockRef target = mir_llvm_block_get(blocks, term->target);
    if (!target) {
      fprintf(stderr, "MIR to LLVM lowering could not find target bb%u in %s\n",
              term->target, fn && fn->name ? fn->name : "<anonymous>");
      return false;
    }
    LLVMBuildBr(builder, target);
    return true;
  }
  case MIR_TERM_COND: {
    LLVMValueRef cond = mir_llvm_value_get_rvalue(values, term->cond, builder);
    LLVMBasicBlockRef then_block = mir_llvm_block_get(blocks, term->then_block);
    LLVMBasicBlockRef else_block = mir_llvm_block_get(blocks, term->else_block);
    if (!cond || !then_block || !else_block) {
      fprintf(stderr,
              "MIR to LLVM lowering could not lower conditional terminator in "
              "%s\n",
              fn && fn->name ? fn->name : "<anonymous>");
      return false;
    }
    LLVMBuildCondBr(builder, cond, then_block, else_block);
    return true;
  }
  case MIR_TERM_UNREACHABLE:
    LLVMBuildUnreachable(builder);
    return true;
  case MIR_TERM_NONE:
    if (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind) {
      LLVMBuildRetVoid(builder);
      return true;
    }
    fprintf(stderr,
            "MIR to LLVM lowering found unterminated non-void function %s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return false;
  default:
    fprintf(stderr,
            "MIR to LLVM lowering does not support terminator kind %d\n",
            term->kind);
    return false;
  }
}

static size_t lower_mir_terminator_successors(MirTerminator *term,
                                              MirBlockId out[2]) {
  if (!term || !out) {
    return 0;
  }

  switch (term->kind) {
  case MIR_TERM_BR:
    if (term->target == MIR_NO_BLOCK) {
      return 0;
    }
    out[0] = term->target;
    return 1;
  case MIR_TERM_COND: {
    size_t len = 0;
    if (term->then_block != MIR_NO_BLOCK) {
      out[len++] = term->then_block;
    }
    if (term->else_block != MIR_NO_BLOCK &&
        term->else_block != term->then_block) {
      out[len++] = term->else_block;
    }
    return len;
  }
  default:
    return 0;
  }
}

static bool lower_mir_block_body(MirLlvmCtx *lctx, MirFunction *fn,
                                 MirLlvmValueMap *values,
                                 MirLlvmBlockMap *blocks, bool *visited,
                                 MirBlockId block_id, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  if (!lctx || !fn || !values || !blocks || !visited ||
      block_id == MIR_NO_BLOCK || block_id >= fn->blocks.len ||
      block_id >= blocks->len) {
    return false;
  }

  if (visited[block_id]) {
    return true;
  }
  visited[block_id] = true;

  MirBlock *mir_block = fn->blocks.items[block_id];
  if (!mir_block || !blocks->items[block_id]) {
    return false;
  }

  LLVMPositionBuilderAtEnd(builder, blocks->items[block_id]);

  for (size_t j = 0; j < mir_block->instrs.len; j++) {
    MirInstr *instr = mir_block->instrs.items + j;
    LLVMValueRef value = lower_mir_instr(instr, values, fn, lctx, module,
                                         builder, &lctx->jit_ctx);
    if (!value || !mir_llvm_value_set(values, instr->result, value)) {
      return false;
    }
  }

  LLVMTypeRef ret_type = LLVMGetReturnType(lctx->function_types[fn->id]);
  if (!lower_mir_terminator(fn, &mir_block->term, values, blocks, ret_type,
                            builder)) {
    return false;
  }

  MirBlockId successors[2] = {MIR_NO_BLOCK, MIR_NO_BLOCK};
  size_t successors_len =
      lower_mir_terminator_successors(&mir_block->term, successors);
  for (size_t i = 0; i < successors_len; i++) {
    if (!lower_mir_block_body(lctx, fn, values, blocks, visited, successors[i],
                              module, builder)) {
      return false;
    }
  }

  return true;
}

static LLVMValueRef lower_mir_phi_incoming_value(MirLlvmValueMap *values,
                                                 MirLlvmBlockMap *blocks,
                                                 MirPhiIncoming incoming,
                                                 LLVMBuilderRef builder) {
  if (!values || !blocks || incoming.value == MIR_NO_VALUE ||
      incoming.value >= values->len || incoming.block == MIR_NO_BLOCK ||
      incoming.block >= blocks->len) {
    return NULL;
  }

  if (values->slots && values->slots[incoming.value]) {
    LLVMTypeRef type =
        values->slot_types ? values->slot_types[incoming.value] : NULL;
    LLVMBasicBlockRef block = blocks->items[incoming.block];
    if (!type || !block) {
      return NULL;
    }

    LLVMValueRef terminator = LLVMGetBasicBlockTerminator(block);
    if (terminator) {
      LLVMPositionBuilderBefore(builder, terminator);
    } else {
      LLVMPositionBuilderAtEnd(builder, block);
    }
    return LLVMBuildLoad2(builder, type, values->slots[incoming.value],
                          "phi.in");
  }

  return values->items[incoming.value];
}

static bool lower_mir_add_phi_incomings(MirFunction *fn,
                                        MirLlvmValueMap *values,
                                        MirLlvmBlockMap *blocks,
                                        LLVMBuilderRef builder) {
  if (!fn || !values || !blocks) {
    return false;
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }

    for (size_t j = 0; j < block->instrs.len; j++) {
      MirInstr *instr = &block->instrs.items[j];
      if (instr->kind != MIR_PHI) {
        continue;
      }

      if (instr->result == MIR_NO_VALUE || instr->result >= values->len) {
        return false;
      }

      LLVMValueRef phi = values->items[instr->result];
      if (!phi) {
        return false;
      }

      for (size_t k = 0; k < instr->data.phi.incoming.len; k++) {
        MirPhiIncoming incoming = instr->data.phi.incoming.items[k];
        if (incoming.block == MIR_NO_BLOCK || incoming.block >= blocks->len) {
          return false;
        }

        LLVMBasicBlockRef incoming_block = blocks->items[incoming.block];
        LLVMValueRef incoming_value =
            lower_mir_phi_incoming_value(values, blocks, incoming, builder);
        if (!incoming_block || !incoming_value) {
          return false;
        }

        LLVMAddIncoming(phi, &incoming_value, &incoming_block, 1);
      }
    }
  }

  return true;
}

static bool lower_mir_function_body(MirLlvmCtx *lctx, MirFunction *fn,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  if (!lctx || !fn || fn->id >= lctx->functions_len ||
      !lctx->functions[fn->id] || !lctx->function_types[fn->id]) {
    return false;
  }

  LLVMValueRef llvm_fn = lctx->functions[fn->id];
  if (LLVMCountBasicBlocks(llvm_fn) != 0) {
    return true;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  if (fn->blocks.len == 0) {
    return false;
  }

  MirLlvmValueMap values = {
      .items =
          calloc(fn->values.len ? fn->values.len : 1, sizeof(LLVMValueRef)),
      .slots =
          calloc(fn->values.len ? fn->values.len : 1, sizeof(LLVMValueRef)),
      .slot_types =
          calloc(fn->values.len ? fn->values.len : 1, sizeof(LLVMTypeRef)),
      .len = fn->values.len,
  };
  MirLlvmBlockMap blocks = {
      .items = calloc(fn->blocks.len, sizeof(LLVMBasicBlockRef)),
      .len = fn->blocks.len,
  };
  bool *visited = calloc(fn->blocks.len, sizeof(bool));
  if (!values.items || !values.slots || !values.slot_types || !blocks.items ||
      !visited) {
    free(values.items);
    free(values.slots);
    free(values.slot_types);
    free(blocks.items);
    free(visited);
    return false;
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *mir_block = fn->blocks.items[i];
    if (!mir_block || mir_block->id >= blocks.len) {
      continue;
    }
    blocks.items[mir_block->id] = LLVMAppendBasicBlockInContext(
        llvm_ctx, llvm_fn, mir_block->name ? mir_block->name : "bb");
  }

  size_t llvm_param = 0;
  bool ok = true;
  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (!lower_mir_param_is_lowered(param)) {
      continue;
    }

    LLVMValueRef llvm_value = LLVMGetParam(llvm_fn, (unsigned)llvm_param);
    llvm_param++;
    if (!mir_llvm_value_set(&values, param->value, llvm_value)) {
      ok = false;
      break;
    }
  }

  if (ok) {
    ok = lower_mir_block_body(lctx, fn, &values, &blocks, visited, 0, module,
                              builder);
  }
  for (size_t i = 0; ok && i < fn->blocks.len; i++) {
    if (!visited[i]) {
      ok = lower_mir_block_body(lctx, fn, &values, &blocks, visited,
                                (MirBlockId)i, module, builder);
    }
  }
  if (ok) {
    ok = lower_mir_add_phi_incomings(fn, &values, &blocks, builder);
  }

  free(values.items);
  free(values.slots);
  free(values.slot_types);
  free(blocks.items);
  free(visited);
  if (!ok) {
    LLVMDeleteFunction(llvm_fn);
    lctx->functions[fn->id] = NULL;
    lctx->function_types[fn->id] = NULL;
  }
  return ok;
}

LLVMValueRef lower_mir(MirProgram *prog, LLVMModuleRef module,
                       LLVMBuilderRef builder) {
  if (!prog || !module || !builder) {
    return NULL;
  }

  MirLlvmCtx lctx = {
      .program = prog,
      .jit_ctx = {.env = prog->type_env},
      .functions_len = prog->functions.len,
  };

  lctx.functions =
      calloc(lctx.functions_len ? lctx.functions_len : 1, sizeof(LLVMValueRef));
  lctx.function_types =
      calloc(lctx.functions_len ? lctx.functions_len : 1, sizeof(LLVMTypeRef));
  if (!lctx.functions || !lctx.function_types) {
    free(lctx.functions);
    free(lctx.function_types);
    return NULL;
  }

  MirFunction *top_fn = NULL;
  for (size_t i = 0; i < prog->functions.len; i++) {
    MirFunction *fn = prog->functions.items ? prog->functions.items[i] : NULL;
    if (!fn) {
      continue;
    }

    if (fn->name && strcmp(fn->name, "$top") == 0) {
      top_fn = fn;
    }
    lower_mir_declare_function(&lctx, fn, module);
  }

  if (!top_fn) {
    free(lctx.functions);
    free(lctx.function_types);
    return lower_mir_void_stub(module, builder);
  }

  for (size_t i = 0; i < prog->functions.len; i++) {
    MirFunction *fn = prog->functions.items ? prog->functions.items[i] : NULL;
    if (!fn || !lctx.functions[fn->id]) {
      continue;
    }
    if (lower_mir_type_has_unresolved_vars(fn->type)) {
      continue;
    }

    bool ok = lower_mir_function_body(&lctx, fn, module, builder);
    if (!ok) {
      free(lctx.functions);
      free(lctx.function_types);
      return NULL;
    }
  }

  LLVMValueRef top =
      top_fn->id < lctx.functions_len ? lctx.functions[top_fn->id] : NULL;
  free(lctx.functions);
  free(lctx.function_types);
  return top ? top : lower_mir_void_stub(module, builder);
}
