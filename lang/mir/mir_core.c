#include "./mir.h"
#include "types/builtins.h"
#include <string.h>

typedef enum {
  MIR_BUILTIN_DATA_ARITHMETIC,
  MIR_BUILTIN_DATA_COMPARISON,
} MirBuiltinDataKind;

typedef struct MirArithmeticBuiltin {
  MirBuiltinDataKind kind;
  MirPrimitiveOp int_op;
  MirPrimitiveOp uint_op;
  MirPrimitiveOp float_op;
} MirArithmeticBuiltin;

typedef struct MirComparisonBuiltin {
  MirBuiltinDataKind kind;
  MirPrimitiveOp int_op;
  MirPrimitiveOp uint_op;
  MirPrimitiveOp float_op;
  MirPrimitiveOp char_op;
} MirComparisonBuiltin;

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
      return def->data.op.kind == MIR_OP_KIND_CAST ? def->data.op.to_type
                                                   : fallback;
    default:
      break;
    }
  }

  return mir_type_is_primitive_eq(fallback) ? fallback : type;
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
  if (!builder || !type || !is_array_type(type)) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_ARRAY_LITERAL;
  instr.data.construct.items = items;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_array_size(MirBuilder *builder, Ast *origin,
                                 MirValueId array) {
  if (!builder || array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, &t_int, origin);
  instr.data.op.kind = MIR_OP_KIND_ARRAY_SIZE;
  instr.data.op.argc = 1;
  instr.data.op.operands[0] = array;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_array_at(MirBuilder *builder, Type *type, Ast *origin,
                               MirValueId array, MirValueId index) {
  if (!builder || !type || array == MIR_NO_VALUE || index == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_ARRAY_AT;
  instr.data.extract.value = array;
  instr.data.extract.index_value = index;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_array_set(MirBuilder *builder, Type *type, Ast *origin,
                                MirValueId array, MirValueId index,
                                MirValueId value) {
  if (!builder || !type || array == MIR_NO_VALUE || index == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, type, origin);
  instr.data.op.kind = MIR_OP_KIND_ARRAY_SET;
  instr.data.op.argc = 3;
  instr.data.op.operands[0] = array;
  instr.data.op.operands[1] = index;
  instr.data.op.operands[2] = value;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_array_fill_const(MirBuilder *builder, Type *type,
                                       Ast *origin, MirValueId size,
                                       MirValueId value) {
  if (!builder || !type || !is_array_type(type) || size == MIR_NO_VALUE ||
      value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_ARRAY_FILL_CONST;
  instr.data.construct.operands[0] = size;
  instr.data.construct.operands[1] = value;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_array_fill(MirBuilder *builder, Type *type, Ast *origin,
                                 MirValueId size, MirValueId fill_fn) {
  if (!builder || !type || !is_array_type(type) || size == MIR_NO_VALUE ||
      fill_fn == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_ARRAY_FILL;
  instr.data.construct.operands[0] = size;
  instr.data.construct.operands[1] = fill_fn;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_array_range(MirBuilder *builder, Type *type, Ast *origin,
                                  MirValueId offset, MirValueId size,
                                  MirValueId array) {
  if (!builder || !type || !is_array_type(type) || offset == MIR_NO_VALUE ||
      size == MIR_NO_VALUE || array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_ARRAY_RANGE;
  instr.data.construct.operands[0] = offset;
  instr.data.construct.operands[1] = size;
  instr.data.construct.operands[2] = array;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_array_succ(MirBuilder *builder, Type *type, Ast *origin,
                                 MirValueId array) {
  if (!builder || !type || !is_array_type(type) || array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_ARRAY_SUCC;
  instr.data.extract.value = array;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_array_offset(MirBuilder *builder, Type *type,
                                   Ast *origin, MirValueId offset,
                                   MirValueId array) {
  if (!builder || !type || !is_array_type(type) || offset == MIR_NO_VALUE ||
      array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_ARRAY_OFFSET;
  instr.data.extract.index_value = offset;
  instr.data.extract.value = array;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_value_op(MirBuilder *builder, MirOpKind kind,
                                 Type *type, Ast *origin, MirValueId value) {
  if (!builder || !type || value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, type, origin);
  instr.data.op.kind = kind;
  instr.data.op.argc = 1;
  instr.data.op.operands[0] = value;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_primitive_ordered_binop(MirBuilder *builder, Ast *app,
                                              MirCtx *ctx,
                                              MirComparisonBuiltin *ops) {
  if (!builder || !app || app->tag != AST_APPLICATION || !ctx || !ops ||
      app->data.AST_APPLICATION.len != 2) {
    return MIR_NO_VALUE;
  }

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

static MirValueId mir_primitive_eq_binop(MirBuilder *builder, Ast *app,
                                         MirCtx *ctx, bool negate) {
  if (!builder || !app || app->tag != AST_APPLICATION || !ctx ||
      app->data.AST_APPLICATION.len != 2) {
    return MIR_NO_VALUE;
  }

  Ast *lhs_ast = app->data.AST_APPLICATION.args;
  Ast *rhs_ast = app->data.AST_APPLICATION.args + 1;
  MirValueId lhs = mir_expr(builder, lhs_ast, ctx);
  MirValueId rhs = mir_expr(builder, rhs_ast, ctx);

  Type *lhs_type = mir_value_primitive_type(builder, lhs, lhs_ast->type);
  Type *rhs_type = mir_value_primitive_type(builder, rhs, rhs_ast->type);
  Type *target_type = mir_primitive_target_type(lhs_type, rhs_type, "eq");
  if (!mir_type_is_primitive_eq(target_type)) {
    return MIR_NO_VALUE;
  }

  lhs = mir_primitive_cast_if_needed(builder, lhs, lhs_type, target_type,
                                     lhs_ast);
  rhs = mir_primitive_cast_if_needed(builder, rhs, rhs_type, target_type,
                                     rhs_ast);
  if (lhs == MIR_NO_VALUE || rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueId eq = MIR_NO_VALUE;
  switch (target_type->kind) {
  case T_INT:
    eq = mir_ieq(builder, app, lhs, rhs);
    break;
  case T_UINT64:
    eq = mir_ueq(builder, app, lhs, rhs);
    break;
  case T_NUM:
    eq = mir_feq(builder, app, lhs, rhs);
    break;
  case T_CHAR:
    eq = mir_ceq(builder, app, lhs, rhs);
    break;
  case T_BOOL:
    eq = mir_beq(builder, app, lhs, rhs);
    break;
  default:
    return MIR_NO_VALUE;
  }

  return negate ? mir_lnot(builder, app, eq) : eq;
}

static MirValueId mir_short_circuit_bool(MirBuilder *builder, Ast *app,
                                         MirCtx *ctx, bool is_or) {
  if (!builder || !builder->fn || !builder->block || !app ||
      app->tag != AST_APPLICATION || app->data.AST_APPLICATION.len != 2 ||
      !ctx) {
    return MIR_NO_VALUE;
  }

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
  mir_phi_incoming_vec_push(builder->fn->arena, &incoming,
                            (MirPhiIncoming){.block = short_pred,
                                             .value = short_value});
  mir_builder_set_br(builder, continuation_block->id);

  mir_builder_position_at_end(builder, rhs_block);
  MirValueId rhs = mir_expr(builder, rhs_ast, ctx);
  if (rhs == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }
  if (builder->block && builder->block->term.kind == MIR_TERM_NONE) {
    mir_phi_incoming_vec_push(builder->fn->arena, &incoming,
                              (MirPhiIncoming){.block = builder->block->id,
                                               .value = rhs});
    mir_builder_set_br(builder, continuation_block->id);
  }

  mir_builder_position_at_end(builder, continuation_block);
  return mir_phi(builder, &t_bool, app, incoming);
}

static MirValueId mir_primitive_arithmetic_binop(MirBuilder *builder, Ast *app,
                                                 MirCtx *ctx,
                                                 MirArithmeticBuiltin *ops) {
  if (!builder || !app || app->tag != AST_APPLICATION || !ctx || !ops ||
      app->data.AST_APPLICATION.len != 2) {
    return MIR_NO_VALUE;
  }

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

static MirValueId mir_builtin_arg(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                  size_t index) {
  if (!builder || !app || app->tag != AST_APPLICATION ||
      index >= app->data.AST_APPLICATION.len) {
    return MIR_NO_VALUE;
  }
  return mir_expr(builder, app->data.AST_APPLICATION.args + index, ctx);
}

static MirValueId MirConstructorHandler(MirBuilder *builder, Ast *app,
                                        MirCtx *ctx, MirBuiltinSymbol *symbol) {
  if (!app || app->tag != AST_APPLICATION || !symbol) {
    return MIR_NO_VALUE;
  }

  return mir_constructor_call(builder, app, app->type, symbol->name,
                              app->data.AST_APPLICATION.args,
                              app->data.AST_APPLICATION.len, ctx);
}

static MirValueId MirListPrependHandler(MirBuilder *builder, Ast *app,
                                        MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!builder || !mir_builtin_arity(app, 2) || !is_list_type(app->type)) {
    return MIR_NO_VALUE;
  }

  MirValueId head = mir_builtin_arg(builder, app, ctx, 0);
  MirValueId tail = mir_builtin_arg(builder, app, ctx, 1);
  return mir_list_cons(builder, app->type, app, head, tail);
}

static MirValueId MirArithmeticHandler(MirBuilder *builder, Ast *app,
                                       MirCtx *ctx, MirBuiltinSymbol *symbol) {
  return mir_primitive_arithmetic_binop(builder, app, ctx,
                                        symbol ? symbol->data : NULL);
}

static MirValueId MirGtHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                               MirBuiltinSymbol *symbol) {
  return mir_primitive_ordered_binop(builder, app, ctx,
                                     symbol ? symbol->data : NULL);
}

static MirValueId MirGteHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  return mir_primitive_ordered_binop(builder, app, ctx,
                                     symbol ? symbol->data : NULL);
}

static MirValueId MirLtHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                               MirBuiltinSymbol *symbol) {
  return mir_primitive_ordered_binop(builder, app, ctx,
                                     symbol ? symbol->data : NULL);
}

static MirValueId MirLteHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                MirBuiltinSymbol *symbol) {
  return mir_primitive_ordered_binop(builder, app, ctx,
                                     symbol ? symbol->data : NULL);
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
  if (!builder || !mir_builtin_arity(app, 1) || !ctx) {
    return MIR_NO_VALUE;
  }

  Ast *arg = app->data.AST_APPLICATION.args;
  if (!arg->type || arg->type->kind != T_BOOL) {
    return MIR_NO_VALUE;
  }

  return mir_lnot(builder, app, mir_expr(builder, arg, ctx));
}

static MirValueId MirArraySizeHandler(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }
  return mir_array_size(builder, app, mir_builtin_arg(builder, app, ctx, 0));
}

static MirValueId MirArrayAtHandler(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                    MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }
  return mir_array_at(builder, app->type, app,
                      mir_builtin_arg(builder, app, ctx, 0),
                      mir_builtin_arg(builder, app, ctx, 1));
}

static MirValueId MirArraySetHandler(MirBuilder *builder, Ast *app,
                                     MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 3)) {
    return MIR_NO_VALUE;
  }
  return mir_array_set(builder, app->type, app,
                       mir_builtin_arg(builder, app, ctx, 0),
                       mir_builtin_arg(builder, app, ctx, 1),
                       mir_builtin_arg(builder, app, ctx, 2));
}

static MirValueId MirArrayFillConstHandler(MirBuilder *builder, Ast *app,
                                           MirCtx *ctx,
                                           MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }
  return mir_array_fill_const(builder, app->type, app,
                              mir_builtin_arg(builder, app, ctx, 0),
                              mir_builtin_arg(builder, app, ctx, 1));
}

static MirValueId MirArrayFillHandler(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }
  return mir_array_fill(builder, app->type, app,
                        mir_builtin_arg(builder, app, ctx, 0),
                        mir_builtin_arg(builder, app, ctx, 1));
}

static MirValueId MirArrayRangeHandler(MirBuilder *builder, Ast *app,
                                       MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 3)) {
    return MIR_NO_VALUE;
  }
  return mir_array_range(builder, app->type, app,
                         mir_builtin_arg(builder, app, ctx, 0),
                         mir_builtin_arg(builder, app, ctx, 1),
                         mir_builtin_arg(builder, app, ctx, 2));
}

static MirValueId MirArraySuccHandler(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 1)) {
    return MIR_NO_VALUE;
  }
  return mir_array_succ(builder, app->type, app,
                        mir_builtin_arg(builder, app, ctx, 0));
}

static MirValueId MirArrayOffsetHandler(MirBuilder *builder, Ast *app,
                                        MirCtx *ctx, MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!mir_builtin_arity(app, 2)) {
    return MIR_NO_VALUE;
  }
  return mir_array_offset(builder, app->type, app,
                          mir_builtin_arg(builder, app, ctx, 0),
                          mir_builtin_arg(builder, app, ctx, 1));
}

static MirValueId MirUnaryValueOpHandler(MirBuilder *builder, Ast *app,
                                         MirCtx *ctx,
                                         MirBuiltinSymbol *symbol) {
  if (!builder || !mir_builtin_arity(app, 1) || !ctx || !symbol ||
      !symbol->data) {
    return MIR_NO_VALUE;
  }
  MirOpKind kind = *(MirOpKind *)symbol->data;
  return mir_value_op(builder, kind, app->type, app,
                      mir_builtin_arg(builder, app, ctx, 0));
}

static bool mir_builtin_arithmetic_ops(MirBuiltinSymbol *symbol,
                                       MirPrimitiveOp *int_op,
                                       MirPrimitiveOp *uint_op,
                                       MirPrimitiveOp *float_op) {
  if (!symbol || !symbol->data || !int_op || !uint_op || !float_op) {
    return false;
  }
  MirArithmeticBuiltin *ops = symbol->data;
  if (ops->kind != MIR_BUILTIN_DATA_ARITHMETIC) {
    return false;
  }
  *int_op = ops->int_op;
  *uint_op = ops->uint_op;
  *float_op = ops->float_op;
  return true;
}

static bool mir_builtin_comparison_ops(MirBuiltinSymbol *symbol,
                                       MirComparisonBuiltin *out) {
  if (!symbol || !symbol->data || !out) {
    return false;
  }

  MirComparisonBuiltin *ops = symbol->data;
  if (ops->kind != MIR_BUILTIN_DATA_COMPARISON) {
    return false;
  }
  *out = *ops;
  return true;
}

static MirValueId mir_lower_specialized_arithmetic_call(MirBuilder *builder,
                                                        MirInstr *call) {
  if (!builder || !builder->fn || !call || !call->data.call.builtin ||
      call->data.call.operands.len != 2 ||
      !mir_type_is_primitive_numeric(call->type)) {
    return MIR_NO_VALUE;
  }

  MirPrimitiveOp int_op;
  MirPrimitiveOp uint_op;
  MirPrimitiveOp float_op;
  if (!mir_builtin_arithmetic_ops(call->data.call.builtin, &int_op, &uint_op,
                                  &float_op)) {
    return MIR_NO_VALUE;
  }

  MirValueId lhs = call->data.call.operands.items[0];
  MirValueId rhs = call->data.call.operands.items[1];
  Type *lhs_type = mir_function_value_type(builder->fn, lhs);
  Type *rhs_type = mir_function_value_type(builder->fn, rhs);
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
    return mir_primitive_instr(builder, int_op, call->type, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_UINT64:
    return mir_primitive_instr(builder, uint_op, call->type, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  case T_NUM:
    return mir_primitive_instr(builder, float_op, call->type, call->origin,
                               (MirValueId[]){lhs, rhs}, 2);
  default:
    return MIR_NO_VALUE;
  }
}

static MirValueId mir_lower_specialized_ordered_call(MirBuilder *builder,
                                                     MirInstr *call) {
  if (!builder || !builder->fn || !call || !call->data.call.builtin ||
      call->data.call.operands.len != 2) {
    return MIR_NO_VALUE;
  }

  MirComparisonBuiltin ops;
  if (!mir_builtin_comparison_ops(call->data.call.builtin, &ops)) {
    return MIR_NO_VALUE;
  }

  MirValueId lhs = call->data.call.operands.items[0];
  MirValueId rhs = call->data.call.operands.items[1];
  Type *lhs_type = mir_function_value_type(builder->fn, lhs);
  Type *rhs_type = mir_function_value_type(builder->fn, rhs);
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
  if (!builder || !builder->fn || !call || call->data.call.operands.len != 2) {
    return MIR_NO_VALUE;
  }

  MirValueId lhs = call->data.call.operands.items[0];
  MirValueId rhs = call->data.call.operands.items[1];
  Type *lhs_type = mir_function_value_type(builder->fn, lhs);
  Type *rhs_type = mir_function_value_type(builder->fn, rhs);
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

MirValueId mir_lower_specialized_builtin_call(MirBuilder *builder,
                                              MirInstr *call) {
  if (!call || !call->data.call.builtin) {
    return MIR_NO_VALUE;
  }

  MirPrimitiveOp int_op;
  MirPrimitiveOp uint_op;
  MirPrimitiveOp float_op;
  if (mir_builtin_arithmetic_ops(call->data.call.builtin, &int_op, &uint_op,
                                 &float_op)) {
    return mir_lower_specialized_arithmetic_call(builder, call);
  }

  MirComparisonBuiltin comparison_ops;
  if (mir_builtin_comparison_ops(call->data.call.builtin, &comparison_ops)) {
    return mir_lower_specialized_ordered_call(builder, call);
  }

  if (call->data.call.builtin->handler == MirEqAppHandler) {
    return mir_lower_specialized_eq_call(builder, call, false);
  }
  if (call->data.call.builtin->handler == MirNeqHandler) {
    return mir_lower_specialized_eq_call(builder, call, true);
  }
  return MIR_NO_VALUE;
}

static MirBuiltinSymbol *mir_register_builtin_env(MirProgram *program,
                                                  TypeEnv *entry,
                                                  MirBuiltinHandler handler,
                                                  void *data) {
  if (!entry) {
    return NULL;
  }
  return mir_program_register_builtin(program, entry->name, entry->type,
                                      handler, data);
}

static void mir_builtin_set_param_use(MirBuiltinSymbol *symbol, size_t index,
                                      MirOperandUse use) {
  if (!symbol || index >= symbol->summary.param_uses.len ||
      !symbol->summary.param_uses.items) {
    return;
  }
  symbol->summary.param_uses.items[index] = use;
}

static void mir_builtin_set_param_uses(MirBuiltinSymbol *symbol,
                                       const MirOperandUse *uses,
                                       size_t len) {
  if (!symbol || !uses) {
    return;
  }
  for (size_t i = 0; i < len; i++) {
    mir_builtin_set_param_use(symbol, i, uses[i]);
  }
}

static void mir_builtin_set_result(MirBuiltinSymbol *symbol,
                                   MirResultOwnership result) {
  if (!symbol) {
    return;
  }
  symbol->summary.result = result;
}

static MirBuiltinSymbol *
mir_register_builtin_env_uses(MirProgram *program, TypeEnv *entry,
                              MirBuiltinHandler handler, void *data,
                              const MirOperandUse *uses, size_t uses_len) {
  MirBuiltinSymbol *symbol =
      mir_register_builtin_env(program, entry, handler, data);
  mir_builtin_set_param_uses(symbol, uses, uses_len);
  return symbol;
}

void mir_register_core_builtins(MirProgram *program) {
  static MirArithmeticBuiltin add_ops = {
      MIR_BUILTIN_DATA_ARITHMETIC, MIR_OP_IADD, MIR_OP_UADD, MIR_OP_FADD};
  static MirArithmeticBuiltin sub_ops = {
      MIR_BUILTIN_DATA_ARITHMETIC, MIR_OP_ISUB, MIR_OP_USUB, MIR_OP_FSUB};
  static MirArithmeticBuiltin mul_ops = {
      MIR_BUILTIN_DATA_ARITHMETIC, MIR_OP_IMUL, MIR_OP_UMUL, MIR_OP_FMUL};
  static MirArithmeticBuiltin div_ops = {
      MIR_BUILTIN_DATA_ARITHMETIC, MIR_OP_IDIV, MIR_OP_UDIV, MIR_OP_FDIV};
  static MirArithmeticBuiltin mod_ops = {
      MIR_BUILTIN_DATA_ARITHMETIC, MIR_OP_IMOD, MIR_OP_UMOD, MIR_OP_FMOD};
  static MirComparisonBuiltin gt_ops = {
      MIR_BUILTIN_DATA_COMPARISON, MIR_OP_IGT, MIR_OP_UGT, MIR_OP_FGT,
      MIR_OP_CGT};
  static MirComparisonBuiltin gte_ops = {
      MIR_BUILTIN_DATA_COMPARISON, MIR_OP_IGTE, MIR_OP_UGTE, MIR_OP_FGTE,
      MIR_OP_CGTE};
  static MirComparisonBuiltin lt_ops = {
      MIR_BUILTIN_DATA_COMPARISON, MIR_OP_ILT, MIR_OP_ULT, MIR_OP_FLT,
      MIR_OP_CLT};
  static MirComparisonBuiltin lte_ops = {
      MIR_BUILTIN_DATA_COMPARISON, MIR_OP_ILTE, MIR_OP_ULTE, MIR_OP_FLTE,
      MIR_OP_CLTE};
  static MirOpKind str_op = MIR_OP_KIND_STR;
  static MirOpKind print_op = MIR_OP_KIND_PRINT;
  static MirOpKind cstr_op = MIR_OP_KIND_CSTR;
  static MirOpKind sizeof_op = MIR_OP_KIND_SIZEOF;
  static MirOpKind dlopen_op = MIR_OP_KIND_DLOPEN;
  static MirOpKind asbytes_op = MIR_OP_KIND_AS_BYTES;
  static MirOpKind typeof_op = MIR_OP_KIND_TYPEOF;
  static const MirOperandUse borrow1[] = {MIR_OPERAND_USE_BORROW};
  static const MirOperandUse borrow2[] = {MIR_OPERAND_USE_BORROW,
                                          MIR_OPERAND_USE_BORROW};
  static const MirOperandUse borrow3[] = {
      MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW,
      MIR_OPERAND_USE_BORROW};
  static const MirOperandUse borrow_borrow_consume[] = {
      MIR_OPERAND_USE_BORROW, MIR_OPERAND_USE_BORROW,
      MIR_OPERAND_USE_CONSUME};
  static const MirOperandUse borrow_consume[] = {MIR_OPERAND_USE_BORROW,
                                                MIR_OPERAND_USE_CONSUME};

  mir_register_builtin_env_uses(program, builtin_envs.arith_add,
                                MirArithmeticHandler, &add_ops, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.arith_sub,
                                MirArithmeticHandler, &sub_ops, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.arith_mul,
                                MirArithmeticHandler, &mul_ops, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.arith_div,
                                MirArithmeticHandler, &div_ops, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.arith_mod,
                                MirArithmeticHandler, &mod_ops, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.gt, MirGtHandler, &gt_ops,
                                borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.gte, MirGteHandler,
                                &gte_ops, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.lt, MirLtHandler, &lt_ops,
                                borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.lte, MirLteHandler,
                                &lte_ops, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.eq, MirEqAppHandler, NULL,
                                borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.neq, MirNeqHandler, NULL,
                                borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.logical_and,
                                MirLogicalAndHandler, NULL, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.logical_or,
                                MirLogicalOrHandler, NULL, borrow2, 2);
  mir_register_builtin_env_uses(program, builtin_envs.logical_not,
                                MirLogicalNotHandler, NULL, borrow1, 1);
  mir_register_builtin_env(program, builtin_envs.some, MirConstructorHandler,
                           NULL);
  mir_register_builtin_env(program, builtin_envs.list_prepend,
                           MirListPrependHandler, NULL);
  mir_register_builtin_env_uses(program, builtin_envs.print,
                                MirUnaryValueOpHandler, &print_op, borrow1, 1);
  mir_register_builtin_env_uses(program, builtin_envs.array_size,
                                MirArraySizeHandler, NULL, borrow1, 1);
  MirBuiltinSymbol *array_at = mir_register_builtin_env_uses(
      program, builtin_envs.array_at, MirArrayAtHandler, NULL, borrow2, 2);
  mir_builtin_set_result(array_at, MIR_RESULT_BORROWED);
  MirBuiltinSymbol *array_set = mir_register_builtin_env_uses(
      program, builtin_envs.array_set, MirArraySetHandler, NULL,
      borrow_borrow_consume, 3);
  mir_builtin_set_result(array_set, MIR_RESULT_BORROWED);
  mir_register_builtin_env_uses(program, builtin_envs.array_fill_const,
                                MirArrayFillConstHandler, NULL,
                                borrow_consume, 2);
  MirBuiltinSymbol *array_succ = mir_register_builtin_env_uses(
      program, builtin_envs.array_succ, MirArraySuccHandler, NULL, borrow1, 1);
  mir_builtin_set_result(array_succ, MIR_RESULT_BORROWED);
  mir_register_builtin_env_uses(program, builtin_envs.array_fill,
                                MirArrayFillHandler, NULL, borrow2, 2);
  MirBuiltinSymbol *array_range = mir_register_builtin_env_uses(
      program, builtin_envs.array_range, MirArrayRangeHandler, NULL, borrow3,
      3);
  mir_builtin_set_result(array_range, MIR_RESULT_BORROWED);
  MirBuiltinSymbol *array_offset = mir_register_builtin_env_uses(
      program, builtin_envs.array_offset, MirArrayOffsetHandler, NULL, borrow2,
      2);
  mir_builtin_set_result(array_offset, MIR_RESULT_BORROWED);
  mir_register_builtin_env_uses(program, builtin_envs.str,
                                MirUnaryValueOpHandler, &str_op, borrow1, 1);
  mir_register_builtin_env_uses(program, builtin_envs.cstr,
                                MirUnaryValueOpHandler, &cstr_op, borrow1, 1);
  mir_register_builtin_env_uses(program, builtin_envs.sizeof_env,
                                MirUnaryValueOpHandler, &sizeof_op, borrow1,
                                1);
  mir_register_builtin_env_uses(program, builtin_envs.dlopen_env,
                                MirUnaryValueOpHandler, &dlopen_op, borrow1,
                                1);
  mir_register_builtin_env_uses(program, builtin_envs.asbytes,
                                MirUnaryValueOpHandler, &asbytes_op, borrow1,
                                1);
  mir_register_builtin_env_uses(program, builtin_envs.typeof_env,
                                MirUnaryValueOpHandler, &typeof_op, borrow1,
                                1);
}
