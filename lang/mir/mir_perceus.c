#include "./mir.h"
#include "escape_analysis.h"
#include "types/builtins.h"
#include "types/type.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static bool mir_perceus_value_in_range(MirFunction *fn, MirValueId value,
                                       size_t limit) {
  return fn && value != MIR_NO_VALUE && value < fn->values.len && value < limit;
}

static bool mir_perceus_is_managed_type(Type *type) {
  return type && (is_array_type(type) || is_list_type(type) ||
                  is_string_type(type) || is_closure(type));
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

static void mir_perceus_count_use(MirFunction *fn, uint32_t *uses,
                                  MirValueId value) {
  if (!fn || !uses || value == MIR_NO_VALUE || value >= fn->values.len) {
    return;
  }
  uses[value]++;
}

typedef struct {
  MirFunction *fn;
  uint32_t *uses;
} MirPerceusCountCtx;

static bool mir_perceus_count_operand(MirInstr *instr, MirOperand operand,
                                      void *ctx) {
  (void)instr;

  MirPerceusCountCtx *count_ctx = ctx;
  if (!count_ctx) {
    return true;
  }

  mir_perceus_count_use(count_ctx->fn, count_ctx->uses, operand.value);
  return true;
}

static void mir_perceus_count_instr_uses(MirFunction *fn, uint32_t *uses,
                                         MirInstr *instr) {
  if (!fn || !uses || !instr) {
    return;
  }

  MirPerceusCountCtx ctx = {
      .fn = fn,
      .uses = uses,
  };
  mir_instr_for_each_operand(instr, mir_perceus_count_operand, &ctx);
}

static void mir_perceus_count_term_uses(MirFunction *fn, uint32_t *uses,
                                        MirTerminator *term) {
  if (!fn || !uses || !term) {
    return;
  }

  MirPerceusCountCtx ctx = {
      .fn = fn,
      .uses = uses,
  };
  mir_term_for_each_operand(term, mir_perceus_count_operand, &ctx);
}

static void mir_perceus_emit_marker(MirFunction *fn, MirInstrVec *out,
                                    MirInstrKind kind, MirValueId value,
                                    Ast *origin) {
  if (!fn || !out || value == MIR_NO_VALUE) {
    return;
  }

  MirInstr marker = mir_make_instr(kind, &t_void, origin);
  marker.data.value_op.value = value;
  marker.result = mir_function_add_value(fn, &t_void, origin);
  mir_instr_vec_push(fn->arena, out, marker);
}

static void mir_perceus_process_use(MirFunction *fn, MirInstrVec *out,
                                    uint32_t *remaining, size_t value_limit,
                                    MirValueIdVec *post_releases,
                                    MirValueId value, bool consumes,
                                    Ast *origin) {
  if (!mir_perceus_value_in_range(fn, value, value_limit) || !remaining) {
    return;
  }

  bool managed = mir_perceus_manages_value(fn, value, value_limit);
  if (managed && consumes && remaining[value] > 1) {
    mir_perceus_emit_marker(fn, out, MIR_DUP, value, origin);
  }

  if (remaining[value] > 0) {
    remaining[value]--;
  }
}

static bool mir_perceus_projection_borrows_result(MirInstr *instr) {
  if (!instr) {
    return false;
  }

  switch (instr->kind) {
  case MIR_TUPLE_GET:
  case MIR_VARIANT_PAYLOAD:
  case MIR_LIST_HEAD:
  case MIR_LIST_TAIL:
  case MIR_ARRAY_AT:
  case MIR_CLOSURE_GET:
  case MIR_CLOSURE_GET_ENV:
    return true;
  default:
    return false;
  }
}

typedef struct {
  MirFunction *fn;
  MirInstrVec *out;
  uint32_t *remaining;
  size_t value_limit;
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
      process_ctx->value_limit, process_ctx->post_releases, operand.value,
      operand.use == MIR_OPERAND_USE_CONSUME, instr ? instr->origin : NULL);
  return true;
}

static void mir_perceus_process_instr_uses(MirFunction *fn, MirInstrVec *out,
                                           uint32_t *remaining,
                                           size_t value_limit,
                                           MirValueIdVec *post_releases,
                                           MirInstr *instr) {
  if (!fn || !out || !remaining || !instr) {
    return;
  }

  MirPerceusProcessCtx ctx = {
      .fn = fn,
      .out = out,
      .remaining = remaining,
      .value_limit = value_limit,
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
    mir_perceus_emit_marker(fn, out, MIR_DROP, post_releases->items[i], origin);
  }
}

static void mir_perceus_own_projected_result(MirFunction *fn, MirInstrVec *out,
                                             MirInstr *instr,
                                             size_t value_limit) {
  if (!instr || !mir_perceus_projection_borrows_result(instr) ||
      !mir_perceus_manages_value(fn, instr->result, value_limit)) {
    return;
  }

  mir_perceus_emit_marker(fn, out, MIR_DUP, instr->result, instr->origin);
}

static void mir_perceus_process_term_uses(MirFunction *fn, MirInstrVec *out,
                                          uint32_t *remaining,
                                          size_t value_limit,
                                          MirTerminator *term) {
  MirValueIdVec post_releases = {0};
  MirPerceusProcessCtx ctx = {
      .fn = fn,
      .out = out,
      .remaining = remaining,
      .value_limit = value_limit,
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
      case MIR_DUP:
      case MIR_DROP:
        return true;
      default:
        break;
      }
    }
  }
  return false;
}

static void mir_perceus_instrument_function(MirFunction *fn) {
  if (!fn || fn->values.len == 0 || mir_perceus_function_has_markers(fn)) {
    return;
  }

  size_t original_values_len = fn->values.len;
  uint32_t *remaining = calloc(original_values_len, sizeof(uint32_t));
  if (!remaining) {
    return;
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }
    for (size_t j = 0; j < block->instrs.len; j++) {
      mir_perceus_count_instr_uses(fn, remaining, &block->instrs.items[j]);
    }
    mir_perceus_count_term_uses(fn, remaining, &block->term);
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }

    MirInstrVec old_instrs = block->instrs;
    block->instrs = (MirInstrVec){0};

    for (size_t j = 0; j < old_instrs.len; j++) {
      MirInstr *instr = &old_instrs.items[j];
      MirValueIdVec post_releases = {0};
      mir_perceus_process_instr_uses(fn, &block->instrs, remaining,
                                     original_values_len, &post_releases,
                                     instr);
      mir_instr_vec_push(fn->arena, &block->instrs, *instr);
      mir_perceus_own_projected_result(fn, &block->instrs, instr,
                                       original_values_len);
      mir_perceus_emit_post_releases(fn, &block->instrs, &post_releases,
                                     instr->origin);
    }

    mir_perceus_process_term_uses(fn, &block->instrs, remaining,
                                  original_values_len, &block->term);
  }

  free(remaining);
}

void mir_perceus_instrumentation(MirProgram *program) {
  if (!program) {
    return;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    mir_perceus_instrument_function(program->functions.items[i]);
  }
}
