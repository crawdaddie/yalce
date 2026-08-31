#include "./lowering.h"
#include "backend_llvm/adt.h"
#include "backend_llvm/array.h"
#include "backend_llvm/coroutines/coroutines.h"
#include "backend_llvm/globals.h"
#include "backend_llvm/lib_registry.h"
#include "backend_llvm/list.h"
#include "backend_llvm/module.h"
#include "backend_llvm/strings.h"
#include "backend_llvm/types.h"
#include "config.h"
#include "escape_analysis.h"
#include "input.h"
#include "types/type_expressions.h"
#include <ctype.h>
#include <dlfcn.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
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
  LLVMBasicBlockRef *exits;
  size_t len;
} MirLlvmBlockMap;

typedef struct {
  MirProgram *program;
  JITLangCtx jit_ctx;
  LLVMValueRef *functions;
  LLVMTypeRef *function_types;
  size_t functions_len;
} MirLlvmCtx;

static MirProgram *lower_mir_current_program = NULL;

static const char *mir_instr_kind_name(MirInstrKind kind) {
  switch (kind) {
  case MIR_CONST:
    return "const";
  case MIR_OP:
    return "op";
  case MIR_PHI:
    return "phi";
  case MIR_EXTRACT:
    return "extract";
  case MIR_CONSTRUCT:
    return "construct";
  case MIR_FN_REF:
    return "fn_ref";
  case MIR_CALL:
    return "call";
  case MIR_CORO_NEW:
    return "coro.new";
  case MIR_CORO_NEXT:
    return "coro.next";
  case MIR_CORO_RESET:
    return "coro.reset";
  }
  return "unknown";
}

typedef struct {
  bool active;
  Type *yield_type;
  LLVMTypeRef llvm_yield_type;
  LLVMTypeRef promise_type;
  LLVMValueRef promise_alloca;
  LLVMValueRef coro_id;
  LLVMValueRef handle;
  LLVMBasicBlockRef cleanup_bb;
  LLVMBasicBlockRef suspend_bb;
  LLVMBasicBlockRef initial_return_bb;
  LLVMBasicBlockRef start_bb;
} MirLlvmCoroCtx;

static LLVMTypeRef lower_mir_generic_ptr_type(LLVMModuleRef module);
static Type *lower_mir_resolve_rc_type(Type *type, JITLangCtx *ctx);
static Type *lower_mir_resolve_rc_type_uncached(Type *type, JITLangCtx *ctx);

/* Per-program memoization cache for lower_mir_resolve_rc_type. The same Type*
   is shared across all MIR values of a given type, so resolving it once and
   reusing the result avoids repeatedly running resolve_type_in_env +
   deep_copy_type + the program-wide equivalent-alias search for the hundreds
   of RC dup/drop markers that lower the same types. */
#define LOWER_MIR_RC_CACHE_CAP 1024
static struct {
  Type *in;
  Type *out;
} lower_mir_rc_cache[LOWER_MIR_RC_CACHE_CAP];
static bool lower_mir_rc_cache_valid[LOWER_MIR_RC_CACHE_CAP];

static void lower_mir_rc_cache_reset(void) {
  memset(lower_mir_rc_cache_valid, 0, sizeof(lower_mir_rc_cache_valid));
}

static Type *lower_mir_rc_cache_get(Type *type) {
  if (!type) {
    return NULL;
  }
  uintptr_t h = (uintptr_t)type;
  h ^= h >> 16;
  h *= 0x9E3779B1u;
  size_t idx = h & (LOWER_MIR_RC_CACHE_CAP - 1);
  for (size_t probe = 0; probe < LOWER_MIR_RC_CACHE_CAP; probe++) {
    size_t i = (idx + probe) & (LOWER_MIR_RC_CACHE_CAP - 1);
    if (!lower_mir_rc_cache_valid[i]) {
      return NULL;
    }
    if (lower_mir_rc_cache[i].in == type) {
      return lower_mir_rc_cache[i].out;
    }
  }
  return NULL;
}

static void lower_mir_rc_cache_put(Type *type, Type *resolved) {
  if (!type) {
    return;
  }
  uintptr_t h = (uintptr_t)type;
  h ^= h >> 16;
  h *= 0x9E3779B1u;
  size_t idx = h & (LOWER_MIR_RC_CACHE_CAP - 1);
  for (size_t probe = 0; probe < LOWER_MIR_RC_CACHE_CAP; probe++) {
    size_t i = (idx + probe) & (LOWER_MIR_RC_CACHE_CAP - 1);
    if (!lower_mir_rc_cache_valid[i]) {
      lower_mir_rc_cache[i].in = type;
      lower_mir_rc_cache[i].out = resolved;
      lower_mir_rc_cache_valid[i] = true;
      return;
    }
    if (lower_mir_rc_cache[i].in == type) {
      lower_mir_rc_cache[i].out = resolved;
      return;
    }
  }
  /* Cache full; drop the new entry (cache is advisory, miss falls back to the
     slow path). This only happens with >1024 distinct resolved types. */
}

typedef struct MirDlopenCacheEntry {
  LLVMModuleRef module;
  char *path;
  struct MirDlopenCacheEntry *next;
} MirDlopenCacheEntry;

static MirDlopenCacheEntry *mir_dlopen_cache = NULL;

static bool lower_mir_link_llvm_bitcode_dependencies(MirProgram *prog,
                                                     LLVMModuleRef module) {
  if (!prog || !module) {
    return true;
  }

  for (size_t i = 0; i < prog->llvm_bitcode_paths.len; i++) {
    const char *path = prog->llvm_bitcode_paths.items
                           ? prog->llvm_bitcode_paths.items[i]
                           : NULL;
    if (!path) {
      continue;
    }
    if (!ylc_link_llvm_bitcode_file(module, path)) {
      return false;
    }
  }

  return true;
}

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
  bool has_env_param = false;
  for (size_t i = 0; type && type->kind == T_FN && i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (param && param->name && strcmp(param->name, "$env") == 0) {
      has_env_param = true;
      if (!(type->data.T_FN.from && param->type &&
            types_equal(type->data.T_FN.from, param->type))) {
        continue;
      }
    }
    type = type->data.T_FN.to;
  }

  if (has_env_param && type && type->kind == T_FN && type->data.T_FN.from &&
      type->data.T_FN.from->kind == T_VOID) {
    return type->data.T_FN.to;
  }

  // A nullary function (T_FN from T_VOID to X) has no real parameter, so its
  // return type is X. A closure value is also a T_FN, but it is a value type,
  // not a function type, so it must not be unwrapped here.
  if (type && type->kind == T_FN && !is_closure(type) && type->data.T_FN.from &&
      type->data.T_FN.from->kind == T_VOID) {
    return type->data.T_FN.to;
  }
  return type;
}

static LLVMTypeRef lower_mir_type(Type *type, JITLangCtx *ctx,
                                  LLVMModuleRef module, LLVMTypeRef fallback) {
  LLVMTypeRef llvm_type = type_to_llvm_type(type, ctx, module);
  return llvm_type ? llvm_type : fallback;
}

static LLVMTypeRef lower_mir_aggregate_type(Type *type, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMTypeRef fallback) {
  LLVMTypeRef llvm_type = type_to_llvm_aggregate_type(type, ctx, module);
  return llvm_type ? llvm_type : fallback;
}

static Type *lower_mir_sizeof_target_type(MirInstr *instr, JITLangCtx *ctx) {
  if (!instr || !instr->origin || instr->origin->tag != AST_APPLICATION ||
      instr->origin->data.AST_APPLICATION.len != 1 ||
      !instr->origin->data.AST_APPLICATION.args) {
    return NULL;
  }

  Ast *expr = instr->origin->data.AST_APPLICATION.args;
  TICtx type_ctx = {.env = ctx ? ctx->env : NULL};
  Type *type = NULL;
  if (expr->tag == AST_TYPE_DECL && expr->data.AST_LET.expr) {
    type = compute_type_expression(expr->data.AST_LET.expr, &type_ctx);
  } else {
    type = compute_type_expression(expr, &type_ctx);
  }
  return type ? type : expr->type;
}

static LLVMValueRef lower_mir_sizeof(MirInstr *instr, LLVMModuleRef module,
                                     JITLangCtx *ctx) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  unsigned long long size = 0;
  Type *target_type = lower_mir_sizeof_target_type(instr, ctx);
  LLVMTypeRef llvm_type = type_to_llvm_type(target_type, ctx, module);
  LLVMTargetDataRef target_data = LLVMGetModuleDataLayout(module);
  if (llvm_type && target_data) {
    size = LLVMStoreSizeOfType(target_data, llvm_type);
  }
  return LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), size, 0);
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

static bool lower_mir_is_c_abi_view_type(Type *type) {
  return type && (is_array_type(type) || is_string_type(type));
}

static LLVMTypeRef lower_mir_c_abi_view_data_ptr_type(Type *type,
                                                      JITLangCtx *ctx,
                                                      LLVMModuleRef module) {
  LLVMTypeRef view_type = lower_mir_abi_value_type(type, ctx, module, NULL);
  if (view_type && LLVMGetTypeKind(view_type) == LLVMStructTypeKind &&
      LLVMCountStructElementTypes(view_type) >= 3) {
    return LLVMStructGetTypeAtIndex(view_type, 2);
  }
  return lower_mir_generic_ptr_type(module);
}

static LLVMTypeRef lower_mir_c_abi_return_type(Type *type, JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMTypeRef fallback) {
  if (!lower_mir_is_c_abi_view_type(type)) {
    return lower_mir_abi_value_type(type, ctx, module, fallback);
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef fields[] = {
      LLVMInt64TypeInContext(llvm_ctx),
      lower_mir_c_abi_view_data_ptr_type(type, ctx, module),
  };
  return LLVMStructTypeInContext(llvm_ctx, fields, 2, 0);
}

static size_t lower_mir_c_abi_param_type_count(Type *type) {
  return lower_mir_is_c_abi_view_type(type) ? 2 : 1;
}

static bool lower_mir_append_c_abi_param_types(Type *type,
                                               LLVMTypeRef *param_types,
                                               size_t *count, JITLangCtx *ctx,
                                               LLVMModuleRef module) {
  if (!param_types || !count) {
    return false;
  }

  if (lower_mir_is_c_abi_view_type(type)) {
    LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
    param_types[(*count)++] = LLVMInt64TypeInContext(llvm_ctx);
    param_types[(*count)++] =
        lower_mir_c_abi_view_data_ptr_type(type, ctx, module);
    return param_types[*count - 1] != NULL;
  }

  param_types[*count] = lower_mir_abi_value_type(type, ctx, module, NULL);
  if (!param_types[*count]) {
    return false;
  }
  (*count)++;
  return true;
}

static bool
lower_mir_append_c_abi_call_arg(LLVMValueRef *args, size_t *arg_count,
                                LLVMValueRef value, Type *type, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  if (!args || !arg_count || !value) {
    return false;
  }

  if (!lower_mir_is_c_abi_view_type(type)) {
    args[(*arg_count)++] = value;
    return true;
  }

  if (LLVMGetTypeKind(LLVMTypeOf(value)) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(LLVMTypeOf(value)) < 3) {
    return false;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i64_type = LLVMInt64TypeInContext(llvm_ctx);
  LLVMValueRef size = LLVMBuildExtractValue(builder, value, 0, "cabi.size");
  LLVMValueRef offset = LLVMBuildExtractValue(builder, value, 1, "cabi.offset");
  LLVMValueRef data = LLVMBuildExtractValue(builder, value, 2, "cabi.data");

  LLVMValueRef size64 = LLVMBuildZExt(builder, size, i64_type, "cabi.size64");
  LLVMValueRef offset64 =
      LLVMBuildZExt(builder, offset, i64_type, "cabi.offset64");
  LLVMValueRef shifted =
      LLVMBuildShl(builder, offset64, LLVMConstInt(i64_type, 32, false),
                   "cabi.offset.shift");
  LLVMValueRef header =
      LLVMBuildOr(builder, size64, shifted, "cabi.size_offset");

  LLVMTypeRef data_type = lower_mir_c_abi_view_data_ptr_type(type, ctx, module);
  if (data_type && LLVMTypeOf(data) != data_type &&
      LLVMGetTypeKind(LLVMTypeOf(data)) == LLVMPointerTypeKind &&
      LLVMGetTypeKind(data_type) == LLVMPointerTypeKind) {
    data = LLVMBuildPointerCast(builder, data, data_type, "cabi.data.cast");
  }

  args[(*arg_count)++] = header;
  args[(*arg_count)++] = data;
  return true;
}

static LLVMValueRef lower_mir_unpack_c_abi_view_result(Type *type,
                                                       LLVMValueRef value,
                                                       JITLangCtx *ctx,
                                                       LLVMModuleRef module,
                                                       LLVMBuilderRef builder) {
  if (!value || !lower_mir_is_c_abi_view_type(type)) {
    return value;
  }

  LLVMTypeRef result_type = lower_mir_abi_value_type(type, ctx, module, NULL);
  if (!result_type ||
      LLVMGetTypeKind(LLVMTypeOf(value)) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(LLVMTypeOf(value)) < 2) {
    return value;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i32_type = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef i64_type = LLVMInt64TypeInContext(llvm_ctx);

  LLVMValueRef header =
      LLVMBuildExtractValue(builder, value, 0, "cabi.ret.size_offset");
  LLVMValueRef data = LLVMBuildExtractValue(builder, value, 1, "cabi.ret.data");
  LLVMValueRef size =
      LLVMBuildTrunc(builder, header, i32_type, "cabi.ret.size");
  LLVMValueRef offset64 = LLVMBuildLShr(
      builder, header, LLVMConstInt(i64_type, 32, false), "cabi.ret.offset64");
  LLVMValueRef offset =
      LLVMBuildTrunc(builder, offset64, i32_type, "cabi.ret.offset");

  LLVMTypeRef data_type = LLVMStructGetTypeAtIndex(result_type, 2);
  if (data_type && LLVMTypeOf(data) != data_type &&
      LLVMGetTypeKind(LLVMTypeOf(data)) == LLVMPointerTypeKind &&
      LLVMGetTypeKind(data_type) == LLVMPointerTypeKind) {
    data = LLVMBuildPointerCast(builder, data, data_type, "cabi.ret.data.cast");
  }

  LLVMValueRef result = LLVMGetUndef(result_type);
  result = LLVMBuildInsertValue(builder, result, size, 0, "cabi.ret.view.size");
  result =
      LLVMBuildInsertValue(builder, result, offset, 1, "cabi.ret.view.offset");
  return LLVMBuildInsertValue(builder, result, data, 2, "cabi.ret.view.data");
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

static Type *lower_mir_coro_constructor_yield_type(Type *type) {
  if (!type || !is_coroutine_constructor_type(type)) {
    return NULL;
  }

  Type *ret = fn_return_type(type);
  if (!ret || !is_coroutine_type(ret) || !ret->data.T_CONS.args ||
      ret->data.T_CONS.num_args < 1) {
    return NULL;
  }

  Type *yield_type = ret->data.T_CONS.args[0];
  if (is_coroutine_type(yield_type) && yield_type->data.T_CONS.args &&
      yield_type->data.T_CONS.num_args > 0) {
    yield_type = yield_type->data.T_CONS.args[0];
  }
  return yield_type;
}

static Type *lower_mir_coro_instance_yield_type(Type *type) {
  if (!type || !is_coroutine_type(type) || !type->data.T_CONS.args ||
      type->data.T_CONS.num_args < 1) {
    return NULL;
  }
  return type->data.T_CONS.args[0];
}

static LLVMTypeRef lower_mir_coro_promise_type(Type *yield_type,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMTypeRef *llvm_yield_type) {
  LLVMTypeRef yielded = type_to_llvm_type(yield_type, ctx, module);
  if (!yielded) {
    return NULL;
  }
  if (llvm_yield_type) {
    *llvm_yield_type = yielded;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef fields[] = {
      yielded,
      LLVMInt1TypeInContext(llvm_ctx),
      lower_mir_generic_ptr_type(module),
      lower_mir_generic_ptr_type(module),
  };
  return LLVMStructTypeInContext(llvm_ctx, fields, 4, 0);
}

static LLVMTypeRef lower_mir_rc_header_type(LLVMModuleRef module) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef header_type = LLVMGetTypeByName2(llvm_ctx, "YlcRcHeader");
  if (header_type) {
    return header_type;
  }

  header_type = LLVMStructCreateNamed(llvm_ctx, "YlcRcHeader");
  LLVMTypeRef i32 = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef fields[] = {i32, i32};
  LLVMStructSetBody(header_type, fields, 2, 0);
  return header_type;
}

static LLVMValueRef lower_mir_heap_alloc_payload(LLVMModuleRef module,
                                                 LLVMBuilderRef builder,
                                                 LLVMTypeRef payload_type,
                                                 unsigned tag_or_size_class,
                                                 const char *name) {
  if (!module || !builder || !payload_type) {
    return NULL;
  }

  LLVMTypeRef header_type = lower_mir_rc_header_type(module);
  LLVMTypeRef object_fields[] = {header_type, payload_type};
  LLVMTypeRef object_type = LLVMStructTypeInContext(
      LLVMGetModuleContext(module), object_fields, 2, 0);
  LLVMValueRef object =
      LLVMBuildMalloc(builder, object_type, name ? name : "rc.object");
  if (!object) {
    return NULL;
  }

  LLVMTypeRef i32 = LLVMInt32TypeInContext(LLVMGetModuleContext(module));
  LLVMValueRef header_ptr =
      LLVMBuildStructGEP2(builder, object_type, object, 0, "rc.header.ptr");
  LLVMValueRef rc_ptr =
      LLVMBuildStructGEP2(builder, header_type, header_ptr, 0, "rc.count.ptr");
  LLVMBuildStore(builder, LLVMConstInt(i32, 1, false), rc_ptr);
  LLVMValueRef tag_ptr =
      LLVMBuildStructGEP2(builder, header_type, header_ptr, 1, "rc.tag.ptr");
  LLVMBuildStore(builder, LLVMConstInt(i32, tag_or_size_class, false), tag_ptr);

  return LLVMBuildStructGEP2(builder, object_type, object, 1, "rc.payload.ptr");
}

static LLVMValueRef lower_mir_stack_alloc_payload(LLVMModuleRef module,
                                                  LLVMBuilderRef builder,
                                                  LLVMTypeRef payload_type,
                                                  unsigned tag_or_size_class,
                                                  const char *name) {
  if (!module || !builder || !payload_type) {
    return NULL;
  }

  LLVMTypeRef header_type = lower_mir_rc_header_type(module);
  LLVMTypeRef object_fields[] = {header_type, payload_type};
  LLVMTypeRef object_type = LLVMStructTypeInContext(
      LLVMGetModuleContext(module), object_fields, 2, 0);
  LLVMValueRef object =
      LLVMBuildAlloca(builder, object_type, name ? name : "rc.object.stack");
  if (!object) {
    return NULL;
  }

  LLVMTypeRef i32 = LLVMInt32TypeInContext(LLVMGetModuleContext(module));
  LLVMValueRef header_ptr =
      LLVMBuildStructGEP2(builder, object_type, object, 0, "rc.header.ptr");
  LLVMValueRef rc_ptr =
      LLVMBuildStructGEP2(builder, header_type, header_ptr, 0, "rc.count.ptr");
  /* rc == 0 is the stack/immortal sentinel: dup and drop no-op on it, so
     stack-allocated managed values are never freed or refcounted. */
  LLVMBuildStore(builder, LLVMConstInt(i32, 0, false), rc_ptr);
  LLVMValueRef tag_ptr =
      LLVMBuildStructGEP2(builder, header_type, header_ptr, 1, "rc.tag.ptr");
  LLVMBuildStore(builder, LLVMConstInt(i32, tag_or_size_class, false), tag_ptr);

  return LLVMBuildStructGEP2(builder, object_type, object, 1, "rc.payload.ptr");
}

static LLVMValueRef lower_mir_malloc_i32(LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         LLVMValueRef size, const char *name) {
  if (!module || !builder || !size) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i32 = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  if (LLVMTypeOf(size) != i32) {
    size = LLVMBuildIntCast(builder, size, i32, "malloc.size");
  }

  LLVMValueRef malloc_fn = LLVMGetNamedFunction(module, "malloc");
  LLVMTypeRef malloc_type = LLVMFunctionType(ptr_type, &i32, 1, 0);
  if (!malloc_fn) {
    malloc_fn = LLVMAddFunction(module, "malloc", malloc_type);
  }
  return LLVMBuildCall2(builder, malloc_type, malloc_fn, &size, 1,
                        name ? name : "malloc");
}

static LLVMValueRef lower_mir_heap_alloc_array_payload(LLVMModuleRef module,
                                                       LLVMBuilderRef builder,
                                                       LLVMTypeRef element_type,
                                                       LLVMValueRef size,
                                                       const char *name) {
  if (!module || !builder || !element_type || !size) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i32 = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef i8 = LLVMInt8TypeInContext(llvm_ctx);
  if (LLVMTypeOf(size) != i32) {
    size = LLVMBuildIntCast(builder, size, i32, "array.size.i32");
  }

  LLVMValueRef element_size =
      LLVMConstTruncOrBitCast(LLVMSizeOf(element_type), i32);
  LLVMValueRef payload_size =
      LLVMBuildMul(builder, size, element_size, "array.payload.bytes");
  LLVMValueRef header_size = LLVMConstInt(i32, 8, false);
  LLVMValueRef total_size =
      LLVMBuildAdd(builder, header_size, payload_size, "array.alloc.bytes");
  LLVMValueRef object = lower_mir_malloc_i32(module, builder, total_size,
                                             name ? name : "array.data.heap");
  if (!object) {
    return NULL;
  }

  LLVMTypeRef header_type = lower_mir_rc_header_type(module);
  LLVMValueRef rc_ptr =
      LLVMBuildStructGEP2(builder, header_type, object, 0, "rc.count.ptr");
  LLVMBuildStore(builder, LLVMConstInt(i32, 1, false), rc_ptr);
  LLVMValueRef tag_ptr =
      LLVMBuildStructGEP2(builder, header_type, object, 1, "rc.tag.ptr");
  LLVMBuildStore(builder, LLVMConstInt(i32, 0, false), tag_ptr);

  LLVMValueRef payload =
      LLVMBuildGEP2(builder, i8, object, &header_size, 1, "array.payload.ptr");
  LLVMTypeRef data_ptr_type = LLVMPointerType(element_type, 0);
  return LLVMBuildPointerCast(builder, payload, data_ptr_type,
                              "array.data.ptr");
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
    Type *field_type =
        env_type->data.T_CONS.args ? env_type->data.T_CONS.args[i] : NULL;
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

  type = lower_mir_resolve_rc_type(type, ctx);
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
                                                    Type *type, JITLangCtx *ctx,
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

static LLVMTypeRef lower_mir_coroutine_function_type(MirFunction *fn,
                                                     JITLangCtx *ctx,
                                                     LLVMModuleRef module) {
  if (!fn || !is_coroutine_constructor_type(fn->type)) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef hidden_size_ptr =
      LLVMPointerType(LLVMInt64TypeInContext(llvm_ctx), 0);
  LLVMTypeRef ret_type = lower_mir_generic_ptr_type(module);

  size_t lowered_params = 1;
  for (size_t i = 0; i < fn->params.len; i++) {
    if (lower_mir_param_is_lowered(&fn->params.items[i])) {
      lowered_params++;
    }
  }

  LLVMTypeRef *param_types = calloc(lowered_params, sizeof(LLVMTypeRef));
  if (!param_types) {
    return NULL;
  }

  param_types[0] = hidden_size_ptr;
  size_t llvm_param = 1;
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
      LLVMFunctionType(ret_type, param_types, (unsigned)lowered_params, 0);
  free(param_types);
  return fn_type;
}

static MirFunction *lower_mir_extern_symbol_function(MirLlvmCtx *lctx,
                                                     MirFunction *fn) {
  if (!lctx || !lctx->program || !fn || !fn->is_extern) {
    return fn;
  }

  MirFunction *symbol_fn = fn;
  MirFunction *source = fn->specialization_of;
  while (source) {
    if (!source->is_extern) {
      break;
    }

    symbol_fn = source;
    if (!source->specialization_of || source->specialization_of == source) {
      break;
    }
    source = source->specialization_of;
  }

  return symbol_fn;
}

static bool lower_mir_function_uses_c_abi(MirLlvmCtx *lctx, MirFunction *fn) {
  MirFunction *symbol_fn = lower_mir_extern_symbol_function(lctx, fn);
  return symbol_fn && symbol_fn->is_extern;
}

static const char *lower_mir_function_symbol_name(MirLlvmCtx *lctx,
                                                  MirFunction *fn) {
  MirFunction *symbol_fn = lower_mir_extern_symbol_function(lctx, fn);
  const char *name = NULL;
  if (symbol_fn && symbol_fn->name) {
    name = symbol_fn->name;
  } else if (fn) {
    name = fn->name;
  }
  if (!name) {
    return "<anonymous>";
  }
  if (strcmp(name, "$top") == 0) {
    return "top";
  }
  // Durable functions (bodies allocated from the persistent durable
  // arena) are declared in every per-REPL-input LLVM module added to the
  // same JIT dylib; a fixed name would collide across modules. Append the
  // program generation for a per-input-unique LLVM symbol. The MIR
  // identity is the pointer, so only the LLVM symbol changes. Only in
  // interactive REPL mode: one-shot script compiles don't share a dylib
  // across inputs, so the suffix is unnecessary (and would change the
  // expected symbol name).
  if (ylc_config.interactive_mode && lctx && lctx->program &&
      lctx->program->durable_arena && fn &&
      fn->arena == lctx->program->durable_arena && !fn->is_extern) {
    return mir_arena_printf(lctx->program->arena, "%s.%u", name,
                            lctx->program->generation);
  }
  return name;
}

static LLVMTypeRef lower_mir_function_type(MirFunction *fn, JITLangCtx *ctx,
                                           LLVMModuleRef module) {
  if (!fn) {
    return NULL;
  }

  if (is_coroutine_constructor_type(fn->type)) {
    return lower_mir_coroutine_function_type(fn, ctx, module);
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  Type *ret_type = mir_function_return_type(fn);
  LLVMTypeRef llvm_ret_type =
      fn->is_extern
          ? lower_mir_c_abi_return_type(ret_type, ctx, module,
                                        LLVMVoidTypeInContext(llvm_ctx))
          : lower_mir_abi_value_type(ret_type, ctx, module,
                                     LLVMVoidTypeInContext(llvm_ctx));
  if (!llvm_ret_type) {
    return NULL;
  }

  size_t lowered_params = 0;
  for (size_t i = 0; i < fn->params.len; i++) {
    if (lower_mir_param_is_lowered(&fn->params.items[i])) {
      lowered_params +=
          fn->is_extern
              ? lower_mir_c_abi_param_type_count(fn->params.items[i].type)
              : 1;
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

    if (fn->is_extern) {
      if (!lower_mir_append_c_abi_param_types(param->type, param_types,
                                              &llvm_param, ctx, module)) {
        free(param_types);
        return NULL;
      }
    } else {
      param_types[llvm_param] = lower_mir_param_abi_type(param, ctx, module);
      if (!param_types[llvm_param]) {
        free(param_types);
        return NULL;
      }
      llvm_param++;
    }
  }

  LLVMTypeRef fn_type =
      LLVMFunctionType(llvm_ret_type, param_types, (unsigned)lowered_params, 0);
  free(param_types);
  return fn_type;
}

static bool lower_mir_name_function_params(MirFunction *fn,
                                           LLVMValueRef llvm_fn) {
  if (!fn || !llvm_fn) {
    return false;
  }

  unsigned llvm_param = 0;
  unsigned llvm_param_count = LLVMCountParams(llvm_fn);
  if (is_coroutine_constructor_type(fn->type)) {
    const char *name = "frame_size_out";
    if (llvm_param >= llvm_param_count) {
      fprintf(stderr, "MIR function `%s` has fewer LLVM params than expected\n",
              fn->name ? fn->name : "<anonymous>");
      return false;
    }
    LLVMValueRef param = LLVMGetParam(llvm_fn, llvm_param++);
    if (param) {
      LLVMSetValueName2(param, name, strlen(name));
    }
  }

  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (!lower_mir_param_is_lowered(param)) {
      continue;
    }

    const char *name = param->name ? param->name : "_";
    size_t abi_count =
        fn->is_extern ? lower_mir_c_abi_param_type_count(param->type) : 1;
    for (size_t abi_index = 0; abi_index < abi_count; abi_index++) {
      if (llvm_param >= llvm_param_count) {
        fprintf(stderr,
                "MIR function `%s` has fewer LLVM params than expected\n",
                fn->name ? fn->name : "<anonymous>");
        return false;
      }
      LLVMValueRef llvm_value = LLVMGetParam(llvm_fn, llvm_param++);
      if (llvm_value && name && name[0] != '\0') {
        LLVMSetValueName2(llvm_value, name, strlen(name));
      }
    }
  }
  return true;
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

  const char *name = lower_mir_function_symbol_name(lctx, fn);
  LLVMValueRef llvm_fn = LLVMGetNamedFunction(module, name);
  if (!llvm_fn) {
    llvm_fn = LLVMAddFunction(module, name, fn_type);
  }
  if (!llvm_fn) {
    return NULL;
  }
  if (!lower_mir_name_function_params(fn, llvm_fn)) {
    return NULL;
  }

  LLVMSetLinkage(llvm_fn, LLVMExternalLinkage);
  if (is_coroutine_constructor_type(fn->type)) {
    LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
    LLVMAttributeRef attr =
        LLVMCreateEnumAttribute(llvm_ctx, PRESPLIT_COROUTINE_KIND_ID, 0);
    LLVMAddAttributeAtIndex(llvm_fn, LLVMAttributeFunctionIndex, attr);
  }
  if (fn->is_extern) {
    LLVMSetFunctionCallConv(llvm_fn, LLVMCCallConv);
    if (strcmp(name, "hash_string") == 0) {
      set_memory_effects(llvm_fn, MEM_ARGMEM_REF);
    }
  }
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
    MirFunction *fn =
        program->functions.items ? program->functions.items[i] : NULL;
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

static MirInstr *lower_mir_find_callable_def(MirFunction *fn,
                                             MirValueId value) {
  for (size_t depth = 0; depth < 16 && fn && value != MIR_NO_VALUE; depth++) {
    MirInstr *def = mir_function_find_def_instr(fn, value);
    if (!def) {
      return NULL;
    }

    if (def->kind == MIR_OP && def->data.op.argc > 0) {
      switch (def->data.op.kind) {
      case MIR_OP_KIND_CAST:
      case MIR_OP_KIND_DUP:
      case MIR_OP_KIND_DROP:
      case MIR_OP_KIND_DROP_REUSE:
        value = def->data.op.operands[0];
        continue;
      default:
        break;
      }
    }

    return def;
  }

  return NULL;
}

static MirFunction *lower_mir_resolve_fn_ref_target(MirLlvmCtx *lctx,
                                                    MirInstr *fn_ref) {
  if (!lctx || !fn_ref || fn_ref->kind != MIR_FN_REF) {
    return NULL;
  }

  MirFunction *target = fn_ref->data.fn_ref.fn;
  if ((!target || target->id >= lctx->functions_len) &&
      fn_ref->data.fn_ref.name) {
    MirFunction *named = lower_mir_find_function_by_name(
        lctx->program, fn_ref->data.fn_ref.name);
    if (named) {
      target = named;
    }
  }

  return target;
}

static LLVMValueRef lower_mir_get_function_value(MirLlvmCtx *lctx,
                                                 MirFunction *target,
                                                 LLVMModuleRef module,
                                                 LLVMTypeRef *out_type) {
  if (!lctx || !target) {
    return NULL;
  }

  LLVMValueRef fn = NULL;
  LLVMTypeRef fn_type = NULL;
  if (target->id < lctx->functions_len) {
    fn = lctx->functions[target->id];
    fn_type = lctx->function_types[target->id];
  }

  if (!fn) {
    const char *name = lower_mir_function_symbol_name(lctx, target);
    fn = LLVMGetNamedFunction(module, name);
  }
  if (!fn && target->id < lctx->functions_len) {
    fn = lower_mir_declare_function(lctx, target, module);
  }
  if (!fn_type) {
    fn_type = lower_mir_function_type(target, &lctx->jit_ctx, module);
  }
  if (!fn && fn_type) {
    const char *name = lower_mir_function_symbol_name(lctx, target);
    fn = LLVMAddFunction(module, name, fn_type);
    if (fn) {
      LLVMSetLinkage(fn, LLVMExternalLinkage);
      if (lower_mir_function_uses_c_abi(lctx, target)) {
        LLVMSetFunctionCallConv(fn, LLVMCCallConv);
      }
    }
  }

  if (out_type) {
    *out_type = fn_type;
  }
  return fn;
}

static const char *lower_mir_raw_function_symbol_name(const char *name) {
  if (!name) {
    return NULL;
  }
  return strcmp(name, "$top") == 0 ? "top" : name;
}

static LLVMValueRef lower_mir_get_named_function_value(const char *name,
                                                       LLVMTypeRef fn_type,
                                                       LLVMModuleRef module) {
  const char *symbol = lower_mir_raw_function_symbol_name(name);
  if (!symbol || !fn_type) {
    return NULL;
  }

  LLVMValueRef fn = LLVMGetNamedFunction(module, symbol);
  if (!fn) {
    fn = LLVMAddFunction(module, symbol, fn_type);
    if (fn) {
      LLVMSetLinkage(fn, LLVMExternalLinkage);
    }
  }
  return fn;
}

static LLVMTypeRef
lower_mir_function_type_from_value_type(Type *fn_type, JITLangCtx *ctx,
                                        LLVMModuleRef module) {
  if (!fn_type || fn_type->kind != T_FN ||
      lower_mir_type_has_unresolved_vars(fn_type)) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  size_t param_capacity = 0;
  for (Type *cursor = fn_type; cursor && cursor->kind == T_FN;
       cursor = cursor->data.T_FN.to) {
    Type *param_type = cursor->data.T_FN.from;
    if (param_type && param_type->kind != T_VOID) {
      param_capacity++;
    }
  }

  LLVMTypeRef *param_types =
      param_capacity ? calloc(param_capacity, sizeof(LLVMTypeRef)) : NULL;
  if (param_capacity && !param_types) {
    return NULL;
  }

  size_t param_count = 0;
  Type *cursor = fn_type;
  while (cursor && cursor->kind == T_FN) {
    Type *param_type = cursor->data.T_FN.from;
    if (param_type && param_type->kind != T_VOID) {
      param_types[param_count] =
          lower_mir_abi_value_type(param_type, ctx, module, NULL);
      if (!param_types[param_count]) {
        free(param_types);
        return NULL;
      }
      param_count++;
    }
    cursor = cursor->data.T_FN.to;
  }

  LLVMTypeRef ret_type = lower_mir_abi_value_type(
      cursor, ctx, module, LLVMVoidTypeInContext(llvm_ctx));
  if (!ret_type) {
    free(param_types);
    return NULL;
  }

  LLVMTypeRef llvm_fn_type =
      LLVMFunctionType(ret_type, param_types, (unsigned)param_count, 0);
  free(param_types);
  return llvm_fn_type;
}

typedef struct {
  MirValueId value;
  bool allowed;
  bool has_dlopen;
} MirDlopenStringUseCtx;

static bool lower_mir_dlopen_string_use_visitor(MirInstr *instr,
                                                MirOperand operand, void *ctx) {
  MirDlopenStringUseCtx *use_ctx = ctx;
  if (!use_ctx || operand.value != use_ctx->value) {
    return true;
  }

  if (!instr || instr->kind != MIR_OP) {
    use_ctx->allowed = false;
    return false;
  }

  switch (instr->data.op.kind) {
  case MIR_OP_KIND_DLOPEN:
    use_ctx->has_dlopen = true;
    return true;
  case MIR_OP_KIND_DUP:
  case MIR_OP_KIND_DROP:
  case MIR_OP_KIND_DROP_REUSE:
    return true;
  default:
    use_ctx->allowed = false;
    return false;
  }
}

static bool lower_mir_const_string_only_feeds_dlopen(MirFunction *fn,
                                                     MirValueId value) {
  if (!fn || value == MIR_NO_VALUE) {
    return false;
  }

  MirDlopenStringUseCtx use_ctx = {
      .value = value,
      .allowed = true,
      .has_dlopen = false,
  };

  for (size_t i = 0; i < fn->blocks.len && use_ctx.allowed; i++) {
    MirBlock *block = fn->blocks.items ? fn->blocks.items[i] : NULL;
    if (!block) {
      continue;
    }
    for (size_t j = 0; j < block->instrs.len && use_ctx.allowed; j++) {
      MirInstr *instr = block->instrs.items + j;
      if (instr->result == value) {
        continue;
      }
      mir_instr_for_each_operand(instr, lower_mir_dlopen_string_use_visitor,
                                 &use_ctx);
    }
    if (use_ctx.allowed) {
      mir_term_for_each_operand(&block->term,
                                lower_mir_dlopen_string_use_visitor, &use_ctx);
    }
  }

  return use_ctx.allowed && use_ctx.has_dlopen;
}

static bool lower_mir_value_allocates_on_stack(MirFunction *fn,
                                               MirValueId value) {
  EscapeMeta *meta = mir_value_escape_meta(fn, value);
  return meta && meta->status == EA_STACK_ALLOC;
}

static LLVMValueRef lower_mir_fn_ref(MirInstr *instr, MirLlvmCtx *lctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  if (!instr || !lctx) {
    return NULL;
  }

  MirFunction *target = lower_mir_resolve_fn_ref_target(lctx, instr);

  LLVMValueRef fn = lower_mir_get_function_value(lctx, target, module, NULL);
  if (!fn && instr->data.fn_ref.name) {
    LLVMTypeRef fn_type = lower_mir_function_type_from_value_type(
        instr->type, &lctx->jit_ctx, module);
    fn = lower_mir_get_named_function_value(instr->data.fn_ref.name, fn_type,
                                            module);
  }
  if (!fn) {
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

static LLVMValueRef lower_mir_const_string(MirFunction *fn, MirInstr *instr,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  if (!instr || !instr->type || !is_string_type(instr->type)) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  if (lower_mir_const_string_only_feeds_dlopen(fn, instr->result)) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  LLVMTypeRef char_type = LLVMInt8TypeInContext(llvm_ctx);
  size_t len = instr->data.const_value.as.string_value.len;
  const char *chars = instr->data.const_value.as.string_value.chars;
  if (!chars) {
    chars = "";
    len = 0;
  }

  unsigned storage_len = (unsigned)(len + 1);
  LLVMTypeRef backing_type = LLVMArrayType(char_type, storage_len);
  LLVMValueRef data =
      lower_mir_value_allocates_on_stack(fn, instr->result)
          ? lower_mir_stack_alloc_payload(module, builder, backing_type, 0,
                                          "string.data.stack")
          : lower_mir_heap_alloc_payload(module, builder, backing_type, 0,
                                         "string.data.heap");
  if (!data) {
    return NULL;
  }

  LLVMValueRef str_const =
      LLVMConstStringInContext(llvm_ctx, chars, (unsigned)len, 0);
  LLVMBuildStore(builder, str_const, data);

  LLVMValueRef result = LLVMGetUndef(codegen_string_type(char_type));
  result = LLVMBuildInsertValue(
      builder, result,
      LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), (uint64_t)len, false), 0,
      "string.size");
  result = LLVMBuildInsertValue(
      builder, result, LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false),
      1, "string.offset");
  return LLVMBuildInsertValue(builder, result, data, 2, "string.data");
}

static LLVMValueRef lower_mir_const(MirFunction *fn, MirInstr *instr,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder, JITLangCtx *ctx) {
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
    return LLVMConstInt(type, instr->data.const_value.as.bool_value ? 1 : 0,
                        false);
  }
  case MIR_CONST_KIND_VOID:
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  case MIR_CONST_KIND_UNDEF: {
    LLVMTypeRef type = lower_mir_type(instr->type, ctx, module,
                                      LLVMVoidTypeInContext(llvm_ctx));
    return type ? LLVMGetUndef(type) : NULL;
  }
  case MIR_CONST_KIND_STRING:
    if (instr->type && is_pointer_type(instr->type)) {
      return LLVMBuildGlobalStringPtr(
          builder,
          instr->data.const_value.as.string_value.chars
              ? instr->data.const_value.as.string_value.chars
              : "",
          "mir.cstr");
    }
    return lower_mir_const_string(fn, instr, module, builder);
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
  LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false);
  array_struct =
      LLVMBuildInsertValue(builder, array_struct, zero, 1, "array.offset");

  if (instr->data.construct.items.len == 0) {
    LLVMValueRef null_data = LLVMConstNull(LLVMPointerType(element_type, 0));
    return LLVMBuildInsertValue(builder, array_struct, null_data, 2,
                                "array.data");
  }

  LLVMTypeRef backing_type =
      LLVMArrayType(element_type, (unsigned)instr->data.construct.items.len);
  unsigned array_count = (unsigned)instr->data.construct.items.len;
  LLVMValueRef data =
      lower_mir_value_allocates_on_stack(fn, instr->result)
          ? lower_mir_stack_alloc_payload(module, builder, backing_type,
                                          array_count, "array.data.stack")
          : lower_mir_heap_alloc_payload(module, builder, backing_type,
                                         array_count, "array.data.heap");
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

  return LLVMBuildInsertValue(builder, array_struct, data, 2, "array.data");
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
  LLVMTypeRef element_type = lower_mir_value_storage_type(
      array_type->data.T_CONS.args[0], ctx, module);
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

static Type *lower_mir_pointer_pointee_type(MirFunction *fn,
                                            MirValueId ptr_id) {
  Type *ptr_type = mir_function_value_type(fn, ptr_id);
  if (!ptr_type || !is_pointer_type(ptr_type) || !ptr_type->data.T_CONS.args ||
      ptr_type->data.T_CONS.num_args < 1) {
    return NULL;
  }
  return ptr_type->data.T_CONS.args[0];
}

static LLVMValueRef lower_mir_cast_pointer_to_pointee(LLVMValueRef ptr,
                                                      LLVMTypeRef pointee_type,
                                                      LLVMBuilderRef builder,
                                                      const char *name) {
  if (!ptr || !pointee_type) {
    return NULL;
  }

  LLVMTypeRef typed_ptr_type = LLVMPointerType(pointee_type, 0);
  if (LLVMTypeOf(ptr) == typed_ptr_type) {
    return ptr;
  }
  if (LLVMGetTypeKind(LLVMTypeOf(ptr)) != LLVMPointerTypeKind) {
    return NULL;
  }
  return LLVMBuildPointerCast(builder, ptr, typed_ptr_type,
                              name ? name : "ptr.cast");
}

static LLVMValueRef lower_mir_ptr_offset(MirFunction *fn, MirInstr *instr,
                                         MirLlvmValueMap *values,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         JITLangCtx *ctx) {
  if (!fn || !instr || !values || instr->data.op.argc != 2) {
    return NULL;
  }

  MirValueId ptr_id = instr->data.op.operands[0];
  Type *pointee = lower_mir_pointer_pointee_type(fn, ptr_id);
  LLVMTypeRef pointee_type = lower_mir_value_storage_type(pointee, ctx, module);
  LLVMValueRef ptr = mir_llvm_value_get_rvalue(values, ptr_id, builder);
  LLVMValueRef index =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[1], builder);
  if (!pointee_type || !ptr || !index) {
    return NULL;
  }

  ptr = lower_mir_cast_pointer_to_pointee(ptr, pointee_type, builder,
                                          "ptr.offset.base");
  if (!ptr) {
    return NULL;
  }

  LLVMValueRef offset =
      LLVMBuildGEP2(builder, pointee_type, ptr, &index, 1, "ptr.offset");
  LLVMTypeRef storage_type =
      lower_mir_value_storage_type(instr->type, ctx, module);
  if (storage_type && LLVMTypeOf(offset) != storage_type &&
      LLVMGetTypeKind(storage_type) == LLVMPointerTypeKind) {
    offset =
        LLVMBuildPointerCast(builder, offset, storage_type, "ptr.offset.cast");
  }
  return offset;
}

static LLVMValueRef lower_mir_ptr_load(MirFunction *fn, MirInstr *instr,
                                       MirLlvmValueMap *values,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder,
                                       JITLangCtx *ctx) {
  if (!fn || !instr || !values || instr->data.op.argc != 1) {
    return NULL;
  }

  LLVMTypeRef pointee_type =
      lower_mir_value_storage_type(instr->type, ctx, module);
  LLVMValueRef ptr =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
  if (!pointee_type || !ptr) {
    return NULL;
  }

  ptr = lower_mir_cast_pointer_to_pointee(ptr, pointee_type, builder,
                                          "ptr.load.ptr");
  if (!ptr) {
    return NULL;
  }
  return LLVMBuildLoad2(builder, pointee_type, ptr, "ptr.load");
}

static LLVMValueRef lower_mir_ptr_store(MirFunction *fn, MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx) {
  if (!fn || !instr || !values || instr->data.op.argc != 2) {
    return NULL;
  }

  MirValueId ptr_id = instr->data.op.operands[0];
  MirValueId value_id = instr->data.op.operands[1];
  Type *pointee = lower_mir_pointer_pointee_type(fn, ptr_id);
  if (!pointee) {
    pointee = mir_function_value_type(fn, value_id);
  }

  LLVMTypeRef pointee_type = lower_mir_value_storage_type(pointee, ctx, module);
  LLVMValueRef ptr = mir_llvm_value_get_rvalue(values, ptr_id, builder);
  LLVMValueRef value = mir_llvm_value_get_rvalue(values, value_id, builder);
  if (!pointee_type || !ptr || !value) {
    return NULL;
  }

  ptr = lower_mir_cast_pointer_to_pointee(ptr, pointee_type, builder,
                                          "ptr.store.ptr");
  value = lower_mir_cast_value_to_storage(value, pointee, ctx, module, builder,
                                          "ptr.store.value");
  if (!ptr || !value) {
    return NULL;
  }

  LLVMBuildStore(builder, value, ptr);
  return LLVMGetUndef(LLVMVoidTypeInContext(LLVMGetModuleContext(module)));
}

static LLVMValueRef lower_mir_global_value(LLVMModuleRef module,
                                           const char *name,
                                           LLVMTypeRef storage_type) {
  if (!module || !name || !storage_type) {
    return NULL;
  }

  LLVMValueRef global = LLVMGetNamedGlobal(module, name);
  if (global) {
    return global;
  }

  global = LLVMAddGlobal(module, storage_type, name);
  if (!global) {
    return NULL;
  }
  LLVMSetLinkage(global, LLVMInternalLinkage);
  LLVMSetInitializer(global, LLVMConstNull(storage_type));
  return global;
}

static LLVMTypeRef lower_mir_global_slot_type(LLVMModuleRef module) {
  LLVMContextRef ctx = LLVMGetModuleContext(module);
  return LLVMArrayType(LLVMPointerType(LLVMInt8TypeInContext(ctx), 0), 1024);
}

// Lower a global_load. When the instr carries a durable slot (>= 0), load
// through the process-global @global_storage_array[slot] (a void* slot),
// which persists across per-REPL-input LLVM modules. Otherwise fall back
// to a module-local per-name LLVM global (does not persist).
static LLVMValueRef lower_mir_global_load(MirInstr *instr, LLVMModuleRef module,
                                          LLVMBuilderRef builder,
                                          JITLangCtx *ctx) {
  if (!instr || !instr->data.op.global_name) {
    return NULL;
  }

  LLVMTypeRef storage_type =
      lower_mir_value_storage_type(instr->type, ctx, module);
  if (!storage_type) {
    return NULL;
  }

  // Defensive: the build side only stamps global_slot >= 0 for durable
  // globals, but guard explicitly so a negative value reaching here
  // (future code path) cannot be cast to a huge unsigned GEP index.
  if (instr->data.op.global_slot >= 0) {
    LLVMValueRef storage_array = get_global_storage_array(module);
    if (!storage_array) {
      return NULL;
    }
    LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
    LLVMValueRef slot_index =
        LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx),
                     (unsigned long)instr->data.op.global_slot, true);
    LLVMValueRef indices[] = {
        LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, true), slot_index};
    LLVMValueRef slot_ptr =
        LLVMBuildGEP2(builder, lower_mir_global_slot_type(module),
                      storage_array, indices, 2, "global.slot.ptr");
    LLVMValueRef void_ptr = LLVMBuildLoad2(
        builder, LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0), slot_ptr,
        "global.void.ptr");
    LLVMValueRef typed_ptr =
        LLVMBuildBitCast(builder, void_ptr, LLVMPointerType(storage_type, 0),
                         "global.typed.ptr");
    return LLVMBuildLoad2(builder, storage_type, typed_ptr, "global.load");
  }

  LLVMValueRef global =
      lower_mir_global_value(module, instr->data.op.global_name, storage_type);
  if (!global) {
    return NULL;
  }
  return LLVMBuildLoad2(builder, storage_type, global, "global.load");
}

static LLVMValueRef lower_mir_global_store(MirFunction *fn, MirInstr *instr,
                                           MirLlvmValueMap *values,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder,
                                           JITLangCtx *ctx) {
  if (!fn || !instr || !values || !instr->data.op.global_name ||
      instr->data.op.argc != 1) {
    return NULL;
  }

  MirValueId value_id = instr->data.op.operands[0];
  Type *type = instr->data.op.to_type ? instr->data.op.to_type
                                      : mir_function_value_type(fn, value_id);
  LLVMTypeRef storage_type = lower_mir_value_storage_type(type, ctx, module);
  LLVMValueRef value = mir_llvm_value_get_rvalue(values, value_id, builder);
  if (!storage_type || !value) {
    return NULL;
  }

  value = lower_mir_cast_value_to_storage(value, type, ctx, module, builder,
                                          "global.store.value");
  if (!value) {
    return NULL;
  }

  // Defensive: guard explicitly (see lower_mir_global_load) so a
  // negative slot cannot be cast to a huge unsigned GEP index.
  if (instr->data.op.global_slot >= 0) {
    // Persist the value across REPL inputs by boxing it in the
    // process-global storage array slot. On rebind (same name in a
    // later input) the slot already holds a box of the same type, so
    // reuse it: store the new value into the existing box in place
    // rather than mallocing a fresh one (avoids leaking the old box).
    LLVMValueRef storage_array = get_global_storage_array(module);
    if (!storage_array) {
      return NULL;
    }
    LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
    LLVMValueRef slot_index =
        LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx),
                     (unsigned long)instr->data.op.global_slot, true);
    LLVMValueRef indices[] = {
        LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, true), slot_index};
    LLVMValueRef slot_ptr =
        LLVMBuildGEP2(builder, lower_mir_global_slot_type(module),
                      storage_array, indices, 2, "global.slot.ptr");

    LLVMTypeRef i8_ptr_type =
        LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0);
    LLVMValueRef existing =
        LLVMBuildLoad2(builder, i8_ptr_type, slot_ptr, "global.existing.ptr");
    LLVMValueRef is_null =
        LLVMBuildIsNull(builder, existing, "global.slot.null");

    LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
    LLVMBasicBlockRef reuse_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, fn, "global.slot.reuse");
    LLVMBasicBlockRef alloc_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, fn, "global.slot.alloc");
    LLVMBasicBlockRef cont_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, fn, "global.slot.cont");
    LLVMBuildCondBr(builder, is_null, alloc_bb, reuse_bb);

    // Existing box: bitcast and store the new value into it in place.
    LLVMPositionBuilderAtEnd(builder, reuse_bb);
    LLVMValueRef reuse_typed =
        LLVMBuildBitCast(builder, existing, LLVMPointerType(storage_type, 0),
                         "global.reuse.ptr");
    LLVMBuildStore(builder, value, reuse_typed);
    LLVMBuildBr(builder, cont_bb);

    // No existing box: malloc a fresh one, store the value, and record
    // its address in the slot.
    LLVMPositionBuilderAtEnd(builder, alloc_bb);
    LLVMValueRef malloced =
        LLVMBuildMalloc(builder, storage_type, "global.malloc");
    LLVMBuildStore(builder, value, malloced);
    LLVMValueRef new_ptr =
        LLVMBuildBitCast(builder, malloced, i8_ptr_type, "global.box.ptr");
    LLVMBuildStore(builder, new_ptr, slot_ptr);
    LLVMBuildBr(builder, cont_bb);

    LLVMPositionBuilderAtEnd(builder, cont_bb);
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  LLVMValueRef global =
      lower_mir_global_value(module, instr->data.op.global_name, storage_type);
  if (!global) {
    return NULL;
  }
  LLVMBuildStore(builder, value, global);
  return LLVMGetUndef(LLVMVoidTypeInContext(LLVMGetModuleContext(module)));
}

static LLVMValueRef lower_mir_array_succ(MirFunction *fn, MirInstr *instr,
                                         MirLlvmValueMap *values,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder,
                                         JITLangCtx *ctx) {
  if (!fn || !instr || !values) {
    return NULL;
  }

  MirValueId array_id = instr->data.extract.value;
  Type *array_type = mir_function_value_type(fn, array_id);
  if (!array_type || !is_array_type(array_type) ||
      !array_type->data.T_CONS.args || array_type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  Type *element_type_ref = array_type->data.T_CONS.args[0];
  LLVMTypeRef element_type =
      lower_mir_value_storage_type(element_type_ref, ctx, module);
  LLVMValueRef array = mir_llvm_value_get_rvalue(values, array_id, builder);
  if (!element_type || !array) {
    return NULL;
  }

  LLVMTypeRef llvm_array_type = codegen_array_type(element_type);
  LLVMValueRef current_size =
      LLVMBuildExtractValue(builder, array, 0, "array.current_size");
  LLVMValueRef is_size_gt_zero = LLVMBuildICmp(
      builder, LLVMIntSGT, current_size,
      LLVMConstInt(LLVMTypeOf(current_size), 0, false), "array.has_items");
  LLVMValueRef size_mask = LLVMBuildZExt(
      builder, is_size_gt_zero, LLVMTypeOf(current_size), "array.succ.mask");
  LLVMValueRef new_size =
      LLVMBuildSub(builder, current_size, size_mask, "array.succ.size");
  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array, 2, "array.data");
  LLVMValueRef current_offset =
      LLVMBuildExtractValue(builder, array, 1, "array.current_offset");
  LLVMValueRef new_offset =
      LLVMBuildAdd(builder, current_offset, size_mask, "array.succ.offset");
  LLVMValueRef new_data_ptr = LLVMBuildGEP2(builder, element_type, data_ptr,
                                            &size_mask, 1, "array.succ.data");

  LLVMValueRef result = LLVMGetUndef(llvm_array_type);
  result = LLVMBuildInsertValue(builder, result, new_size, 0, "array.size");
  result = LLVMBuildInsertValue(builder, result, new_offset, 1, "array.offset");
  return LLVMBuildInsertValue(builder, result, new_data_ptr, 2, "array.data");
}

static LLVMValueRef lower_mir_array_offset(MirFunction *fn, MirInstr *instr,
                                           MirLlvmValueMap *values,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder,
                                           JITLangCtx *ctx) {
  if (!fn || !instr || !values) {
    return NULL;
  }

  MirValueId array_id = instr->data.extract.value;
  Type *array_type = mir_function_value_type(fn, array_id);
  if (!array_type || !is_array_type(array_type) ||
      !array_type->data.T_CONS.args || array_type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  Type *element_type_ref = array_type->data.T_CONS.args[0];
  LLVMTypeRef element_type =
      lower_mir_value_storage_type(element_type_ref, ctx, module);
  LLVMValueRef array = mir_llvm_value_get_rvalue(values, array_id, builder);
  LLVMValueRef offset = mir_llvm_value_get_rvalue(
      values, instr->data.extract.index_value, builder);
  if (!element_type || !array || !offset) {
    return NULL;
  }

  LLVMTypeRef llvm_array_type = codegen_array_type(element_type);
  LLVMValueRef current_size =
      LLVMBuildExtractValue(builder, array, 0, "array.current_size");
  if (LLVMTypeOf(offset) != LLVMTypeOf(current_size)) {
    offset =
        LLVMBuildIntCast(builder, offset, LLVMTypeOf(current_size), "offset");
  }
  LLVMValueRef is_size_gt_zero = LLVMBuildICmp(
      builder, LLVMIntSGT, current_size,
      LLVMConstInt(LLVMTypeOf(current_size), 0, false), "array.has_items");
  LLVMValueRef size_mask = LLVMBuildZExt(
      builder, is_size_gt_zero, LLVMTypeOf(current_size), "array.offset.mask");
  LLVMValueRef size_decrement =
      LLVMBuildMul(builder, offset, size_mask, "array.offset.decrement");
  LLVMValueRef new_size =
      LLVMBuildSub(builder, current_size, size_decrement, "array.offset.size");
  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array, 2, "array.data");
  LLVMValueRef current_offset =
      LLVMBuildExtractValue(builder, array, 1, "array.current_offset");
  LLVMValueRef new_offset = LLVMBuildAdd(builder, current_offset,
                                         size_decrement, "array.offset.offset");
  LLVMValueRef new_data_ptr = LLVMBuildGEP2(
      builder, element_type, data_ptr, &size_decrement, 1, "array.offset.data");

  LLVMValueRef result = LLVMGetUndef(llvm_array_type);
  result = LLVMBuildInsertValue(builder, result, new_size, 0, "array.size");
  result = LLVMBuildInsertValue(builder, result, new_offset, 1, "array.offset");
  return LLVMBuildInsertValue(builder, result, new_data_ptr, 2, "array.data");
}

static LLVMValueRef lower_mir_array_range(MirFunction *fn, MirInstr *instr,
                                          MirLlvmValueMap *values,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder,
                                          JITLangCtx *ctx) {
  if (!fn || !instr || !values || !instr->type || !is_array_type(instr->type) ||
      !instr->type->data.T_CONS.args || instr->type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  Type *element_type_ref = instr->type->data.T_CONS.args[0];
  LLVMTypeRef element_type =
      lower_mir_value_storage_type(element_type_ref, ctx, module);
  LLVMValueRef offset = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[0], builder);
  LLVMValueRef size = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[1], builder);
  LLVMValueRef array = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[2], builder);
  if (!element_type || !offset || !size || !array) {
    return NULL;
  }

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array, 2, "array.data");
  LLVMValueRef current_offset =
      LLVMBuildExtractValue(builder, array, 1, "array.current_offset");
  LLVMValueRef new_offset =
      LLVMBuildAdd(builder, current_offset, offset, "array.range.offset");
  LLVMValueRef new_data_ptr = LLVMBuildGEP2(builder, element_type, data_ptr,
                                            &offset, 1, "array.range.data");
  LLVMValueRef result = LLVMGetUndef(codegen_array_type(element_type));
  result = LLVMBuildInsertValue(builder, result, size, 0, "array.size");
  result = LLVMBuildInsertValue(builder, result, new_offset, 1, "array.offset");
  return LLVMBuildInsertValue(builder, result, new_data_ptr, 2, "array.data");
}

static LLVMValueRef lower_mir_array_fill_data(MirFunction *fn, MirInstr *instr,
                                              MirLlvmValueMap *values,
                                              LLVMModuleRef module,
                                              LLVMBuilderRef builder,
                                              JITLangCtx *ctx,
                                              bool call_fill_fn) {
  if (!fn || !instr || !values || !instr->type || !is_array_type(instr->type) ||
      !instr->type->data.T_CONS.args || instr->type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  Type *element_type_ref = instr->type->data.T_CONS.args[0];
  LLVMTypeRef element_type =
      lower_mir_value_storage_type(element_type_ref, ctx, module);
  LLVMValueRef size = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[0], builder);
  LLVMValueRef fill_source = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[1], builder);
  if (!element_type || !size || !fill_source) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i32 = LLVMInt32TypeInContext(llvm_ctx);
  if (LLVMTypeOf(size) != i32) {
    size = LLVMBuildIntCast(builder, size, i32, "array.fill.size");
  }

  LLVMValueRef data_ptr =
      lower_mir_value_allocates_on_stack(fn, instr->result)
          ? LLVMBuildArrayAlloca(builder, element_type, size,
                                 "array.fill.stack")
          : lower_mir_heap_alloc_array_payload(module, builder, element_type,
                                               size, "array.fill.heap");
  if (!data_ptr) {
    return NULL;
  }

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMValueRef parent = LLVMGetBasicBlockParent(entry_block);
  LLVMBasicBlockRef loop_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "array.fill.loop");
  LLVMBasicBlockRef after_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "array.fill.after");

  LLVMValueRef zero = LLVMConstInt(i32, 0, false);
  LLVMValueRef has_items =
      LLVMBuildICmp(builder, LLVMIntUGT, size, zero, "array.fill.nonempty");
  LLVMBuildCondBr(builder, has_items, loop_block, after_block);

  LLVMPositionBuilderAtEnd(builder, loop_block);
  LLVMValueRef index = LLVMBuildPhi(builder, i32, "array.fill.index");
  LLVMAddIncoming(index, &zero, &entry_block, 1);

  LLVMValueRef element = fill_source;
  if (call_fill_fn) {
    LLVMTypeRef fill_fn_type = LLVMFunctionType(element_type, &i32, 1, 0);
    element = LLVMBuildCall2(builder, fill_fn_type, fill_source, &index, 1,
                             "array.fill.element");
  }
  Type *fill_type =
      mir_function_value_type(fn, instr->data.construct.operands[1]);
  if (!call_fill_fn) {
    element = lower_mir_cast_value_to_storage(element, fill_type, ctx, module,
                                              builder, "array.fill.cast");
  }
  if (!element) {
    return NULL;
  }

  LLVMValueRef element_ptr = LLVMBuildGEP2(builder, element_type, data_ptr,
                                           &index, 1, "array.fill.ptr");
  LLVMBuildStore(builder, element, element_ptr);

  LLVMValueRef one = LLVMConstInt(i32, 1, false);
  LLVMValueRef next = LLVMBuildAdd(builder, index, one, "array.fill.next");
  LLVMAddIncoming(index, &next, &loop_block, 1);
  LLVMValueRef more =
      LLVMBuildICmp(builder, LLVMIntULT, next, size, "array.fill.more");
  LLVMBuildCondBr(builder, more, loop_block, after_block);

  LLVMPositionBuilderAtEnd(builder, after_block);
  LLVMValueRef result = LLVMGetUndef(codegen_array_type(element_type));
  result = LLVMBuildInsertValue(builder, result, size, 0, "array.size");
  result = LLVMBuildInsertValue(builder, result, zero, 1, "array.offset");
  return LLVMBuildInsertValue(builder, result, data_ptr, 2, "array.data");
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

  LLVMValueRef head = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[0], builder);
  LLVMValueRef tail = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[1], builder);
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
  LLVMValueRef node = NULL;
  MirValueId reuse_id = instr->data.construct.reuse_token;
  if (reuse_id != MIR_NO_VALUE) {
    /* Reuse-aware cons: the token is a payload pointer (null => fresh malloc).
       A recycled node keeps rc == 1 (drop-reuse left it) and the same
       {header, payload} layout, so we overwrite the payload slot in place. */
    LLVMValueRef token = mir_llvm_value_get_rvalue(values, reuse_id, builder);
    if (token) {
      LLVMTypeRef generic_ptr2 = lower_mir_generic_ptr_type(module);
      if (LLVMTypeOf(token) != generic_ptr2) {
        token =
            LLVMBuildBitCast(builder, token, generic_ptr2, "list.reuse.cast");
      }
      LLVMValueRef fresh =
          lower_mir_value_allocates_on_stack(fn, instr->result)
              ? lower_mir_stack_alloc_payload(module, builder, node_type, 0,
                                              "list.node.stack")
              : lower_mir_heap_alloc_payload(module, builder, node_type, 0,
                                             "list.node.heap");
      LLVMValueRef is_null =
          LLVMBuildICmp(builder, LLVMIntEQ, token, LLVMConstNull(generic_ptr2),
                        "list.reuse.is_null");
      node = LLVMBuildSelect(builder, is_null, fresh, token, "list.node");
    }
  }
  if (!node) {
    node = lower_mir_value_allocates_on_stack(fn, instr->result)
               ? lower_mir_stack_alloc_payload(module, builder, node_type, 0,
                                               "list.node.stack")
               : lower_mir_heap_alloc_payload(module, builder, node_type, 0,
                                              "list.node.heap");
  }
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
                                    LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!instr || !instr->type) {
    return NULL;
  }

  LLVMTypeRef tuple_type =
      lower_mir_aggregate_type(instr->type, ctx, module, NULL);
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
    if (LLVMGetTypeKind(tuple_type) == LLVMStructTypeKind &&
        i < LLVMCountStructElementTypes(tuple_type)) {
      LLVMTypeRef field_type =
          LLVMStructGetTypeAtIndex(tuple_type, (unsigned)i);
      if (field_type && LLVMTypeOf(item) != field_type &&
          LLVMGetTypeKind(LLVMTypeOf(item)) == LLVMPointerTypeKind &&
          LLVMGetTypeKind(field_type) == LLVMPointerTypeKind) {
        item = LLVMBuildPointerCast(builder, item, field_type,
                                    "tuple.field.ptr.cast");
      }
    }

    tuple = LLVMBuildInsertValue(builder, tuple, item, (unsigned)i, "tuple");
  }

  if (type_uses_boxed_recursive_storage(instr->type)) {
    LLVMValueRef boxed = LLVMBuildMalloc(builder, tuple_type, "boxed.record");
    if (!boxed) {
      return NULL;
    }
    LLVMBuildStore(builder, tuple, boxed);
    return boxed;
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
      lower_mir_aggregate_type(constructor_type, ctx, module, NULL);
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
    if (constructor_type->kind == T_CONS &&
        constructor_type->data.T_CONS.args &&
        i < (size_t)constructor_type->data.T_CONS.num_args) {
      field_type = constructor_type->data.T_CONS.args[i];
    }
    if (!field_type) {
      field_type = mir_function_value_type(fn, field_id);
    }

    field = lower_mir_cast_value_to_storage(field, field_type, ctx, module,
                                            builder, "variant.field.cast");
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
  LLVMBuildStore(builder, LLVMConstNull(union_type), storage);
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
  return LLVMBuildInsertValue(builder, wrapped, payload, 0, "variant.payload");
}

static LLVMValueRef lower_mir_variant(MirFunction *fn, MirInstr *instr,
                                      MirLlvmValueMap *values,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!instr || !instr->type || instr->data.construct.constructor_index < 0) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  if (is_simple_enum(instr->type)) {
    return LLVMConstInt(LLVMInt8TypeInContext(llvm_ctx),
                        (uint64_t)instr->data.construct.constructor_index,
                        false);
  }

  LLVMTypeRef variant_type = lower_mir_type(instr->type, ctx, module, NULL);
  if (!variant_type || LLVMGetTypeKind(variant_type) != LLVMStructTypeKind ||
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
    return LLVMBuildInsertValue(builder, variant, LLVMConstNull(storage_type),
                                1, "variant.payload");
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

  return LLVMBuildICmp(builder, LLVMIntEQ, list,
                       LLVMConstNull(LLVMTypeOf(list)), "list.is_empty");
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

static const char *lower_mir_list_eq_helper_name(Type *element_type) {
  if (!element_type) {
    return NULL;
  }

  switch (element_type->kind) {
  case T_BOOL:
    return "__ylc_mir_list_eq_bool";
  case T_CHAR:
    return "__ylc_mir_list_eq_char";
  case T_INT:
    return "__ylc_mir_list_eq_int";
  case T_UINT64:
    return "__ylc_mir_list_eq_uint64";
  case T_NUM:
    return "__ylc_mir_list_eq_num";
  default:
    return NULL;
  }
}

static LLVMValueRef lower_mir_primitive_eq_value(Type *type, LLVMValueRef lhs,
                                                 LLVMValueRef rhs,
                                                 LLVMBuilderRef builder) {
  if (!type || !lhs || !rhs || !builder) {
    return NULL;
  }

  switch (type->kind) {
  case T_BOOL:
  case T_CHAR:
  case T_INT:
  case T_UINT64:
    return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "list.elem.eq");
  case T_NUM:
    return LLVMBuildFCmp(builder, LLVMRealOEQ, lhs, rhs, "list.elem.eq");
  default:
    return NULL;
  }
}

static bool lower_mir_type_is_numeric_scalar(Type *type) {
  return type &&
         (type->kind == T_INT || type->kind == T_UINT64 || type->kind == T_NUM);
}

static bool lower_mir_type_is_eq_scalar(Type *type) {
  return type && (type->kind == T_BOOL || type->kind == T_CHAR ||
                  lower_mir_type_is_numeric_scalar(type));
}

static Type *lower_mir_scalar_eq_target_type(Type *lhs, Type *rhs) {
  if (!lower_mir_type_is_eq_scalar(lhs) || !lower_mir_type_is_eq_scalar(rhs)) {
    return NULL;
  }
  if (lhs->kind == rhs->kind) {
    return lhs;
  }
  if (lower_mir_type_is_numeric_scalar(lhs) &&
      lower_mir_type_is_numeric_scalar(rhs)) {
    if (lhs->kind == T_NUM || rhs->kind == T_NUM) {
      return lhs->kind == T_NUM ? lhs : rhs;
    }
    if (lhs->kind == T_UINT64 || rhs->kind == T_UINT64) {
      return lhs->kind == T_UINT64 ? lhs : rhs;
    }
    return lhs;
  }
  return NULL;
}

static LLVMValueRef lower_mir_cast_scalar_for_eq(LLVMValueRef value,
                                                 Type *from_type, Type *to_type,
                                                 LLVMModuleRef module,
                                                 LLVMBuilderRef builder,
                                                 JITLangCtx *ctx) {
  if (!value || !from_type || !to_type) {
    return NULL;
  }
  if (from_type->kind == to_type->kind) {
    return value;
  }

  LLVMTypeRef to_llvm_type = lower_mir_type(to_type, ctx, module, NULL);
  if (!to_llvm_type) {
    return NULL;
  }

  if (from_type->kind == T_NUM &&
      mir_llvm_is_integral_type(to_type, ctx, module)) {
    return mir_llvm_is_signed_integral_type(to_type)
               ? LLVMBuildFPToSI(builder, value, to_llvm_type, "eq.cast")
               : LLVMBuildFPToUI(builder, value, to_llvm_type, "eq.cast");
  }
  if (mir_llvm_is_integral_type(from_type, ctx, module) &&
      to_type->kind == T_NUM) {
    return mir_llvm_is_signed_integral_type(from_type)
               ? LLVMBuildSIToFP(builder, value, to_llvm_type, "eq.cast")
               : LLVMBuildUIToFP(builder, value, to_llvm_type, "eq.cast");
  }
  if (mir_llvm_is_integral_type(from_type, ctx, module) &&
      mir_llvm_is_integral_type(to_type, ctx, module)) {
    unsigned from_width = mir_llvm_int_width(from_type, ctx, module);
    unsigned to_width = mir_llvm_int_width(to_type, ctx, module);
    if (from_width < to_width) {
      return mir_llvm_is_signed_integral_type(from_type)
                 ? LLVMBuildSExt(builder, value, to_llvm_type, "eq.cast")
                 : LLVMBuildZExt(builder, value, to_llvm_type, "eq.cast");
    }
    if (from_width > to_width) {
      return LLVMBuildTrunc(builder, value, to_llvm_type, "eq.cast");
    }
    return LLVMBuildBitCast(builder, value, to_llvm_type, "eq.cast");
  }

  return NULL;
}

static LLVMValueRef lower_mir_eq_values(Type *lhs_type, Type *rhs_type,
                                        LLVMValueRef lhs, LLVMValueRef rhs,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx);

static LLVMValueRef
lower_mir_scalar_eq_values(Type *lhs_type, Type *rhs_type, LLVMValueRef lhs,
                           LLVMValueRef rhs, LLVMModuleRef module,
                           LLVMBuilderRef builder, JITLangCtx *ctx,
                           const char *name) {
  Type *target_type = lower_mir_scalar_eq_target_type(lhs_type, rhs_type);
  if (!target_type) {
    return NULL;
  }

  lhs = lower_mir_cast_scalar_for_eq(lhs, lhs_type, target_type, module,
                                     builder, ctx);
  rhs = lower_mir_cast_scalar_for_eq(rhs, rhs_type, target_type, module,
                                     builder, ctx);
  if (!lhs || !rhs) {
    return NULL;
  }

  return target_type->kind == T_NUM
             ? LLVMBuildFCmp(builder, LLVMRealOEQ, lhs, rhs, name)
             : LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, name);
}

static LLVMValueRef
lower_mir_builtin_scalar_eq_call(MirFunction *fn, MirInstr *instr,
                                 MirLlvmValueMap *values, LLVMModuleRef module,
                                 LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!fn || !instr || !instr->data.call.builtin || !values ||
      instr->data.call.operands.len != 2) {
    return NULL;
  }

  const char *name = instr->data.call.builtin->name;
  bool negate = false;
  if (name && strcmp(name, "!=") == 0) {
    negate = true;
  } else if (!name || strcmp(name, "==") != 0) {
    return NULL;
  }

  MirValueId lhs_id = instr->data.call.operands.items[0];
  MirValueId rhs_id = instr->data.call.operands.items[1];
  Type *lhs_type = mir_function_value_type(fn, lhs_id);
  Type *rhs_type = mir_function_value_type(fn, rhs_id);
  LLVMValueRef lhs = mir_llvm_value_get_rvalue(values, lhs_id, builder);
  LLVMValueRef rhs = mir_llvm_value_get_rvalue(values, rhs_id, builder);
  LLVMValueRef eq = lower_mir_scalar_eq_values(
      lhs_type, rhs_type, lhs, rhs, module, builder, ctx, "scalar.eq");
  if (!eq) {
    return NULL;
  }
  return negate ? LLVMBuildNot(builder, eq, "scalar.neq") : eq;
}

static LLVMValueRef
lower_mir_option_eq_values(Type *lhs_type, Type *rhs_type, LLVMValueRef lhs,
                           LLVMValueRef rhs, LLVMModuleRef module,
                           LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!is_option_type(lhs_type) || !is_option_type(rhs_type)) {
    return NULL;
  }

  Type *lhs_payload_type = type_of_option(lhs_type);
  Type *rhs_payload_type = type_of_option(rhs_type);

  if (!lhs || !rhs || LLVMGetTypeKind(LLVMTypeOf(lhs)) != LLVMStructTypeKind ||
      LLVMGetTypeKind(LLVMTypeOf(rhs)) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(LLVMTypeOf(lhs)) < 2 ||
      LLVMCountStructElementTypes(LLVMTypeOf(rhs)) < 2) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMValueRef parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMBasicBlockRef tag_mismatch =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "option.eq.tag_mismatch");
  LLVMBasicBlockRef same_tag =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "option.eq.same_tag");
  LLVMBasicBlockRef some_payload =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "option.eq.some");
  LLVMBasicBlockRef none_payload =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "option.eq.none");
  LLVMBasicBlockRef merge =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "option.eq.merge");

  LLVMValueRef lhs_tag = LLVMBuildExtractValue(builder, lhs, 0, "lhs.tag");
  LLVMValueRef rhs_tag = LLVMBuildExtractValue(builder, rhs, 0, "rhs.tag");
  LLVMValueRef tags_equal =
      LLVMBuildICmp(builder, LLVMIntEQ, lhs_tag, rhs_tag, "option.tags.eq");
  LLVMBuildCondBr(builder, tags_equal, same_tag, tag_mismatch);

  LLVMPositionBuilderAtEnd(builder, tag_mismatch);
  LLVMBuildBr(builder, merge);

  LLVMPositionBuilderAtEnd(builder, same_tag);
  LLVMValueRef some_tag = LLVMConstInt(LLVMTypeOf(lhs_tag), 0, false);
  LLVMValueRef is_some =
      LLVMBuildICmp(builder, LLVMIntEQ, lhs_tag, some_tag, "option.is_some");
  LLVMBuildCondBr(builder, is_some, some_payload, none_payload);

  LLVMPositionBuilderAtEnd(builder, none_payload);
  LLVMBuildBr(builder, merge);

  LLVMPositionBuilderAtEnd(builder, some_payload);
  LLVMValueRef lhs_payload =
      LLVMBuildExtractValue(builder, lhs, 1, "lhs.payload");
  LLVMValueRef rhs_payload =
      LLVMBuildExtractValue(builder, rhs, 1, "rhs.payload");
  LLVMValueRef payloads_equal =
      lower_mir_eq_values(lhs_payload_type, rhs_payload_type, lhs_payload,
                          rhs_payload, module, builder, ctx);
  if (!payloads_equal) {
    return NULL;
  }
  LLVMBuildBr(builder, merge);

  LLVMBasicBlockRef some_payload_exit = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, merge);
  LLVMTypeRef i1_type = LLVMInt1TypeInContext(llvm_ctx);
  LLVMValueRef result = LLVMBuildPhi(builder, i1_type, "option.eq");
  LLVMValueRef false_value = LLVMConstInt(i1_type, 0, false);
  LLVMValueRef true_value = LLVMConstInt(i1_type, 1, false);
  LLVMValueRef incoming_values[] = {false_value, true_value, payloads_equal};
  LLVMBasicBlockRef incoming_blocks[] = {tag_mismatch, none_payload,
                                         some_payload_exit};
  LLVMAddIncoming(result, incoming_values, incoming_blocks, 3);

  return result;
}

static LLVMValueRef
lower_mir_builtin_option_eq_call(MirFunction *fn, MirInstr *instr,
                                 MirLlvmValueMap *values, LLVMModuleRef module,
                                 LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!fn || !instr || !instr->data.call.builtin || !values ||
      instr->data.call.operands.len != 2) {
    return NULL;
  }

  const char *name = instr->data.call.builtin->name;
  bool negate = false;
  if (name && strcmp(name, "!=") == 0) {
    negate = true;
  } else if (!name || strcmp(name, "==") != 0) {
    return NULL;
  }

  MirValueId lhs_id = instr->data.call.operands.items[0];
  MirValueId rhs_id = instr->data.call.operands.items[1];
  Type *lhs_type = mir_function_value_type(fn, lhs_id);
  Type *rhs_type = mir_function_value_type(fn, rhs_id);
  LLVMValueRef lhs = mir_llvm_value_get_rvalue(values, lhs_id, builder);
  LLVMValueRef rhs = mir_llvm_value_get_rvalue(values, rhs_id, builder);
  LLVMValueRef result = lower_mir_option_eq_values(lhs_type, rhs_type, lhs, rhs,
                                                   module, builder, ctx);
  if (!result) {
    return NULL;
  }
  return negate ? LLVMBuildNot(builder, result, "option.neq") : result;
}

static LLVMValueRef lower_mir_builtin_logical_call(MirFunction *fn,
                                                   MirInstr *instr,
                                                   MirLlvmValueMap *values,
                                                   LLVMBuilderRef builder) {
  if (!fn || !instr || !instr->data.call.builtin || !values) {
    return NULL;
  }

  const char *name = instr->data.call.builtin->name;
  if (!name) {
    return NULL;
  }

  if (strcmp(name, "!") == 0) {
    if (instr->data.call.operands.len != 1) {
      return NULL;
    }
    MirValueId value_id = instr->data.call.operands.items[0];
    Type *value_type = mir_function_value_type(fn, value_id);
    if (!value_type || value_type->kind != T_BOOL) {
      return NULL;
    }
    LLVMValueRef value = mir_llvm_value_get_rvalue(values, value_id, builder);
    return value ? LLVMBuildNot(builder, value, "logical.not") : NULL;
  }

  bool is_and = strcmp(name, "&&") == 0;
  bool is_or = strcmp(name, "||") == 0;
  if (!is_and && !is_or) {
    return NULL;
  }
  if (instr->data.call.operands.len != 2) {
    return NULL;
  }

  MirValueId lhs_id = instr->data.call.operands.items[0];
  MirValueId rhs_id = instr->data.call.operands.items[1];
  Type *lhs_type = mir_function_value_type(fn, lhs_id);
  Type *rhs_type = mir_function_value_type(fn, rhs_id);
  if (!lhs_type || !rhs_type || lhs_type->kind != T_BOOL ||
      rhs_type->kind != T_BOOL) {
    return NULL;
  }

  LLVMValueRef lhs = mir_llvm_value_get_rvalue(values, lhs_id, builder);
  LLVMValueRef rhs = mir_llvm_value_get_rvalue(values, rhs_id, builder);
  if (!lhs || !rhs) {
    return NULL;
  }
  return is_and ? LLVMBuildAnd(builder, lhs, rhs, "logical.and")
                : LLVMBuildOr(builder, lhs, rhs, "logical.or");
}

static LLVMValueRef lower_mir_get_list_eq_helper(Type *list_type,
                                                 LLVMModuleRef module,
                                                 JITLangCtx *ctx) {
  if (!list_type || !is_list_type(list_type) || !module) {
    return NULL;
  }

  Type *element_type_ref = type_of_list(list_type);
  const char *name = lower_mir_list_eq_helper_name(element_type_ref);
  if (!name) {
    return NULL;
  }

  LLVMValueRef existing = LLVMGetNamedFunction(module, name);
  if (existing) {
    return existing;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i1_type = LLVMInt1TypeInContext(llvm_ctx);
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  LLVMTypeRef params[] = {ptr_type, ptr_type};
  LLVMTypeRef fn_type = LLVMFunctionType(i1_type, params, 2, 0);
  LLVMValueRef helper = LLVMAddFunction(module, name, fn_type);
  if (!helper) {
    return NULL;
  }
  LLVMSetLinkage(helper, LLVMInternalLinkage);

  LLVMTypeRef element_type =
      lower_mir_value_storage_type(element_type_ref, ctx, module);
  if (!element_type) {
    return NULL;
  }
  LLVMTypeRef node_type = llnode_type(element_type);

  LLVMBuilderRef helper_builder = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(llvm_ctx, helper, "entry");
  LLVMBasicBlockRef loop =
      LLVMAppendBasicBlockInContext(llvm_ctx, helper, "list.eq.loop");
  LLVMBasicBlockRef lhs_empty =
      LLVMAppendBasicBlockInContext(llvm_ctx, helper, "list.eq.lhs_empty");
  LLVMBasicBlockRef rhs_empty =
      LLVMAppendBasicBlockInContext(llvm_ctx, helper, "list.eq.rhs_empty");
  LLVMBasicBlockRef compare =
      LLVMAppendBasicBlockInContext(llvm_ctx, helper, "list.eq.compare");
  LLVMBasicBlockRef advance =
      LLVMAppendBasicBlockInContext(llvm_ctx, helper, "list.eq.advance");
  LLVMBasicBlockRef true_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, helper, "list.eq.true");
  LLVMBasicBlockRef false_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, helper, "list.eq.false");

  LLVMPositionBuilderAtEnd(helper_builder, entry);
  LLVMValueRef lhs_start = LLVMGetParam(helper, 0);
  LLVMValueRef rhs_start = LLVMGetParam(helper, 1);
  LLVMBuildBr(helper_builder, loop);

  LLVMPositionBuilderAtEnd(helper_builder, loop);
  LLVMValueRef lhs_cursor = LLVMBuildPhi(helper_builder, ptr_type, "lhs");
  LLVMValueRef rhs_cursor = LLVMBuildPhi(helper_builder, ptr_type, "rhs");
  LLVMAddIncoming(lhs_cursor, &lhs_start, &entry, 1);
  LLVMAddIncoming(rhs_cursor, &rhs_start, &entry, 1);
  LLVMValueRef null_ptr = LLVMConstNull(ptr_type);
  LLVMValueRef lhs_is_empty = LLVMBuildICmp(
      helper_builder, LLVMIntEQ, lhs_cursor, null_ptr, "lhs.is_empty");
  LLVMBuildCondBr(helper_builder, lhs_is_empty, lhs_empty, rhs_empty);

  LLVMPositionBuilderAtEnd(helper_builder, lhs_empty);
  LLVMValueRef rhs_also_empty = LLVMBuildICmp(
      helper_builder, LLVMIntEQ, rhs_cursor, null_ptr, "rhs.is_empty");
  LLVMBuildCondBr(helper_builder, rhs_also_empty, true_block, false_block);

  LLVMPositionBuilderAtEnd(helper_builder, rhs_empty);
  LLVMValueRef rhs_is_empty = LLVMBuildICmp(
      helper_builder, LLVMIntEQ, rhs_cursor, null_ptr, "rhs.is_empty");
  LLVMBuildCondBr(helper_builder, rhs_is_empty, false_block, compare);

  LLVMPositionBuilderAtEnd(helper_builder, compare);
  LLVMValueRef lhs_head_ptr = LLVMBuildStructGEP2(
      helper_builder, node_type, lhs_cursor, 0, "lhs.head.ptr");
  LLVMValueRef rhs_head_ptr = LLVMBuildStructGEP2(
      helper_builder, node_type, rhs_cursor, 0, "rhs.head.ptr");
  LLVMValueRef lhs_head =
      LLVMBuildLoad2(helper_builder, element_type, lhs_head_ptr, "lhs.head");
  LLVMValueRef rhs_head =
      LLVMBuildLoad2(helper_builder, element_type, rhs_head_ptr, "rhs.head");
  LLVMValueRef heads_equal = lower_mir_primitive_eq_value(
      element_type_ref, lhs_head, rhs_head, helper_builder);
  if (!heads_equal) {
    LLVMDisposeBuilder(helper_builder);
    return NULL;
  }
  LLVMBuildCondBr(helper_builder, heads_equal, advance, false_block);

  LLVMPositionBuilderAtEnd(helper_builder, advance);
  LLVMValueRef lhs_tail_ptr = LLVMBuildStructGEP2(
      helper_builder, node_type, lhs_cursor, 1, "lhs.tail.ptr");
  LLVMValueRef rhs_tail_ptr = LLVMBuildStructGEP2(
      helper_builder, node_type, rhs_cursor, 1, "rhs.tail.ptr");
  LLVMValueRef lhs_tail =
      LLVMBuildLoad2(helper_builder, ptr_type, lhs_tail_ptr, "lhs.tail");
  LLVMValueRef rhs_tail =
      LLVMBuildLoad2(helper_builder, ptr_type, rhs_tail_ptr, "rhs.tail");
  LLVMAddIncoming(lhs_cursor, &lhs_tail, &advance, 1);
  LLVMAddIncoming(rhs_cursor, &rhs_tail, &advance, 1);
  LLVMBuildBr(helper_builder, loop);

  LLVMPositionBuilderAtEnd(helper_builder, true_block);
  LLVMBuildRet(helper_builder, LLVMConstInt(i1_type, 1, false));

  LLVMPositionBuilderAtEnd(helper_builder, false_block);
  LLVMBuildRet(helper_builder, LLVMConstInt(i1_type, 0, false));

  LLVMDisposeBuilder(helper_builder);
  return helper;
}

static LLVMValueRef lower_mir_list_eq_values(Type *lhs_type, Type *rhs_type,
                                             LLVMValueRef lhs, LLVMValueRef rhs,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder,
                                             JITLangCtx *ctx) {
  if (!is_list_type(lhs_type) || !is_list_type(rhs_type) ||
      !types_equal(type_of_list(lhs_type), type_of_list(rhs_type))) {
    return NULL;
  }

  LLVMValueRef helper = lower_mir_get_list_eq_helper(lhs_type, module, ctx);
  if (!helper || !lhs || !rhs) {
    return NULL;
  }

  LLVMValueRef args[] = {lhs, rhs};
  return LLVMBuildCall2(builder, LLVMGlobalGetValueType(helper), helper, args,
                        2, "list.eq");
}

static LLVMValueRef
lower_mir_builtin_list_eq_call(MirFunction *fn, MirInstr *instr,
                               MirLlvmValueMap *values, LLVMModuleRef module,
                               LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!fn || !instr || !instr->data.call.builtin || !values ||
      instr->data.call.operands.len != 2) {
    return NULL;
  }

  const char *name = instr->data.call.builtin->name;
  bool negate = false;
  if (name && strcmp(name, "!=") == 0) {
    negate = true;
  } else if (!name || strcmp(name, "==") != 0) {
    return NULL;
  }

  MirValueId lhs_id = instr->data.call.operands.items[0];
  MirValueId rhs_id = instr->data.call.operands.items[1];
  Type *lhs_type = mir_function_value_type(fn, lhs_id);
  Type *rhs_type = mir_function_value_type(fn, rhs_id);

  LLVMValueRef lhs = mir_llvm_value_get_rvalue(values, lhs_id, builder);
  LLVMValueRef rhs = mir_llvm_value_get_rvalue(values, rhs_id, builder);
  LLVMValueRef result = lower_mir_list_eq_values(lhs_type, rhs_type, lhs, rhs,
                                                 module, builder, ctx);
  if (!result) {
    return NULL;
  }

  return negate ? LLVMBuildNot(builder, result, "list.neq") : result;
}

static LLVMValueRef
lower_mir_array_eq_values(Type *lhs_type, Type *rhs_type, LLVMValueRef lhs,
                          LLVMValueRef rhs, LLVMModuleRef module,
                          LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!is_array_type(lhs_type) || !is_array_type(rhs_type) ||
      !lhs_type->data.T_CONS.args || lhs_type->data.T_CONS.num_args < 1 ||
      !rhs_type->data.T_CONS.args || rhs_type->data.T_CONS.num_args < 1 ||
      !types_equal(lhs_type->data.T_CONS.args[0],
                   rhs_type->data.T_CONS.args[0])) {
    return NULL;
  }

  Type *element_type_ref = lhs_type->data.T_CONS.args[0];
  LLVMTypeRef element_type =
      lower_mir_value_storage_type(element_type_ref, ctx, module);
  if (!element_type || !lhs || !rhs) {
    return NULL;
  }

  LLVMValueRef lhs_size = codegen_get_array_size(builder, lhs, element_type);
  LLVMValueRef rhs_size = codegen_get_array_size(builder, rhs, element_type);
  if (!lhs_size || !rhs_size) {
    return NULL;
  }

  LLVMValueRef sizes_equal =
      LLVMBuildICmp(builder, LLVMIntEQ, lhs_size, rhs_size, "array.sizes.eq");

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMValueRef parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef loop_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "array.eq.loop");
  LLVMBasicBlockRef compare_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "array.eq.compare");
  LLVMBasicBlockRef advance_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "array.eq.advance");
  LLVMBasicBlockRef true_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "array.eq.true");
  LLVMBasicBlockRef false_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "array.eq.false");
  LLVMBasicBlockRef merge_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, parent, "array.eq.merge");

  LLVMBuildCondBr(builder, sizes_equal, loop_block, false_block);

  LLVMPositionBuilderAtEnd(builder, loop_block);
  LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(lhs_size), 0, false);
  LLVMValueRef index = LLVMBuildPhi(builder, LLVMTypeOf(lhs_size), "array.idx");
  LLVMAddIncoming(index, &zero, &entry_block, 1);
  LLVMValueRef done =
      LLVMBuildICmp(builder, LLVMIntEQ, index, lhs_size, "array.eq.done");
  LLVMBuildCondBr(builder, done, true_block, compare_block);

  LLVMPositionBuilderAtEnd(builder, compare_block);
  LLVMValueRef lhs_element =
      get_array_element(builder, lhs, index, element_type);
  LLVMValueRef rhs_element =
      get_array_element(builder, rhs, index, element_type);
  LLVMValueRef elements_equal =
      lower_mir_eq_values(element_type_ref, rhs_type->data.T_CONS.args[0],
                          lhs_element, rhs_element, module, builder, ctx);
  if (!elements_equal) {
    return NULL;
  }
  LLVMBuildCondBr(builder, elements_equal, advance_block, false_block);

  LLVMPositionBuilderAtEnd(builder, advance_block);
  LLVMValueRef one = LLVMConstInt(LLVMTypeOf(lhs_size), 1, false);
  LLVMValueRef next = LLVMBuildAdd(builder, index, one, "array.idx.next");
  LLVMAddIncoming(index, &next, &advance_block, 1);
  LLVMBuildBr(builder, loop_block);

  LLVMPositionBuilderAtEnd(builder, true_block);
  LLVMBuildBr(builder, merge_block);

  LLVMPositionBuilderAtEnd(builder, false_block);
  LLVMBuildBr(builder, merge_block);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMTypeRef i1_type = LLVMInt1TypeInContext(llvm_ctx);
  LLVMValueRef result = LLVMBuildPhi(builder, i1_type, "array.eq");
  LLVMValueRef true_value = LLVMConstInt(i1_type, 1, false);
  LLVMValueRef false_value = LLVMConstInt(i1_type, 0, false);
  LLVMValueRef incoming_values[] = {true_value, false_value};
  LLVMBasicBlockRef incoming_blocks[] = {true_block, false_block};
  LLVMAddIncoming(result, incoming_values, incoming_blocks, 2);

  return result;
}

static LLVMValueRef
lower_mir_builtin_array_eq_call(MirFunction *fn, MirInstr *instr,
                                MirLlvmValueMap *values, LLVMModuleRef module,
                                LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!fn || !instr || !instr->data.call.builtin || !values ||
      instr->data.call.operands.len != 2) {
    return NULL;
  }

  const char *name = instr->data.call.builtin->name;
  bool negate = false;
  if (name && strcmp(name, "!=") == 0) {
    negate = true;
  } else if (!name || strcmp(name, "==") != 0) {
    return NULL;
  }

  MirValueId lhs_id = instr->data.call.operands.items[0];
  MirValueId rhs_id = instr->data.call.operands.items[1];
  Type *lhs_type = mir_function_value_type(fn, lhs_id);
  Type *rhs_type = mir_function_value_type(fn, rhs_id);
  LLVMValueRef lhs = mir_llvm_value_get_rvalue(values, lhs_id, builder);
  LLVMValueRef rhs = mir_llvm_value_get_rvalue(values, rhs_id, builder);
  LLVMValueRef result = lower_mir_array_eq_values(lhs_type, rhs_type, lhs, rhs,
                                                  module, builder, ctx);
  if (!result) {
    return NULL;
  }

  return negate ? LLVMBuildNot(builder, result, "array.neq") : result;
}

static LLVMValueRef lower_mir_eq_values(Type *lhs_type, Type *rhs_type,
                                        LLVMValueRef lhs, LLVMValueRef rhs,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx) {
  LLVMValueRef result = lower_mir_scalar_eq_values(lhs_type, rhs_type, lhs, rhs,
                                                   module, builder, ctx, "eq");
  if (result) {
    return result;
  }

  result = lower_mir_option_eq_values(lhs_type, rhs_type, lhs, rhs, module,
                                      builder, ctx);
  if (result) {
    return result;
  }

  result = lower_mir_list_eq_values(lhs_type, rhs_type, lhs, rhs, module,
                                    builder, ctx);
  if (result) {
    return result;
  }

  return lower_mir_array_eq_values(lhs_type, rhs_type, lhs, rhs, module,
                                   builder, ctx);
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

  LLVMValueRef expected = LLVMConstInt(
      LLVMTypeOf(tag), (uint64_t)instr->data.op.constructor_index, false);
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

  LLVMValueRef value =
      mir_llvm_value_get_rvalue(values, instr->data.extract.value, builder);
  if (!value) {
    return NULL;
  }

  LLVMTypeRef value_type = LLVMTypeOf(value);
  if (LLVMGetTypeKind(value_type) == LLVMStructTypeKind &&
      LLVMCountStructElementTypes(value_type) > 1) {
    LLVMValueRef payload =
        LLVMBuildExtractValue(builder, value, 1, "variant.payload.raw");
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

static bool lower_mir_is_zero_int(MirFunction *fn, MirValueId value);

static LLVMValueRef lower_mir_primitive_cast(MirFunction *fn, MirInstr *instr,
                                             MirLlvmValueMap *values,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder,
                                             JITLangCtx *ctx) {
  LLVMValueRef value =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
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

  if (mir_llvm_is_integral_type(from_type, ctx, module) &&
      is_pointer_type(to_type)) {
    if (lower_mir_is_zero_int(fn, instr->data.op.operands[0])) {
      return LLVMConstNull(to_llvm_type);
    }
    return LLVMBuildIntToPtr(builder, value, to_llvm_type, "inttoptr");
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

static LLVMValueRef lower_mir_primitive(MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMBuilderRef builder) {
  if (!instr || instr->kind != MIR_OP ||
      instr->data.op.kind != MIR_OP_KIND_PRIMITIVE) {
    return NULL;
  }

  if (instr->data.op.primitive == MIR_OP_LNOT) {
    if (instr->data.op.argc != 1) {
      return NULL;
    }
    LLVMValueRef value =
        mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
    return value ? LLVMBuildNot(builder, value, "not") : NULL;
  }

  if (instr->data.op.argc != 2) {
    return NULL;
  }
  LLVMValueRef lhs =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
  LLVMValueRef rhs =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[1], builder);
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

static Type *lower_mir_call_param_type(Type *callee_type, size_t arg_index) {
  size_t index = 0;
  for (Type *cursor = callee_type; cursor && cursor->kind == T_FN;
       cursor = cursor->data.T_FN.to) {
    Type *param_type = cursor->data.T_FN.from;
    if (!param_type || param_type->kind == T_VOID) {
      continue;
    }

    if (index == arg_index) {
      return param_type;
    }
    index++;
  }

  return NULL;
}

static bool lower_mir_is_ptr_int(Type *type) {
  return type && (type->kind == T_INT || type->kind == T_UINT64);
}

static bool lower_mir_is_zero_int(MirFunction *fn, MirValueId value) {
  static const int64_t null_ptr_literal = 0;

  MirInstr *def = mir_function_find_def_instr(fn, value);
  return def && def->kind == MIR_CONST &&
         ((def->data.const_value.kind == MIR_CONST_KIND_INT &&
           def->data.const_value.as.int_value == null_ptr_literal) ||
          (def->data.const_value.kind == MIR_CONST_KIND_UINT64 &&
           def->data.const_value.as.uint64_value == (uint64_t)null_ptr_literal));
}

static LLVMValueRef lower_mir_int_to_ptr_arg(MirFunction *fn,
                                             MirValueId value,
                                             LLVMValueRef arg,
                                             Type *arg_type,
                                             Type *param_type, JITLangCtx *ctx,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  if (!arg || !lower_mir_is_ptr_int(arg_type) || !param_type) {
    return NULL;
  }

  LLVMTypeRef abi_type = lower_mir_abi_value_type(param_type, ctx, module, NULL);
  if (!abi_type || LLVMGetTypeKind(abi_type) != LLVMPointerTypeKind) {
    return NULL;
  }

  if (lower_mir_is_zero_int(fn, value)) {
    return LLVMConstNull(abi_type);
  }

  if (LLVMGetTypeKind(LLVMTypeOf(arg)) != LLVMIntegerTypeKind) {
    return NULL;
  }
  return LLVMBuildIntToPtr(builder, arg, abi_type, "inttoptr");
}

static LLVMTypeRef lower_mir_extract_record_type(MirFunction *fn,
                                                 MirValueId value,
                                                 JITLangCtx *ctx,
                                                 LLVMModuleRef module) {
  Type *logical_type = mir_function_value_type(fn, value);
  if (lower_mir_value_is_env(fn, value)) {
    return lower_mir_closure_env_record_type(logical_type, ctx, module);
  }
  return lower_mir_aggregate_type(logical_type, ctx, module, NULL);
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
    return LLVMBuildExtractValue(
        builder, source, (unsigned)instr->data.extract.index, "extract.field");
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

  LLVMValueRef field_ptr = LLVMBuildStructGEP2(
      builder, record_type, source, (unsigned)instr->data.extract.index,
      "extract.field.ptr");
  return LLVMBuildLoad2(builder, field_type, field_ptr, "extract.field");
}

static LLVMTypeRef
lower_mir_indirect_call_type(MirFunction *fn, MirInstr *instr,
                             Type *callee_type, Type *result_type,
                             JITLangCtx *ctx, LLVMModuleRef module) {
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
      param_types[param_count] =
          lower_mir_call_operand_abi_type(fn, operand, param_type, ctx, module);
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

static LLVMTypeRef lower_mir_coro_indirect_constructor_type(
    MirFunction *fn, MirInstr *instr, JITLangCtx *ctx, LLVMModuleRef module) {
  if (!instr || !is_coroutine_constructor_type(instr->data.call.callee_type)) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef hidden_size_ptr =
      LLVMPointerType(LLVMInt64TypeInContext(llvm_ctx), 0);
  LLVMTypeRef ret_type = lower_mir_generic_ptr_type(module);

  size_t operand_count = instr->data.call.operands.len;
  LLVMTypeRef *param_types = calloc(operand_count + 1, sizeof(LLVMTypeRef));
  if (!param_types) {
    return NULL;
  }

  param_types[0] = hidden_size_ptr;
  size_t param_count = 1;
  size_t operand_index = 0;
  Type *cursor = instr->data.call.callee_type;
  while (cursor && cursor->kind == T_FN && operand_index < operand_count) {
    Type *param_type = cursor->data.T_FN.from;
    MirValueId operand = instr->data.call.operands.items[operand_index++];
    if (param_type && param_type->kind != T_VOID) {
      param_types[param_count] =
          lower_mir_call_operand_abi_type(fn, operand, param_type, ctx, module);
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

static MirFunction *lower_mir_call_fn_ref_target(MirLlvmCtx *lctx,
                                                 MirFunction *fn,
                                                 MirInstr *instr) {
  if (!lctx || !fn || !instr) {
    return NULL;
  }

  if (instr->data.call.specialized_fn &&
      !lower_mir_type_has_unresolved_vars(
          instr->data.call.specialized_fn->type)) {
    return instr->data.call.specialized_fn;
  }

  if (instr->data.call.callee == MIR_NO_VALUE) {
    return NULL;
  }

  MirInstr *callee_def =
      lower_mir_find_callable_def(fn, instr->data.call.callee);
  if (!callee_def || callee_def->kind != MIR_FN_REF) {
    return NULL;
  }

  MirFunction *target = lower_mir_resolve_fn_ref_target(lctx, callee_def);
  if (!target || lower_mir_type_has_unresolved_vars(target->type)) {
    return NULL;
  }

  return target;
}

static void lower_mir_emit_value_rc_marker(LLVMBuilderRef builder,
                                           LLVMValueRef value, Type *type,
                                           LLVMModuleRef module,
                                           JITLangCtx *ctx, bool is_dup);

static LLVMValueRef lower_mir_emit_coro_reset_handle(Type *yield_type,
                                                     LLVMValueRef handle,
                                                     MirLlvmCtx *lctx,
                                                     LLVMModuleRef module,
                                                     LLVMBuilderRef builder) {
  if (!yield_type || !handle || !lctx || !module || !builder) {
    return NULL;
  }

  LLVMTypeRef llvm_yield_type =
      type_to_llvm_type(yield_type, &lctx->jit_ctx, module);
  if (!llvm_yield_type) {
    return NULL;
  }

  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_yield_type);
  LLVMValueRef promise = GET_PROMISE_PTR(handle, promise_type);
  LLVMValueRef reset_fn = PROMISE_GET_RESET_FN(promise, promise_type);
  LLVMValueRef args_ptr = PROMISE_GET_ARGS_PTR(promise, promise_type);

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMValueRef llvm_fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  LLVMBasicBlockRef current = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef do_reset =
      LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "coro.reset.do");
  LLVMBasicBlockRef done =
      LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "coro.reset.done");

  LLVMTypeRef generic_ptr = lower_mir_generic_ptr_type(module);
  LLVMValueRef reset_is_null =
      LLVMBuildICmp(builder, LLVMIntEQ, reset_fn, LLVMConstNull(generic_ptr),
                    "coro.reset.is_null");
  LLVMBuildCondBr(builder, reset_is_null, done, do_reset);

  LLVMPositionBuilderAtEnd(builder, do_reset);
  LLVMValueRef frame_size_slot = LLVMBuildAlloca(
      builder, LLVMInt64TypeInContext(llvm_ctx), "coro.reset.frame_size");
  LLVMValueRef fresh = LLVMBuildCall2(
      builder, CORO_RESET_FN_TYPE, reset_fn,
      (LLVMValueRef[]){frame_size_slot, args_ptr}, 2, "coro.reset.fresh");
  if (LLVMTypeOf(fresh) != LLVMTypeOf(handle)) {
    fresh =
        LLVMBuildBitCast(builder, fresh, LLVMTypeOf(handle), "coro.reset.cast");
  }
  LLVMBuildBr(builder, done);
  LLVMBasicBlockRef do_reset_exit = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, done);
  LLVMValueRef result =
      LLVMBuildPhi(builder, LLVMTypeOf(handle), "coro.reset.result");
  LLVMAddIncoming(result, (LLVMValueRef[]){handle},
                  (LLVMBasicBlockRef[]){current}, 1);
  LLVMAddIncoming(result, (LLVMValueRef[]){fresh},
                  (LLVMBasicBlockRef[]){do_reset_exit}, 1);
  return result;
}

static const char *lower_mir_coro_reset_name(void) {
  static unsigned counter = 0;
  static char name[64];
  snprintf(name, sizeof(name), "$mir.coro.reset.%u", counter++);
  return name;
}

static bool lower_mir_coro_new_attach_reset(
    MirInstr *instr, LLVMValueRef handle, LLVMValueRef callee,
    LLVMTypeRef callee_type, LLVMValueRef *arg_values, LLVMTypeRef *arg_types,
    Type **arg_mir_types, size_t arg_count, MirLlvmCtx *lctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {
  Type *yield_type = lower_mir_coro_instance_yield_type(instr->type);
  LLVMTypeRef llvm_yield_type =
      type_to_llvm_type(yield_type, &lctx->jit_ctx, module);
  if (!yield_type || !llvm_yield_type || !handle || !callee || !callee_type) {
    return false;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef generic_ptr = lower_mir_generic_ptr_type(module);
  LLVMTypeRef *fields = calloc(arg_count + 1, sizeof(LLVMTypeRef));
  if (!fields) {
    return false;
  }
  fields[0] = generic_ptr;
  for (size_t i = 0; i < arg_count; i++) {
    fields[i + 1] = arg_types[i];
  }
  LLVMTypeRef args_type =
      LLVMStructTypeInContext(llvm_ctx, fields, (unsigned)(arg_count + 1), 0);
  free(fields);

  LLVMValueRef reset_fn =
      LLVMAddFunction(module, lower_mir_coro_reset_name(), CORO_RESET_FN_TYPE);
  LLVMSetLinkage(reset_fn, LLVMInternalLinkage);

  LLVMBasicBlockRef prev = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(llvm_ctx, reset_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef frame_size_out = LLVMGetParam(reset_fn, 0);
  LLVMValueRef args_raw = LLVMGetParam(reset_fn, 1);
  LLVMValueRef args = LLVMBuildBitCast(
      builder, args_raw, LLVMPointerType(args_type, 0), "coro.reset.args");

  LLVMValueRef reset_callee = LLVMBuildLoad2(
      builder, generic_ptr,
      LLVMBuildStructGEP2(builder, args_type, args, 0, "coro.reset.fn.ptr"),
      "coro.reset.fn");

  LLVMValueRef *reset_args = calloc(arg_count + 1, sizeof(LLVMValueRef));
  if (!reset_args) {
    LLVMPositionBuilderAtEnd(builder, prev);
    return false;
  }
  reset_args[0] = frame_size_out;
  for (size_t i = 0; i < arg_count; i++) {
    LLVMValueRef arg = LLVMBuildLoad2(
        builder, arg_types[i],
        LLVMBuildStructGEP2(builder, args_type, args, (unsigned)(i + 1),
                            "coro.reset.arg.ptr"),
        "coro.reset.arg");
    Type *arg_mir_type = arg_mir_types ? arg_mir_types[i] : NULL;
    Type *arg_yield_type = lower_mir_coro_instance_yield_type(arg_mir_type);
    if (arg_yield_type) {
      arg = lower_mir_emit_coro_reset_handle(arg_yield_type, arg, lctx, module,
                                             builder);
      if (!arg) {
        free(reset_args);
        LLVMPositionBuilderAtEnd(builder, prev);
        return false;
      }
    }
    reset_args[i + 1] = arg;
  }

  LLVMValueRef fresh =
      LLVMBuildCall2(builder, callee_type, reset_callee, reset_args,
                     (unsigned)(arg_count + 1), "coro.reset.handle");
  free(reset_args);

  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_yield_type);
  LLVMValueRef fresh_promise = GET_PROMISE_PTR(fresh, promise_type);
  PROMISE_SET_RESET_FN(fresh_promise, promise_type, reset_fn);
  PROMISE_SET_ARGS_PTR(fresh_promise, promise_type, args_raw);
  LLVMBuildRet(builder, fresh);

  LLVMPositionBuilderAtEnd(builder, prev);

  LLVMValueRef args_value = LLVMGetUndef(args_type);
  LLVMValueRef stored_callee = callee;
  if (LLVMTypeOf(stored_callee) != generic_ptr) {
    stored_callee =
        LLVMBuildBitCast(builder, stored_callee, generic_ptr, "coro.fn.cast");
  }
  args_value =
      LLVMBuildInsertValue(builder, args_value, stored_callee, 0, "coro.fn");
  for (size_t i = 0; i < arg_count; i++) {
    lower_mir_emit_value_rc_marker(builder, arg_values[i],
                                   arg_mir_types ? arg_mir_types[i] : NULL,
                                   module, &lctx->jit_ctx, true);
    args_value = LLVMBuildInsertValue(builder, args_value, arg_values[i],
                                      (unsigned)(i + 1), "coro.arg");
  }

  LLVMValueRef args_ptr =
      LLVMBuildMalloc(builder, args_type, "coro.reset.args.alloc");
  LLVMBuildStore(builder, args_value, args_ptr);

  LLVMValueRef promise = GET_PROMISE_PTR(handle, promise_type);
  PROMISE_SET_RESET_FN(promise, promise_type, reset_fn);
  PROMISE_SET_ARGS_PTR(promise, promise_type, args_ptr);
  return true;
}

static LLVMValueRef lower_mir_coro_new(MirFunction *fn, MirInstr *instr,
                                       MirLlvmValueMap *values,
                                       MirLlvmCtx *lctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  if (!fn || !instr || !values || !lctx) {
    return NULL;
  }

  MirFunction *target = lower_mir_call_fn_ref_target(lctx, fn, instr);
  LLVMValueRef callee = NULL;
  LLVMTypeRef callee_type = NULL;
  if (target) {
    callee = lower_mir_get_function_value(lctx, target, module, &callee_type);
  }

  if (!callee && instr->data.call.callee != MIR_NO_VALUE) {
    callee =
        mir_llvm_value_get_rvalue(values, instr->data.call.callee, builder);
  }
  if (!callee) {
    fprintf(stderr,
            "MIR to LLVM lowering could not resolve coroutine constructor in "
            "%s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return NULL;
  }

  if (!callee_type && target) {
    callee_type = lower_mir_function_type(target, &lctx->jit_ctx, module);
  }
  if (!callee_type) {
    callee_type = lower_mir_coro_indirect_constructor_type(
        fn, instr, &lctx->jit_ctx, module);
  }
  if (!callee_type) {
    fprintf(stderr,
            "MIR to LLVM lowering could not resolve coroutine constructor "
            "type in %s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return NULL;
  }

  size_t operand_capacity = instr->data.call.operands.len;
  LLVMValueRef *args = calloc(operand_capacity + 1, sizeof(LLVMValueRef));
  LLVMValueRef *arg_values =
      operand_capacity ? calloc(operand_capacity, sizeof(LLVMValueRef)) : NULL;
  LLVMTypeRef *arg_types =
      operand_capacity ? calloc(operand_capacity, sizeof(LLVMTypeRef)) : NULL;
  Type **arg_mir_types =
      operand_capacity ? calloc(operand_capacity, sizeof(Type *)) : NULL;
  if (!args ||
      (operand_capacity && (!arg_values || !arg_types || !arg_mir_types))) {
    free(args);
    free(arg_values);
    free(arg_types);
    free(arg_mir_types);
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  args[0] = LLVMBuildAlloca(builder, LLVMInt64TypeInContext(llvm_ctx),
                            "coro.size.slot");
  size_t arg_count = 1;
  size_t captured_arg_count = 0;
  for (size_t i = 0; i < instr->data.call.operands.len; i++) {
    MirValueId operand_id = instr->data.call.operands.items[i];
    Type *operand_type = mir_function_value_type(fn, operand_id);
    if (operand_type && operand_type->kind == T_VOID) {
      continue;
    }

    LLVMValueRef arg = mir_llvm_value_get_rvalue(values, operand_id, builder);
    if (!arg) {
      free(args);
      free(arg_values);
      free(arg_types);
      free(arg_mir_types);
      return NULL;
    }
    args[arg_count++] = arg;
    arg_values[captured_arg_count] = arg;
    arg_types[captured_arg_count] = LLVMTypeOf(arg);
    arg_mir_types[captured_arg_count] = operand_type;
    captured_arg_count++;
  }

  LLVMValueRef handle = LLVMBuildCall2(builder, callee_type, callee, args,
                                       (unsigned)arg_count, "coro.handle");
  lower_mir_coro_new_attach_reset(instr, handle, callee, callee_type,
                                  arg_values, arg_types, arg_mir_types,
                                  captured_arg_count, lctx, module, builder);
  free(args);
  free(arg_values);
  free(arg_types);
  free(arg_mir_types);
  return handle;
}

static LLVMValueRef lower_mir_coro_reset(MirFunction *fn, MirInstr *instr,
                                         MirLlvmValueMap *values,
                                         MirLlvmCtx *lctx, LLVMModuleRef module,
                                         LLVMBuilderRef builder) {
  if (!fn || !instr || !values || !lctx ||
      instr->data.call.callee == MIR_NO_VALUE) {
    return NULL;
  }

  Type *coro_type = mir_function_value_type(fn, instr->data.call.callee);
  if (!is_coroutine_type(coro_type)) {
    coro_type = instr->data.call.callee_type;
  }
  Type *yield_type = lower_mir_coro_instance_yield_type(coro_type);
  if (!yield_type) {
    return NULL;
  }

  LLVMValueRef handle =
      mir_llvm_value_get_rvalue(values, instr->data.call.callee, builder);
  if (!handle) {
    return NULL;
  }

  return lower_mir_emit_coro_reset_handle(yield_type, handle, lctx, module,
                                          builder);
}

static LLVMValueRef lower_mir_coro_resume_call(MirFunction *fn, MirInstr *instr,
                                               MirLlvmValueMap *values,
                                               MirLlvmCtx *lctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {
  if (!fn || !instr || !values || instr->data.call.callee == MIR_NO_VALUE) {
    return NULL;
  }

  Type *coro_type = mir_function_value_type(fn, instr->data.call.callee);
  if (!is_coroutine_type(coro_type)) {
    coro_type = instr->data.call.callee_type;
  }
  Type *yield_type = lower_mir_coro_instance_yield_type(coro_type);
  if (!yield_type) {
    return NULL;
  }

  for (size_t i = 0; i < instr->data.call.operands.len; i++) {
    Type *operand_type =
        mir_function_value_type(fn, instr->data.call.operands.items[i]);
    if (!operand_type || operand_type->kind != T_VOID) {
      fprintf(stderr,
              "MIR to LLVM lowering only supports nullary coroutine resume "
              "calls in %s\n",
              fn && fn->name ? fn->name : "<anonymous>");
      return NULL;
    }
  }

  LLVMValueRef handle =
      mir_llvm_value_get_rvalue(values, instr->data.call.callee, builder);
  if (!handle) {
    return NULL;
  }

  LLVMTypeRef llvm_yield_type =
      type_to_llvm_type(yield_type, &lctx->jit_ctx, module);
  if (!llvm_yield_type) {
    return NULL;
  }
  return codegen_handle_resume(handle, llvm_yield_type, &lctx->jit_ctx, module,
                               builder);
}

static LLVMValueRef lower_mir_call(MirFunction *fn, MirInstr *instr,
                                   MirLlvmValueMap *values, MirLlvmCtx *lctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  if (!fn || !instr || !lctx) {
    return NULL;
  }

  LLVMValueRef coro_resume =
      lower_mir_coro_resume_call(fn, instr, values, lctx, module, builder);
  if (coro_resume) {
    return coro_resume;
  }

  LLVMValueRef builtin_result =
      lower_mir_builtin_logical_call(fn, instr, values, builder);
  if (builtin_result) {
    return builtin_result;
  }
  builtin_result = lower_mir_builtin_scalar_eq_call(fn, instr, values, module,
                                                    builder, &lctx->jit_ctx);
  if (builtin_result) {
    return builtin_result;
  }
  builtin_result = lower_mir_builtin_option_eq_call(fn, instr, values, module,
                                                    builder, &lctx->jit_ctx);
  if (builtin_result) {
    return builtin_result;
  }
  builtin_result = lower_mir_builtin_list_eq_call(fn, instr, values, module,
                                                  builder, &lctx->jit_ctx);
  if (builtin_result) {
    return builtin_result;
  }
  builtin_result = lower_mir_builtin_array_eq_call(fn, instr, values, module,
                                                   builder, &lctx->jit_ctx);
  if (builtin_result) {
    return builtin_result;
  }

  LLVMValueRef callee = NULL;
  LLVMTypeRef callee_type = NULL;
  MirFunction *callee_target = NULL;

  if (instr->data.call.specialized_fn &&
      !lower_mir_type_has_unresolved_vars(
          instr->data.call.specialized_fn->type)) {
    callee_target = instr->data.call.specialized_fn;
    callee =
        lower_mir_get_function_value(lctx, callee_target, module, &callee_type);
  }

  if (!callee && instr->data.call.specialized_name) {
    MirFunction *specialized = lower_mir_find_function_by_name(
        lctx->program, instr->data.call.specialized_name);
    if (specialized && !lower_mir_type_has_unresolved_vars(specialized->type)) {
      callee_target = specialized;
      callee =
          lower_mir_get_function_value(lctx, specialized, module, &callee_type);
    }
    if (!callee) {
      callee_type =
          lower_mir_indirect_call_type(fn, instr, instr->data.call.callee_type,
                                       instr->type, &lctx->jit_ctx, module);
      callee = lower_mir_get_named_function_value(
          instr->data.call.specialized_name, callee_type, module);
    }
  }

  if (!callee && instr->data.call.callee != MIR_NO_VALUE) {
    MirInstr *callee_def =
        lower_mir_find_callable_def(fn, instr->data.call.callee);
    if (callee_def && callee_def->kind == MIR_FN_REF) {
      MirFunction *target = lower_mir_resolve_fn_ref_target(lctx, callee_def);
      if (!target) {
        callee_type = lower_mir_indirect_call_type(
            fn, instr, instr->data.call.callee_type, instr->type,
            &lctx->jit_ctx, module);
        callee = lower_mir_get_named_function_value(
            callee_def->data.fn_ref.name, callee_type, module);
      } else if (lower_mir_type_has_unresolved_vars(target->type)) {
        fprintf(stderr,
                "MIR to LLVM lowering cannot lower unspecialized generic call "
                "$%s in %s\n",
                target->name ? target->name : "<anonymous>",
                fn && fn->name ? fn->name : "<anonymous>");
        return NULL;
      } else {
        callee_target = target;
        callee =
            lower_mir_get_function_value(lctx, target, module, &callee_type);
      }
    } else if (callee_def && callee_def->kind == MIR_EXTRACT &&
               callee_def->data.extract.kind == MIR_EXTRACT_CLOSURE_FN) {
      MirInstr *closure_def =
          lower_mir_find_callable_def(fn, callee_def->data.extract.value);
      if (closure_def && closure_def->kind == MIR_CONSTRUCT &&
          closure_def->data.construct.kind == MIR_CONSTRUCT_CLOSURE &&
          closure_def->data.construct.impl_fn < lctx->functions_len &&
          lctx->program && lctx->program->functions.items) {
        MirFunction *target =
            lctx->program->functions.items[closure_def->data.construct.impl_fn];
        if (target) {
          callee_target = target;
          callee =
              lower_mir_get_function_value(lctx, target, module, &callee_type);
        }
      }
    }
  }

  if (!callee && instr->data.call.callee != MIR_NO_VALUE) {
    callee =
        mir_llvm_value_get_rvalue(values, instr->data.call.callee, builder);
  }

  if (!callee) {
    fprintf(stderr,
            "MIR to LLVM lowering could not resolve call callee in %s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return NULL;
  }

  bool callee_uses_c_abi = lower_mir_function_uses_c_abi(lctx, callee_target);
  LLVMValueRef *args = NULL;
  size_t arg_capacity = instr->data.call.operands.len;
  if (callee_uses_c_abi) {
    arg_capacity *= 2;
  }
  if (arg_capacity > 0) {
    args = calloc(arg_capacity, sizeof(LLVMValueRef));
    if (!args) {
      return NULL;
    }
  }

  size_t arg_count = 0;
  size_t logical_arg_index = 0;
  Type *call_type = callee_target ? callee_target->type
                                  : instr->data.call.callee_type;
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

    Type *param_type =
        lower_mir_call_param_type(call_type, logical_arg_index);
    LLVMValueRef arg = mir_llvm_value_get_rvalue(values, operand_id, builder);
    if (!arg) {
      free(args);
      return NULL;
    }
    LLVMValueRef ptr_arg = lower_mir_int_to_ptr_arg(
        fn, operand_id, arg, operand_type, param_type, &lctx->jit_ctx, module,
        builder);
    if (ptr_arg) {
      arg = ptr_arg;
    }
    logical_arg_index++;

    if (callee_uses_c_abi) {
      if (!lower_mir_append_c_abi_call_arg(args, &arg_count, arg, operand_type,
                                           &lctx->jit_ctx, module, builder)) {
        free(args);
        return NULL;
      }
    } else {
      args[arg_count++] = arg;
    }
  }

  if (!callee_type) {
    callee_type =
        lower_mir_indirect_call_type(fn, instr, instr->data.call.callee_type,
                                     instr->type, &lctx->jit_ctx, module);
  }

  if (!callee_type) {
    free(args);
    return NULL;
  }

  LLVMValueRef result =
      LLVMBuildCall2(builder, callee_type, callee, args, (unsigned)arg_count,
                     instr->type && instr->type->kind == T_VOID ? "" : "call");
  free(args);
  return callee_uses_c_abi
             ? lower_mir_unpack_c_abi_view_result(
                   instr->type, result, &lctx->jit_ctx, module, builder)
             : result;
}

static LLVMValueRef lower_mir_phi(MirInstr *instr, LLVMModuleRef module,
                                  JITLangCtx *ctx, LLVMBuilderRef builder) {
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

  unsigned env_field_count = 0;
  if (instr->type && instr->type->closure_meta &&
      instr->type->closure_meta->kind == T_CONS) {
    env_field_count = (unsigned)instr->type->closure_meta->data.T_CONS.num_args;
  }
  LLVMValueRef env =
      lower_mir_value_allocates_on_stack(fn, instr->result)
          ? lower_mir_stack_alloc_payload(module, builder, record_type,
                                          env_field_count, "closure.env.stack")
          : lower_mir_heap_alloc_payload(module, builder, record_type,
                                         env_field_count, "closure.env");
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
    if (instr->type &&
        (instr->type->kind == T_CONS || instr->type->kind == T_SUM) &&
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
                                      LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!instr || !values) {
    return NULL;
  }

  LLVMTypeRef closure_type = lower_mir_closure_value_type(module);
  if (!closure_type) {
    return NULL;
  }

  LLVMValueRef fn = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[0], builder);
  LLVMValueRef env = mir_llvm_value_get_rvalue(
      values, instr->data.construct.operands[1], builder);
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

  LLVMValueRef env = LLVMBuildExtractValue(builder, closure, 1, "closure.env");
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
                                      LLVMBuilderRef builder, JITLangCtx *ctx) {
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
    return lower_mir_array_succ(fn, instr, values, module, builder, ctx);
  case MIR_EXTRACT_ARRAY_OFFSET:
    return lower_mir_array_offset(fn, instr, values, module, builder, ctx);
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
    return lower_mir_array_fill_data(fn, instr, values, module, builder, ctx,
                                     false);
  case MIR_CONSTRUCT_ARRAY_FILL:
    return lower_mir_array_fill_data(fn, instr, values, module, builder, ctx,
                                     true);
  case MIR_CONSTRUCT_ARRAY_RANGE:
    return lower_mir_array_range(fn, instr, values, module, builder, ctx);
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

static bool lower_mir_llvm_type_is_sized(LLVMTypeRef type) {
  if (!type) {
    return false;
  }

  switch (LLVMGetTypeKind(type)) {
  case LLVMVoidTypeKind:
  case LLVMLabelTypeKind:
  case LLVMMetadataTypeKind:
  case LLVMFunctionTypeKind:
    return false;
  case LLVMStructTypeKind:
    if (LLVMIsOpaqueStruct(type)) {
      return false;
    }
    for (unsigned i = 0; i < LLVMCountStructElementTypes(type); i++) {
      if (!lower_mir_llvm_type_is_sized(LLVMStructGetTypeAtIndex(type, i))) {
        return false;
      }
    }
    return true;
  case LLVMArrayTypeKind:
    return lower_mir_llvm_type_is_sized(LLVMGetElementType(type));
  default:
    return true;
  }
}

static Type *lower_mir_resolve_rc_type(Type *type, JITLangCtx *ctx);

static LLVMValueRef lower_mir_rc_managed_ptr(MirFunction *fn, MirInstr *instr,
                                             MirLlvmValueMap *values,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder,
                                             JITLangCtx *ctx) {
  if (!fn || !instr || !values) {
    return NULL;
  }

  MirValueId value_id = instr->data.op.operands[0];
  LLVMValueRef value = mir_llvm_value_get_rvalue(values, value_id, builder);
  if (!value) {
    return NULL;
  }

  Type *type = mir_function_value_type(fn, value_id);
  type = lower_mir_resolve_rc_type(type, ctx);
  LLVMTypeRef value_type = LLVMTypeOf(value);
  LLVMTypeKind value_kind = LLVMGetTypeKind(value_type);
  if (value_kind == LLVMPointerTypeKind) {
    /* Already lowered as a managed pointer, usually for recursive storage. */
  } else if (type && is_array_type(type) && type->data.T_CONS.args &&
             type->data.T_CONS.num_args > 0) {
    if (value_kind != LLVMStructTypeKind) {
      return NULL;
    }
    LLVMTypeRef element_type =
        lower_mir_value_storage_type(type->data.T_CONS.args[0], ctx, module);
    LLVMValueRef offset =
        LLVMBuildExtractValue(builder, value, 1, "rc.array.offset");
    LLVMValueRef data =
        LLVMBuildExtractValue(builder, value, 2, "rc.array.data");
    if (!element_type || !offset || !data) {
      return NULL;
    }
    if (lower_mir_llvm_type_is_sized(element_type)) {
      LLVMValueRef neg_offset =
          LLVMBuildNeg(builder, offset, "rc.array.base.offset");
      value = LLVMBuildGEP2(builder, element_type, data, &neg_offset, 1,
                            "rc.array.base");
    } else {
      value = data;
    }
  } else if (type && is_closure(type)) {
    if (value_kind != LLVMStructTypeKind) {
      return NULL;
    }
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

static LLVMValueRef lower_mir_free_fn(LLVMModuleRef module) {
  LLVMValueRef free_fn = LLVMGetNamedFunction(module, "free");
  if (free_fn) {
    return free_fn;
  }
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  LLVMTypeRef fn_type = LLVMFunctionType(
      LLVMVoidTypeInContext(LLVMGetModuleContext(module)), &ptr_type, 1, 0);
  return LLVMAddFunction(module, "free", fn_type);
}

static Type *lower_mir_find_rc_alias_type_in_type(Type *type, const char *alias,
                                                  int depth) {
  if (!type || !alias || depth > 12) {
    return NULL;
  }

  if ((type->kind == T_CONS || type->kind == T_SUM) && type->alias &&
      strcmp(type->alias, alias) == 0) {
    return type;
  }

  /* A T_RECURSIVE_REF is a back-edge to the canonical declaration, not an
     expansion site. Resolving it directly to the declaration's aliased type
     (when the decl carries the alias) avoids re-traversing the whole recursive
     structure, which would blow up exponentially (fanout^depth). */
  if (type->kind == T_RECURSIVE_REF && type->data.T_RECURSIVE_REF.name &&
      strcmp(type->data.T_RECURSIVE_REF.name, alias) == 0 &&
      type->data.T_RECURSIVE_REF.decl &&
      type->data.T_RECURSIVE_REF.decl->type) {
    Type *decl_type = type->data.T_RECURSIVE_REF.decl->type;
    if ((decl_type->kind == T_CONS || decl_type->kind == T_SUM) &&
        decl_type->alias && strcmp(decl_type->alias, alias) == 0) {
      return decl_type;
    }
  }

  if (type->closure_meta) {
    Type *found = lower_mir_find_rc_alias_type_in_type(type->closure_meta,
                                                       alias, depth + 1);
    if (found) {
      return found;
    }
  }

  switch (type->kind) {
  case T_CONS:
  case T_SUM:
    for (int i = 0; type->data.T_CONS.args && i < type->data.T_CONS.num_args;
         i++) {
      Type *found = lower_mir_find_rc_alias_type_in_type(
          type->data.T_CONS.args[i], alias, depth + 1);
      if (found) {
        return found;
      }
    }
    return NULL;

  case T_FN: {
    Type *found = lower_mir_find_rc_alias_type_in_type(type->data.T_FN.from,
                                                       alias, depth + 1);
    if (found) {
      return found;
    }
    return lower_mir_find_rc_alias_type_in_type(type->data.T_FN.to, alias,
                                                depth + 1);
  }

  default:
    return NULL;
  }
}

static Type *lower_mir_find_rc_alias_type(const char *alias) {
  MirProgram *program = lower_mir_current_program;
  if (!program || !alias) {
    return NULL;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    MirFunction *fn =
        program->functions.items ? program->functions.items[i] : NULL;
    if (!fn) {
      continue;
    }

    Type *found = lower_mir_find_rc_alias_type_in_type(fn->type, alias, 0);
    if (found) {
      return found;
    }

    for (size_t j = 0; j < fn->params.len; j++) {
      found = lower_mir_find_rc_alias_type_in_type(fn->params.items[j].type,
                                                   alias, 0);
      if (found) {
        return found;
      }
    }

    for (size_t j = 0; j < fn->values.len; j++) {
      found = lower_mir_find_rc_alias_type_in_type(fn->values.items[j].type,
                                                   alias, 0);
      if (found) {
        return found;
      }
    }

    for (size_t b = 0; b < fn->blocks.len; b++) {
      MirBlock *block = fn->blocks.items ? fn->blocks.items[b] : NULL;
      if (!block) {
        continue;
      }
      for (size_t j = 0; j < block->instrs.len; j++) {
        MirInstr *instr = &block->instrs.items[j];
        found = lower_mir_find_rc_alias_type_in_type(instr->type, alias, 0);
        if (found) {
          return found;
        }
        if (instr->kind == MIR_CONSTRUCT) {
          found = lower_mir_find_rc_alias_type_in_type(
              instr->data.construct.constructor_type, alias, 0);
          if (found) {
            return found;
          }
        } else if (instr->kind == MIR_CALL) {
          found = lower_mir_find_rc_alias_type_in_type(
              instr->data.call.callee_type, alias, 0);
          if (found) {
            return found;
          }
        }
      }
    }
  }

  return NULL;
}

static const char *lower_mir_type_decl_name(Type *type) {
  if (!type) {
    return NULL;
  }
  if (type->kind == T_RECURSIVE_REF) {
    return type->data.T_RECURSIVE_REF.name;
  }
  if (type->kind == T_VAR && type->is_recursive_type_ref) {
    return type->data.T_VAR.name;
  }
  if (type->kind == T_CONS || type->kind == T_SUM) {
    return type->alias ? type->alias : type->data.T_CONS.name;
  }
  return NULL;
}

static bool lower_mir_type_refers_to_name(Type *type, const char *name) {
  const char *decl_name = lower_mir_type_decl_name(type);
  return decl_name && name && strcmp(decl_name, name) == 0;
}

static bool lower_mir_types_same_rc_shape(Type *lhs, Type *rhs, int depth) {
  if (lhs == rhs) {
    return true;
  }
  if (!lhs || !rhs || depth > 16) {
    return false;
  }

  /* Two aliased aggregate types naming the same alias are the same canonical
     recursive shape. Short-circuiting here keeps the comparison O(1) for the
     common recursive case instead of expanding both trees to the depth cap
     (which would blow up as fanout^depth). */
  if ((lhs->kind == T_CONS || lhs->kind == T_SUM) && lhs->alias &&
      (rhs->kind == T_CONS || rhs->kind == T_SUM) && rhs->alias &&
      strcmp(lhs->alias, rhs->alias) == 0) {
    return true;
  }

  if ((lhs->kind == T_VAR && lhs->is_recursive_type_ref) ||
      lhs->kind == T_RECURSIVE_REF) {
    const char *name = lower_mir_type_decl_name(lhs);
    return lower_mir_type_refers_to_name(rhs, name);
  }
  if ((rhs->kind == T_VAR && rhs->is_recursive_type_ref) ||
      rhs->kind == T_RECURSIVE_REF) {
    const char *name = lower_mir_type_decl_name(rhs);
    return lower_mir_type_refers_to_name(lhs, name);
  }

  if (lhs->kind != rhs->kind) {
    return false;
  }

  switch (lhs->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_STRING:
  case T_EMPTY_LIST:
    return true;

  case T_VAR:
    return lhs->data.T_VAR.id == rhs->data.T_VAR.id;

  case T_CONS:
  case T_SUM:
    if (!lhs->data.T_CONS.name || !rhs->data.T_CONS.name ||
        strcmp(lhs->data.T_CONS.name, rhs->data.T_CONS.name) != 0 ||
        lhs->data.T_CONS.num_args != rhs->data.T_CONS.num_args) {
      return false;
    }
    for (int i = 0; i < lhs->data.T_CONS.num_args; i++) {
      Type *lhs_arg = lhs->data.T_CONS.args ? lhs->data.T_CONS.args[i] : NULL;
      Type *rhs_arg = rhs->data.T_CONS.args ? rhs->data.T_CONS.args[i] : NULL;
      if (!lower_mir_types_same_rc_shape(lhs_arg, rhs_arg, depth + 1)) {
        return false;
      }
    }
    return true;

  case T_FN:
    return lower_mir_types_same_rc_shape(lhs->data.T_FN.from,
                                         rhs->data.T_FN.from, depth + 1) &&
           lower_mir_types_same_rc_shape(lhs->data.T_FN.to, rhs->data.T_FN.to,
                                         depth + 1) &&
           lower_mir_types_same_rc_shape(lhs->closure_meta, rhs->closure_meta,
                                         depth + 1);

  default:
    return false;
  }
}

static Type *lower_mir_find_equivalent_rc_alias_type_in_type(Type *type,
                                                             Type *target,
                                                             int depth) {
  if (!type || !target || depth > 12) {
    return NULL;
  }

  if ((type->kind == T_CONS || type->kind == T_SUM) && type->alias &&
      lower_mir_types_same_rc_shape(type, target, 0)) {
    return type;
  }

  if (type->closure_meta) {
    Type *found = lower_mir_find_equivalent_rc_alias_type_in_type(
        type->closure_meta, target, depth + 1);
    if (found) {
      return found;
    }
  }

  switch (type->kind) {
  case T_CONS:
  case T_SUM:
    for (int i = 0; type->data.T_CONS.args && i < type->data.T_CONS.num_args;
         i++) {
      Type *found = lower_mir_find_equivalent_rc_alias_type_in_type(
          type->data.T_CONS.args[i], target, depth + 1);
      if (found) {
        return found;
      }
    }
    break;
  case T_FN: {
    Type *found = lower_mir_find_equivalent_rc_alias_type_in_type(
        type->data.T_FN.from, target, depth + 1);
    if (found) {
      return found;
    }
    return lower_mir_find_equivalent_rc_alias_type_in_type(type->data.T_FN.to,
                                                           target, depth + 1);
  }
  case T_RECURSIVE_REF:
    /* A T_RECURSIVE_REF is a back-edge to the canonical declaration. Resolve
       it to the declaration's type as a single node and compare that, rather
       than re-expanding the declaration's body (which would blow up
       exponentially for recursive aggregates). */
    if (type->data.T_RECURSIVE_REF.decl &&
        type->data.T_RECURSIVE_REF.decl->type &&
        type->data.T_RECURSIVE_REF.decl->type != type) {
      Type *decl_type = type->data.T_RECURSIVE_REF.decl->type;
      if ((decl_type->kind == T_CONS || decl_type->kind == T_SUM) &&
          decl_type->alias &&
          lower_mir_types_same_rc_shape(decl_type, target, 0)) {
        return decl_type;
      }
    }
    break;
  default:
    break;
  }

  return NULL;
}

static Type *lower_mir_find_equivalent_rc_alias_type(Type *target) {
  MirProgram *program = lower_mir_current_program;
  if (!program || !target) {
    return NULL;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    MirFunction *fn =
        program->functions.items ? program->functions.items[i] : NULL;
    if (!fn) {
      continue;
    }

    Type *found =
        lower_mir_find_equivalent_rc_alias_type_in_type(fn->type, target, 0);
    if (found) {
      return found;
    }

    for (size_t j = 0; j < fn->params.len; j++) {
      found = lower_mir_find_equivalent_rc_alias_type_in_type(
          fn->params.items[j].type, target, 0);
      if (found) {
        return found;
      }
    }

    for (size_t j = 0; j < fn->values.len; j++) {
      found = lower_mir_find_equivalent_rc_alias_type_in_type(
          fn->values.items[j].type, target, 0);
      if (found) {
        return found;
      }
    }

    for (size_t b = 0; b < fn->blocks.len; b++) {
      MirBlock *block = fn->blocks.items ? fn->blocks.items[b] : NULL;
      if (!block) {
        continue;
      }
      for (size_t j = 0; j < block->instrs.len; j++) {
        MirInstr *instr = &block->instrs.items[j];
        found = lower_mir_find_equivalent_rc_alias_type_in_type(instr->type,
                                                                target, 0);
        if (found) {
          return found;
        }
        if (instr->kind == MIR_CONSTRUCT) {
          found = lower_mir_find_equivalent_rc_alias_type_in_type(
              instr->data.construct.constructor_type, target, 0);
          if (found) {
            return found;
          }
        } else if (instr->kind == MIR_CALL) {
          found = lower_mir_find_equivalent_rc_alias_type_in_type(
              instr->data.call.callee_type, target, 0);
          if (found) {
            return found;
          }
        }
      }
    }
  }

  return NULL;
}

static Type *lower_mir_resolve_rc_type_refs(Type *type, JITLangCtx *ctx,
                                            int depth) {
  if (!type || !ctx || !ctx->env || depth > 6) {
    return type;
  }

  if ((type->kind == T_CONS || type->kind == T_SUM) && type->alias) {
    return type;
  }

  if (type->kind == T_VAR && type->is_recursive_type_ref &&
      type->data.T_VAR.name) {
    Type *aliased = lower_mir_find_rc_alias_type(type->data.T_VAR.name);
    if (aliased) {
      return deep_copy_type(aliased);
    }
    Type *decl_type = env_lookup(ctx->env, type->data.T_VAR.name);
    if (decl_type &&
        !(decl_type->kind == T_VAR && types_equal(decl_type, type))) {
      Type *resolved = resolve_type_in_env(deep_copy_type(decl_type), ctx->env);
      return lower_mir_resolve_rc_type_refs(resolved, ctx, depth + 1);
    }
    return type;
  }

  if (type->kind == T_RECURSIVE_REF && type->data.T_RECURSIVE_REF.name) {
    Type *aliased =
        lower_mir_find_rc_alias_type(type->data.T_RECURSIVE_REF.name);
    if (aliased) {
      return deep_copy_type(aliased);
    }
    Type *decl_type = type->data.T_RECURSIVE_REF.decl
                          ? type->data.T_RECURSIVE_REF.decl->type
                          : NULL;
    if (decl_type && decl_type != type) {
      return lower_mir_resolve_rc_type_refs(deep_copy_type(decl_type), ctx,
                                            depth + 1);
    }
    return type;
  }

  if (type->closure_meta) {
    type->closure_meta =
        lower_mir_resolve_rc_type_refs(type->closure_meta, ctx, depth + 1);
  }

  switch (type->kind) {
  case T_CONS:
  case T_SUM:
    for (int i = 0; type->data.T_CONS.args && i < type->data.T_CONS.num_args;
         i++) {
      type->data.T_CONS.args[i] = lower_mir_resolve_rc_type_refs(
          type->data.T_CONS.args[i], ctx, depth + 1);
    }
    break;
  case T_FN:
    type->data.T_FN.from =
        lower_mir_resolve_rc_type_refs(type->data.T_FN.from, ctx, depth + 1);
    type->data.T_FN.to =
        lower_mir_resolve_rc_type_refs(type->data.T_FN.to, ctx, depth + 1);
    break;
  default:
    break;
  }

  return type;
}

static Type *lower_mir_resolve_rc_type(Type *type, JITLangCtx *ctx) {
  if (!type || !ctx || !ctx->env) {
    return type;
  }

  Type *cached = lower_mir_rc_cache_get(type);
  if (cached) {
    return cached;
  }

  Type *result = lower_mir_resolve_rc_type_uncached(type, ctx);
  lower_mir_rc_cache_put(type, result);
  return result;
}

static Type *lower_mir_resolve_rc_type_uncached(Type *type, JITLangCtx *ctx) {
  if (!type || !ctx || !ctx->env) {
    return type;
  }

  /* An aliased aggregate is already the canonical recursive fixed point (e.g.
     RBTree, TensorRef). Skip the expensive program-wide equivalent-alias search
     and recursive ref expansion, which would otherwise traverse the recursive
     structure exponentially. */
  if ((type->kind == T_CONS || type->kind == T_SUM) && type->alias) {
    return type;
  }

  Type *resolved = resolve_type_in_env(deep_copy_type(type), ctx->env);
  if (resolved && (resolved->kind == T_CONS || resolved->kind == T_SUM) &&
      resolved->alias) {
    return resolved;
  }

  Type *equivalent_alias = lower_mir_find_equivalent_rc_alias_type(resolved);
  if (equivalent_alias) {
    return deep_copy_type(equivalent_alias);
  }

  if (resolved && resolved->kind == T_VAR && resolved->is_recursive_type_ref &&
      resolved->data.T_VAR.name) {
    Type *aliased = lower_mir_find_rc_alias_type(resolved->data.T_VAR.name);
    if (aliased) {
      return deep_copy_type(aliased);
    }
    Type *decl_type = env_lookup(ctx->env, resolved->data.T_VAR.name);
    if (decl_type &&
        !(decl_type->kind == T_VAR && types_equal(decl_type, resolved))) {
      return lower_mir_resolve_rc_type_refs(
          resolve_type_in_env(deep_copy_type(decl_type), ctx->env), ctx, 0);
    }
  }

  if (resolved && resolved->kind == T_RECURSIVE_REF &&
      resolved->data.T_RECURSIVE_REF.name) {
    Type *aliased =
        lower_mir_find_rc_alias_type(resolved->data.T_RECURSIVE_REF.name);
    if (aliased) {
      return deep_copy_type(aliased);
    }
    Type *decl_type = resolved->data.T_RECURSIVE_REF.decl
                          ? resolved->data.T_RECURSIVE_REF.decl->type
                          : NULL;
    if (decl_type && decl_type != resolved) {
      return lower_mir_resolve_rc_type_refs(deep_copy_type(decl_type), ctx, 0);
    }
  }

  return lower_mir_resolve_rc_type_refs(resolved, ctx, 0);
}

static bool lower_mir_type_has_rc_header(Type *type) {
  /* Coroutines use a distinct frame allocation without an RC header, so they
     are not yet RC-managed. Keep their dup/drop as the uniform no-op hooks. */
  return type && (is_array_type(type) || is_list_type(type) ||
                  is_string_type(type) || is_closure(type));
}

static bool lower_mir_type_is_managed_depth(Type *type, int depth) {
  if (!type || depth > 12) {
    return false;
  }

  if (lower_mir_type_has_rc_header(type)) {
    return true;
  }

  if (type->kind == T_CONS && is_pointer_type(type)) {
    return false;
  }

  if (type->kind == T_RECURSIVE_REF && type->data.T_RECURSIVE_REF.decl &&
      type->data.T_RECURSIVE_REF.decl->type) {
    Type *decl_type = type->data.T_RECURSIVE_REF.decl->type;
    /* A back-edge to a managed declaration (array/list/string/closure) is
       managed. Otherwise recurse into the declaration's body once; the body's
       own recursive occurrences are back-edges that re-enter here, so this
       terminates in O(1) rather than re-expanding the whole structure. */
    if (lower_mir_type_has_rc_header(decl_type) ||
        (decl_type->kind == T_CONS && is_pointer_type(decl_type))) {
      return lower_mir_type_has_rc_header(decl_type);
    }
    return lower_mir_type_is_managed_depth(decl_type, depth + 1);
  }

  if ((type->kind == T_CONS || type->kind == T_SUM) && type->data.T_CONS.args) {
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (lower_mir_type_is_managed_depth(type->data.T_CONS.args[i],
                                          depth + 1)) {
        return true;
      }
    }
  }

  return false;
}

static bool lower_mir_type_is_managed(Type *type) {
  return lower_mir_type_is_managed_depth(type, 0);
}

static bool lower_mir_type_is_managed_ctx(Type *type, JITLangCtx *ctx) {
  return lower_mir_type_is_managed(lower_mir_resolve_rc_type(type, ctx));
}

static const char *lower_mir_drop_type_mangle(Type *type);
static LLVMValueRef lower_mir_ensure_drop_fn(Type *type, LLVMModuleRef module,
                                             JITLangCtx *ctx);

static char *lower_mir_drop_sanitize(const char *text) {
  if (!text || !*text) {
    return strdup("Type");
  }
  size_t len = strlen(text);
  char *out = malloc(len + 1);
  if (!out) {
    return NULL;
  }
  size_t used = 0;
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)text[i];
    if (isalnum(c) || c == '_') {
      out[used++] = (char)c;
    } else if (used > 0 && out[used - 1] != '_') {
      out[used++] = '_';
    }
  }
  while (used > 0 && out[used - 1] == '_') {
    used--;
  }
  if (used == 0) {
    free(out);
    return strdup("Type");
  }
  out[used] = '\0';
  return out;
}

static char *lower_mir_drop_type_fragment(Type *type) {
  if (!type) {
    return strdup("Type");
  }
  switch (type->kind) {
  case T_INT:
    return strdup("Int");
  case T_UINT64:
    return strdup("Uint64");
  case T_NUM:
    return strdup("Double");
  case T_CHAR:
    return strdup("Char");
  case T_BOOL:
    return strdup("Bool");
  case T_VOID:
    return strdup("Void");
  case T_STRING:
    return strdup("String");
  case T_EMPTY_LIST:
    return strdup("EmptyList");
  case T_MODULE:
    return strdup("Module");
  case T_VAR:
    return lower_mir_drop_sanitize(type->data.T_VAR.name ? type->data.T_VAR.name
                                                         : "TypeVar");
  case T_RECURSIVE_REF:
    return lower_mir_drop_sanitize(type->data.T_RECURSIVE_REF.name
                                       ? type->data.T_RECURSIVE_REF.name
                                       : "RecursiveRef");
  case T_CONS:
  case T_SUM: {
    if (type->alias) {
      return lower_mir_drop_sanitize(type->alias);
    }

    const char *name = type->data.T_CONS.name;
    if (!name) {
      name = type->kind == T_SUM ? "Sum" : "Cons";
    }
    char *result = lower_mir_drop_sanitize(name);
    if (!result) {
      return NULL;
    }
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      Type *arg = type->data.T_CONS.args ? type->data.T_CONS.args[i] : NULL;
      char *arg_frag = lower_mir_drop_type_fragment(arg);
      if (!arg_frag) {
        free(result);
        return NULL;
      }
      char *combined = NULL;
      if (asprintf(&combined, "%s_%s", result, arg_frag) < 0) {
        free(result);
        free(arg_frag);
        return NULL;
      }
      free(result);
      free(arg_frag);
      result = combined;
    }
    return result;
  }
  case T_FN: {
    char *result = strdup("Fn");
    Type *cur = type;
    while (cur && cur->kind == T_FN) {
      char *arg_frag = lower_mir_drop_type_fragment(cur->data.T_FN.from);
      if (!arg_frag) {
        free(result);
        return NULL;
      }
      char *combined = NULL;
      if (asprintf(&combined, "%s_%s", result, arg_frag) < 0) {
        free(result);
        free(arg_frag);
        return NULL;
      }
      free(result);
      free(arg_frag);
      result = combined;
      cur = cur->data.T_FN.to;
    }
    if (cur) {
      char *ret_frag = lower_mir_drop_type_fragment(cur);
      if (ret_frag) {
        char *combined = NULL;
        if (asprintf(&combined, "%s_%s", result, ret_frag) >= 0) {
          free(result);
          free(ret_frag);
          return combined;
        }
        free(ret_frag);
      }
    }
    return result;
  }
  }
  return strdup("Type");
}

static const char *lower_mir_drop_type_mangle(Type *type) {
  if (!type) {
    return "__ylc_drop_unknown";
  }
  char *frag = lower_mir_drop_type_fragment(type);
  if (!frag) {
    return "__ylc_drop_unknown";
  }
  char *full = NULL;
  if (asprintf(&full, "__ylc_drop_%s", frag) < 0 || !full) {
    free(frag);
    return "__ylc_drop_unknown";
  }
  free(frag);
  return full;
}

static LLVMValueRef lower_mir_drop_fn_get_or_declare(Type *type,
                                                     LLVMModuleRef module) {
  const char *name = lower_mir_drop_type_mangle(type);
  LLVMValueRef fn = LLVMGetNamedFunction(module, name);
  if (fn) {
    return fn;
  }
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  LLVMTypeRef fn_type = LLVMFunctionType(
      LLVMVoidTypeInContext(LLVMGetModuleContext(module)), &ptr_type, 1, 0);
  fn = LLVMAddFunction(module, name, fn_type);

  // Interactive chunks may each need the same generated helper name.
  if (ylc_config.interactive_mode) {
    LLVMSetLinkage(fn, LLVMInternalLinkage);
  }

  return fn;
}

static LLVMValueRef lower_mir_rc_ptr_from_value(LLVMBuilderRef builder,
                                                LLVMValueRef value, Type *type,
                                                LLVMModuleRef module,
                                                JITLangCtx *ctx) {
  type = lower_mir_resolve_rc_type(type, ctx);
  if (!builder || !value || !type || !lower_mir_type_has_rc_header(type)) {
    return NULL;
  }

  LLVMTypeRef value_type = LLVMTypeOf(value);
  LLVMTypeKind value_kind = LLVMGetTypeKind(value_type);
  if (value_kind == LLVMPointerTypeKind) {
    return value;
  }

  if (is_array_type(type) && type->data.T_CONS.args &&
      type->data.T_CONS.num_args > 0) {
    if (value_kind != LLVMStructTypeKind) {
      return NULL;
    }
    LLVMTypeRef element_type =
        lower_mir_value_storage_type(type->data.T_CONS.args[0], ctx, module);
    LLVMValueRef offset =
        LLVMBuildExtractValue(builder, value, 1, "rc.array.offset");
    LLVMValueRef data =
        LLVMBuildExtractValue(builder, value, 2, "rc.array.data");
    if (!element_type || !offset || !data) {
      return NULL;
    }
    if (!lower_mir_llvm_type_is_sized(element_type)) {
      return data;
    }
    LLVMValueRef neg_offset =
        LLVMBuildNeg(builder, offset, "rc.array.base.offset");
    return LLVMBuildGEP2(builder, element_type, data, &neg_offset, 1,
                         "rc.array.base");
  }

  if (is_closure(type)) {
    if (value_kind != LLVMStructTypeKind ||
        LLVMCountStructElementTypes(value_type) < 2) {
      return NULL;
    }
    return LLVMBuildExtractValue(builder, value, 1, "rc.payload");
  }

  return NULL;
}

static void lower_mir_emit_value_rc_marker(LLVMBuilderRef builder,
                                           LLVMValueRef value, Type *type,
                                           LLVMModuleRef module,
                                           JITLangCtx *ctx, bool is_dup) {
  type = lower_mir_resolve_rc_type(type, ctx);
  if (!builder || !value || !type || !lower_mir_type_is_managed(type)) {
    return;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  if (lower_mir_type_has_rc_header(type)) {
    LLVMValueRef ptr =
        lower_mir_rc_ptr_from_value(builder, value, type, module, ctx);
    if (!ptr) {
      return;
    }
    if (LLVMTypeOf(ptr) != ptr_type &&
        LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind) {
      ptr = LLVMBuildBitCast(builder, ptr, ptr_type,
                             is_dup ? "dup.ptr" : "drop.ptr");
    }
    if (LLVMGetTypeKind(LLVMTypeOf(ptr)) != LLVMPointerTypeKind) {
      return;
    }

    LLVMTypeRef fn_type =
        LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
    if (is_dup) {
      LLVMValueRef hook = lower_mir_rc_hook(module, "__ylc_dup");
      if (hook) {
        LLVMBuildCall2(builder, fn_type, hook, &ptr, 1, "");
      }
      return;
    }

    LLVMValueRef drop_fn = lower_mir_ensure_drop_fn(type, module, ctx);
    if (drop_fn) {
      LLVMBuildCall2(builder, fn_type, drop_fn, &ptr, 1, "");
      return;
    }
    LLVMValueRef hook = lower_mir_rc_hook(module, "__ylc_drop");
    if (hook) {
      LLVMBuildCall2(builder, fn_type, hook, &ptr, 1, "");
    }
    return;
  }

  if (type->kind == T_SUM && type->data.T_CONS.args &&
      LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMStructTypeKind &&
      LLVMCountStructElementTypes(LLVMTypeOf(value)) > 1) {
    LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
    LLVMValueRef parent_fn =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
    if (!parent_fn) {
      return;
    }

    LLVMValueRef tag = LLVMBuildExtractValue(builder, value, 0, "rc.sum.tag");
    LLVMValueRef raw_payload =
        LLVMBuildExtractValue(builder, value, 1, "rc.sum.payload.raw");
    LLVMBasicBlockRef done_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, parent_fn, "rc.sum.done");

    LLVMBasicBlockRef next_test = NULL;
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      Type *variant_type = type->data.T_CONS.args[i];
      LLVMBasicBlockRef case_bb =
          LLVMAppendBasicBlockInContext(llvm_ctx, parent_fn, "rc.sum.case");
      next_test =
          LLVMAppendBasicBlockInContext(llvm_ctx, parent_fn, "rc.sum.next");
      LLVMValueRef expected = LLVMConstInt(LLVMTypeOf(tag), (unsigned)i, false);
      LLVMValueRef is_case =
          LLVMBuildICmp(builder, LLVMIntEQ, tag, expected, "rc.sum.is_case");
      LLVMBuildCondBr(builder, is_case, case_bb, next_test);

      LLVMPositionBuilderAtEnd(builder, case_bb);
      LLVMValueRef payload = NULL;
      LLVMTypeRef payload_type =
          lower_mir_value_storage_type(variant_type, ctx, module);
      if (payload_type && LLVMTypeOf(raw_payload) == payload_type) {
        payload = raw_payload;
      } else if (payload_type) {
        payload = lower_mir_wrap_single_field_payload(raw_payload, payload_type,
                                                      builder);
        if (!payload) {
          payload = cast_union(raw_payload, variant_type, ctx, module, builder);
        }
      }
      if (payload && lower_mir_type_is_managed(variant_type)) {
        lower_mir_emit_value_rc_marker(builder, payload, variant_type, module,
                                       ctx, is_dup);
      }
      LLVMBuildBr(builder, done_bb);

      LLVMPositionBuilderAtEnd(builder, next_test);
    }

    if (next_test) {
      LLVMBuildBr(builder, done_bb);
    }
    LLVMPositionBuilderAtEnd(builder, done_bb);
    return;
  }

  if (type->kind == T_CONS && type->data.T_CONS.args &&
      LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMStructTypeKind) {
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      Type *field_type = type->data.T_CONS.args[i];
      if (!lower_mir_type_is_managed(field_type)) {
        continue;
      }
      LLVMValueRef field = LLVMBuildExtractValue(
          builder, value, (unsigned)i, is_dup ? "dup.field" : "drop.field");
      lower_mir_emit_value_rc_marker(builder, field, field_type, module, ctx,
                                     is_dup);
    }
  }
}

static void lower_mir_drop_emit_child_drop(LLVMBuilderRef builder,
                                           LLVMValueRef child_value,
                                           Type *field_type,
                                           LLVMModuleRef module,
                                           JITLangCtx *ctx) {
  field_type = lower_mir_resolve_rc_type(field_type, ctx);
  if (!child_value || !field_type ||
      !lower_mir_type_has_rc_header(field_type)) {
    return;
  }
  LLVMValueRef child_drop_fn =
      lower_mir_ensure_drop_fn(field_type, module, ctx);
  if (!child_drop_fn) {
    return;
  }
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  if (LLVMTypeOf(child_value) != ptr_type &&
      LLVMGetTypeKind(LLVMTypeOf(child_value)) == LLVMPointerTypeKind) {
    child_value =
        LLVMBuildBitCast(builder, child_value, ptr_type, "drop.child.cast");
  }
  if (LLVMGetTypeKind(LLVMTypeOf(child_value)) != LLVMPointerTypeKind) {
    return;
  }
  LLVMTypeRef fn_type = LLVMFunctionType(
      LLVMVoidTypeInContext(LLVMGetModuleContext(module)), &ptr_type, 1, 0);
  LLVMBuildCall2(builder, fn_type, child_drop_fn, &child_value, 1, "");
}

static void lower_mir_drop_emit_field(LLVMBuilderRef builder,
                                      LLVMValueRef payload_ptr,
                                      LLVMTypeRef payload_type,
                                      Type *field_type, unsigned field_index,
                                      LLVMModuleRef module, JITLangCtx *ctx) {
  if (!field_type || !lower_mir_type_is_managed(field_type)) {
    return;
  }
  LLVMValueRef field_ptr = LLVMBuildStructGEP2(
      builder, payload_type, payload_ptr, field_index, "drop.field.ptr");
  LLVMTypeRef field_storage =
      lower_mir_value_storage_type(field_type, ctx, module);
  if (!field_storage) {
    return;
  }
  LLVMValueRef field_value =
      LLVMBuildLoad2(builder, field_storage, field_ptr, "drop.field.value");
  lower_mir_emit_value_rc_marker(builder, field_value, field_type, module, ctx,
                                 false);
}

static void lower_mir_build_drop_fn_body(Type *type, LLVMValueRef drop_fn,
                                         LLVMModuleRef module,
                                         JITLangCtx *ctx) {
  if (!type || !drop_fn || !module) {
    return;
  }
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef payload = LLVMGetParam(drop_fn, 0);
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  if (LLVMTypeOf(payload) != ptr_type) {
    payload = LLVMBuildBitCast(builder, payload, ptr_type, "drop.payload");
  }

  if (is_list_type(type)) {
    LLVMBasicBlockRef null_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.null");
    LLVMBasicBlockRef live_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.live");
    LLVMBasicBlockRef dead_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.dead");
    LLVMBasicBlockRef free_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.free");
    LLVMBasicBlockRef done_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.done");

    LLVMValueRef is_null = LLVMBuildICmp(
        builder, LLVMIntEQ, payload, LLVMConstNull(ptr_type), "drop.is_null");
    LLVMBuildCondBr(builder, is_null, null_bb, live_bb);

    LLVMPositionBuilderAtEnd(builder, live_bb);
    LLVMTypeRef header_type = lower_mir_rc_header_type(module);
    LLVMValueRef header_ptr =
        LLVMBuildGEP2(builder, LLVMInt8TypeInContext(llvm_ctx), payload,
                      &(LLVMValueRef){LLVMConstInt(
                          LLVMInt32TypeInContext(llvm_ctx), -8, true)},
                      1, "drop.header");
    LLVMValueRef rc_ptr = LLVMBuildBitCast(
        builder, header_ptr,
        LLVMPointerType(LLVMInt32TypeInContext(llvm_ctx), 0), "drop.rc.ptr");
    LLVMValueRef rc = LLVMBuildLoad2(builder, LLVMInt32TypeInContext(llvm_ctx),
                                     rc_ptr, "drop.rc");
    LLVMValueRef is_stack =
        LLVMBuildICmp(builder, LLVMIntEQ, rc,
                      LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false),
                      "drop.is_stack");
    LLVMBasicBlockRef dec_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.dec");
    LLVMBuildCondBr(builder, is_stack, done_bb, dec_bb);

    LLVMPositionBuilderAtEnd(builder, dec_bb);
    LLVMValueRef decremented = LLVMBuildSub(
        builder, rc, LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 1, false),
        "drop.rc.dec");
    LLVMBuildStore(builder, decremented, rc_ptr);
    LLVMValueRef is_zero =
        LLVMBuildICmp(builder, LLVMIntEQ, decremented,
                      LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false),
                      "drop.rc.zero");
    LLVMBuildCondBr(builder, is_zero, dead_bb, done_bb);
    (void)header_type;

    LLVMPositionBuilderAtEnd(builder, dead_bb);
    Type *elt_type = type_of_list(type);
    LLVMTypeRef elt_storage =
        lower_mir_value_storage_type(elt_type, ctx, module);
    LLVMTypeRef node_type = elt_storage ? llnode_type(elt_storage) : NULL;
    if (elt_type && node_type && lower_mir_type_is_managed(elt_type)) {
      lower_mir_drop_emit_field(builder, payload, node_type, elt_type, 0,
                                module, ctx);
    }
    LLVMValueRef tail_ptr =
        LLVMBuildStructGEP2(builder, node_type, payload, 1, "drop.tail.ptr");
    LLVMValueRef tail =
        LLVMBuildLoad2(builder, ptr_type, tail_ptr, "drop.tail");
    lower_mir_drop_emit_child_drop(builder, tail, type, module, ctx);
    LLVMBuildBr(builder, free_bb);

    LLVMPositionBuilderAtEnd(builder, free_bb);
    LLVMValueRef free_fn = lower_mir_free_fn(module);
    LLVMValueRef free_arg =
        LLVMBuildBitCast(builder, header_ptr, ptr_type, "drop.free.arg");
    LLVMTypeRef free_type =
        LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
    LLVMBuildCall2(builder, free_type, free_fn, &free_arg, 1, "");
    LLVMBuildBr(builder, done_bb);

    LLVMPositionBuilderAtEnd(builder, null_bb);
    LLVMBuildBr(builder, done_bb);

    LLVMPositionBuilderAtEnd(builder, done_bb);
    LLVMBuildRetVoid(builder);
    LLVMDisposeBuilder(builder);
    return;
  }

  LLVMBasicBlockRef null_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.null");
  LLVMBasicBlockRef live_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.live");
  LLVMBasicBlockRef dead_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.dead");
  LLVMBasicBlockRef free_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.free");
  LLVMBasicBlockRef done_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.done");

  LLVMValueRef is_null = LLVMBuildICmp(builder, LLVMIntEQ, payload,
                                       LLVMConstNull(ptr_type), "drop.is_null");
  LLVMBuildCondBr(builder, is_null, null_bb, live_bb);

  LLVMPositionBuilderAtEnd(builder, live_bb);

  LLVMTypeRef header_type = lower_mir_rc_header_type(module);
  LLVMValueRef header_ptr = LLVMBuildGEP2(
      builder, LLVMInt8TypeInContext(llvm_ctx), payload,
      &(LLVMValueRef){LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), -8, true)},
      1, "drop.header");
  LLVMValueRef rc_ptr = LLVMBuildBitCast(
      builder, header_ptr, LLVMPointerType(LLVMInt32TypeInContext(llvm_ctx), 0),
      "drop.rc.ptr");
  LLVMValueRef rc = LLVMBuildLoad2(builder, LLVMInt32TypeInContext(llvm_ctx),
                                   rc_ptr, "drop.rc");
  LLVMValueRef is_stack =
      LLVMBuildICmp(builder, LLVMIntEQ, rc,
                    LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false),
                    "drop.is_stack");
  LLVMBasicBlockRef dec_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.dec");
  LLVMBuildCondBr(builder, is_stack, done_bb, dec_bb);

  LLVMPositionBuilderAtEnd(builder, dec_bb);
  LLVMValueRef decremented = LLVMBuildSub(
      builder, rc, LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 1, false),
      "drop.rc.dec");
  LLVMBuildStore(builder, decremented, rc_ptr);
  LLVMValueRef is_zero = LLVMBuildICmp(
      builder, LLVMIntEQ, decremented,
      LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false), "drop.rc.zero");
  LLVMBuildCondBr(builder, is_zero, dead_bb, done_bb);
  (void)header_type;

  LLVMPositionBuilderAtEnd(builder, dead_bb);
  if (is_closure(type)) {
    Type *env_type = type->closure_meta;
    if (env_type && (env_type->kind == T_CONS || env_type->kind == T_SUM) &&
        env_type->data.T_CONS.args) {
      LLVMTypeRef env_storage =
          lower_mir_closure_env_record_type(env_type, ctx, module);
      for (int i = 0; i < env_type->data.T_CONS.num_args; i++) {
        Type *field_type = env_type->data.T_CONS.args[i];
        lower_mir_drop_emit_field(builder, payload, env_storage, field_type,
                                  (unsigned)i, module, ctx);
      }
    }
  } else if (is_string_type(type)) {
    /* strings own only their backing buffer; no managed children */
  } else if (is_array_type(type)) {
    /* Iterate the backing store using the element count stored in the header's
       tag_or_size_class field, dropping each managed element. */
    Type *elt_type = (type->data.T_CONS.args && type->data.T_CONS.num_args > 0)
                         ? type->data.T_CONS.args[0]
                         : NULL;
    elt_type = lower_mir_resolve_rc_type(elt_type, ctx);
    if (elt_type && lower_mir_type_is_managed_ctx(elt_type, ctx)) {
      LLVMTypeRef i32 = LLVMInt32TypeInContext(llvm_ctx);
      LLVMValueRef tag_ptr = LLVMBuildStructGEP2(builder, header_type,
                                                 header_ptr, 1, "drop.tag.ptr");
      LLVMValueRef count = LLVMBuildLoad2(builder, i32, tag_ptr, "drop.count");

      LLVMTypeRef elt_storage =
          lower_mir_value_storage_type(elt_type, ctx, module);
      if (elt_storage) {
        LLVMBasicBlockRef loop_bb =
            LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.arr.loop");
        LLVMBasicBlockRef body_bb =
            LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.arr.body");
        LLVMBasicBlockRef after_bb =
            LLVMAppendBasicBlockInContext(llvm_ctx, drop_fn, "drop.arr.after");
        LLVMValueRef zero = LLVMConstInt(i32, 0, false);
        LLVMValueRef one = LLVMConstInt(i32, 1, false);
        LLVMBasicBlockRef preloop_bb = LLVMGetInsertBlock(builder);
        LLVMValueRef cmp =
            LLVMBuildICmp(builder, LLVMIntEQ, count, zero, "drop.arr.empty");
        LLVMBuildCondBr(builder, cmp, after_bb, loop_bb);

        LLVMPositionBuilderAtEnd(builder, loop_bb);
        LLVMValueRef idx = LLVMBuildPhi(builder, i32, "drop.arr.idx");
        LLVMAddIncoming(idx, &(LLVMValueRef){zero}, &preloop_bb, 1);
        LLVMValueRef cont =
            LLVMBuildICmp(builder, LLVMIntSLT, idx, count, "drop.arr.cont");
        LLVMBuildCondBr(builder, cont, body_bb, after_bb);

        LLVMPositionBuilderAtEnd(builder, body_bb);
        LLVMValueRef elem_ptr = LLVMBuildGEP2(builder, elt_storage, payload,
                                              &idx, 1, "drop.arr.elem.ptr");
        LLVMValueRef elem =
            LLVMBuildLoad2(builder, elt_storage, elem_ptr, "drop.arr.elem");
        lower_mir_emit_value_rc_marker(builder, elem, elt_type, module, ctx,
                                       false);
        LLVMBasicBlockRef backedge_bb = LLVMGetInsertBlock(builder);
        LLVMValueRef next = LLVMBuildAdd(builder, idx, one, "drop.arr.next");
        LLVMAddIncoming(idx, &next, &backedge_bb, 1);
        LLVMBuildBr(builder, loop_bb);

        LLVMPositionBuilderAtEnd(builder, after_bb);
      }
    }
  } else if (is_coroutine_type(type)) {
    /* coroutines are not RC-managed (deferred) */
  }
  LLVMBuildBr(builder, free_bb);

  LLVMPositionBuilderAtEnd(builder, free_bb);
  LLVMValueRef free_fn = lower_mir_free_fn(module);
  LLVMValueRef free_arg =
      LLVMBuildBitCast(builder, header_ptr, ptr_type, "drop.free.arg");
  LLVMTypeRef free_type =
      LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
  LLVMBuildCall2(builder, free_type, free_fn, &free_arg, 1, "");
  LLVMBuildBr(builder, done_bb);

  LLVMPositionBuilderAtEnd(builder, null_bb);
  LLVMBuildBr(builder, done_bb);

  LLVMPositionBuilderAtEnd(builder, done_bb);
  LLVMBuildRetVoid(builder);
  LLVMDisposeBuilder(builder);
}

static LLVMValueRef lower_mir_ensure_drop_fn(Type *type, LLVMModuleRef module,
                                             JITLangCtx *ctx) {
  type = lower_mir_resolve_rc_type(type, ctx);
  if (!type) {
    return NULL;
  }
  LLVMValueRef fn = lower_mir_drop_fn_get_or_declare(type, module);
  if (!fn) {
    return NULL;
  }
  if (LLVMGetFirstBasicBlock(fn)) {
    return fn;
  }
  LLVMValueRef dup_hook = lower_mir_rc_hook(module, "__ylc_dup");
  JITLangCtx dummy_ctx = {0};
  JITLangCtx *drop_ctx = ctx ? ctx : &dummy_ctx;
  lower_mir_build_drop_fn_body(type, fn, module, drop_ctx);
  (void)dup_hook;
  return fn;
}

static LLVMValueRef lower_mir_rc_marker(MirFunction *fn, MirInstr *instr,
                                        MirLlvmValueMap *values,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder,
                                        JITLangCtx *ctx) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  if (!instr || (fn && fn->skip_rc_markers)) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  MirInstr *operand_def =
      mir_function_find_def_instr(fn, instr->data.op.operands[0]);
  if (operand_def && operand_def->kind == MIR_CONST) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  Type *operand_type = mir_function_value_type(fn, instr->data.op.operands[0]);
  Type *resolved_operand_type = lower_mir_resolve_rc_type(operand_type, ctx);
  if (!resolved_operand_type ||
      !lower_mir_type_is_managed(resolved_operand_type)) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }
  if (lower_mir_type_has_rc_header(resolved_operand_type) &&
      lower_mir_value_allocates_on_stack(fn, instr->data.op.operands[0])) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  if (!lower_mir_type_has_rc_header(resolved_operand_type)) {
    LLVMValueRef value =
        mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
    lower_mir_emit_value_rc_marker(builder, value, resolved_operand_type,
                                   module, ctx,
                                   instr->data.op.kind == MIR_OP_KIND_DUP);
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  LLVMValueRef ptr =
      lower_mir_rc_managed_ptr(fn, instr, values, module, builder, ctx);
  if (!ptr) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }

  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);

  if (instr->data.op.kind == MIR_OP_KIND_DUP) {
    LLVMValueRef hook = lower_mir_rc_hook(module, "__ylc_dup");
    if (!hook) {
      return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
    }
    LLVMTypeRef fn_type =
        LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
    return LLVMBuildCall2(builder, fn_type, hook, &ptr, 1, "");
  }

  {
    LLVMValueRef drop_fn = lower_mir_ensure_drop_fn(operand_type, module, ctx);
    if (drop_fn) {
      LLVMValueRef arg = ptr;
      if (LLVMTypeOf(arg) != ptr_type) {
        arg = LLVMBuildBitCast(builder, arg, ptr_type, "drop.arg");
      }
      LLVMTypeRef fn_type =
          LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
      return LLVMBuildCall2(builder, fn_type, drop_fn, &arg, 1, "");
    }
  }

  LLVMValueRef hook = lower_mir_rc_hook(module, "__ylc_drop");
  if (!hook) {
    return LLVMGetUndef(LLVMVoidTypeInContext(llvm_ctx));
  }
  LLVMTypeRef fn_type =
      LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
  return LLVMBuildCall2(builder, fn_type, hook, &ptr, 1, "");
}

static LLVMValueRef lower_mir_rc_drop_reuse(MirFunction *fn, MirInstr *instr,
                                            MirLlvmValueMap *values,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder,
                                            JITLangCtx *ctx) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  if (!instr || (fn && fn->skip_rc_markers)) {
    return LLVMConstNull(lower_mir_generic_ptr_type(module));
  }

  MirInstr *operand_def =
      mir_function_find_def_instr(fn, instr->data.op.operands[0]);
  if (operand_def && operand_def->kind == MIR_CONST) {
    return LLVMConstNull(lower_mir_generic_ptr_type(module));
  }

  if (lower_mir_value_allocates_on_stack(fn, instr->data.op.operands[0])) {
    return LLVMConstNull(lower_mir_generic_ptr_type(module));
  }

  LLVMValueRef ptr =
      lower_mir_rc_managed_ptr(fn, instr, values, module, builder, ctx);
  if (!ptr) {
    return LLVMConstNull(lower_mir_generic_ptr_type(module));
  }

  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  Type *operand_type = mir_function_value_type(fn, instr->data.op.operands[0]);
  Type *resolved_operand_type = lower_mir_resolve_rc_type(operand_type, ctx);
  if (!resolved_operand_type ||
      !lower_mir_type_has_rc_header(resolved_operand_type)) {
    return LLVMConstNull(ptr_type);
  }
  if (LLVMTypeOf(ptr) != ptr_type) {
    ptr = LLVMBuildBitCast(builder, ptr, ptr_type, "drop_reuse.ptr");
  }

  LLVMTypeRef i32 = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef header_type = lower_mir_rc_header_type(module);
  LLVMValueRef header_ptr = LLVMBuildGEP2(
      builder, LLVMInt8TypeInContext(llvm_ctx), ptr,
      &(LLVMValueRef){LLVMConstInt(i32, -8, true)}, 1, "drop_reuse.header");
  LLVMValueRef rc_ptr = LLVMBuildBitCast(
      builder, header_ptr, LLVMPointerType(i32, 0), "drop_reuse.rc.ptr");
  LLVMValueRef rc = LLVMBuildLoad2(builder, i32, rc_ptr, "drop_reuse.rc");

  /* If the value is uniquely owned (rc == 1), recycle its storage: keep rc at 1
     and hand the payload pointer to the constructor that will overwrite it in
     place. Otherwise fall back to a normal drop and yield null so the
     constructor mallocs a fresh object. */
  LLVMValueRef is_unique = LLVMBuildICmp(
      builder, LLVMIntEQ, rc, LLVMConstInt(i32, 1, false), "drop_reuse.unique");
  LLVMValueRef token = LLVMBuildSelect(
      builder, is_unique, ptr, LLVMConstNull(ptr_type), "drop_reuse.token");
  LLVMBasicBlockRef reuse_bb = LLVMAppendBasicBlockInContext(
      llvm_ctx, LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)),
      "drop_reuse.reuse");
  LLVMBasicBlockRef drop_bb = LLVMAppendBasicBlockInContext(
      llvm_ctx, LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)),
      "drop_reuse.drop");
  LLVMBasicBlockRef cont_bb = LLVMAppendBasicBlockInContext(
      llvm_ctx, LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)),
      "drop_reuse.cont");
  LLVMBuildCondBr(builder, is_unique, reuse_bb, drop_bb);

  LLVMPositionBuilderAtEnd(builder, reuse_bb);
  /* rc already 1; leave header intact for the reusing constructor */
  LLVMBuildBr(builder, cont_bb);

  LLVMPositionBuilderAtEnd(builder, drop_bb);
  LLVMValueRef drop_fn = lower_mir_ensure_drop_fn(operand_type, module, ctx);
  if (drop_fn) {
    LLVMTypeRef drop_fn_type =
        LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), &ptr_type, 1, 0);
    LLVMBuildCall2(builder, drop_fn_type, drop_fn, &ptr, 1, "");
  }
  LLVMBuildBr(builder, cont_bb);

  LLVMPositionBuilderAtEnd(builder, cont_bb);
  LLVMValueRef phi = LLVMBuildPhi(builder, ptr_type, "drop_reuse.result");
  LLVMAddIncoming(phi, &ptr, &reuse_bb, 1);
  LLVMAddIncoming(phi, &(LLVMValueRef){LLVMConstNull(ptr_type)}, &drop_bb, 1);
  (void)header_type;
  (void)token;
  return phi;
}

static LLVMValueRef lower_mir_print(MirFunction *fn, MirInstr *instr,
                                    MirLlvmValueMap *values,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  (void)fn;
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef void_type = LLVMVoidTypeInContext(llvm_ctx);
  if (!instr || instr->data.op.argc != 1) {
    return LLVMGetUndef(void_type);
  }

  LLVMValueRef str =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
  if (!str) {
    return NULL;
  }

  LLVMTypeRef i32_type = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef i8_ptr_type = lower_mir_generic_ptr_type(module);
  LLVMTypeRef printf_type =
      LLVMFunctionType(i32_type, (LLVMTypeRef[]){i8_ptr_type}, 1, 1);
  LLVMValueRef printf_func = LLVMGetNamedFunction(module, "printf");
  if (!printf_func) {
    printf_func = LLVMAddFunction(module, "printf", printf_type);
  }
  set_memory_effects(printf_func, MEM_ARGMEM_REF | MEM_INACCESSIBLE_MODREF);

  LLVMValueRef format_str =
      LLVMBuildGlobalStringPtr(builder, "%.*s", "print.fmt");
  LLVMValueRef len = LLVMBuildExtractValue(builder, str, 0, "print.len");
  LLVMValueRef chars = LLVMBuildExtractValue(builder, str, 2, "print.chars");
  LLVMValueRef printf_args[] = {format_str, len, chars};
  LLVMBuildCall2(builder, printf_type, printf_func, printf_args, 3, "");
  return LLVMGetUndef(void_type);
}

static LLVMValueRef lower_mir_fprint(MirFunction *fn, MirInstr *instr,
                                     MirLlvmValueMap *values,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  (void)fn;
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef void_type = LLVMVoidTypeInContext(llvm_ctx);
  if (!instr || instr->data.op.argc != 2) {
    return LLVMGetUndef(void_type);
  }

  LLVMValueRef file =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
  LLVMValueRef str =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[1], builder);
  if (!file || !str) {
    return NULL;
  }

  return fprint_str(file, str, module, builder);
}

static LLVMValueRef lower_mir_flush(MirInstr *instr, LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef void_type = LLVMVoidTypeInContext(llvm_ctx);
  if (!instr || instr->data.op.argc != 0) {
    return LLVMGetUndef(void_type);
  }

  LLVMTypeRef i32_type = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef i8_ptr_type = lower_mir_generic_ptr_type(module);
  LLVMTypeRef fflush_type =
      LLVMFunctionType(i32_type, (LLVMTypeRef[]){i8_ptr_type}, 1, 0);
  LLVMValueRef fflush_func = LLVMGetNamedFunction(module, "fflush");
  if (!fflush_func) {
    fflush_func = LLVMAddFunction(module, "fflush", fflush_type);
  }
  set_memory_effects(fflush_func, MEM_ARGMEM_MODREF | MEM_INACCESSIBLE_MODREF);

  LLVMValueRef null_ptr = LLVMConstNull(i8_ptr_type);
  LLVMBuildCall2(builder, fflush_type, fflush_func, &null_ptr, 1, "");
  return LLVMGetUndef(void_type);
}

static LLVMValueRef lower_mir_cstr(MirInstr *instr, MirLlvmValueMap *values,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef ptr_type = lower_mir_generic_ptr_type(module);
  if (!instr || !values || instr->data.op.argc != 1) {
    return LLVMConstNull(ptr_type);
  }

  LLVMValueRef value =
      mir_llvm_value_get_rvalue(values, instr->data.op.operands[0], builder);
  if (!value) {
    return NULL;
  }

  if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMPointerTypeKind) {
    return value;
  }

  LLVMValueRef data = LLVMBuildExtractValue(builder, value, 2, "cstr.data");
  if (!data) {
    return NULL;
  }

  if (LLVMTypeOf(data) != ptr_type &&
      LLVMGetTypeKind(LLVMTypeOf(data)) == LLVMPointerTypeKind) {
    data = LLVMBuildBitCast(builder, data, ptr_type, "cstr.ptr");
  }

  return LLVMGetTypeKind(LLVMTypeOf(data)) == LLVMPointerTypeKind
             ? data
             : LLVMConstNull(
                   LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0));
}

static bool lower_mir_dlopen_should_resolve_from_source(const char *path) {
  if (!path || path[0] == '\0' || path[0] == '/' || path[0] == '~' ||
      path[0] == '@' || strstr(path, "://")) {
    return false;
  }
  return strchr(path, '/') != NULL;
}

static const char *lower_mir_dlopen_source_path(MirFunction *fn,
                                                MirInstr *instr) {
  if (instr && instr->origin && instr->origin->loc_info &&
      instr->origin->loc_info->src_file) {
    return instr->origin->loc_info->src_file;
  }
  if (fn && fn->origin && fn->origin->loc_info &&
      fn->origin->loc_info->src_file) {
    return fn->origin->loc_info->src_file;
  }
  return module_path;
}

static char *lower_mir_resolve_dlopen_path(const char *path,
                                           const char *source_path) {
  if (!path) {
    return NULL;
  }

  if (!lower_mir_dlopen_should_resolve_from_source(path) || !source_path) {
    return strdup(path);
  }

  char *source_dir = get_dirname(source_path);
  if (!source_dir) {
    return strdup(path);
  }

  char *full_path = resolve_relative_path(source_dir, path);
  free(source_dir);
  if (!full_path) {
    return strdup(path);
  }

  char *normalized = normalize_path(full_path);
  if (!normalized) {
    return full_path;
  }
  free(full_path);
  return normalized;
}

static bool lower_mir_dlopen_cache_contains(LLVMModuleRef module,
                                            const char *path) {
  if (!path) {
    return false;
  }
  for (MirDlopenCacheEntry *entry = mir_dlopen_cache; entry;
       entry = entry->next) {
    if (entry->module == module && entry->path &&
        strcmp(entry->path, path) == 0) {
      return true;
    }
  }
  return false;
}

static void lower_mir_dlopen_cache_insert(LLVMModuleRef module,
                                          const char *path) {
  if (!path || lower_mir_dlopen_cache_contains(module, path)) {
    return;
  }
  MirDlopenCacheEntry *entry = calloc(1, sizeof(MirDlopenCacheEntry));
  if (!entry) {
    return;
  }
  entry->module = module;
  entry->path = strdup(path);
  if (!entry->path) {
    free(entry);
    return;
  }
  entry->next = mir_dlopen_cache;
  mir_dlopen_cache = entry;
}

static const char *lower_mir_const_string_operand(MirFunction *fn,
                                                  MirValueId value) {
  MirInstr *def = mir_function_find_def_instr(fn, value);
  if (!def || def->kind != MIR_CONST ||
      def->data.const_value.kind != MIR_CONST_KIND_STRING) {
    return NULL;
  }

  const char *chars = def->data.const_value.as.string_value.chars;
  return chars ? chars : "";
}

static LLVMValueRef lower_mir_dlopen(MirFunction *fn, MirInstr *instr,
                                     MirLlvmCtx *lctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef void_type = LLVMVoidTypeInContext(llvm_ctx);
  if (!fn || !instr || instr->data.op.argc != 1) {
    return LLVMGetUndef(void_type);
  }

  const char *path =
      lower_mir_const_string_operand(fn, instr->data.op.operands[0]);
  if (!path) {
    fprintf(stderr, "MIR dlopen requires a string literal path\n");
    return LLVMGetUndef(void_type);
  }

  const char *source_path = lower_mir_dlopen_source_path(fn, instr);
  char *full_path = lower_mir_resolve_dlopen_path(path, source_path);
  if (!full_path) {
    return LLVMGetUndef(void_type);
  }

  if (lower_mir_dlopen_cache_contains(module, full_path)) {
    free(full_path);
    return LLVMGetUndef(void_type);
  }

  lower_mir_dlopen_cache_insert(module, full_path);

  ylc_jit_ctx = lctx ? &lctx->jit_ctx : NULL;
  ylc_jit_module = module;
  ylc_jit_builder = builder;
  ylc_mir_program = NULL;
  ylc_mir_ctx = NULL;

  ylc_runtime_load_fn = NULL;
  void *handle = dlopen(full_path, RTLD_GLOBAL | RTLD_LAZY);
  if (ylc_runtime_load_fn) {
    ylc_runtime_load_fn();
    ylc_runtime_load_fn = NULL;
  }

  ylc_jit_ctx = NULL;
  ylc_jit_module = NULL;
  ylc_jit_builder = NULL;
  ylc_mir_program = NULL;
  ylc_mir_ctx = NULL;

  if (!handle) {
    fprintf(stderr, "Failed to load library globally: %s\n", dlerror());
  } else {
    fprintf(stderr, "loaded %s\n", full_path);
  }

  free(full_path);
  return LLVMGetUndef(void_type);
}

static LLVMValueRef lower_mir_str(MirFunction *fn, MirInstr *instr,
                                  MirLlvmValueMap *values, LLVMModuleRef module,
                                  LLVMBuilderRef builder, JITLangCtx *ctx) {
  if (!fn || !instr || !values || instr->data.op.argc != 1) {
    return NULL;
  }

  MirValueId value_id = instr->data.op.operands[0];
  LLVMValueRef value = mir_llvm_value_get_rvalue(values, value_id, builder);
  Type *type = mir_function_value_type(fn, value_id);
  if (!value || !type) {
    return NULL;
  }
  return stringify_value(value, type, ctx, module, builder);
}

static LLVMValueRef lower_mir_as_bytes(MirFunction *fn, MirInstr *instr,
                                       MirLlvmValueMap *values,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder,
                                       JITLangCtx *ctx) {
  if (!fn || !instr || !values || instr->data.op.argc != 1) {
    return NULL;
  }

  MirValueId value_id = instr->data.op.operands[0];
  LLVMValueRef value = mir_llvm_value_get_rvalue(values, value_id, builder);
  Type *type = mir_function_value_type(fn, value_id);
  if (!value || !type) {
    return NULL;
  }

  type = resolve_type_in_env(type, ctx ? ctx->env : NULL);
  if (!type ||
      !(type->kind == T_INT || type->kind == T_UINT64 || type->kind == T_NUM)) {
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef char_type = LLVMInt8TypeInContext(llvm_ctx);
  LLVMTypeRef value_type = lower_mir_type(type, ctx, module, LLVMTypeOf(value));
  if (!value_type) {
    return NULL;
  }

  if (type->kind == T_NUM) {
    value_type = LLVMInt64TypeInContext(llvm_ctx);
    value = LLVMBuildBitCast(builder, value, value_type, "double.as.int");
  }

  LLVMTargetDataRef target_data = LLVMGetModuleDataLayout(module);
  unsigned width = (unsigned)LLVMStoreSizeOfType(target_data, value_type);
  if (width == 0) {
    return NULL;
  }

  LLVMTypeRef array_type = LLVMArrayType(char_type, width);
  LLVMValueRef byte_array_ptr = lower_mir_heap_alloc_payload(
      module, builder, array_type, 0, "bytes.heap");
  if (!byte_array_ptr) {
    return NULL;
  }

  LLVMBuildStore(builder, value, byte_array_ptr);

  LLVMTypeRef string_type = codegen_string_type(char_type);
  LLVMValueRef result = LLVMGetUndef(string_type);
  result = LLVMBuildInsertValue(
      builder, result, LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), width, 0),
      0, "bytes.size");
  result = LLVMBuildInsertValue(
      builder, result, LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, 0), 1,
      "bytes.offset");
  return LLVMBuildInsertValue(builder, result, byte_array_ptr, 2, "bytes.data");
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
  case MIR_OP_KIND_TRUNC_TO_INT:
    return lower_mir_primitive_cast(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_PRIMITIVE:
    return lower_mir_primitive(instr, values, builder);
  case MIR_OP_KIND_ARRAY_SIZE:
    return lower_mir_array_size(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_ARRAY_SET:
    return lower_mir_array_set(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_PTR_OFFSET:
    return lower_mir_ptr_offset(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_LOAD:
  case MIR_OP_KIND_LOAD_OWNED:
    return lower_mir_ptr_load(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_STORE:
    return lower_mir_ptr_store(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_GLOBAL_LOAD:
    return lower_mir_global_load(instr, module, builder, ctx);
  case MIR_OP_KIND_GLOBAL_STORE:
    return lower_mir_global_store(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_SIZEOF:
    return lower_mir_sizeof(instr, module, ctx);
  case MIR_OP_KIND_LIST_IS_EMPTY:
    return lower_mir_list_is_empty(fn, instr, values, builder);
  case MIR_OP_KIND_TAG_EQ:
    return lower_mir_tag_eq(instr, values, builder);
  case MIR_OP_KIND_STR:
    return lower_mir_str(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_CSTR:
    return lower_mir_cstr(instr, values, module, builder);
  case MIR_OP_KIND_DLOPEN:
    return lower_mir_dlopen(fn, instr, lctx, module, builder);
  case MIR_OP_KIND_AS_BYTES:
    return lower_mir_as_bytes(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_DUP:
  case MIR_OP_KIND_DROP:
    return lower_mir_rc_marker(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_DROP_REUSE:
    return lower_mir_rc_drop_reuse(fn, instr, values, module, builder, ctx);
  case MIR_OP_KIND_PRINT:
    return lower_mir_print(fn, instr, values, module, builder);
  case MIR_OP_KIND_FPRINT:
    return lower_mir_fprint(fn, instr, values, module, builder);
  case MIR_OP_KIND_FLUSH:
    return lower_mir_flush(instr, module, builder);
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
    return lower_mir_const(fn, instr, module, builder, ctx);

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

  case MIR_CORO_NEW:
    return lower_mir_coro_new(fn, instr, values, lctx, module, builder);

  case MIR_CORO_NEXT:
    return lower_mir_coro_resume_call(fn, instr, values, lctx, module, builder);

  case MIR_CORO_RESET:
    return lower_mir_coro_reset(fn, instr, values, lctx, module, builder);

  default:
    fprintf(stderr,
            "MIR to LLVM lowering does not support instruction kind %d\n",
            instr->kind);
    return NULL;
  }
}

static bool
lower_mir_coro_yield_terminator(MirFunction *fn, MirTerminator *term,
                                MirLlvmValueMap *values,
                                MirLlvmBlockMap *blocks, MirLlvmCoroCtx *coro,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  if (!coro || !coro->active || !coro->handle || !coro->promise_alloca ||
      !coro->promise_type || !coro->cleanup_bb || !coro->suspend_bb) {
    fprintf(stderr,
            "MIR to LLVM lowering found yield outside coroutine context in "
            "%s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return false;
  }

  LLVMValueRef value = mir_llvm_value_get_rvalue(values, term->value, builder);
  LLVMBasicBlockRef resume_block = mir_llvm_block_get(blocks, term->target);
  if (!value || !resume_block) {
    fprintf(stderr,
            "MIR to LLVM lowering could not lower yield terminator in %s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return false;
  }

  LLVMBasicBlockRef yield_block = LLVMGetInsertBlock(builder);
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMValueRef yield_ptr = LLVMBuildStructGEP2(
      builder, coro->promise_type, coro->promise_alloca, 0, "yield.ptr");
  LLVMBuildStore(builder, value, yield_ptr);

  LLVMValueRef save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){coro->handle}, 1,
      "coro.save");
  LLVMValueRef suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save,
                       LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 0, false)},
      2, "coro.suspend");

  LLVMValueRef llvm_fn = LLVMGetBasicBlockParent(yield_block);
  LLVMBasicBlockRef return_block =
      LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "yield.return");
  LLVMValueRef switch_inst = LLVMBuildSwitch(builder, suspend, return_block, 2);
  LLVMAddCase(switch_inst,
              LLVMConstInt(LLVMInt8TypeInContext(llvm_ctx), 0, false),
              resume_block);
  LLVMAddCase(switch_inst,
              LLVMConstInt(LLVMInt8TypeInContext(llvm_ctx), 1, false),
              coro->cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, return_block);
  LLVMBuildBr(builder, coro->suspend_bb);
  LLVMPositionBuilderAtEnd(builder, yield_block);
  return true;
}

static bool lower_mir_coro_done_terminator(MirFunction *fn,
                                           MirLlvmCoroCtx *coro,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  if (!coro || !coro->active || !coro->handle || !coro->promise_alloca ||
      !coro->promise_type || !coro->cleanup_bb || !coro->suspend_bb) {
    fprintf(stderr,
            "MIR to LLVM lowering found coro.done outside coroutine context "
            "in %s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return false;
  }

  LLVMBasicBlockRef done_block = LLVMGetInsertBlock(builder);
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMValueRef done_ptr = LLVMBuildStructGEP2(
      builder, coro->promise_type, coro->promise_alloca, 1, "coro.done.ptr");
  LLVMBuildStore(builder,
                 LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 1, false),
                 done_ptr);

  LLVMValueRef save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){coro->handle}, 1,
      "final.save");
  LLVMValueRef suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save,
                       LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 1, false)},
      2, "final.suspend");

  LLVMValueRef llvm_fn = LLVMGetBasicBlockParent(done_block);
  LLVMBasicBlockRef final_return =
      LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "final.return");
  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend, coro->suspend_bb, 2);
  LLVMAddCase(switch_inst,
              LLVMConstInt(LLVMInt8TypeInContext(llvm_ctx), 0, false),
              final_return);
  LLVMAddCase(switch_inst,
              LLVMConstInt(LLVMInt8TypeInContext(llvm_ctx), 1, false),
              coro->cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, final_return);
  LLVMBuildBr(builder, coro->suspend_bb);
  LLVMPositionBuilderAtEnd(builder, done_block);
  return true;
}

static bool lower_mir_coro_restart_terminator(
    MirFunction *fn, MirTerminator *term, MirLlvmValueMap *values,
    MirLlvmBlockMap *blocks, MirLlvmCoroCtx *coro, LLVMBuilderRef builder) {
  if (!fn || !term || !values || !blocks || !coro || !coro->active) {
    fprintf(stderr,
            "MIR to LLVM lowering found coro.restart outside coroutine "
            "context in %s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return false;
  }

  LLVMBasicBlockRef target = mir_llvm_block_get(blocks, term->target);
  if (!target) {
    fprintf(stderr,
            "MIR to LLVM lowering could not find coroutine restart target "
            "bb%u in %s\n",
            term->target, fn && fn->name ? fn->name : "<anonymous>");
    return false;
  }

  size_t arg_index = 0;
  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (!lower_mir_param_is_lowered(param) || lower_mir_param_is_env(param)) {
      continue;
    }

    if (arg_index >= term->args.len) {
      fprintf(stderr,
              "MIR to LLVM lowering found too few coroutine restart args in "
              "%s\n",
              fn && fn->name ? fn->name : "<anonymous>");
      return false;
    }

    LLVMValueRef slot = param->value < values->len && values->slots
                            ? values->slots[param->value]
                            : NULL;
    LLVMTypeRef slot_type = param->value < values->len && values->slot_types
                                ? values->slot_types[param->value]
                                : NULL;
    LLVMValueRef value =
        mir_llvm_value_get_rvalue(values, term->args.items[arg_index], builder);
    arg_index++;
    if (!slot || !slot_type || !value) {
      fprintf(stderr,
              "MIR to LLVM lowering could not lower coroutine restart arg in "
              "%s\n",
              fn && fn->name ? fn->name : "<anonymous>");
      return false;
    }
    LLVMBuildStore(builder, value, slot);
  }

  if (arg_index != term->args.len) {
    fprintf(stderr,
            "MIR to LLVM lowering found too many coroutine restart args in "
            "%s\n",
            fn && fn->name ? fn->name : "<anonymous>");
    return false;
  }

  LLVMBuildBr(builder, target);
  return true;
}

static bool lower_mir_terminator(MirFunction *fn, MirTerminator *term,
                                 MirLlvmValueMap *values,
                                 MirLlvmBlockMap *blocks, LLVMTypeRef ret_type,
                                 MirLlvmCoroCtx *coro, LLVMModuleRef module,
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
  case MIR_TERM_YIELD: {
    return lower_mir_coro_yield_terminator(fn, term, values, blocks, coro,
                                           module, builder);
  }
  case MIR_TERM_CORO_RESTART: {
    return lower_mir_coro_restart_terminator(fn, term, values, blocks, coro,
                                             builder);
  }
  case MIR_TERM_CORO_DONE: {
    return lower_mir_coro_done_terminator(fn, coro, module, builder);
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
  case MIR_TERM_YIELD:
    if (term->target == MIR_NO_BLOCK) {
      return 0;
    }
    out[0] = term->target;
    return 1;
  case MIR_TERM_CORO_RESTART:
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
                                 LLVMBuilderRef builder, MirLlvmCoroCtx *coro) {
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
      fprintf(stderr,
              "MIR to LLVM lowering failed instruction %s result %u in "
              "%s bb%u\n",
              mir_instr_kind_name(instr->kind), instr->result,
              fn && fn->name ? fn->name : "<anonymous>", mir_block->id);
      return false;
    }
  }

  LLVMTypeRef ret_type = LLVMGetReturnType(lctx->function_types[fn->id]);
  if (!lower_mir_terminator(fn, &mir_block->term, values, blocks, ret_type,
                            coro, module, builder)) {
    return false;
  }
  if (blocks->exits && block_id < blocks->len) {
    blocks->exits[block_id] = LLVMGetInsertBlock(builder);
  }

  MirBlockId successors[2] = {MIR_NO_BLOCK, MIR_NO_BLOCK};
  size_t successors_len =
      lower_mir_terminator_successors(&mir_block->term, successors);
  for (size_t i = 0; i < successors_len; i++) {
    if (!lower_mir_block_body(lctx, fn, values, blocks, visited, successors[i],
                              module, builder, coro)) {
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
    LLVMBasicBlockRef block = blocks->exits && blocks->exits[incoming.block]
                                  ? blocks->exits[incoming.block]
                                  : blocks->items[incoming.block];
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

        LLVMBasicBlockRef incoming_block =
            blocks->exits && blocks->exits[incoming.block]
                ? blocks->exits[incoming.block]
                : blocks->items[incoming.block];
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
  if (fn && (fn->is_extern || fn->blocks.len == 0)) {
    return true;
  }

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
      .exits = calloc(fn->blocks.len, sizeof(LLVMBasicBlockRef)),
      .len = fn->blocks.len,
  };
  bool *visited = calloc(fn->blocks.len, sizeof(bool));
  if (!values.items || !values.slots || !values.slot_types || !blocks.items ||
      !blocks.exits || !visited) {
    free(values.items);
    free(values.slots);
    free(values.slot_types);
    free(blocks.items);
    free(blocks.exits);
    free(visited);
    return false;
  }

  bool is_coro = is_coroutine_constructor_type(fn->type);
  MirLlvmCoroCtx coro = {0};
  if (is_coro) {
    coro.active = true;
    coro.cleanup_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "cleanup");
    coro.suspend_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "suspend");
    coro.initial_return_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "initial.return");
    coro.start_bb = LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "start");
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *mir_block = fn->blocks.items[i];
    if (!mir_block || mir_block->id >= blocks.len) {
      continue;
    }
    blocks.items[mir_block->id] = LLVMAppendBasicBlockInContext(
        llvm_ctx, llvm_fn, mir_block->name ? mir_block->name : "bb");
  }

  bool ok = true;
  if (is_coro) {
    LLVMBasicBlockRef entry_bb =
        LLVMAppendBasicBlockInContext(llvm_ctx, llvm_fn, "entry");
    LLVMMoveBasicBlockBefore(entry_bb, coro.cleanup_bb);
    LLVMPositionBuilderAtEnd(builder, entry_bb);

    coro.yield_type = lower_mir_coro_constructor_yield_type(fn->type);
    coro.promise_type = lower_mir_coro_promise_type(
        coro.yield_type, &lctx->jit_ctx, module, &coro.llvm_yield_type);
    if (!coro.promise_type) {
      ok = false;
    }

    if (ok) {
      coro.promise_alloca =
          LLVMBuildAlloca(builder, coro.promise_type, "promise");
      LLVMValueRef done_ptr = LLVMBuildStructGEP2(
          builder, coro.promise_type, coro.promise_alloca, 1, "is_done.ptr");
      LLVMBuildStore(builder,
                     LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 0, false),
                     done_ptr);
      LLVMValueRef reset_ptr = LLVMBuildStructGEP2(
          builder, coro.promise_type, coro.promise_alloca, 2, "reset.ptr");
      LLVMBuildStore(builder, LLVMConstNull(lower_mir_generic_ptr_type(module)),
                     reset_ptr);
      LLVMValueRef args_ptr = LLVMBuildStructGEP2(
          builder, coro.promise_type, coro.promise_alloca, 3, "args.ptr");
      LLVMBuildStore(builder, LLVMConstNull(lower_mir_generic_ptr_type(module)),
                     args_ptr);

      coro.coro_id = LLVMBuildCall2(
          builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
          get_coro_id_intrinsic(module),
          (LLVMValueRef[]){
              LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false),
              coro.promise_alloca,
              LLVMConstNull(lower_mir_generic_ptr_type(module)),
              LLVMConstNull(lower_mir_generic_ptr_type(module))},
          4, "coro.id");

      LLVMValueRef size = LLVMBuildCall2(
          builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
          get_coro_size_intrinsic(module), NULL, 0, "coro.size");
      LLVMValueRef size_ptr = LLVMGetParam(llvm_fn, 0);
      LLVMBuildStore(builder, size, size_ptr);

      size_t llvm_param = 1;
      for (size_t i = 0; i < fn->params.len; i++) {
        MirParam *param = &fn->params.items[i];
        if (!lower_mir_param_is_lowered(param)) {
          continue;
        }

        LLVMValueRef llvm_value = LLVMGetParam(llvm_fn, (unsigned)llvm_param);
        llvm_param++;
        LLVMTypeRef slot_type =
            lower_mir_param_abi_type(param, &lctx->jit_ctx, module);
        if (!slot_type) {
          ok = false;
          break;
        }
        LLVMValueRef slot =
            LLVMBuildAlloca(builder, slot_type, "coro.param.spill");
        LLVMBuildStore(builder, llvm_value, slot);
        if (!mir_llvm_value_set_slot(&values, param->value, slot, slot_type)) {
          ok = false;
          break;
        }
      }
    }

    if (ok) {
      LLVMValueRef size = LLVMBuildCall2(
          builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
          get_coro_size_intrinsic(module), NULL, 0, "coro.size");
      LLVMValueRef frame = LLVMBuildArrayMalloc(
          builder, LLVMInt8TypeInContext(llvm_ctx), size, "coro.frame");
      coro.handle = LLVMBuildCall2(
          builder, LLVMGlobalGetValueType(get_coro_begin_intrinsic(module)),
          get_coro_begin_intrinsic(module),
          (LLVMValueRef[]){coro.coro_id, frame}, 2, "coro.handle");

      coro_emit_initial_suspend(&lctx->jit_ctx, module, builder, coro.handle,
                                coro.cleanup_bb, coro.suspend_bb,
                                coro.initial_return_bb, coro.start_bb);
      if (blocks.items[0]) {
        LLVMBuildBr(builder, blocks.items[0]);
      } else {
        ok = false;
      }
    }
  } else {
    size_t llvm_param = 0;
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
  }

  if (ok) {
    ok = lower_mir_block_body(lctx, fn, &values, &blocks, visited, 0, module,
                              builder, is_coro ? &coro : NULL);
  }
  for (size_t i = 0; ok && i < fn->blocks.len; i++) {
    if (!visited[i]) {
      ok = lower_mir_block_body(lctx, fn, &values, &blocks, visited,
                                (MirBlockId)i, module, builder,
                                is_coro ? &coro : NULL);
    }
  }
  if (ok) {
    ok = lower_mir_add_phi_incomings(fn, &values, &blocks, builder);
  }
  if (ok && is_coro) {
    coro_emit_cleanup_and_suspend(&lctx->jit_ctx, module, builder, coro.coro_id,
                                  coro.handle, coro.cleanup_bb,
                                  coro.suspend_bb);
  }

  free(values.items);
  free(values.slots);
  free(values.slot_types);
  free(blocks.items);
  free(blocks.exits);
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
  MirProgram *saved_current_program = lower_mir_current_program;
  lower_mir_current_program = prog;
  lower_mir_rc_cache_reset();

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
    lower_mir_current_program = saved_current_program;
    return NULL;
  }

  if (!lower_mir_link_llvm_bitcode_dependencies(prog, module)) {
    free(lctx.functions);
    free(lctx.function_types);
    lower_mir_current_program = saved_current_program;
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
    lower_mir_current_program = saved_current_program;
    return lower_mir_void_stub(module, builder);
  }

  for (size_t i = 0; i < prog->functions.len; i++) {
    MirFunction *fn = prog->functions.items ? prog->functions.items[i] : NULL;
    if (!fn || !lctx.functions[fn->id]) {
      continue;
    }
    if (fn->is_extern || fn->blocks.len == 0 ||
        lower_mir_type_has_unresolved_vars(fn->type)) {
      continue;
    }

    bool ok = lower_mir_function_body(&lctx, fn, module, builder);
    if (!ok) {
      free(lctx.functions);
      free(lctx.function_types);
      lower_mir_current_program = saved_current_program;
      return NULL;
    }
  }

  LLVMValueRef top =
      top_fn->id < lctx.functions_len ? lctx.functions[top_fn->id] : NULL;
  free(lctx.functions);
  free(lctx.function_types);
  lower_mir_current_program = saved_current_program;
  return top ? top : lower_mir_void_stub(module, builder);
}
