#include "./mir.h"
#include "escape_analysis.h"
#include "types/type.h"
#include <stdbool.h>
#include <stdlib.h>

#define MIR_ESCAPE_ALIGNOF(T) __alignof__(T)

typedef struct {
  bool *alloc_site;
  bool *escaped;
  bool *mutable;
  uint32_t *alloc_id;
  uint32_t next_alloc_id;
} MirEscapeState;

EscapeMeta *mir_value_escape_meta(MirFunction *fn, MirValueId value) {
  if (!fn || value == MIR_NO_VALUE || value >= fn->values.len) {
    return NULL;
  }
  return fn->values.items[value].ea_md;
}

static bool mir_escape_value_in_range(MirFunction *fn, MirValueId value) {
  return fn && value != MIR_NO_VALUE && value < fn->values.len;
}

static bool mir_escape_mark(bool *set, MirFunction *fn, MirValueId value) {
  if (!set || !mir_escape_value_in_range(fn, value) || set[value]) {
    return false;
  }
  set[value] = true;
  return true;
}

static bool mir_escape_is_tracked_type(Type *type) {
  return type && (is_array_type(type) || is_list_type(type) ||
                  is_string_type(type) || is_closure(type));
}

static bool mir_escape_may_be_tracked_type(Type *type) {
  if (mir_escape_is_tracked_type(type)) {
    return true;
  }
  if (!type) {
    return false;
  }

  switch (type->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
    return false;
  default:
    return true;
  }
}

static bool mir_escape_is_alloc_site(MirInstr *instr) {
  if (!instr) {
    return false;
  }

  switch (instr->kind) {
  case MIR_CLOSURE_ENV:
    return true;
  default:
    break;
  }

  if (!mir_escape_is_tracked_type(instr->type)) {
    return false;
  }

  switch (instr->kind) {
  case MIR_ARRAY_LITERAL:
  case MIR_ARRAY_FILL_CONST:
  case MIR_ARRAY_FILL:
  case MIR_AS_BYTES:
  case MIR_STR:
  case MIR_TYPEOF:
  case MIR_LIST_CONS:
    return true;
  default:
    return false;
  }
}

typedef struct {
  MirFunction *fn;
  bool *set;
  bool changed;
  bool consume_only;
} MirEscapeMarkOperandsCtx;

static bool mir_escape_mark_operand(MirInstr *instr, MirOperand operand,
                                    void *ctx) {
  (void)instr;
  MirEscapeMarkOperandsCtx *mark_ctx = ctx;
  if (!mark_ctx) {
    return true;
  }

  if (mark_ctx->consume_only && operand.use != MIR_OPERAND_USE_CONSUME) {
    return true;
  }

  mark_ctx->changed |=
      mir_escape_mark(mark_ctx->set, mark_ctx->fn, operand.value);
  return true;
}

static bool mir_escape_mark_operands(MirFunction *fn, MirInstr *instr,
                                     bool *set, bool consume_only) {
  MirEscapeMarkOperandsCtx ctx = {
      .fn = fn,
      .set = set,
      .changed = false,
      .consume_only = consume_only,
  };
  mir_instr_for_each_operand(instr, mir_escape_mark_operand, &ctx);
  return ctx.changed;
}

static bool mir_escape_seed_terminator(MirFunction *fn, MirTerminator *term,
                                       MirEscapeState *state) {
  if (!term || !state) {
    return false;
  }

  MirEscapeMarkOperandsCtx ctx = {
      .fn = fn,
      .set = state->escaped,
      .changed = false,
      .consume_only = true,
  };
  mir_term_for_each_operand(term, mir_escape_mark_operand, &ctx);
  return ctx.changed;
}

static bool mir_escape_seed_instruction(MirFunction *fn, MirInstr *instr,
                                        MirEscapeState *state) {
  bool changed = false;
  if (!instr || !state) {
    return false;
  }

  if (mir_escape_is_alloc_site(instr)) {
    state->alloc_site[instr->result] = true;
    state->alloc_id[instr->result] = state->next_alloc_id++;
  }

  switch (instr->kind) {
  case MIR_CALL:
    changed |= mir_escape_mark_operands(fn, instr, state->escaped, true);
    break;
  case MIR_ARRAY_SET:
    changed |= mir_escape_mark(state->mutable, fn, instr->data.array_set.array);
    break;
  default:
    break;
  }

  return changed;
}

static bool mir_escape_propagate_from_result(MirFunction *fn, MirInstr *instr,
                                             MirEscapeState *state) {
  if (!instr || !state || !mir_escape_value_in_range(fn, instr->result) ||
      !state->escaped[instr->result]) {
    return false;
  }

  bool changed = false;
  switch (instr->kind) {
  case MIR_TUPLE:
  case MIR_VARIANT:
  case MIR_LIST_CONS:
  case MIR_ARRAY_LITERAL:
  case MIR_CLOSURE_ENV:
  case MIR_CLOSURE:
    changed |= mir_escape_mark_operands(fn, instr, state->escaped, true);
    break;
  case MIR_LIST_TAIL:
    changed |= mir_escape_mark(state->escaped, fn, instr->data.list_op.list);
    break;
  case MIR_LIST_HEAD:
    if (mir_escape_may_be_tracked_type(instr->type)) {
      changed |= mir_escape_mark(state->escaped, fn, instr->data.list_op.list);
    }
    break;
  case MIR_ARRAY_AT:
    if (mir_escape_may_be_tracked_type(instr->type)) {
      changed |=
          mir_escape_mark(state->escaped, fn, instr->data.array_at.array);
    }
    break;
  case MIR_ARRAY_FILL_CONST:
    changed |=
        mir_escape_mark(state->escaped, fn, instr->data.array_fill_const.value);
    break;
  case MIR_ARRAY_SET:
    changed |= mir_escape_mark(state->escaped, fn, instr->data.array_set.array);
    changed |= mir_escape_mark(state->escaped, fn, instr->data.array_set.value);
    break;
  case MIR_ARRAY_SUCC:
    changed |=
        mir_escape_mark(state->escaped, fn, instr->data.array_unop.array);
    break;
  case MIR_ARRAY_RANGE:
    changed |=
        mir_escape_mark(state->escaped, fn, instr->data.array_range.array);
    break;
  case MIR_ARRAY_OFFSET:
    changed |=
        mir_escape_mark(state->escaped, fn, instr->data.array_offset.array);
    break;
  case MIR_TUPLE_GET:
    if (mir_escape_may_be_tracked_type(instr->type)) {
      changed |=
          mir_escape_mark(state->escaped, fn, instr->data.tuple_get.tuple);
    }
    break;
  case MIR_VARIANT_PAYLOAD:
    if (mir_escape_may_be_tracked_type(instr->type)) {
      changed |= mir_escape_mark(state->escaped, fn,
                                 instr->data.variant_payload.value);
    }
    break;
  case MIR_CLOSURE_GET:
    if (mir_escape_may_be_tracked_type(instr->type)) {
      changed |=
          mir_escape_mark(state->escaped, fn, instr->data.closure_get.env);
    }
    break;
  case MIR_CLOSURE_GET_ENV:
    changed |=
        mir_escape_mark(state->escaped, fn, instr->data.closure_part.closure);
    break;
  default:
    break;
  }

  return changed;
}

static bool mir_escape_propagate_mutability(MirFunction *fn, MirInstr *instr,
                                            MirEscapeState *state) {
  if (!instr || !state || !mir_escape_value_in_range(fn, instr->result) ||
      !state->mutable[instr->result]) {
    return false;
  }

  switch (instr->kind) {
  case MIR_ARRAY_SET:
    return mir_escape_mark(state->mutable, fn, instr->data.array_set.array);
  case MIR_ARRAY_SUCC:
    return mir_escape_mark(state->mutable, fn, instr->data.array_unop.array);
  case MIR_ARRAY_RANGE:
    return mir_escape_mark(state->mutable, fn, instr->data.array_range.array);
  case MIR_ARRAY_OFFSET:
    return mir_escape_mark(state->mutable, fn, instr->data.array_offset.array);
  default:
    return false;
  }
}

static bool mir_escape_propagate_array_store(MirFunction *fn, MirInstr *instr,
                                             MirEscapeState *state) {
  if (!instr || instr->kind != MIR_ARRAY_SET || !state) {
    return false;
  }

  bool array_escapes =
      mir_escape_value_in_range(fn, instr->data.array_set.array) &&
      state->escaped[instr->data.array_set.array];
  bool result_escapes = mir_escape_value_in_range(fn, instr->result) &&
                        state->escaped[instr->result];
  if (!array_escapes && !result_escapes) {
    return false;
  }

  bool changed = false;
  changed |= mir_escape_mark(state->escaped, fn, instr->data.array_set.array);
  changed |= mir_escape_mark(state->escaped, fn, instr->data.array_set.value);
  changed |= mir_escape_mark(state->escaped, fn, instr->result);
  return changed;
}

static bool mir_escape_propagate_instruction(MirFunction *fn, MirInstr *instr,
                                             MirEscapeState *state) {
  bool changed = false;
  changed |= mir_escape_propagate_from_result(fn, instr, state);
  changed |= mir_escape_propagate_mutability(fn, instr, state);
  changed |= mir_escape_propagate_array_store(fn, instr, state);
  return changed;
}

static void mir_escape_attach_metadata(MirFunction *fn, MirEscapeState *state) {
  if (!fn || !fn->arena || !state) {
    return;
  }

  for (size_t i = 0; i < fn->values.len; i++) {
    if (!state->alloc_site[i]) {
      fn->values.items[i].ea_md = NULL;
      continue;
    }

    EscapeMeta *meta = mir_arena_alloc(fn->arena, sizeof(EscapeMeta),
                                       MIR_ESCAPE_ALIGNOF(EscapeMeta));
    if (!meta) {
      continue;
    }

    *meta = (EscapeMeta){
        .status = state->escaped[i] ? EA_HEAP_ALLOC : EA_STACK_ALLOC,
        .id = state->alloc_id[i],
        .attributes = state->mutable[i] ? EA_ATTR_MUTABLE : 0,
    };
    fn->values.items[i].ea_md = meta;
  }
}

void mir_escape_analysis_function(MirFunction *fn) {
  if (!fn || fn->values.len == 0) {
    return;
  }

  MirEscapeState state = {
      .alloc_site = calloc(fn->values.len, sizeof(bool)),
      .escaped = calloc(fn->values.len, sizeof(bool)),
      .mutable = calloc(fn->values.len, sizeof(bool)),
      .alloc_id = calloc(fn->values.len, sizeof(uint32_t)),
      .next_alloc_id = 0,
  };
  if (!state.alloc_site || !state.escaped || !state.mutable ||
      !state.alloc_id) {
    free(state.alloc_site);
    free(state.escaped);
    free(state.mutable);
    free(state.alloc_id);
    return;
  }

  bool changed = false;
  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }
    for (size_t j = 0; j < block->instrs.len; j++) {
      changed |=
          mir_escape_seed_instruction(fn, &block->instrs.items[j], &state);
    }
    changed |= mir_escape_seed_terminator(fn, &block->term, &state);
  }

  do {
    changed = false;
    for (size_t i = 0; i < fn->blocks.len; i++) {
      MirBlock *block = fn->blocks.items[i];
      if (!block) {
        continue;
      }
      for (size_t j = 0; j < block->instrs.len; j++) {
        changed |= mir_escape_propagate_instruction(fn, &block->instrs.items[j],
                                                    &state);
      }
    }
  } while (changed);

  mir_escape_attach_metadata(fn, &state);

  free(state.alloc_site);
  free(state.escaped);
  free(state.mutable);
  free(state.alloc_id);
}

void mir_escape_analysis(MirProgram *program) {
  if (!program) {
    return;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    mir_escape_analysis_function(program->functions.items[i]);
  }
}
