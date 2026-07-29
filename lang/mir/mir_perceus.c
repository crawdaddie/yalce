#include "./mir.h"
#include "escape_analysis.h"
#include "types/builtins.h"
#include "types/type.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  size_t block_count;
  size_t value_count;
  bool *use;
  bool *def;
  bool *live_in;
  bool *live_out;
  bool *edge_use_out;
  MirValueId *borrow_parent;
} MirPerceusLiveness;

static bool mir_perceus_value_in_range(MirFunction *fn, MirValueId value,
                                       size_t limit) {
  return fn && value != MIR_NO_VALUE && value < fn->values.len && value < limit;
}

static bool *mir_perceus_block_set(bool *sets, size_t value_count,
                                   MirBlockId block) {
  if (!sets || block == MIR_NO_BLOCK) {
    return NULL;
  }
  return sets + ((size_t)block * value_count);
}

static bool mir_perceus_set_contains(const bool *set, size_t value_count,
                                     MirValueId value) {
  return set && value != MIR_NO_VALUE && value < value_count && set[value];
}

static bool mir_perceus_set_mark(bool *set, size_t value_count,
                                 MirValueId value) {
  if (!set || value == MIR_NO_VALUE || value >= value_count || set[value]) {
    return false;
  }
  set[value] = true;
  return true;
}

static size_t mir_perceus_term_successors(MirTerminator *term,
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

static bool mir_perceus_is_managed_type(Type *type) {
  return type && (is_array_type(type) || is_list_type(type) ||
                  is_string_type(type) || is_closure(type) ||
                  is_coroutine_type(type));
}

static bool mir_perceus_is_stack_alloc(MirFunction *fn, MirValueId value) {
  EscapeMeta *meta = mir_value_escape_meta(fn, value);
  return meta && meta->status == EA_STACK_ALLOC;
}

static bool mir_perceus_is_heap_alloc(MirFunction *fn, MirValueId value) {
  EscapeMeta *meta = mir_value_escape_meta(fn, value);
  return meta && meta->status == EA_HEAP_ALLOC;
}

static bool mir_perceus_manages_value(MirFunction *fn, MirValueId value,
                                      size_t value_limit) {
  if (!mir_perceus_value_in_range(fn, value, value_limit) ||
      mir_perceus_is_stack_alloc(fn, value)) {
    return false;
  }

  if (mir_perceus_is_heap_alloc(fn, value)) {
    return true;
  }

  return mir_perceus_is_managed_type(mir_function_value_type(fn, value));
}

static bool mir_perceus_has_stack_borrow_parent(
    MirFunction *fn, MirValueId value, size_t value_limit,
    const MirValueId *borrow_parent) {
  for (size_t depth = 0; borrow_parent &&
                         mir_perceus_value_in_range(fn, value, value_limit) &&
                         depth < value_limit;
       depth++) {
    MirValueId parent = borrow_parent[value];
    if (!mir_perceus_value_in_range(fn, parent, value_limit)) {
      return false;
    }
    if (mir_perceus_is_stack_alloc(fn, parent)) {
      return true;
    }
    value = parent;
  }
  return false;
}

static bool mir_perceus_manages_value_with_borrow_parent(
    MirFunction *fn, MirValueId value, size_t value_limit,
    const MirValueId *borrow_parent) {
  if (mir_perceus_has_stack_borrow_parent(fn, value, value_limit,
                                          borrow_parent)) {
    return false;
  }
  return mir_perceus_manages_value(fn, value, value_limit);
}

static bool mir_perceus_value_may_own_with_borrow_parent(
    MirFunction *fn, MirValueId value, size_t value_limit,
    const MirValueId *borrow_parent) {
  if (!mir_perceus_manages_value_with_borrow_parent(fn, value, value_limit,
                                                    borrow_parent)) {
    return false;
  }

  for (size_t i = 0; fn && i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (param->value == value) {
      return mir_function_param_use(fn, i) == MIR_OPERAND_USE_CONSUME;
    }
  }

  return true;
}

static MirValueId mir_perceus_borrow_parent_for_instr(MirInstr *instr) {
  if (!instr) {
    return MIR_NO_VALUE;
  }

  switch (instr->kind) {
  case MIR_CONSTRUCT:
    if (instr->data.construct.kind == MIR_CONSTRUCT_TUPLE &&
        is_array_type(instr->type) && instr->data.construct.items.len > 1) {
      return instr->data.construct.items
          .items[instr->data.construct.items.len - 1];
    }
    return MIR_NO_VALUE;
  case MIR_EXTRACT:
    return instr->data.extract.kind == MIR_EXTRACT_VARIANT_TAG
               ? MIR_NO_VALUE
               : instr->data.extract.value;
  case MIR_OP:
    switch (instr->data.op.kind) {
    case MIR_OP_KIND_ARRAY_SET:
    case MIR_OP_KIND_PTR_OFFSET:
    case MIR_OP_KIND_LOAD:
      return instr->data.op.operands[0];
    default:
      return MIR_NO_VALUE;
    }
  default:
    return MIR_NO_VALUE;
  }
}

static void mir_perceus_count_use(MirFunction *fn, uint32_t *uses,
                                  const MirValueId *borrow_parent,
                                  size_t value_limit, MirValueId value) {
  if (!fn || !uses || value == MIR_NO_VALUE || value >= fn->values.len) {
    return;
  }
  if (value >= value_limit) {
    return;
  }
  uses[value]++;

  for (size_t depth = 0; borrow_parent && value < value_limit &&
                         depth < value_limit;
       depth++) {
    MirValueId parent = borrow_parent[value];
    if (parent == MIR_NO_VALUE || parent >= value_limit) {
      return;
    }
    uses[parent]++;
    value = parent;
  }
}

typedef struct {
  MirFunction *fn;
  uint32_t *uses;
  const MirValueId *borrow_parent;
  size_t value_limit;
} MirPerceusCountCtx;

static bool mir_perceus_count_operand(MirInstr *instr, MirOperand operand,
                                      void *ctx) {
  (void)instr;

  MirPerceusCountCtx *count_ctx = ctx;
  if (!count_ctx) {
    return true;
  }

  mir_perceus_count_use(count_ctx->fn, count_ctx->uses,
                        count_ctx->borrow_parent, count_ctx->value_limit,
                        operand.value);
  return true;
}

static void mir_perceus_count_instr_uses(MirFunction *fn, uint32_t *uses,
                                         const MirValueId *borrow_parent,
                                         size_t value_limit, MirInstr *instr) {
  if (!fn || !uses || !instr) {
    return;
  }
  if (instr->kind == MIR_PHI) {
    return;
  }

  MirPerceusCountCtx ctx = {
      .fn = fn,
      .uses = uses,
      .borrow_parent = borrow_parent,
      .value_limit = value_limit,
  };
  mir_instr_for_each_operand(instr, mir_perceus_count_operand, &ctx);
}

static void mir_perceus_count_term_uses(MirFunction *fn, uint32_t *uses,
                                        const MirValueId *borrow_parent,
                                        size_t value_limit,
                                        MirTerminator *term) {
  if (!fn || !uses || !term) {
    return;
  }

  MirPerceusCountCtx ctx = {
      .fn = fn,
      .uses = uses,
      .borrow_parent = borrow_parent,
      .value_limit = value_limit,
  };
  mir_term_for_each_operand(term, mir_perceus_count_operand, &ctx);
}

typedef struct {
  MirFunction *fn;
  MirPerceusLiveness *liveness;
  MirBlockId block;
} MirPerceusUseDefCtx;

static void mir_perceus_note_value_use(MirFunction *fn,
                                       MirPerceusLiveness *liveness,
                                       MirBlockId block, MirValueId value) {
  if (!liveness || !mir_perceus_value_in_range(fn, value,
                                               liveness->value_count)) {
    return;
  }

  for (size_t depth = 0; value != MIR_NO_VALUE &&
                         value < liveness->value_count &&
                         depth < liveness->value_count;
       depth++) {
    bool *defs =
        mir_perceus_block_set(liveness->def, liveness->value_count, block);
    bool *uses =
        mir_perceus_block_set(liveness->use, liveness->value_count, block);
    if (!mir_perceus_set_contains(defs, liveness->value_count, value)) {
      mir_perceus_set_mark(uses, liveness->value_count, value);
    }

    if (!liveness->borrow_parent) {
      break;
    }
    value = liveness->borrow_parent[value];
  }
}

static bool mir_perceus_note_block_use(MirInstr *instr, MirOperand operand,
                                       void *ctx) {
  (void)instr;

  MirPerceusUseDefCtx *use_ctx = ctx;
  if (!use_ctx) {
    return true;
  }

  mir_perceus_note_value_use(use_ctx->fn, use_ctx->liveness, use_ctx->block,
                             operand.value);
  return true;
}

static void mir_perceus_note_phi_edge_use(MirFunction *fn,
                                          MirPerceusLiveness *liveness,
                                          MirPhiIncoming incoming) {
  if (!liveness || incoming.block == MIR_NO_BLOCK ||
      incoming.block >= liveness->block_count ||
      !mir_perceus_value_in_range(fn, incoming.value, liveness->value_count)) {
    return;
  }

  MirValueId value = incoming.value;
  for (size_t depth = 0; value != MIR_NO_VALUE &&
                         value < liveness->value_count &&
                         depth < liveness->value_count;
       depth++) {
    bool *edge_uses = mir_perceus_block_set(
        liveness->edge_use_out, liveness->value_count, incoming.block);
    mir_perceus_set_mark(edge_uses, liveness->value_count, value);

    if (!liveness->borrow_parent) {
      break;
    }
    value = liveness->borrow_parent[value];
  }
}

static void mir_perceus_note_block_def(MirFunction *fn,
                                       MirPerceusLiveness *liveness,
                                       MirBlockId block, MirValueId value) {
  if (!liveness || !mir_perceus_value_in_range(fn, value,
                                               liveness->value_count) ||
      block == MIR_NO_BLOCK || block >= liveness->block_count) {
    return;
  }

  bool *defs = mir_perceus_block_set(liveness->def, liveness->value_count,
                                     block);
  mir_perceus_set_mark(defs, liveness->value_count, value);
}

static void mir_perceus_build_use_def(MirFunction *fn,
                                      MirPerceusLiveness *liveness) {
  if (!fn || !liveness) {
    return;
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block || block->id >= liveness->block_count) {
      continue;
    }

    MirPerceusUseDefCtx ctx = {
        .fn = fn,
        .liveness = liveness,
        .block = block->id,
    };

    for (size_t j = 0; j < block->instrs.len; j++) {
      MirInstr *instr = &block->instrs.items[j];
      if (instr->kind == MIR_PHI) {
        for (size_t k = 0; k < instr->data.phi.incoming.len; k++) {
          mir_perceus_note_phi_edge_use(fn, liveness,
                                        instr->data.phi.incoming.items[k]);
        }
        mir_perceus_note_block_def(fn, liveness, block->id, instr->result);
        continue;
      }

      mir_instr_for_each_operand(instr, mir_perceus_note_block_use, &ctx);

      mir_perceus_note_block_def(fn, liveness, block->id, instr->result);
    }

    mir_term_for_each_operand(&block->term, mir_perceus_note_block_use, &ctx);
  }
}

static void mir_perceus_build_borrow_parents(MirFunction *fn,
                                             MirPerceusLiveness *liveness) {
  if (!fn || !liveness || !liveness->borrow_parent) {
    return;
  }

  for (size_t i = 0; i < liveness->value_count; i++) {
    liveness->borrow_parent[i] = MIR_NO_VALUE;
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }
    for (size_t j = 0; j < block->instrs.len; j++) {
      MirInstr *instr = &block->instrs.items[j];
      MirValueId parent = mir_perceus_borrow_parent_for_instr(instr);
      if (mir_perceus_value_in_range(fn, instr->result,
                                     liveness->value_count) &&
          mir_perceus_value_in_range(fn, parent, liveness->value_count)) {
        liveness->borrow_parent[instr->result] = parent;
      }
    }
  }
}

static bool mir_perceus_liveness_solve(MirFunction *fn,
                                       MirPerceusLiveness *liveness) {
  if (!fn || !liveness) {
    return false;
  }

  bool changed = false;
  do {
    changed = false;
    for (size_t bi = liveness->block_count; bi > 0; bi--) {
      size_t i = bi - 1;
      MirBlock *block = i < fn->blocks.len ? fn->blocks.items[i] : NULL;
      if (!block || block->id >= liveness->block_count) {
        continue;
      }

      bool *live_out = mir_perceus_block_set(
          liveness->live_out, liveness->value_count, block->id);
      bool *live_in = mir_perceus_block_set(liveness->live_in,
                                            liveness->value_count, block->id);
      bool *uses = mir_perceus_block_set(liveness->use,
                                         liveness->value_count, block->id);
      bool *defs = mir_perceus_block_set(liveness->def,
                                         liveness->value_count, block->id);
      bool *edge_uses = mir_perceus_block_set(
          liveness->edge_use_out, liveness->value_count, block->id);

      MirBlockId successors[2] = {MIR_NO_BLOCK, MIR_NO_BLOCK};
      size_t successors_len =
          mir_perceus_term_successors(&block->term, successors);

      for (size_t value = 0; value < liveness->value_count; value++) {
        bool out = edge_uses && edge_uses[value];
        for (size_t s = 0; s < successors_len; s++) {
          MirBlockId succ = successors[s];
          if (succ == MIR_NO_BLOCK || succ >= liveness->block_count) {
            continue;
          }
          bool *succ_live_in = mir_perceus_block_set(
              liveness->live_in, liveness->value_count, succ);
          out = out || succ_live_in[value];
        }

        bool in = uses[value] || (out && !defs[value]);
        if (live_out[value] != out) {
          live_out[value] = out;
          changed = true;
        }
        if (live_in[value] != in) {
          live_in[value] = in;
          changed = true;
        }
      }
    }
  } while (changed);

  return true;
}

static bool mir_perceus_liveness_init(MirFunction *fn,
                                      MirPerceusLiveness *liveness,
                                      size_t value_count) {
  if (!fn || !liveness) {
    return false;
  }

  *liveness = (MirPerceusLiveness){
      .block_count = fn->blocks.len,
      .value_count = value_count,
      .use = calloc(fn->blocks.len * value_count, sizeof(bool)),
      .def = calloc(fn->blocks.len * value_count, sizeof(bool)),
      .live_in = calloc(fn->blocks.len * value_count, sizeof(bool)),
      .live_out = calloc(fn->blocks.len * value_count, sizeof(bool)),
      .edge_use_out = calloc(fn->blocks.len * value_count, sizeof(bool)),
      .borrow_parent = calloc(value_count, sizeof(MirValueId)),
  };

  if (!liveness->use || !liveness->def || !liveness->live_in ||
      !liveness->live_out || !liveness->edge_use_out ||
      !liveness->borrow_parent) {
    free(liveness->use);
    free(liveness->def);
    free(liveness->live_in);
    free(liveness->live_out);
    free(liveness->edge_use_out);
    free(liveness->borrow_parent);
    *liveness = (MirPerceusLiveness){0};
    return false;
  }

  mir_perceus_build_borrow_parents(fn, liveness);
  mir_perceus_build_use_def(fn, liveness);
  return mir_perceus_liveness_solve(fn, liveness);
}

static void mir_perceus_liveness_destroy(MirPerceusLiveness *liveness) {
  if (!liveness) {
    return;
  }

  free(liveness->use);
  free(liveness->def);
  free(liveness->live_in);
  free(liveness->live_out);
  free(liveness->edge_use_out);
  free(liveness->borrow_parent);
  *liveness = (MirPerceusLiveness){0};
}

static void mir_perceus_emit_marker(MirFunction *fn, MirInstrVec *out,
                                    MirOpKind kind, MirValueId value,
                                    Ast *origin) {
  if (!fn || !out || value == MIR_NO_VALUE) {
    return;
  }

  MirInstr marker = mir_make_instr(MIR_OP, &t_void, origin);
  marker.data.op.kind = kind;
  marker.data.op.argc = 1;
  marker.data.op.operands[0] = value;
  marker.result = mir_function_add_value(fn, &t_void, origin);
  mir_instr_vec_push(fn->arena, out, marker);
}

static Type *mir_perceus_array_element_type(MirFunction *fn,
                                            MirValueId array) {
  Type *array_type = mir_function_value_type(fn, array);
  if (!array_type || !is_array_type(array_type) ||
      !array_type->data.T_CONS.args || array_type->data.T_CONS.num_args < 1) {
    return NULL;
  }
  return array_type->data.T_CONS.args[0];
}

static MirValueId mir_perceus_emit_array_set_old_slot_load(
    MirFunction *fn, MirInstrVec *out, MirInstr *instr) {
  if (!fn || !out || !instr || instr->kind != MIR_OP ||
      instr->data.op.kind != MIR_OP_KIND_ARRAY_SET) {
    return MIR_NO_VALUE;
  }

  Type *element_type =
      mir_perceus_array_element_type(fn, instr->data.op.operands[0]);
  if (!mir_perceus_is_managed_type(element_type)) {
    return MIR_NO_VALUE;
  }

  MirInstr old_slot = mir_make_instr(MIR_EXTRACT, element_type, instr->origin);
  old_slot.data.extract.kind = MIR_EXTRACT_ARRAY_AT;
  old_slot.data.extract.value = instr->data.op.operands[0];
  old_slot.data.extract.index_value = instr->data.op.operands[1];
  old_slot.result = mir_function_add_value(fn, element_type, instr->origin);
  mir_instr_vec_push(fn->arena, out, old_slot);
  return old_slot.result;
}

static Type *mir_perceus_pointer_pointee_type(MirFunction *fn,
                                              MirValueId ptr) {
  Type *ptr_type = mir_function_value_type(fn, ptr);
  if (!ptr_type || !is_pointer_type(ptr_type) || !ptr_type->data.T_CONS.args ||
      ptr_type->data.T_CONS.num_args < 1) {
    return NULL;
  }
  return ptr_type->data.T_CONS.args[0];
}

static MirValueId mir_perceus_emit_store_old_slot_load(MirFunction *fn,
                                                       MirInstrVec *out,
                                                       MirInstr *instr) {
  if (!fn || !out || !instr || instr->kind != MIR_OP ||
      instr->data.op.kind != MIR_OP_KIND_STORE || instr->data.op.argc < 2) {
    return MIR_NO_VALUE;
  }

  Type *element_type =
      mir_perceus_pointer_pointee_type(fn, instr->data.op.operands[0]);
  if (!mir_perceus_is_managed_type(element_type)) {
    return MIR_NO_VALUE;
  }

  MirInstr old_slot = mir_make_instr(MIR_OP, element_type, instr->origin);
  old_slot.data.op.kind = MIR_OP_KIND_LOAD_OWNED;
  old_slot.data.op.argc = 1;
  old_slot.data.op.operands[0] = instr->data.op.operands[0];
  old_slot.result = mir_function_add_value(fn, element_type, instr->origin);
  mir_instr_vec_push(fn->arena, out, old_slot);
  return old_slot.result;
}

static void mir_perceus_process_use(MirFunction *fn, MirInstrVec *out,
                                    uint32_t *remaining, size_t value_limit,
                                    const MirValueId *borrow_parent,
                                    const bool *live_out, bool *moved,
                                    MirValueIdVec *post_releases,
                                    MirValueId value, bool consumes,
                                    Ast *origin) {
  if (!mir_perceus_value_in_range(fn, value, value_limit) || !remaining) {
    return;
  }

  bool managed = mir_perceus_manages_value_with_borrow_parent(
      fn, value, value_limit, borrow_parent);
  bool live_after_block =
      mir_perceus_set_contains(live_out, value_limit, value);
  bool has_future_use = remaining[value] > 1 || live_after_block;
  if (managed && consumes && has_future_use) {
    mir_perceus_emit_marker(fn, out, MIR_OP_KIND_DUP, value, origin);
  }

  if (remaining[value] > 0) {
    remaining[value]--;
  }

  /* A consumed operand whose last use is this consume transfers ownership to
     the callee (the call drops it on its side). The caller must not also drop
     it; mark it moved so dead-definition and entry-param drops skip it. A
     dup'd consume keeps the original ref, so it is not moved. */
  if (managed && consumes && !has_future_use && moved) {
    moved[value] = true;
  }

  if (managed && !consumes && remaining[value] == 0 && !live_after_block &&
      mir_perceus_value_may_own_with_borrow_parent(fn, value, value_limit,
                                                   borrow_parent)) {
    mir_value_id_vec_push(fn->arena, post_releases, value);
    /* A post-release drop owns this value's last reference; mark it moved so
       the later dead-definition and unused-entry-param drops do not drop it
       again (which would be a double-free under real RC). */
    if (moved) {
      moved[value] = true;
    }
  }
}

static bool mir_perceus_extraction_borrows_result(MirInstr *instr) {
  if (!instr) {
    return false;
  }

  switch (instr->kind) {
  case MIR_CONSTRUCT:
    return instr->data.construct.kind == MIR_CONSTRUCT_TUPLE &&
           is_array_type(instr->type);
  case MIR_EXTRACT:
    return instr->data.extract.kind != MIR_EXTRACT_VARIANT_TAG;
  case MIR_OP:
    return instr->data.op.kind == MIR_OP_KIND_ARRAY_SET ||
           instr->data.op.kind == MIR_OP_KIND_LOAD;
  default:
    return false;
  }
}

typedef struct {
  MirFunction *fn;
  MirInstrVec *out;
  uint32_t *remaining;
  size_t value_limit;
  const bool *live_out;
  const MirValueId *borrow_parent;
  bool *moved;
  MirValueIdVec *post_releases;
} MirPerceusProcessCtx;

static bool mir_perceus_process_operand(MirInstr *instr, MirOperand operand,
                                        void *ctx) {
  MirPerceusProcessCtx *process_ctx = ctx;
  if (!process_ctx) {
    return true;
  }

  mir_perceus_process_use(
      process_ctx->fn, process_ctx->out, process_ctx->remaining,
      process_ctx->value_limit, process_ctx->borrow_parent,
      process_ctx->live_out, process_ctx->moved,
      process_ctx->post_releases, operand.value,
      operand.use == MIR_OPERAND_USE_CONSUME, instr ? instr->origin : NULL);

  MirValueId value = operand.value;
  for (size_t depth = 0; process_ctx->borrow_parent &&
                         value != MIR_NO_VALUE &&
                         value < process_ctx->value_limit &&
                         depth < process_ctx->value_limit;
       depth++) {
    MirValueId parent = process_ctx->borrow_parent[value];
    if (parent == MIR_NO_VALUE || parent >= process_ctx->value_limit) {
      break;
    }
    mir_perceus_process_use(process_ctx->fn, process_ctx->out,
                            process_ctx->remaining, process_ctx->value_limit,
                            process_ctx->borrow_parent, process_ctx->live_out,
                            process_ctx->moved, process_ctx->post_releases,
                            parent, false, instr ? instr->origin : NULL);
    value = parent;
  }
  return true;
}

static void mir_perceus_process_instr_uses(MirFunction *fn, MirInstrVec *out,
                                           uint32_t *remaining,
                                           size_t value_limit,
                                           const bool *live_out,
                                           const MirValueId *borrow_parent,
                                           bool *moved,
                                           MirValueIdVec *post_releases,
                                           MirInstr *instr) {
  if (!fn || !out || !remaining || !instr) {
    return;
  }
  if (instr->kind == MIR_PHI) {
    return;
  }

  MirPerceusProcessCtx ctx = {
      .fn = fn,
      .out = out,
      .remaining = remaining,
      .value_limit = value_limit,
      .live_out = live_out,
      .borrow_parent = borrow_parent,
      .moved = moved,
      .post_releases = post_releases,
  };
  mir_instr_for_each_operand(instr, mir_perceus_process_operand, &ctx);
}

static void mir_perceus_emit_post_releases(MirFunction *fn, MirInstrVec *out,
                                           MirValueIdVec *post_releases,
                                           Ast *origin) {
  if (!post_releases) {
    return;
  }
  for (size_t i = 0; i < post_releases->len; i++) {
    mir_perceus_emit_marker(fn, out, MIR_OP_KIND_DROP,
                            post_releases->items[i], origin);
  }
}

static void mir_perceus_own_extracted_result(
    MirFunction *fn, MirInstrVec *out, MirInstr *instr, uint32_t *remaining,
    size_t value_limit, const MirValueId *borrow_parent,
    const bool *live_out) {
  if (!instr || !mir_perceus_extraction_borrows_result(instr) ||
      !mir_perceus_manages_value_with_borrow_parent(
          fn, instr->result, value_limit, borrow_parent)) {
    return;
  }

  if ((!remaining || remaining[instr->result] == 0) &&
      !mir_perceus_set_contains(live_out, value_limit, instr->result)) {
    return;
  }

  mir_perceus_emit_marker(fn, out, MIR_OP_KIND_DUP, instr->result,
                          instr->origin);
}

static void mir_perceus_drop_dead_definition(MirFunction *fn, MirInstrVec *out,
                                             MirInstr *instr,
                                             uint32_t *remaining,
                                             size_t value_limit,
                                             const MirValueId *borrow_parent,
                                             const bool *live_out,
                                             const bool *moved) {
  if (!instr ||
      mir_perceus_extraction_borrows_result(instr) ||
      !mir_perceus_value_in_range(fn, instr->result, value_limit) ||
      !remaining || remaining[instr->result] > 0 ||
      mir_perceus_set_contains(live_out, value_limit, instr->result) ||
      (moved && moved[instr->result]) ||
      !mir_perceus_value_may_own_with_borrow_parent(
          fn, instr->result, value_limit, borrow_parent)) {
    return;
  }

  mir_perceus_emit_marker(fn, out, MIR_OP_KIND_DROP, instr->result,
                          instr->origin);
}

static void mir_perceus_drop_unused_entry_params(MirFunction *fn,
                                                 MirBlock *block,
                                                 MirInstrVec *out,
                                                 uint32_t *remaining,
                                                 size_t value_limit,
                                                 const MirValueId *borrow_parent,
                                                 const bool *live_out,
                                                 const bool *moved) {
  if (!fn || !block || block->id != 0 || !out || !remaining) {
    return;
  }

  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (!mir_perceus_value_in_range(fn, param->value, value_limit) ||
        remaining[param->value] > 0 ||
        mir_perceus_set_contains(live_out, value_limit, param->value) ||
        (moved && moved[param->value]) ||
        !mir_perceus_value_may_own_with_borrow_parent(
            fn, param->value, value_limit, borrow_parent)) {
      continue;
    }
    mir_perceus_emit_marker(fn, out, MIR_OP_KIND_DROP, param->value,
                            param->origin);
  }
}

static bool mir_perceus_edge_value_used_by_phi(
    MirFunction *fn, MirBlockId pred, MirBlockId succ, MirValueId value,
    size_t value_limit, const MirValueId *borrow_parent) {
  if (!fn || pred == MIR_NO_BLOCK || succ == MIR_NO_BLOCK ||
      succ >= fn->blocks.len || value == MIR_NO_VALUE || value >= value_limit) {
    return false;
  }

  MirBlock *succ_block = fn->blocks.items[succ];
  if (!succ_block) {
    return false;
  }

  for (size_t i = 0; i < succ_block->instrs.len; i++) {
    MirInstr *instr = &succ_block->instrs.items[i];
    if (instr->kind != MIR_PHI) {
      continue;
    }

    for (size_t j = 0; j < instr->data.phi.incoming.len; j++) {
      MirPhiIncoming incoming = instr->data.phi.incoming.items[j];
      if (incoming.block != pred) {
        continue;
      }

      MirValueId incoming_value = incoming.value;
      for (size_t depth = 0; incoming_value != MIR_NO_VALUE &&
                             incoming_value < value_limit &&
                             depth < value_limit;
           depth++) {
        if (incoming_value == value) {
          return true;
        }
        if (!borrow_parent) {
          break;
        }
        incoming_value = borrow_parent[incoming_value];
      }
    }
  }

  return false;
}

static MirBlockId mir_perceus_split_edge_with_drops(
    MirFunction *fn, MirPerceusLiveness *liveness, MirBlockId pred,
    MirBlockId succ) {
  if (!fn || !liveness || pred == MIR_NO_BLOCK || succ == MIR_NO_BLOCK ||
      pred >= liveness->block_count || succ >= liveness->block_count) {
    return succ;
  }

  const bool *pred_live_out =
      mir_perceus_block_set(liveness->live_out, liveness->value_count, pred);
  const bool *succ_live_in =
      mir_perceus_block_set(liveness->live_in, liveness->value_count, succ);
  if (!pred_live_out || !succ_live_in) {
    return succ;
  }

  MirValueIdVec drops = {0};
  for (MirValueId value = 0; value < liveness->value_count; value++) {
    if (!pred_live_out[value] || succ_live_in[value] ||
        mir_perceus_edge_value_used_by_phi(
            fn, pred, succ, value, liveness->value_count,
            liveness->borrow_parent) ||
        !mir_perceus_value_may_own_with_borrow_parent(
            fn, value, liveness->value_count, liveness->borrow_parent)) {
      continue;
    }
    mir_value_id_vec_push(fn->arena, &drops, value);
  }

  if (drops.len == 0) {
    return succ;
  }

  MirBlock *drop_block = mir_function_add_block(fn, "perceus.edge.drop");
  if (!drop_block) {
    return succ;
  }

  for (size_t i = 0; i < drops.len; i++) {
    mir_perceus_emit_marker(fn, &drop_block->instrs, MIR_OP_KIND_DROP,
                            drops.items[i], NULL);
  }
  drop_block->term = (MirTerminator){.kind = MIR_TERM_BR,
                                     .value = MIR_NO_VALUE,
                                     .cond = MIR_NO_VALUE,
                                     .target = succ,
                                     .then_block = MIR_NO_BLOCK,
                                     .else_block = MIR_NO_BLOCK};
  return drop_block->id;
}

static void mir_perceus_insert_edge_drops(MirFunction *fn,
                                          MirPerceusLiveness *liveness,
                                          MirBlock *block) {
  if (!fn || !liveness || !block || block->id >= liveness->block_count ||
      block->term.kind != MIR_TERM_COND) {
    return;
  }

  block->term.then_block = mir_perceus_split_edge_with_drops(
      fn, liveness, block->id, block->term.then_block);
  block->term.else_block = mir_perceus_split_edge_with_drops(
      fn, liveness, block->id, block->term.else_block);
}

static void mir_perceus_process_term_uses(MirFunction *fn, MirInstrVec *out,
                                          uint32_t *remaining,
                                          size_t value_limit,
                                          const bool *live_out,
                                          const MirValueId *borrow_parent,
                                          bool *moved, MirTerminator *term) {
  MirValueIdVec post_releases = {0};
  MirPerceusProcessCtx ctx = {
      .fn = fn,
      .out = out,
      .remaining = remaining,
      .value_limit = value_limit,
      .live_out = live_out,
      .borrow_parent = borrow_parent,
      .moved = moved,
      .post_releases = &post_releases,
  };
  mir_term_for_each_operand(term, mir_perceus_process_operand, &ctx);
  mir_perceus_emit_post_releases(fn, out, &post_releases, NULL);
}

static bool mir_perceus_function_has_markers(MirFunction *fn) {
  if (!fn) {
    return false;
  }
  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }
    for (size_t j = 0; j < block->instrs.len; j++) {
      switch (block->instrs.items[j].kind) {
      case MIR_OP:
        if (block->instrs.items[j].data.op.kind == MIR_OP_KIND_DUP ||
            block->instrs.items[j].data.op.kind == MIR_OP_KIND_DROP) {
          return true;
        }
        break;
      default:
        break;
      }
    }
  }
  return false;
}

static void mir_perceus_instrument_function(MirFunction *fn);
static void mir_perceus_pair_reuse(MirFunction *fn);

static void mir_perceus_instrument_function(MirFunction *fn) {
  if (!fn || fn->is_extern || fn->blocks.len == 0 || fn->values.len == 0 ||
      mir_perceus_function_has_markers(fn)) {
    return;
  }

  size_t original_values_len = fn->values.len;
  size_t original_blocks_len = fn->blocks.len;
  MirPerceusLiveness liveness = {0};
  if (!mir_perceus_liveness_init(fn, &liveness, original_values_len)) {
    return;
  }

  for (size_t i = 0; i < original_blocks_len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }

    uint32_t *remaining = calloc(original_values_len, sizeof(uint32_t));
    bool *moved = calloc(original_values_len, sizeof(bool));
    if (!remaining || !moved) {
      free(remaining);
      free(moved);
      continue;
    }
    for (size_t j = 0; j < block->instrs.len; j++) {
      mir_perceus_count_instr_uses(fn, remaining, liveness.borrow_parent,
                                   original_values_len,
                                   &block->instrs.items[j]);
    }
    mir_perceus_count_term_uses(fn, remaining, liveness.borrow_parent,
                                original_values_len, &block->term);

    const bool *live_out =
        block->id < liveness.block_count
            ? mir_perceus_block_set(liveness.live_out, liveness.value_count,
                                    block->id)
            : NULL;
    MirInstrVec old_instrs = block->instrs;
    block->instrs = (MirInstrVec){0};

    for (size_t j = 0; j < old_instrs.len; j++) {
      MirInstr *instr = &old_instrs.items[j];
      MirValueIdVec post_releases = {0};
      mir_perceus_process_instr_uses(fn, &block->instrs, remaining,
                                     original_values_len, live_out,
                                     liveness.borrow_parent, moved,
                                     &post_releases, instr);
      MirValueId old_array_slot =
          mir_perceus_emit_array_set_old_slot_load(fn, &block->instrs, instr);
      MirValueId old_store_slot =
          mir_perceus_emit_store_old_slot_load(fn, &block->instrs, instr);
      mir_instr_vec_push(fn->arena, &block->instrs, *instr);
      mir_perceus_own_extracted_result(fn, &block->instrs, instr, remaining,
                                       original_values_len,
                                       liveness.borrow_parent, live_out);
      mir_perceus_drop_dead_definition(fn, &block->instrs, instr, remaining,
                                       original_values_len,
                                       liveness.borrow_parent, live_out, moved);
      mir_perceus_emit_marker(fn, &block->instrs, MIR_OP_KIND_DROP,
                              old_array_slot, instr->origin);
      mir_perceus_emit_marker(fn, &block->instrs, MIR_OP_KIND_DROP,
                              old_store_slot, instr->origin);
      mir_perceus_emit_post_releases(fn, &block->instrs, &post_releases,
                                     instr->origin);
    }

    mir_perceus_drop_unused_entry_params(fn, block, &block->instrs, remaining,
                                         original_values_len,
                                         liveness.borrow_parent, live_out,
                                         moved);
    mir_perceus_process_term_uses(fn, &block->instrs, remaining,
                                  original_values_len, live_out,
                                  liveness.borrow_parent, moved, &block->term);
    mir_perceus_insert_edge_drops(fn, &liveness, block);
    free(remaining);
    free(moved);
  }

  mir_perceus_liveness_destroy(&liveness);
  mir_perceus_pair_reuse(fn);
}

static bool mir_perceus_reuse_compatible(MirFunction *fn, MirValueId dropped,
                                         MirInstr *cons) {
  if (!fn || dropped == MIR_NO_VALUE || !cons ||
      cons->kind != MIR_CONSTRUCT ||
      cons->data.construct.kind != MIR_CONSTRUCT_LIST_CONS) {
    return false;
  }
  Type *dropped_type = mir_function_value_type(fn, dropped);
  if (!dropped_type || !is_list_type(dropped_type)) {
    return false;
  }
  /* Same shape: both are lists of the same element type. Post-specialization
     types are concrete, so pointer equality of the element type is a valid
     same-shape check. */
  Type *dropped_elt = type_of_list(dropped_type);
  Type *cons_elt = type_of_list(cons->type);
  return dropped_elt && cons_elt && dropped_elt == cons_elt;
}

static bool mir_perceus_collect_value(MirInstr *instr, MirOperand operand,
                                       void *ctx) {
  (void)instr;
  MirValueId target = *(MirValueId *)ctx;
  return operand.value != target;
}

static bool mir_perceus_value_used_between(MirFunction *fn, MirInstrVec *instrs,
                                            size_t drop_idx, size_t cons_idx,
                                            MirValueId value) {
  /* Check whether `value` is used by any instruction strictly between the
     drop and the cons. If it is, we cannot reorder the drop-reuse before the
     cons. */
  for (size_t k = drop_idx + 1; k < cons_idx; k++) {
    MirInstr *between = &instrs->items[k];
    MirValueId target = value;
    if (!mir_instr_for_each_operand(between, mir_perceus_collect_value,
                                    &target)) {
      return true;
    }
  }
  (void)fn;
  return false;
}

static void mir_perceus_pair_reuse(MirFunction *fn) {
  if (!fn || fn->is_extern || fn->blocks.len == 0) {
    return;
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }

    /* For each construct.list_cons, search backward for a same-shape drop of a
       value that is not used between the drop and the cons. Rewrite the drop
       into a drop-reuse (yielding a token), move it to just before the cons, and
       set the cons's reuse_token. */
    MirInstrVec *instrs = &block->instrs;
    for (size_t j = 0; j < instrs->len; j++) {
      MirInstr *cons = &instrs->items[j];
      if (cons->kind != MIR_CONSTRUCT ||
          cons->data.construct.kind != MIR_CONSTRUCT_LIST_CONS ||
          cons->data.construct.reuse_token != MIR_NO_VALUE) {
        continue;
      }

      /* Find a same-shape drop anywhere in the block. The value must not be
         used between the drop and the cons (in either direction). */
      ssize_t drop_idx = -1;
      for (ssize_t k = 0; k < (ssize_t)instrs->len; k++) {
        if (k == (ssize_t)j) {
          continue;
        }
        MirInstr *cand = &instrs->items[k];
        if (cand->kind != MIR_OP ||
            cand->data.op.kind != MIR_OP_KIND_DROP) {
          continue;
        }
        if (!mir_perceus_reuse_compatible(fn, cand->data.op.operands[0], cons)) {
          continue;
        }
        MirValueId dropped_val = cand->data.op.operands[0];
        size_t lo = k < (ssize_t)j ? (size_t)k : j;
        size_t hi = k < (ssize_t)j ? j : (size_t)k;
        if (mir_perceus_value_used_between(fn, instrs, lo, hi, dropped_val)) {
          continue;
        }
        drop_idx = k;
        break;
      }
      if (drop_idx < 0) {
        continue;
      }

      MirValueId dropped = instrs->items[drop_idx].data.op.operands[0];
      Ast *origin = instrs->items[drop_idx].origin;

      /* Build the drop-reuse instruction (yields a ptr token). */
      MirInstr drop_reuse = mir_make_instr(MIR_OP, &t_ptr, origin);
      drop_reuse.data.op.kind = MIR_OP_KIND_DROP_REUSE;
      drop_reuse.data.op.argc = 1;
      drop_reuse.data.op.operands[0] = dropped;
      drop_reuse.result = mir_function_add_value(fn, &t_ptr, origin);

      /* Remove the old drop, insert the drop-reuse just before the cons, and
         set the cons's reuse token. */
      MirInstrVec new_instrs = {0};
      for (size_t k = 0; k < instrs->len; k++) {
        if (k == (size_t)drop_idx) {
          continue; /* remove old drop */
        }
        if (k == j) {
          mir_instr_vec_push(fn->arena, &new_instrs, drop_reuse);
          cons->data.construct.reuse_token = drop_reuse.result;
        }
        mir_instr_vec_push(fn->arena, &new_instrs, instrs->items[k]);
      }
      block->instrs = new_instrs;
      instrs = &block->instrs;
      /* Re-scan from the beginning since indices shifted. */
      j = 0;
    }
  }
}

void mir_perceus_instrumentation(MirProgram *program) {
  if (!program) {
    return;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    mir_perceus_instrument_function(program->functions.items[i]);
  }
}
