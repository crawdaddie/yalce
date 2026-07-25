#include "./mir.h"
#include "backend_llvm/lib_registry.h"
#include "input.h"
#include "types/builtins.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct MirArithmeticBuiltin {
  MirPrimitiveOp int_op;
  MirPrimitiveOp uint_op;
  MirPrimitiveOp float_op;
} MirArithmeticBuiltin;

typedef struct MirComparisonBuiltin {
  MirPrimitiveOp int_op;
  MirPrimitiveOp uint_op;
  MirPrimitiveOp float_op;
  MirPrimitiveOp char_op;
} MirComparisonBuiltin;

typedef struct MirDlopenCacheEntry {
  char *path;
  struct MirDlopenCacheEntry *next;
} MirDlopenCacheEntry;

static MirDlopenCacheEntry *mir_compile_time_dlopen_cache = NULL;

static bool mir_type_is_primitive_numeric(Type *type) {
  if (!type) {
    return false;
  }

  switch (type->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
    return true;
  default:
    return false;
  }
}

static bool mir_type_is_primitive_ordered(Type *type) {
  if (!type) {
    return false;
  }

  switch (type->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
    return true;
  default:
    return false;
  }
}

static bool mir_type_is_primitive_eq(Type *type) {
  if (!type) {
    return false;
  }

  switch (type->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
    return true;
  default:
    return false;
  }
}

static Type *mir_primitive_target_type(Type *lhs_type, Type *rhs_type,
                                       const char *trait_name) {
  if (!lhs_type || !rhs_type) {
    return NULL;
  }
  if (types_equal(lhs_type, rhs_type)) {
    return lhs_type;
  }

  double lhs_rank = get_typeclass_rank(lhs_type, trait_name);
  double rhs_rank = get_typeclass_rank(rhs_type, trait_name);
  if (lhs_rank < 0.0 && rhs_rank < 0.0) {
    return NULL;
  }
  return lhs_rank >= rhs_rank ? lhs_type : rhs_type;
}

static Type *mir_value_primitive_type(MirBuilder *builder, MirValueId value,
                                      Type *fallback) {
  Type *type = builder && builder->fn
                   ? mir_function_value_type(builder->fn, value)
                   : NULL;
  if (mir_type_is_primitive_eq(type)) {
    return type;
  }

  MirInstr *def = builder && builder->fn
                      ? mir_function_find_def_instr(builder->fn, value)
                      : NULL;
  if (def && def->kind == MIR_CONST) {
    switch (def->data.const_value.kind) {
    case MIR_CONST_KIND_INT:
      return &t_int;
    case MIR_CONST_KIND_UINT64:
      return &t_uint64;
    case MIR_CONST_KIND_DOUBLE:
      return &t_num;
    case MIR_CONST_KIND_CHAR:
      return &t_char;
    case MIR_CONST_KIND_BOOL:
      return &t_bool;
    default:
      break;
    }
  } else if (def) {
    switch (def->kind) {
    case MIR_OP:
      if (def->data.op.kind == MIR_OP_KIND_CAST) {
        return def->data.op.to_type;
      }
      return def->data.op.kind == MIR_OP_KIND_TRUNC_TO_INT ? &t_int
                                                           : fallback;
    default:
      break;
    }
  }

  return mir_type_is_primitive_eq(fallback) ? fallback : type;
}

static Type *mir_call_expected_operand_type(MirInstr *call, size_t index) {
  Type *cursor = call->data.call.callee_type;
  for (size_t i = 0; cursor && cursor->kind == T_FN; i++) {
    if (i == index) {
      return cursor->data.T_FN.from;
    }
    cursor = cursor->data.T_FN.to;
  }

  return NULL;
}

static Type *mir_call_primitive_operand_type(MirBuilder *builder,
                                             MirInstr *call, size_t index) {
  if (index >= call->data.call.operands.len) {
    return NULL;
  }

  MirValueId value = call->data.call.operands.items[index];
  return mir_value_primitive_type(builder, value,
                                  mir_call_expected_operand_type(call, index));
}

static MirValueId mir_primitive_cast_if_needed(MirBuilder *builder,
                                               MirValueId value,
                                               Type *from_type, Type *to_type,
                                               Ast *origin) {
  if (value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }
  if (!from_type || !to_type || types_equal(from_type, to_type)) {
    return value;
  }
  if (!mir_type_is_primitive_numeric(from_type) ||
      !mir_type_is_primitive_numeric(to_type)) {
    return MIR_NO_VALUE;
  }
  return mir_primitive_cast(builder, from_type, to_type, origin, value);
}

MirValueId mir_array_literal(MirBuilder *builder, Type *type, Ast *origin,
                             MirValueIdVec items) {
  return mir_emit_construct_items(builder, MIR_CONSTRUCT_ARRAY_LITERAL, type,
                                  origin, items);
}

static MirValueId mir_array_size(MirBuilder *builder, Ast *origin,
                                 MirValueId array) {
  if (array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  return mir_tuple_get(builder, &t_int, origin, array, 0);
}

static Type *mir_array_element_type(Type *array_type) {
  if (!array_type || !is_array_type(array_type) ||
      !array_type->data.T_CONS.args || array_type->data.T_CONS.num_args < 1) {
    return NULL;
  }
  return array_type->data.T_CONS.args[0];
}

static MirValueId mir_array_data_ptr(MirBuilder *builder, Type *array_type,
                                     Ast *origin, MirValueId array) {
  Type *element_type = mir_array_element_type(array_type);
  if (!element_type || array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  return mir_tuple_get(builder, ptr_of_type(element_type), origin, array, 2);
}

static MirValueId mir_array_offset_field(MirBuilder *builder, Ast *origin,
                                         MirValueId array) {
  if (array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  return mir_tuple_get(builder, &t_int, origin, array, 1);
}

static MirValueId mir_array_view(MirBuilder *builder, Type *array_type,
                                 Ast *origin, MirValueId size,
                                 MirValueId offset, MirValueId data_ptr) {
  if (size == MIR_NO_VALUE || offset == MIR_NO_VALUE ||
      data_ptr == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueIdVec fields = {0};
  mir_value_id_vec_push(builder->fn->arena, &fields, size);
  mir_value_id_vec_push(builder->fn->arena, &fields, offset);
  mir_value_id_vec_push(builder->fn->arena, &fields, data_ptr);
  return mir_tuple(builder, array_type, origin, fields);
}

static MirValueId mir_array_at(MirBuilder *builder, Type *type, Ast *origin,
                               MirValueId array, MirValueId index) {
  if (array == MIR_NO_VALUE || index == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *array_type = mir_function_value_type(builder->fn, array);
  Type *element_type = mir_array_element_type(array_type);
  if (!element_type) {
    return MIR_NO_VALUE;
  }

  Type *ptr_type = ptr_of_type(element_type);
  MirValueId data_ptr = mir_array_data_ptr(builder, array_type, origin, array);
  MirValueId element_ptr =
      mir_ptr_offset(builder, ptr_type, origin, data_ptr, index);
  return mir_ptr_load(builder, type, origin, element_ptr);
}

static MirValueId mir_array_set(MirBuilder *builder, Type *type, Ast *origin,
                                MirValueId array, MirValueId index,
                                MirValueId value) {
  if (array == MIR_NO_VALUE || index == MIR_NO_VALUE || value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *array_type = mir_function_value_type(builder->fn, array);
  Type *element_type = mir_array_element_type(array_type);
  if (!element_type) {
    return MIR_NO_VALUE;
  }

  Type *ptr_type = ptr_of_type(element_type);
  MirValueId data_ptr = mir_array_data_ptr(builder, array_type, origin, array);
  MirValueId element_ptr =
      mir_ptr_offset(builder, ptr_type, origin, data_ptr, index);
  if (mir_ptr_store(builder, origin, element_ptr, value) == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }
  (void)type;
  return array;
}

static MirValueId mir_array_fill_const(MirBuilder *builder, Type *type,
                                       Ast *origin, MirValueId size,
                                       MirValueId value) {
  return mir_emit_construct_ops2(builder, MIR_CONSTRUCT_ARRAY_FILL_CONST, type,
                                 origin, size, value);
}

static MirValueId mir_array_fill(MirBuilder *builder, Type *type, Ast *origin,
                                 MirValueId size, MirValueId fill_fn) {
  return mir_emit_construct_ops2(builder, MIR_CONSTRUCT_ARRAY_FILL, type,
                                 origin, size, fill_fn);
}

static MirValueId mir_array_range(MirBuilder *builder, Type *type, Ast *origin,
                                  MirValueId offset, MirValueId size,
                                  MirValueId array) {
  if (offset == MIR_NO_VALUE || size == MIR_NO_VALUE || array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *element_type = mir_array_element_type(type);
  if (!element_type) {
    return MIR_NO_VALUE;
  }

  Type *ptr_type = ptr_of_type(element_type);
  MirValueId current_offset = mir_array_offset_field(builder, origin, array);
  MirValueId data_ptr = mir_array_data_ptr(builder, type, origin, array);
  MirValueId new_data_ptr =
      mir_ptr_offset(builder, ptr_type, origin, data_ptr, offset);
  MirValueId new_offset =
      mir_iadd(builder, &t_int, origin, current_offset, offset);
  return mir_array_view(builder, type, origin, size, new_offset, new_data_ptr);
}

static MirValueId mir_array_succ(MirBuilder *builder, Type *type, Ast *origin,
                                 MirValueId array) {
  if (array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *element_type = mir_array_element_type(type);
  if (!element_type) {
    return MIR_NO_VALUE;
  }

  MirValueId size = mir_array_size(builder, origin, array);
  MirValueId zero = mir_const_int(builder, &t_int, origin, 0);
  MirValueId has_items = mir_primitive_instr(
      builder, MIR_OP_IGT, &t_bool, origin, (MirValueId[]){size, zero}, 2);
  MirValueId offset =
      mir_primitive_cast(builder, &t_bool, &t_int, origin, has_items);
  MirValueId new_size = mir_isub(builder, &t_int, origin, size, offset);
  Type *ptr_type = ptr_of_type(element_type);
  MirValueId current_offset = mir_array_offset_field(builder, origin, array);
  MirValueId data_ptr = mir_array_data_ptr(builder, type, origin, array);
  MirValueId new_data_ptr =
      mir_ptr_offset(builder, ptr_type, origin, data_ptr, offset);
  MirValueId new_offset =
      mir_iadd(builder, &t_int, origin, current_offset, offset);
  return mir_array_view(builder, type, origin, new_size, new_offset,
                        new_data_ptr);
}

static MirValueId mir_array_offset(MirBuilder *builder, Type *type, Ast *origin,
                                   MirValueId offset, MirValueId array) {
  if (offset == MIR_NO_VALUE || array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *element_type = mir_array_element_type(type);
  if (!element_type) {
    return MIR_NO_VALUE;
  }

  MirValueId size = mir_array_size(builder, origin, array);
  MirValueId zero = mir_const_int(builder, &t_int, origin, 0);
  MirValueId has_items = mir_primitive_instr(
      builder, MIR_OP_IGT, &t_bool, origin, (MirValueId[]){size, zero}, 2);
  MirValueId mask =
      mir_primitive_cast(builder, &t_bool, &t_int, origin, has_items);
  MirValueId size_decrement = mir_imul(builder, &t_int, origin, offset, mask);
  MirValueId new_size = mir_isub(builder, &t_int, origin, size, size_decrement);
  Type *ptr_type = ptr_of_type(element_type);
  MirValueId current_offset = mir_array_offset_field(builder, origin, array);
  MirValueId data_ptr = mir_array_data_ptr(builder, type, origin, array);
  MirValueId new_data_ptr =
      mir_ptr_offset(builder, ptr_type, origin, data_ptr, size_decrement);
  MirValueId new_offset =
      mir_iadd(builder, &t_int, origin, current_offset, size_decrement);
  return mir_array_view(builder, type, origin, new_size, new_offset,
                        new_data_ptr);
}

static Type *mir_coro_yield_type(Type *type) {
  if (!type || !is_coroutine_type(type) || !type->data.T_CONS.args ||
      type->data.T_CONS.num_args < 1) {
    return NULL;
  }
  return type->data.T_CONS.args[0];
}

static MirValueId mir_coro_new_call_args(MirBuilder *builder, Type *type,
                                         Ast *origin, MirValueId callee,
                                         Type *callee_type,
                                         const MirValueId *args, size_t argc) {
  if (!builder || !builder->fn || callee == MIR_NO_VALUE ||
      (argc > 0 && !args)) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CORO_NEW, type, origin);
  mir_emit_call_init(&instr, callee, NULL, callee_type);
  for (size_t i = 0; i < argc; i++) {
    if (args[i] == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_value_id_vec_push(builder->fn->arena, &instr.data.call.operands,
                          args[i]);
    mir_operand_use_vec_push(builder->fn->arena, &instr.data.call.operand_uses,
                             MIR_OPERAND_USE_CONSUME);
  }
  mir_prepare_call(builder, &instr);
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_coro_new_call(MirBuilder *builder, Type *type,
                                    Ast *origin, MirValueId callee,
                                    Type *callee_type, MirValueId arg) {
  return mir_coro_new_call_args(builder, type, origin, callee, callee_type,
                                &arg, 1);
}

static const char *mir_cor_loop_wrapper_name(MirProgram *program) {
  char name[64];
  snprintf(name, sizeof(name), "$builtin.cor_loop.%u",
           program ? (unsigned)program->functions.len : 0);
  return mir_arena_strdup(program->arena, name);
}

static const char *mir_cor_map_wrapper_name(MirProgram *program) {
  char name[64];
  snprintf(name, sizeof(name), "$builtin.cor_map.%u",
           program ? (unsigned)program->functions.len : 0);
  return mir_arena_strdup(program->arena, name);
}

static const char *mir_cor_zip_wrapper_name(MirProgram *program) {
  char name[64];
  snprintf(name, sizeof(name), "$builtin.cor_zip.%u",
           program ? (unsigned)program->functions.len : 0);
  return mir_arena_strdup(program->arena, name);
}

static const char *mir_iter_wrapper_name(MirProgram *program,
                                         const char *source_kind) {
  char name[64];
  snprintf(name, sizeof(name), "$builtin.iter.%s.%u",
           source_kind ? source_kind : "unknown",
           program ? (unsigned)program->functions.len : 0);
  return mir_arena_strdup(program->arena, name);
}

static MirValueId mir_value_op(MirBuilder *builder, MirOpKind kind, Type *type,
                               Ast *origin, MirValueId value) {
  return mir_emit_op1(builder, kind, type, origin, value);
}

static MirValueId mir_value_op_no_operand(MirBuilder *builder, MirOpKind kind,
                                          Type *type, Ast *origin) {
  return mir_emit_op0(builder, kind, type, origin);
}

static MirValueId mir_primitive_ordered_binop(MirBuilder *builder, Ast *app,
                                              MirCtx *ctx,
                                              MirComparisonBuiltin *ops) {
  Ast *lhs_ast = app->data.AST_APPLICATION.args;
  Ast *rhs_ast = app->data.AST_APPLICATION.args + 1;
  MirValueId lhs = mir_expr(builder, lhs_ast, ctx);
  MirValueId rhs = mir_expr(builder, rhs_ast, ctx);

  Type *lhs_type = mir_value_primitive_type(builder, lhs, lhs_ast->type);
  Type *rhs_type = mir_value_primitive_type(builder, rhs, rhs_ast->type);
  Type *target_type = mir_primitive_target_type(lhs_type, rhs_type, "ord");
  if (!mir_type_is_primitive_ordered(target_type)) {
    return MIR_NO_VALUE;
  }

  lhs = mir_primitive_cast_if_needed(builder, lhs, lhs_type, target_type,
                                     lhs_ast);
  rhs = mir_primitive_cast_if_needed(builder, rhs, rhs_type, target_type,
                                     rhs_ast);
  if (lhs == MIR_NO_VALUE || rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  switch (target_type->kind) {
  case T_INT:
    return mir_primitive_instr(builder, ops->int_op, &t_bool, app,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_UINT64:
    return mir_primitive_instr(builder, ops->uint_op, &t_bool, app,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_NUM:
    return mir_primitive_instr(builder, ops->float_op, &t_bool, app,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_CHAR:
    return mir_primitive_instr(builder, ops->char_op, &t_bool, app,
                               (MirValueId[]){lhs, rhs}, 2);
  default:
    return MIR_NO_VALUE;
  }
}

static MirValueId mir_primitive_eq_values(MirBuilder *builder, Ast *origin,
                                          MirValueId lhs, Type *lhs_hint,
                                          MirValueId rhs, Type *rhs_hint,
                                          bool negate) {
  if (lhs == MIR_NO_VALUE || rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *lhs_type = mir_value_primitive_type(builder, lhs, lhs_hint);
  Type *rhs_type = mir_value_primitive_type(builder, rhs, rhs_hint);
  Type *target_type = mir_primitive_target_type(lhs_type, rhs_type, "eq");
  if (!mir_type_is_primitive_eq(target_type)) {
    return MIR_NO_VALUE;
  }

  lhs =
      mir_primitive_cast_if_needed(builder, lhs, lhs_type, target_type, origin);
  rhs =
      mir_primitive_cast_if_needed(builder, rhs, rhs_type, target_type, origin);
  if (lhs == MIR_NO_VALUE || rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueId eq = MIR_NO_VALUE;
  switch (target_type->kind) {
  case T_INT:
    eq = mir_ieq(builder, origin, lhs, rhs);
    break;
  case T_UINT64:
    eq = mir_ueq(builder, origin, lhs, rhs);
    break;
  case T_NUM:
    eq = mir_feq(builder, origin, lhs, rhs);
    break;
  case T_CHAR:
    eq = mir_ceq(builder, origin, lhs, rhs);
    break;
  case T_BOOL:
    eq = mir_beq(builder, origin, lhs, rhs);
    break;
  default:
    return MIR_NO_VALUE;
  }

  return negate ? mir_lnot(builder, origin, eq) : eq;
}

static MirValueId mir_primitive_eq_binop(MirBuilder *builder, Ast *app,
                                         MirCtx *ctx, bool negate) {
  Ast *lhs_ast = app->data.AST_APPLICATION.args;
  Ast *rhs_ast = app->data.AST_APPLICATION.args + 1;
  Type *target_type =
      mir_primitive_target_type(lhs_ast->type, rhs_ast->type, "eq");
  if (!mir_type_is_primitive_eq(target_type)) {
    return MIR_NO_VALUE;
  }

  MirValueId lhs = mir_expr(builder, lhs_ast, ctx);
  MirValueId rhs = mir_expr(builder, rhs_ast, ctx);
  return mir_primitive_eq_values(builder, app, lhs, lhs_ast->type, rhs,
                                 rhs_ast->type, negate);
}

static MirValueId mir_short_circuit_bool(MirBuilder *builder, Ast *app,
                                         MirCtx *ctx, bool is_or) {
  Ast *lhs_ast = app->data.AST_APPLICATION.args;
  Ast *rhs_ast = app->data.AST_APPLICATION.args + 1;
  if (!lhs_ast->type || lhs_ast->type->kind != T_BOOL || !rhs_ast->type ||
      rhs_ast->type->kind != T_BOOL) {
    return MIR_NO_VALUE;
  }

  MirValueId lhs = mir_expr(builder, lhs_ast, ctx);
  if (lhs == MIR_NO_VALUE || !builder->block ||
      builder->block->term.kind != MIR_TERM_NONE) {
    return MIR_NO_VALUE;
  }

  MirBlock *rhs_block = mir_function_add_block(builder->fn, "logical.rhs");
  MirBlock *short_block = mir_function_add_block(builder->fn, "logical.short");
  MirBlock *continuation_block =
      mir_function_add_block(builder->fn, "logical.cont");
  if (!rhs_block || !short_block || !continuation_block) {
    return MIR_NO_VALUE;
  }

  if (is_or) {
    mir_builder_set_cond(builder, lhs, short_block->id, rhs_block->id);
  } else {
    mir_builder_set_cond(builder, lhs, rhs_block->id, short_block->id);
  }

  MirPhiIncomingVec incoming = {0};

  mir_builder_position_at_end(builder, short_block);
  MirValueId short_value = mir_const_bool(builder, &t_bool, app, is_or);
  if (short_value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }
  MirBlockId short_pred = builder->block ? builder->block->id : MIR_NO_BLOCK;
  mir_phi_incoming_vec_push(
      builder->fn->arena, &incoming,
      (MirPhiIncoming){.block = short_pred, .value = short_value});
  mir_builder_set_br(builder, continuation_block->id);

  mir_builder_position_at_end(builder, rhs_block);
  MirValueId rhs = mir_expr(builder, rhs_ast, ctx);
  if (rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }
  if (builder->block && builder->block->term.kind == MIR_TERM_NONE) {
    mir_phi_incoming_vec_push(
        builder->fn->arena, &incoming,
        (MirPhiIncoming){.block = builder->block->id, .value = rhs});
    mir_builder_set_br(builder, continuation_block->id);
  }

  mir_builder_position_at_end(builder, continuation_block);
  return mir_phi(builder, &t_bool, app, incoming);
}

static MirValueId mir_primitive_arithmetic_binop(MirBuilder *builder, Ast *app,
                                                 MirCtx *ctx,
                                                 MirArithmeticBuiltin *ops) {
  Type *result_type = app->type;
  if (!mir_type_is_primitive_numeric(result_type)) {
    return MIR_NO_VALUE;
  }

  Ast *lhs_ast = app->data.AST_APPLICATION.args;
  Ast *rhs_ast = app->data.AST_APPLICATION.args + 1;

  MirValueId lhs = mir_expr(builder, lhs_ast, ctx);
  MirValueId rhs = mir_expr(builder, rhs_ast, ctx);

  lhs = mir_primitive_cast_if_needed(builder, lhs, lhs_ast->type, result_type,
                                     lhs_ast);
  rhs = mir_primitive_cast_if_needed(builder, rhs, rhs_ast->type, result_type,
                                     rhs_ast);

  switch (result_type->kind) {
  case T_INT:
    return mir_primitive_instr(builder, ops->int_op, result_type, app,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_UINT64:
    return mir_primitive_instr(builder, ops->uint_op, result_type, app,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_NUM:
    return mir_primitive_instr(builder, ops->float_op, result_type, app,
                               (MirValueId[]){lhs, rhs}, 2);
  default:
    return MIR_NO_VALUE;
  }
}

static bool mir_builtin_arity(Ast *app, size_t arity) {
  return app && app->tag == AST_APPLICATION &&
         app->data.AST_APPLICATION.len == arity;
}

static MirValueId MirConstructorHandler(MirBuilder *builder, Ast *app,
                                        MirCtx *ctx, MirBuiltinSymbol *symbol) {
  return mir_constructor_call(builder, app, app->type, symbol->name,
                              app->data.AST_APPLICATION.args,
                              app->data.AST_APPLICATION.len, ctx);
}

static MirValueId MirListPrependHandler(MirBuilder *builder, Ast *app,
                                        MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2) || !is_list_type(app->type)) {
    return MIR_NO_VALUE;
  }

  MirValueId head = mir_expr(builder, app->data.AST_APPLICATION.args, ctx);
  MirValueId tail = mir_expr(builder, app->data.AST_APPLICATION.args + 1, ctx);
  return mir_list_cons(builder, app->type, app, head, tail);
}

static MirValueId MirDoubleConstructorHandler(MirBuilder *builder, Ast *app,
                                              MirCtx *ctx,
                                              MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }

  Ast *arg = app->data.AST_APPLICATION.args;
  Type *target_type = &t_num;
  Type *from_type = arg->type;
  if (!target_type || !from_type ||
      !mir_type_is_primitive_numeric(target_type) ||
      !mir_type_is_primitive_numeric(from_type)) {
    return MIR_NO_VALUE;
  }

  MirValueId value = mir_expr(builder, arg, ctx);
  return mir_primitive_cast_if_needed(builder, value, from_type, target_type,
                                      app);
}

static MirValueId MirAddHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  MirArithmeticBuiltin ops = {MIR_OP_IADD, MIR_OP_UADD, MIR_OP_FADD};
  return mir_primitive_arithmetic_binop(builder, app, ctx, &ops);
}

static MirValueId MirSubHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  MirArithmeticBuiltin ops = {MIR_OP_ISUB, MIR_OP_USUB, MIR_OP_FSUB};
  return mir_primitive_arithmetic_binop(builder, app, ctx, &ops);
}

static MirValueId MirMulHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  MirArithmeticBuiltin ops = {MIR_OP_IMUL, MIR_OP_UMUL, MIR_OP_FMUL};
  return mir_primitive_arithmetic_binop(builder, app, ctx, &ops);
}

static MirValueId MirDivHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  MirArithmeticBuiltin ops = {MIR_OP_IDIV, MIR_OP_UDIV, MIR_OP_FDIV};
  return mir_primitive_arithmetic_binop(builder, app, ctx, &ops);
}

static MirValueId MirModHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  MirArithmeticBuiltin ops = {MIR_OP_IMOD, MIR_OP_UMOD, MIR_OP_FMOD};
  return mir_primitive_arithmetic_binop(builder, app, ctx, &ops);
}

static MirValueId MirGtHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                               MirBuiltinSymbol *symbol) {
  (void)symbol;
  MirComparisonBuiltin ops = {MIR_OP_IGT, MIR_OP_UGT, MIR_OP_FGT, MIR_OP_CGT};
  return mir_primitive_ordered_binop(builder, app, ctx, &ops);
}

static MirValueId MirGteHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  MirComparisonBuiltin ops = {MIR_OP_IGTE, MIR_OP_UGTE, MIR_OP_FGTE,
                              MIR_OP_CGTE};
  return mir_primitive_ordered_binop(builder, app, ctx, &ops);
}

static MirValueId MirLtHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                               MirBuiltinSymbol *symbol) {
  (void)symbol;
  MirComparisonBuiltin ops = {MIR_OP_ILT, MIR_OP_ULT, MIR_OP_FLT, MIR_OP_CLT};
  return mir_primitive_ordered_binop(builder, app, ctx, &ops);
}

static MirValueId MirLteHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  MirComparisonBuiltin ops = {MIR_OP_ILTE, MIR_OP_ULTE, MIR_OP_FLTE,
                              MIR_OP_CLTE};
  return mir_primitive_ordered_binop(builder, app, ctx, &ops);
}

static MirValueId MirEqAppHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                  MirBuiltinSymbol *symbol) {
  (void)symbol;
  return mir_primitive_eq_binop(builder, app, ctx, false);
}

static MirValueId MirNeqHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  return mir_primitive_eq_binop(builder, app, ctx, true);
}

static MirValueId MirLogicalAndHandler(MirBuilder *builder, Ast *app,
                                       MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  return mir_short_circuit_bool(builder, app, ctx, false);
}

static MirValueId MirLogicalOrHandler(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  return mir_short_circuit_bool(builder, app, ctx, true);
}

static MirValueId MirLogicalNotHandler(MirBuilder *builder, Ast *app,
                                       MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }

  Ast *arg = app->data.AST_APPLICATION.args;
  if (!arg->type || arg->type->kind != T_BOOL) {
    return MIR_NO_VALUE;
  }

  return mir_lnot(builder, app, mir_expr(builder, arg, ctx));
}

static MirFunction *mir_build_cor_loop_wrapper(MirBuilder *builder, Ast *app,
                                               Type *coro_type) {
  Type *yield_type = mir_coro_yield_type(coro_type);
  Type *next_type = yield_type ? create_option_type(yield_type) : NULL;
  Type *some_type = next_type && next_type->data.T_CONS.args
                        ? next_type->data.T_CONS.args[0]
                        : NULL;
  if (!some_type) {
    return NULL;
  }

  Type *wrapper_type = type_fn(coro_type, coro_type);
  wrapper_type->data.T_FN.attributes = set_attr(
      wrapper_type->data.T_FN.attributes, FN_ATTR_COROUTINE_CONSTRUCTOR);
  MirFunction *wrapper = mir_program_add_function(
      builder->program, mir_cor_loop_wrapper_name(builder->program),
      wrapper_type, app);
  if (!wrapper) {
    return NULL;
  }

  MirValueId source = mir_function_add_param(wrapper, "source", coro_type, app);
  if (source == MIR_NO_VALUE) {
    return NULL;
  }

  MirBlock *entry = mir_function_add_block(wrapper, "entry");
  MirBlock *check = mir_function_add_block(wrapper, "cor_loop.check");
  MirBlock *value = mir_function_add_block(wrapper, "cor_loop.value");
  MirBlock *resume = mir_function_add_block(wrapper, "cor_loop.resume");
  MirBlock *reset = mir_function_add_block(wrapper, "cor_loop.reset");
  if (!entry || !check || !value || !resume || !reset) {
    return NULL;
  }

  MirBuilder wrapper_builder;
  mir_builder_init(&wrapper_builder, builder->program, wrapper);

  mir_builder_position_at_end(&wrapper_builder, entry);
  mir_builder_set_br(&wrapper_builder, check->id);

  mir_builder_position_at_end(&wrapper_builder, check);
  MirPhiIncomingVec incoming = {0};
  mir_phi_incoming_vec_push(wrapper->arena, &incoming,
                            (MirPhiIncoming){entry->id, source});
  MirValueId current = mir_phi(&wrapper_builder, coro_type, app, incoming);
  MirInstr *current_phi = mir_function_find_def_instr(wrapper, current);
  if (!current_phi || current_phi->kind != MIR_PHI) {
    return NULL;
  }
  mir_phi_incoming_vec_push(wrapper->arena, &current_phi->data.phi.incoming,
                            (MirPhiIncoming){resume->id, current});
  MirValueId next = mir_coro_next(&wrapper_builder, app, current, coro_type);
  MirValueId tag = mir_variant_tag(&wrapper_builder, app, next);
  MirValueId is_some =
      mir_tag_eq(&wrapper_builder, app, tag, 0, TYPE_NAME_SOME);
  if (current == MIR_NO_VALUE || next == MIR_NO_VALUE || tag == MIR_NO_VALUE ||
      is_some == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_cond(&wrapper_builder, is_some, value->id, reset->id);

  mir_builder_position_at_end(&wrapper_builder, value);
  MirValueId payload = mir_variant_payload(&wrapper_builder, app, next,
                                           some_type, 0, TYPE_NAME_SOME);
  MirValueId yielded =
      mir_tuple_get(&wrapper_builder, yield_type, app, payload, 0);
  if (payload == MIR_NO_VALUE || yielded == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_yield(&wrapper_builder, yielded, resume->id);

  mir_builder_position_at_end(&wrapper_builder, resume);
  mir_builder_set_br(&wrapper_builder, check->id);

  mir_builder_position_at_end(&wrapper_builder, reset);
  MirValueId reset_value =
      mir_coro_reset(&wrapper_builder, app, current, coro_type);
  if (reset_value == MIR_NO_VALUE) {
    return NULL;
  }
  mir_phi_incoming_vec_push(wrapper->arena, &current_phi->data.phi.incoming,
                            (MirPhiIncoming){reset->id, reset_value});
  mir_builder_set_br(&wrapper_builder, check->id);

  return wrapper;
}

static MirValueId MirCorLoopHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                    MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1) || !is_coroutine_type(app->type)) {
    return MIR_NO_VALUE;
  }

  Ast *arg = app->data.AST_APPLICATION.args;
  Type *coro_type = is_coroutine_type(arg->type) ? arg->type : app->type;
  MirValueId source = mir_expr(builder, arg, ctx);
  if (source == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirFunction *wrapper = mir_build_cor_loop_wrapper(builder, app, coro_type);
  if (!wrapper) {
    return MIR_NO_VALUE;
  }

  MirValueId wrapper_ref = mir_fn_ref(builder, wrapper->type, app, wrapper);
  return mir_coro_new_call(builder, app->type, app, wrapper_ref, wrapper->type,
                           source);
}

static MirFunction *mir_build_cor_map_wrapper(MirBuilder *builder, Ast *app,
                                              Type *map_type,
                                              Type *source_coro_type,
                                              Type *output_coro_type) {
  Type *input_type = mir_coro_yield_type(source_coro_type);
  Type *output_type = mir_coro_yield_type(output_coro_type);
  Type *next_type = input_type ? create_option_type(input_type) : NULL;
  Type *some_type = next_type && next_type->data.T_CONS.args
                        ? next_type->data.T_CONS.args[0]
                        : NULL;
  if (!map_type || map_type->kind != T_FN || !input_type || !output_type ||
      !some_type) {
    return NULL;
  }

  Type *wrapper_type =
      type_fn(map_type, type_fn(source_coro_type, output_coro_type));
  wrapper_type->data.T_FN.attributes = set_attr(
      wrapper_type->data.T_FN.attributes, FN_ATTR_COROUTINE_CONSTRUCTOR);
  MirFunction *wrapper = mir_program_add_function(
      builder->program, mir_cor_map_wrapper_name(builder->program),
      wrapper_type, app);
  if (!wrapper) {
    return NULL;
  }

  MirValueId mapper = mir_function_add_param(wrapper, "map", map_type, app);
  MirValueId source =
      mir_function_add_param(wrapper, "source", source_coro_type, app);
  if (mapper == MIR_NO_VALUE || source == MIR_NO_VALUE) {
    return NULL;
  }

  MirBlock *entry = mir_function_add_block(wrapper, "entry");
  MirBlock *check = mir_function_add_block(wrapper, "cor_map.check");
  MirBlock *value = mir_function_add_block(wrapper, "cor_map.value");
  MirBlock *resume = mir_function_add_block(wrapper, "cor_map.resume");
  MirBlock *done = mir_function_add_block(wrapper, "cor_map.done");
  if (!entry || !check || !value || !resume || !done) {
    return NULL;
  }

  MirBuilder wrapper_builder;
  mir_builder_init(&wrapper_builder, builder->program, wrapper);

  mir_builder_position_at_end(&wrapper_builder, entry);
  mir_builder_set_br(&wrapper_builder, check->id);

  mir_builder_position_at_end(&wrapper_builder, check);
  MirValueId next =
      mir_coro_next(&wrapper_builder, app, source, source_coro_type);
  MirValueId tag = mir_variant_tag(&wrapper_builder, app, next);
  MirValueId is_some =
      mir_tag_eq(&wrapper_builder, app, tag, 0, TYPE_NAME_SOME);
  if (next == MIR_NO_VALUE || tag == MIR_NO_VALUE || is_some == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_cond(&wrapper_builder, is_some, value->id, done->id);

  mir_builder_position_at_end(&wrapper_builder, value);
  MirValueId payload = mir_variant_payload(&wrapper_builder, app, next,
                                           some_type, 0, TYPE_NAME_SOME);
  MirValueId item =
      mir_tuple_get(&wrapper_builder, input_type, app, payload, 0);
  MirValueId mapped = mir_call_value(&wrapper_builder, output_type, app, mapper,
                                     map_type, &item, 1);
  if (payload == MIR_NO_VALUE || item == MIR_NO_VALUE ||
      mapped == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_yield(&wrapper_builder, mapped, resume->id);

  mir_builder_position_at_end(&wrapper_builder, resume);
  mir_builder_set_br(&wrapper_builder, check->id);

  mir_builder_position_at_end(&wrapper_builder, done);
  mir_builder_set_coro_done(&wrapper_builder);

  return wrapper;
}

static MirValueId MirCorMapHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                   MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2) || !is_coroutine_type(app->type)) {
    return MIR_NO_VALUE;
  }

  Ast *map_arg = app->data.AST_APPLICATION.args;
  Ast *source_arg = app->data.AST_APPLICATION.args + 1;
  Type *map_type = map_arg->type;
  Type *source_coro_type = source_arg->type;
  if (!map_type || map_type->kind != T_FN ||
      is_coroutine_constructor_type(map_type) ||
      !is_coroutine_type(source_coro_type)) {
    return MIR_NO_VALUE;
  }

  MirValueId mapper = mir_expr(builder, map_arg, ctx);
  MirValueId source = mir_expr(builder, source_arg, ctx);
  if (mapper == MIR_NO_VALUE || source == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirFunction *wrapper = mir_build_cor_map_wrapper(builder, app, map_type,
                                                   source_coro_type, app->type);
  if (!wrapper) {
    return MIR_NO_VALUE;
  }

  MirValueId wrapper_ref = mir_fn_ref(builder, wrapper->type, app, wrapper);
  MirValueId args[] = {mapper, source};
  return mir_coro_new_call_args(builder, app->type, app, wrapper_ref,
                                wrapper->type, args, 2);
}

static MirFunction *mir_build_cor_zip_wrapper(MirBuilder *builder, Ast *app,
                                              Type *left_coro_type,
                                              Type *right_coro_type,
                                              Type *output_coro_type) {
  Type *left_type = mir_coro_yield_type(left_coro_type);
  Type *right_type = mir_coro_yield_type(right_coro_type);
  Type *output_type = mir_coro_yield_type(output_coro_type);
  Type *left_next_type = left_type ? create_option_type(left_type) : NULL;
  Type *right_next_type = right_type ? create_option_type(right_type) : NULL;
  Type *left_some_type = left_next_type && left_next_type->data.T_CONS.args
                             ? left_next_type->data.T_CONS.args[0]
                             : NULL;
  Type *right_some_type = right_next_type && right_next_type->data.T_CONS.args
                              ? right_next_type->data.T_CONS.args[0]
                              : NULL;
  if (!left_type || !right_type || !output_type ||
      !is_tuple_type(output_type) || output_type->data.T_CONS.num_args != 2 ||
      !left_some_type || !right_some_type) {
    return NULL;
  }

  Type *wrapper_type =
      type_fn(left_coro_type, type_fn(right_coro_type, output_coro_type));
  wrapper_type->data.T_FN.attributes = set_attr(
      wrapper_type->data.T_FN.attributes, FN_ATTR_COROUTINE_CONSTRUCTOR);
  MirFunction *wrapper = mir_program_add_function(
      builder->program, mir_cor_zip_wrapper_name(builder->program),
      wrapper_type, app);
  if (!wrapper) {
    return NULL;
  }

  MirValueId left =
      mir_function_add_param(wrapper, "left", left_coro_type, app);
  MirValueId right =
      mir_function_add_param(wrapper, "right", right_coro_type, app);
  if (left == MIR_NO_VALUE || right == MIR_NO_VALUE) {
    return NULL;
  }

  MirBlock *entry = mir_function_add_block(wrapper, "entry");
  MirBlock *check_left = mir_function_add_block(wrapper, "cor_zip.check_left");
  MirBlock *check_right =
      mir_function_add_block(wrapper, "cor_zip.check_right");
  MirBlock *value = mir_function_add_block(wrapper, "cor_zip.value");
  MirBlock *resume = mir_function_add_block(wrapper, "cor_zip.resume");
  MirBlock *done = mir_function_add_block(wrapper, "cor_zip.done");
  if (!entry || !check_left || !check_right || !value || !resume || !done) {
    return NULL;
  }

  MirBuilder wrapper_builder;
  mir_builder_init(&wrapper_builder, builder->program, wrapper);

  mir_builder_position_at_end(&wrapper_builder, entry);
  mir_builder_set_br(&wrapper_builder, check_left->id);

  mir_builder_position_at_end(&wrapper_builder, check_left);
  MirValueId left_next =
      mir_coro_next(&wrapper_builder, app, left, left_coro_type);
  MirValueId left_tag = mir_variant_tag(&wrapper_builder, app, left_next);
  MirValueId left_is_some =
      mir_tag_eq(&wrapper_builder, app, left_tag, 0, TYPE_NAME_SOME);
  if (left_next == MIR_NO_VALUE || left_tag == MIR_NO_VALUE ||
      left_is_some == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_cond(&wrapper_builder, left_is_some, check_right->id,
                       done->id);

  mir_builder_position_at_end(&wrapper_builder, check_right);
  MirValueId right_next =
      mir_coro_next(&wrapper_builder, app, right, right_coro_type);
  MirValueId right_tag = mir_variant_tag(&wrapper_builder, app, right_next);
  MirValueId right_is_some =
      mir_tag_eq(&wrapper_builder, app, right_tag, 0, TYPE_NAME_SOME);
  if (right_next == MIR_NO_VALUE || right_tag == MIR_NO_VALUE ||
      right_is_some == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_cond(&wrapper_builder, right_is_some, value->id, done->id);

  mir_builder_position_at_end(&wrapper_builder, value);
  MirValueId left_payload = mir_variant_payload(
      &wrapper_builder, app, left_next, left_some_type, 0, TYPE_NAME_SOME);
  MirValueId right_payload = mir_variant_payload(
      &wrapper_builder, app, right_next, right_some_type, 0, TYPE_NAME_SOME);
  MirValueId left_item =
      mir_tuple_get(&wrapper_builder, left_type, app, left_payload, 0);
  MirValueId right_item =
      mir_tuple_get(&wrapper_builder, right_type, app, right_payload, 0);
  if (left_payload == MIR_NO_VALUE || right_payload == MIR_NO_VALUE ||
      left_item == MIR_NO_VALUE || right_item == MIR_NO_VALUE) {
    return NULL;
  }

  MirValueIdVec items = {0};
  mir_value_id_vec_push(wrapper->arena, &items, left_item);
  mir_value_id_vec_push(wrapper->arena, &items, right_item);
  MirValueId zipped = mir_tuple(&wrapper_builder, output_type, app, items);
  if (zipped == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_yield(&wrapper_builder, zipped, resume->id);

  mir_builder_position_at_end(&wrapper_builder, resume);
  mir_builder_set_br(&wrapper_builder, check_left->id);

  mir_builder_position_at_end(&wrapper_builder, done);
  mir_builder_set_coro_done(&wrapper_builder);

  return wrapper;
}

static MirValueId MirCorZipHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                   MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2) || !is_coroutine_type(app->type)) {
    return MIR_NO_VALUE;
  }

  Ast *left_arg = app->data.AST_APPLICATION.args;
  Ast *right_arg = app->data.AST_APPLICATION.args + 1;
  Type *left_coro_type = left_arg->type;
  Type *right_coro_type = right_arg->type;
  if (!is_coroutine_type(left_coro_type) ||
      !is_coroutine_type(right_coro_type)) {
    return MIR_NO_VALUE;
  }

  MirValueId left = mir_expr(builder, left_arg, ctx);
  MirValueId right = mir_expr(builder, right_arg, ctx);
  if (left == MIR_NO_VALUE || right == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirFunction *wrapper = mir_build_cor_zip_wrapper(builder, app, left_coro_type,
                                                   right_coro_type, app->type);
  if (!wrapper) {
    return MIR_NO_VALUE;
  }

  MirValueId wrapper_ref = mir_fn_ref(builder, wrapper->type, app, wrapper);
  MirValueId args[] = {left, right};
  return mir_coro_new_call_args(builder, app->type, app, wrapper_ref,
                                wrapper->type, args, 2);
}

static MirFunction *mir_build_iter_array_wrapper(MirBuilder *builder, Ast *app,
                                                 Type *array_type,
                                                 Type *coro_type) {
  Type *element_type = mir_array_element_type(array_type);
  if (!element_type || !is_coroutine_type(coro_type)) {
    return NULL;
  }

  Type *wrapper_type = type_fn(array_type, coro_type);
  wrapper_type->data.T_FN.attributes = set_attr(
      wrapper_type->data.T_FN.attributes, FN_ATTR_COROUTINE_CONSTRUCTOR);
  MirFunction *wrapper = mir_program_add_function(
      builder->program, mir_iter_wrapper_name(builder->program, "array"),
      wrapper_type, app);
  if (!wrapper) {
    return NULL;
  }

  MirValueId array = mir_function_add_param(wrapper, "array", array_type, app);
  if (array == MIR_NO_VALUE) {
    return NULL;
  }

  MirBlock *entry = mir_function_add_block(wrapper, "entry");
  MirBlock *check = mir_function_add_block(wrapper, "iter.array.check");
  MirBlock *value = mir_function_add_block(wrapper, "iter.array.value");
  MirBlock *resume = mir_function_add_block(wrapper, "iter.array.resume");
  MirBlock *done = mir_function_add_block(wrapper, "iter.array.done");
  if (!entry || !check || !value || !resume || !done) {
    return NULL;
  }

  MirBuilder wrapper_builder;
  mir_builder_init(&wrapper_builder, builder->program, wrapper);

  mir_builder_position_at_end(&wrapper_builder, entry);
  MirValueId zero = mir_const_int(&wrapper_builder, &t_int, app, 0);
  if (zero == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_br(&wrapper_builder, check->id);

  mir_builder_position_at_end(&wrapper_builder, check);
  MirPhiIncomingVec incoming = {0};
  mir_phi_incoming_vec_push(wrapper->arena, &incoming,
                            (MirPhiIncoming){entry->id, zero});
  MirValueId index = mir_phi(&wrapper_builder, &t_int, app, incoming);
  MirInstr *index_phi = mir_function_find_def_instr(wrapper, index);
  MirValueId size = mir_array_size(&wrapper_builder, app, array);
  MirValueId in_bounds =
      mir_primitive_instr(&wrapper_builder, MIR_OP_ILT, &t_bool, app,
                          (MirValueId[]){index, size}, 2);
  if (index == MIR_NO_VALUE || !index_phi || index_phi->kind != MIR_PHI ||
      size == MIR_NO_VALUE || in_bounds == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_cond(&wrapper_builder, in_bounds, value->id, done->id);

  mir_builder_position_at_end(&wrapper_builder, value);
  MirValueId element =
      mir_array_at(&wrapper_builder, element_type, app, array, index);
  MirValueId one = mir_const_int(&wrapper_builder, &t_int, app, 1);
  MirValueId next_index = mir_iadd(&wrapper_builder, &t_int, app, index, one);
  if (element == MIR_NO_VALUE || one == MIR_NO_VALUE ||
      next_index == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_yield(&wrapper_builder, element, resume->id);

  mir_builder_position_at_end(&wrapper_builder, resume);
  mir_phi_incoming_vec_push(wrapper->arena, &index_phi->data.phi.incoming,
                            (MirPhiIncoming){resume->id, next_index});
  mir_builder_set_br(&wrapper_builder, check->id);

  mir_builder_position_at_end(&wrapper_builder, done);
  mir_builder_set_coro_done(&wrapper_builder);

  return wrapper;
}

static MirFunction *mir_build_iter_list_wrapper(MirBuilder *builder, Ast *app,
                                                Type *list_type,
                                                Type *coro_type) {
  Type *element_type = type_of_list(list_type);
  if (!element_type || !is_coroutine_type(coro_type)) {
    return NULL;
  }

  Type *wrapper_type = type_fn(list_type, coro_type);
  wrapper_type->data.T_FN.attributes = set_attr(
      wrapper_type->data.T_FN.attributes, FN_ATTR_COROUTINE_CONSTRUCTOR);
  MirFunction *wrapper = mir_program_add_function(
      builder->program, mir_iter_wrapper_name(builder->program, "list"),
      wrapper_type, app);
  if (!wrapper) {
    return NULL;
  }

  MirValueId list = mir_function_add_param(wrapper, "list", list_type, app);
  if (list == MIR_NO_VALUE) {
    return NULL;
  }

  MirBlock *entry = mir_function_add_block(wrapper, "entry");
  MirBlock *check = mir_function_add_block(wrapper, "iter.list.check");
  MirBlock *value = mir_function_add_block(wrapper, "iter.list.value");
  MirBlock *resume = mir_function_add_block(wrapper, "iter.list.resume");
  MirBlock *done = mir_function_add_block(wrapper, "iter.list.done");
  if (!entry || !check || !value || !resume || !done) {
    return NULL;
  }

  MirBuilder wrapper_builder;
  mir_builder_init(&wrapper_builder, builder->program, wrapper);

  mir_builder_position_at_end(&wrapper_builder, entry);
  mir_builder_set_br(&wrapper_builder, check->id);

  mir_builder_position_at_end(&wrapper_builder, check);
  MirPhiIncomingVec incoming = {0};
  mir_phi_incoming_vec_push(wrapper->arena, &incoming,
                            (MirPhiIncoming){entry->id, list});
  MirValueId current = mir_phi(&wrapper_builder, list_type, app, incoming);
  MirInstr *current_phi = mir_function_find_def_instr(wrapper, current);
  MirValueId is_empty = mir_list_is_empty(&wrapper_builder, app, current);
  if (current == MIR_NO_VALUE || !current_phi || current_phi->kind != MIR_PHI ||
      is_empty == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_cond(&wrapper_builder, is_empty, done->id, value->id);

  mir_builder_position_at_end(&wrapper_builder, value);
  MirValueId head = mir_list_head(&wrapper_builder, element_type, app, current);
  MirValueId tail = mir_list_tail(&wrapper_builder, list_type, app, current);
  if (head == MIR_NO_VALUE || tail == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_yield(&wrapper_builder, head, resume->id);

  mir_builder_position_at_end(&wrapper_builder, resume);
  mir_phi_incoming_vec_push(wrapper->arena, &current_phi->data.phi.incoming,
                            (MirPhiIncoming){resume->id, tail});
  mir_builder_set_br(&wrapper_builder, check->id);

  mir_builder_position_at_end(&wrapper_builder, done);
  mir_builder_set_coro_done(&wrapper_builder);

  return wrapper;
}

static MirValueId MirIterHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                 MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1) || !is_coroutine_type(app->type)) {
    return MIR_NO_VALUE;
  }

  Ast *arg = app->data.AST_APPLICATION.args;
  Type *source_type = arg->type;
  MirFunction *wrapper = NULL;
  if (is_array_type(source_type)) {
    wrapper =
        mir_build_iter_array_wrapper(builder, app, source_type, app->type);
  } else if (is_list_type(source_type)) {
    wrapper = mir_build_iter_list_wrapper(builder, app, source_type, app->type);
  } else {
    return MIR_NO_VALUE;
  }
  if (!wrapper) {
    return MIR_NO_VALUE;
  }

  MirValueId source = mir_expr(builder, arg, ctx);
  if (source == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueId wrapper_ref = mir_fn_ref(builder, wrapper->type, app, wrapper);
  return mir_coro_new_call(builder, app->type, app, wrapper_ref, wrapper->type,
                           source);
}

static MirValueId MirArraySizeHandler(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }
  return mir_array_size(builder, app,
                        mir_expr(builder, app->data.AST_APPLICATION.args, ctx));
}

static MirValueId MirArrayAtHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                    MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }
  return mir_array_at(
      builder, app->type, app,
      mir_expr(builder, app->data.AST_APPLICATION.args, ctx),
      mir_expr(builder, app->data.AST_APPLICATION.args + 1, ctx));
}

static MirValueId MirArraySetHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                     MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 3)) {
    return MIR_NO_VALUE;
  }
  return mir_array_set(
      builder, app->type, app,
      mir_expr(builder, app->data.AST_APPLICATION.args, ctx),
      mir_expr(builder, app->data.AST_APPLICATION.args + 1, ctx),
      mir_expr(builder, app->data.AST_APPLICATION.args + 2, ctx));
}

static MirValueId MirArrayFillConstHandler(MirBuilder *builder, Ast *app,
                                           MirCtx *ctx,
                                           MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }
  return mir_array_fill_const(
      builder, app->type, app,
      mir_expr(builder, app->data.AST_APPLICATION.args, ctx),
      mir_expr(builder, app->data.AST_APPLICATION.args + 1, ctx));
}

static MirValueId MirArrayFillHandler(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }
  return mir_array_fill(
      builder, app->type, app,
      mir_expr(builder, app->data.AST_APPLICATION.args, ctx),
      mir_expr(builder, app->data.AST_APPLICATION.args + 1, ctx));
}

static MirValueId MirArrayRangeHandler(MirBuilder *builder, Ast *app,
                                       MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 3)) {
    return MIR_NO_VALUE;
  }
  return mir_array_range(
      builder, app->type, app,
      mir_expr(builder, app->data.AST_APPLICATION.args, ctx),
      mir_expr(builder, app->data.AST_APPLICATION.args + 1, ctx),
      mir_expr(builder, app->data.AST_APPLICATION.args + 2, ctx));
}

static MirValueId MirArraySuccHandler(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }
  return mir_array_succ(builder, app->type, app,
                        mir_expr(builder, app->data.AST_APPLICATION.args, ctx));
}

static MirValueId MirArrayOffsetHandler(MirBuilder *builder, Ast *app,
                                        MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }
  return mir_array_offset(
      builder, app->type, app,
      mir_expr(builder, app->data.AST_APPLICATION.args, ctx),
      mir_expr(builder, app->data.AST_APPLICATION.args + 1, ctx));
}

static MirValueId mir_builtin_unary_value_op(MirBuilder *builder, Ast *app,
                                             MirCtx *ctx, MirOpKind kind) {
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }
  return mir_value_op(builder, kind, app->type, app,
                      mir_expr(builder, app->data.AST_APPLICATION.args, ctx));
}

static MirValueId MirStrHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  return mir_builtin_unary_value_op(builder, app, ctx, MIR_OP_KIND_STR);
}

static MirValueId MirCStrHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                 MirBuiltinSymbol *symbol) {
  (void)symbol;
  return mir_builtin_unary_value_op(builder, app, ctx, MIR_OP_KIND_CSTR);
}

static MirValueId MirAsBytesHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                    MirBuiltinSymbol *symbol) {
  (void)symbol;
  return mir_builtin_unary_value_op(builder, app, ctx, MIR_OP_KIND_AS_BYTES);
}

static MirValueId MirTypeOfHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                   MirBuiltinSymbol *symbol) {
  (void)symbol;
  return mir_builtin_unary_value_op(builder, app, ctx, MIR_OP_KIND_TYPEOF);
}

static bool mir_dlopen_path_should_resolve_from_source(const char *path) {
  if (!path || path[0] == '\0' || path[0] == '/' || path[0] == '~' ||
      path[0] == '@' || strstr(path, "://")) {
    return false;
  }
  return strchr(path, '/') != NULL;
}

static const char *mir_dlopen_source_path(MirBuilder *builder, Ast *app) {
  if (app && app->loc_info && app->loc_info->src_file) {
    return app->loc_info->src_file;
  }
  if (builder && builder->fn && builder->fn->origin &&
      builder->fn->origin->loc_info &&
      builder->fn->origin->loc_info->src_file) {
    return builder->fn->origin->loc_info->src_file;
  }
  return NULL;
}

static char *mir_resolve_dlopen_path(const char *path,
                                     const char *source_path) {
  if (!path) {
    return NULL;
  }
  if (!mir_dlopen_path_should_resolve_from_source(path) || !source_path) {
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

static bool mir_compile_time_dlopen_cache_contains(const char *path) {
  if (!path) {
    return false;
  }
  for (MirDlopenCacheEntry *entry = mir_compile_time_dlopen_cache; entry;
       entry = entry->next) {
    if (entry->path && strcmp(entry->path, path) == 0) {
      return true;
    }
  }
  return false;
}

static void mir_compile_time_dlopen_cache_insert(const char *path) {
  if (!path || mir_compile_time_dlopen_cache_contains(path)) {
    return;
  }

  MirDlopenCacheEntry *entry = calloc(1, sizeof(MirDlopenCacheEntry));
  if (!entry) {
    return;
  }
  entry->path = strdup(path);
  if (!entry->path) {
    free(entry);
    return;
  }
  entry->next = mir_compile_time_dlopen_cache;
  mir_compile_time_dlopen_cache = entry;
}

static MirValueId MirDlopenHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                   MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }

  Ast *arg = app->data.AST_APPLICATION.args;
  if (!arg || arg->tag != AST_STRING) {
    return mir_value_op(builder, MIR_OP_KIND_DLOPEN, app->type, app,
                        mir_expr(builder, arg, ctx));
  }

  const char *path = arg->data.AST_STRING.value;
  char *full_path =
      mir_resolve_dlopen_path(path, mir_dlopen_source_path(builder, app));
  if (!full_path) {
    return mir_value_op(builder, MIR_OP_KIND_DLOPEN, app->type, app,
                        mir_expr(builder, arg, ctx));
  }

  if (mir_compile_time_dlopen_cache_contains(full_path)) {
    free(full_path);
    return mir_const_void(builder, app->type, app);
  }

  ylc_jit_ctx = NULL;
  ylc_jit_module = NULL;
  ylc_jit_builder = NULL;
  ylc_mir_program = builder ? builder->program : NULL;
  ylc_mir_ctx = ctx;
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
    MirValueId path_value = mir_expr(builder, arg, ctx);
    MirValueId fallback =
        mir_value_op(builder, MIR_OP_KIND_DLOPEN, app->type, app, path_value);
    free(full_path);
    return fallback;
  }

  mir_compile_time_dlopen_cache_insert(full_path);
  fprintf(stderr, "loaded %s\n", full_path);
  free(full_path);
  return mir_const_void(builder, app->type, app);
}

static MirValueId MirPrintHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                  MirBuiltinSymbol *symbol) {
  // if (!mir_builtin_arity(app, 1)) {
  //   return MIR_NO_VALUE;
  // }

  Ast *arg = app->data.AST_APPLICATION.args;
  if (arg && arg->tag == AST_FMT_STRING) {
    for (int i = 0; i < arg->data.AST_LIST.len; i++) {
      Ast *item = arg->data.AST_LIST.items + i;
      MirValueId value = mir_expr(builder, item, ctx);
      if (value == MIR_NO_VALUE) {
        return MIR_NO_VALUE;
      }
      if (mir_value_op(builder, MIR_OP_KIND_PRINT, &t_void, item, value) ==
          MIR_NO_VALUE) {
        return MIR_NO_VALUE;
      }
    }
    return mir_value_op_no_operand(builder, MIR_OP_KIND_FLUSH, app->type, app);
  }

  MirValueId value = mir_expr(builder, arg, ctx);
  if (value == MIR_NO_VALUE || mir_value_op(builder, MIR_OP_KIND_PRINT, &t_void,
                                            arg, value) == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  return mir_value_op_no_operand(builder, MIR_OP_KIND_FLUSH, app->type, app);
}

static MirValueId mir_fprint_value(MirBuilder *builder, Ast *origin,
                                   MirValueId file, MirValueId value) {
  return mir_emit_op2(builder, MIR_OP_KIND_FPRINT, &t_void, origin, file,
                      value);
}

static MirValueId MirFPrintHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                   MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }

  Ast *file_arg = app->data.AST_APPLICATION.args;
  Ast *arg = app->data.AST_APPLICATION.args + 1;
  MirValueId file = mir_expr(builder, file_arg, ctx);
  if (file == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  if (arg && arg->tag == AST_FMT_STRING) {
    MirValueId result = MIR_NO_VALUE;
    for (int i = 0; i < arg->data.AST_LIST.len; i++) {
      Ast *item = arg->data.AST_LIST.items + i;
      MirValueId value = mir_expr(builder, item, ctx);
      if (value == MIR_NO_VALUE) {
        return MIR_NO_VALUE;
      }
      result = mir_fprint_value(builder, item, file, value);
      if (result == MIR_NO_VALUE) {
        return MIR_NO_VALUE;
      }
    }
    return result == MIR_NO_VALUE ? mir_const_void(builder, app->type, app)
                                  : result;
  }

  MirValueId value = mir_expr(builder, arg, ctx);
  return mir_fprint_value(builder, app, file, value);
}

static MirValueId MirSizeOfHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                   MirBuiltinSymbol *symbol) {
  (void)ctx;
  (void)symbol;
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }
  return mir_value_op_no_operand(builder, MIR_OP_KIND_SIZEOF, app->type, app);
}

static MirValueId
mir_lower_specialized_arithmetic_call(MirBuilder *builder, MirInstr *call,
                                      MirArithmeticBuiltin ops) {
  if (call->data.call.operands.len != 2 ||
      !mir_type_is_primitive_numeric(call->type)) {
    return MIR_NO_VALUE;
  }

  MirValueId lhs = call->data.call.operands.items[0];
  MirValueId rhs = call->data.call.operands.items[1];
  Type *lhs_type = mir_call_primitive_operand_type(builder, call, 0);
  Type *rhs_type = mir_call_primitive_operand_type(builder, call, 1);
  if (!mir_type_is_primitive_numeric(lhs_type) ||
      !mir_type_is_primitive_numeric(rhs_type)) {
    return MIR_NO_VALUE;
  }

  lhs = mir_primitive_cast_if_needed(builder, lhs, lhs_type, call->type,
                                     call->origin);
  rhs = mir_primitive_cast_if_needed(builder, rhs, rhs_type, call->type,
                                     call->origin);
  if (lhs == MIR_NO_VALUE || rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  switch (call->type->kind) {
  case T_INT:
    return mir_primitive_instr(builder, ops.int_op, call->type, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_UINT64:
    return mir_primitive_instr(builder, ops.uint_op, call->type, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_NUM:
    return mir_primitive_instr(builder, ops.float_op, call->type, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  default:
    return MIR_NO_VALUE;
  }
}

static MirValueId mir_lower_specialized_ordered_call(MirBuilder *builder,
                                                     MirInstr *call,
                                                     MirComparisonBuiltin ops) {
  if (call->data.call.operands.len != 2) {
    return MIR_NO_VALUE;
  }

  MirValueId lhs = call->data.call.operands.items[0];
  MirValueId rhs = call->data.call.operands.items[1];
  Type *lhs_type = mir_call_primitive_operand_type(builder, call, 0);
  Type *rhs_type = mir_call_primitive_operand_type(builder, call, 1);
  Type *target_type = mir_primitive_target_type(lhs_type, rhs_type, "ord");
  if (!mir_type_is_primitive_ordered(target_type)) {
    return MIR_NO_VALUE;
  }

  lhs = mir_primitive_cast_if_needed(builder, lhs, lhs_type, target_type,
                                     call->origin);
  rhs = mir_primitive_cast_if_needed(builder, rhs, rhs_type, target_type,
                                     call->origin);
  if (lhs == MIR_NO_VALUE || rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  switch (target_type->kind) {
  case T_INT:
    return mir_primitive_instr(builder, ops.int_op, &t_bool, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_UINT64:
    return mir_primitive_instr(builder, ops.uint_op, &t_bool, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_NUM:
    return mir_primitive_instr(builder, ops.float_op, &t_bool, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_CHAR:
    return mir_primitive_instr(builder, ops.char_op, &t_bool, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  default:
    return MIR_NO_VALUE;
  }
}

static MirValueId mir_lower_specialized_eq_call(MirBuilder *builder,
                                                MirInstr *call, bool negate) {
  if (call->data.call.operands.len != 2) {
    return MIR_NO_VALUE;
  }

  MirValueId lhs = call->data.call.operands.items[0];
  MirValueId rhs = call->data.call.operands.items[1];
  Type *lhs_type = mir_call_primitive_operand_type(builder, call, 0);
  Type *rhs_type = mir_call_primitive_operand_type(builder, call, 1);
  Type *target_type = mir_primitive_target_type(lhs_type, rhs_type, "eq");
  if (!mir_type_is_primitive_eq(target_type)) {
    return MIR_NO_VALUE;
  }

  lhs = mir_primitive_cast_if_needed(builder, lhs, lhs_type, target_type,
                                     call->origin);
  rhs = mir_primitive_cast_if_needed(builder, rhs, rhs_type, target_type,
                                     call->origin);
  if (lhs == MIR_NO_VALUE || rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueId eq = MIR_NO_VALUE;
  switch (target_type->kind) {
  case T_INT:
    eq = mir_ieq(builder, call->origin, lhs, rhs);
    break;
  case T_UINT64:
    eq = mir_ueq(builder, call->origin, lhs, rhs);
    break;
  case T_NUM:
    eq = mir_feq(builder, call->origin, lhs, rhs);
    break;
  case T_CHAR:
    eq = mir_ceq(builder, call->origin, lhs, rhs);
    break;
  case T_BOOL:
    eq = mir_beq(builder, call->origin, lhs, rhs);
    break;
  default:
    return MIR_NO_VALUE;
  }

  return negate ? mir_lnot(builder, call->origin, eq) : eq;
}

static MirValueId
mir_lower_specialized_primitive_constructor_call(MirBuilder *builder,
                                                 MirInstr *call) {
  if (call->data.call.operands.len != 1) {
    return MIR_NO_VALUE;
  }

  MirValueId operand = call->data.call.operands.items[0];
  Type *from_type = mir_call_primitive_operand_type(builder, call, 0);
  Type *target_type = call->type;
  if (!from_type || !target_type || !mir_type_is_primitive_numeric(from_type) ||
      !mir_type_is_primitive_numeric(target_type)) {
    return MIR_NO_VALUE;
  }

  return mir_primitive_cast_if_needed(builder, operand, from_type, target_type,
                                      call->origin);
}

MirValueId mir_lower_specialized_builtin_call(MirBuilder *builder,
                                              MirInstr *call) {
  if (!call || !call->data.call.builtin) {
    return MIR_NO_VALUE;
  }

  MirBuiltinHandler handler = call->data.call.builtin->handler;
  if (handler == MirAddHandler) {
    MirArithmeticBuiltin ops = {MIR_OP_IADD, MIR_OP_UADD, MIR_OP_FADD};
    return mir_lower_specialized_arithmetic_call(builder, call, ops);
  }
  if (handler == MirSubHandler) {
    MirArithmeticBuiltin ops = {MIR_OP_ISUB, MIR_OP_USUB, MIR_OP_FSUB};
    return mir_lower_specialized_arithmetic_call(builder, call, ops);
  }
  if (handler == MirMulHandler) {
    MirArithmeticBuiltin ops = {MIR_OP_IMUL, MIR_OP_UMUL, MIR_OP_FMUL};
    return mir_lower_specialized_arithmetic_call(builder, call, ops);
  }
  if (handler == MirDivHandler) {
    MirArithmeticBuiltin ops = {MIR_OP_IDIV, MIR_OP_UDIV, MIR_OP_FDIV};
    return mir_lower_specialized_arithmetic_call(builder, call, ops);
  }
  if (handler == MirModHandler) {
    MirArithmeticBuiltin ops = {MIR_OP_IMOD, MIR_OP_UMOD, MIR_OP_FMOD};
    return mir_lower_specialized_arithmetic_call(builder, call, ops);
  }
  if (handler == MirGtHandler) {
    MirComparisonBuiltin ops = {MIR_OP_IGT, MIR_OP_UGT, MIR_OP_FGT, MIR_OP_CGT};
    return mir_lower_specialized_ordered_call(builder, call, ops);
  }
  if (handler == MirGteHandler) {
    MirComparisonBuiltin ops = {MIR_OP_IGTE, MIR_OP_UGTE, MIR_OP_FGTE,
                                MIR_OP_CGTE};
    return mir_lower_specialized_ordered_call(builder, call, ops);
  }
  if (handler == MirLtHandler) {
    MirComparisonBuiltin ops = {MIR_OP_ILT, MIR_OP_ULT, MIR_OP_FLT, MIR_OP_CLT};
    return mir_lower_specialized_ordered_call(builder, call, ops);
  }
  if (handler == MirLteHandler) {
    MirComparisonBuiltin ops = {MIR_OP_ILTE, MIR_OP_ULTE, MIR_OP_FLTE,
                                MIR_OP_CLTE};
    return mir_lower_specialized_ordered_call(builder, call, ops);
  }
  if (handler == MirEqAppHandler) {
    return mir_lower_specialized_eq_call(builder, call, false);
  }
  if (handler == MirNeqHandler) {
    return mir_lower_specialized_eq_call(builder, call, true);
  }
  if (handler == MirDoubleConstructorHandler) {
    return mir_lower_specialized_primitive_constructor_call(builder, call);
  }
  return MIR_NO_VALUE;
}

void mir_register_core_builtins(MirProgram *program) {

  mir_register_builtin(
      program, builtin_envs.arith_add, MirAddHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.arith_sub, MirSubHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.arith_mul, MirMulHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.arith_div, MirDivHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.arith_mod, MirModHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.gt, MirGtHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.gte, MirGteHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.lt, MirLtHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.lte, MirLteHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.eq, MirEqAppHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.neq, MirNeqHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.logical_and, MirLogicalAndHandler,
      MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.logical_or, MirLogicalOrHandler,
      MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(program, builtin_envs.logical_not, MirLogicalNotHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1,
                       MIR_RESULT_OWNED);

  mir_register_builtin(program, builtin_envs.cor_loop, MirCorLoopHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_CONSUME}, 1,
                       MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.cor_map, MirCorMapHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_CONSUME, MIR_OPERAND_USE_CONSUME},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.cor_zip, MirCorZipHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_CONSUME, MIR_OPERAND_USE_CONSUME},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.iter, MirIterHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_CONSUME}, 1, MIR_RESULT_OWNED);

  mir_register_builtin(program, builtin_envs.some, MirConstructorHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_CONSUME}, 1,
                       MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.list_prepend, MirListPrependHandler,
      MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_CONSUME, MIR_OPERAND_USE_CONSUME},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(program, lookup_builtin_env(TYPE_NAME_DOUBLE),
                       MirDoubleConstructorHandler, MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1,
                       MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.print, MirPrintHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1, MIR_RESULT_NONE);

  mir_register_builtin(
      program, builtin_envs.fprintf, MirFPrintHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_NONE);

  mir_register_builtin(program, builtin_envs.array_size, MirArraySizeHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1,
                       MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.array_at, MirArrayAtHandler,
      MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_BORROWED);

  mir_register_builtin(program, builtin_envs.array_set, MirArraySetHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW,
                                               MIR_OPERAND_USE_BORROW,
                                               MIR_OPERAND_USE_CONSUME},
                       3, MIR_RESULT_BORROWED);

  mir_register_builtin(
      program, builtin_envs.array_fill_const, MirArrayFillConstHandler,
      MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_CONSUME},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(program, builtin_envs.array_succ, MirArraySuccHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1,
                       MIR_RESULT_BORROWED);

  mir_register_builtin(
      program, builtin_envs.array_fill, MirArrayFillHandler,
      MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_OWNED);

  mir_register_builtin(program, builtin_envs.array_range, MirArrayRangeHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW,
                                               MIR_OPERAND_USE_BORROW,
                                               MIR_OPERAND_USE_BORROW},
                       3, MIR_RESULT_BORROWED);

  mir_register_builtin(
      program, builtin_envs.array_offset, MirArrayOffsetHandler,
      MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW},
      2, MIR_RESULT_BORROWED);

  mir_register_builtin(
      program, builtin_envs.str, MirStrHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1, MIR_RESULT_OWNED);

  mir_register_builtin(
      program, builtin_envs.cstr, MirCStrHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1, MIR_RESULT_OWNED);

  mir_register_builtin(program, builtin_envs.sizeof_env, MirSizeOfHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1,
                       MIR_RESULT_OWNED);

  mir_register_builtin(program, builtin_envs.dlopen_env, MirDlopenHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1,
                       MIR_RESULT_NONE);

  mir_register_builtin(
      program, builtin_envs.asbytes, MirAsBytesHandler, MIR_BUILTIN_SYMBOL_CORE,
      (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1, MIR_RESULT_OWNED);

  mir_register_builtin(program, builtin_envs.typeof_env, MirTypeOfHandler,
                       MIR_BUILTIN_SYMBOL_CORE,
                       (const MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1,
                       MIR_RESULT_OWNED);
}
