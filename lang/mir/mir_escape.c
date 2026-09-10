#include "./mir.h"
#include "escape_analysis.h"
#include "types/type.h"
#include <stdbool.h>
#include <stdlib.h>

/*
 * Escape analysis pass.
 *
 * Determines, for every allocation site in a function, whether the allocation
 * escapes its function (must be heap-allocated with an RC header) or stays
 * local (can be stack-allocated with an rc=0 sentinel header). The result is
 * attached to each value as an `EscapeMeta` that the Perceus pass and the
 * LLVM lowering consult to choose heap vs stack allocation and to suppress
 * RC markers on stack values.
 *
 * The pass runs in two granularities:
 *
 *   1. Program-level interprocedural summary (`mir_escape_analysis`): computes
 *      per-function summaries of which parameters escape, iterated to a
 *      fixpoint across the call graph so a callee's escape behavior is known
 *      at its callers. (mir_escape_program_ctx_*, mir_escape_compute_program_summaries)
 *
 *   2. Function-level intraprocedural analysis (`mir_escape_analysis_function`):
 *      seeds escape from return/terminator/call/store/array_set, then
 *      propagates escape backward through phi/construct/extract/store until a
 *      fixpoint, finally attaching EA_HEAP_ALLOC (escaped) or EA_STACK_ALLOC
 *      (local) metadata to each allocation site. (mir_escape_analyze_function,
 *      mir_escape_attach_metadata)
 *
 * `mutable` tracks allocations that are mutated in place (array_set / store),
 * which the lowering marks heap-mutable so the container survives mutation.
 */

#define MIR_ESCAPE_ALIGNOF(T) __alignof__(T)

typedef struct {
  bool *alloc_site;       /* value is an allocation site (construct/coro/...) */
  bool *escaped;          /* value escapes -> heap-allocated */
  bool *mutable;          /* value is mutated in place -> heap-mutable */
  uint32_t *alloc_id;     /* stable id assigned per allocation site */
  uint32_t next_alloc_id;
} MirEscapeState;

/* Interprocedural summary of one function: which of its parameters escape
   (are returned, stored globally, or passed to a consuming call). */
typedef struct {
  bool *params;
  size_t len;
} MirEscapeFnSummary;

typedef struct {
  MirProgram *program;
  MirEscapeFnSummary *functions;
  size_t functions_len;
} MirEscapeProgramCtx;

/* The escape metadata attached to a value, looked up by the Perceus pass and
   the lowering. NULL means no metadata (value is not an allocation site). */
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

/* A type is tracked if it is a heap-allocatable container (array, list,
   string, closure, coroutine). Only allocations of tracked types are
   classified stack vs heap; primitive allocations are irrelevant. */
static bool mir_escape_is_tracked_type(Type *type) {
  return type && (is_array_type(type) || is_list_type(type) ||
                   is_string_type(type) || is_closure(type) ||
                   is_coroutine_type(type));
}

/* A type may be tracked if it is a tracked container, or it is a
   tuple/variant/record whose fields could themselves be tracked. Primitives
   (Int/Double/Char/Bool/Void/Uint64) are never tracked. Used to decide
   whether a stored/extracted value could carry tracked data. */
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

static bool mir_escape_instr_is_op(MirInstr *instr, MirOpKind kind) {
  return instr && instr->kind == MIR_OP && instr->data.op.kind == kind;
}

static bool mir_escape_op_is_alloc_site(MirInstr *instr) {
  if (!instr || instr->kind != MIR_OP) {
    return false;
  }

  switch (instr->data.op.kind) {
  case MIR_OP_KIND_AS_BYTES:
  case MIR_OP_KIND_STR:
  case MIR_OP_KIND_TYPEOF:
    return true;
  default:
    return false;
  }
}

/* Is this instruction an allocation site? A construct of a tracked type
   (array literal/fill/list cons/closure env), a coroutine new, or a few
   op-coded allocations (as_bytes/str/typeof). Each allocation site gets an
   `alloc_id` and is later classified heap or stack. */
static bool mir_escape_is_alloc_site(MirInstr *instr) {
  if (!instr) {
    return false;
  }

  if (instr->kind == MIR_CONSTRUCT &&
      instr->data.construct.kind == MIR_CONSTRUCT_CLOSURE_ENV) {
    return true;
  }

  if (!mir_escape_is_tracked_type(instr->type)) {
    return false;
  }

  switch (instr->kind) {
  case MIR_CORO_NEW:
    return true;
  case MIR_CONSTRUCT:
    switch (instr->data.construct.kind) {
    case MIR_CONSTRUCT_ARRAY_LITERAL:
    case MIR_CONSTRUCT_ARRAY_FILL_CONST:
    case MIR_CONSTRUCT_ARRAY_FILL:
    case MIR_CONSTRUCT_LIST_CONS:
      return true;
    default:
      return false;
    }
  case MIR_OP:
    return mir_escape_op_is_alloc_site(instr);
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

/* Look up the consume/borrow use mode of a call argument; defaults to consume
   (ownership transferred) when the callee summary is unavailable. */
static MirOperandUse mir_escape_call_operand_use(MirInstr *call, size_t index) {
  if (!call || call->kind != MIR_CALL ||
      index >= call->data.call.operand_uses.len ||
      !call->data.call.operand_uses.items) {
    return MIR_OPERAND_USE_CONSUME;
  }
  return call->data.call.operand_uses.items[index];
}

/* Seed escape from a call: a consumed (ownership-transferring) argument
   escapes, because the callee will drop (free) it. Marking it escaped
   promotes any alloc-site fields it contains to heap with an RC header so the
   callee's drop is safe. Borrowed arguments do not escape here. */
static bool mir_escape_seed_call(MirFunction *fn, MirInstr *call,
                                 MirEscapeState *state,
                                 MirEscapeProgramCtx *program_ctx) {
  if (!fn || !call || call->kind != MIR_CALL || !state) {
    return false;
  }

  (void)program_ctx;
  bool changed = false;
  for (size_t i = 0; i < call->data.call.operands.len; i++) {
    if (mir_escape_call_operand_use(call, i) != MIR_OPERAND_USE_CONSUME) {
      continue;
    }

    /* A consumed call argument transfers ownership to the callee, which the
       Perceus pass will later drop. Mark it escaping so any alloc-site fields
       it contains are promoted to heap (with an RC header) and the callee's
       drop is safe. Stack-allocated managed values still flow through safely
       because the lowering gives them an rc=0 sentinel header that dup/drop
       no-op on. */
    changed |=
        mir_escape_mark(state->escaped, fn, call->data.call.operands.items[i]);
  }
  return changed;
}

/* Seed escape from the terminator: consumed operands of return/yield escape
   (the value leaves the function). */
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

/* Seed escape from an instruction: record allocation sites, and mark values
   that escape via array_set (the container becomes mutable), store/global
   store (the stored tracked value escapes). Calls and terminators are
   handled by their dedicated seeders. */
static bool mir_escape_seed_instruction(MirFunction *fn, MirInstr *instr,
                                        MirEscapeState *state,
                                        MirEscapeProgramCtx *program_ctx) {
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
    changed |= mir_escape_seed_call(fn, instr, state, program_ctx);
    break;
  case MIR_OP:
    if (instr->data.op.kind == MIR_OP_KIND_ARRAY_SET) {
      changed |= mir_escape_mark(state->mutable, fn,
                                 instr->data.op.operands[0]);
    } else if (instr->data.op.kind == MIR_OP_KIND_STORE) {
      changed |= mir_escape_mark(state->mutable, fn,
                                 instr->data.op.operands[0]);
      MirValueId stored = instr->data.op.operands[1];
      if (mir_escape_may_be_tracked_type(mir_function_value_type(fn, stored))) {
        changed |= mir_escape_mark(state->escaped, fn, stored);
      }
    } else if (instr->data.op.kind == MIR_OP_KIND_GLOBAL_STORE) {
      MirValueId stored = instr->data.op.operands[0];
      if (mir_escape_may_be_tracked_type(mir_function_value_type(fn, stored))) {
        changed |= mir_escape_mark(state->escaped, fn, stored);
      }
    }
    break;
  default:
    break;
  }

  return changed;
}

/* Backward propagation: if an instruction's result has escaped, mark its
   consumed operands (or borrowed source for extractions) as escaped too, so
   the escape reaches the allocation sites that produced them. For
   constructs, only consumed fields propagate (a stored aggregate escapes =>
   its owned fields escape). For extracts, the source container escapes if the
   extracted tracked value escapes. */
static bool mir_escape_propagate_from_result(MirFunction *fn, MirInstr *instr,
                                             MirEscapeState *state) {
  if (!instr || !state || !mir_escape_value_in_range(fn, instr->result) ||
      !state->escaped[instr->result]) {
    return false;
  }

  bool changed = false;
  switch (instr->kind) {
  case MIR_PHI:
    changed |= mir_escape_mark_operands(fn, instr, state->escaped, false);
    break;
  case MIR_CONSTRUCT:
    switch (instr->data.construct.kind) {
    case MIR_CONSTRUCT_TUPLE:
    case MIR_CONSTRUCT_VARIANT:
    case MIR_CONSTRUCT_LIST_CONS:
    case MIR_CONSTRUCT_ARRAY_LITERAL:
    case MIR_CONSTRUCT_CLOSURE_ENV:
    case MIR_CONSTRUCT_CLOSURE:
      changed |= mir_escape_mark_operands(fn, instr, state->escaped, true);
      break;
    case MIR_CONSTRUCT_ARRAY_FILL_CONST:
      changed |= mir_escape_mark(state->escaped, fn,
                                 instr->data.construct.operands[1]);
      break;
    case MIR_CONSTRUCT_ARRAY_RANGE:
      changed |= mir_escape_mark(state->escaped, fn,
                                 instr->data.construct.operands[2]);
      break;
    default:
      break;
    }
    break;
  case MIR_OP:
    if (instr->data.op.kind == MIR_OP_KIND_ARRAY_SET) {
      changed |= mir_escape_mark(state->escaped, fn,
                                 instr->data.op.operands[0]);
      changed |= mir_escape_mark(state->escaped, fn,
                                 instr->data.op.operands[2]);
    } else if (instr->data.op.kind == MIR_OP_KIND_PTR_OFFSET) {
      changed |= mir_escape_mark(state->escaped, fn,
                                 instr->data.op.operands[0]);
    } else if (instr->data.op.kind == MIR_OP_KIND_LOAD ||
               instr->data.op.kind == MIR_OP_KIND_LOAD_OWNED) {
      if (mir_escape_may_be_tracked_type(instr->type)) {
        changed |= mir_escape_mark(state->escaped, fn,
                                   instr->data.op.operands[0]);
      }
    }
    break;
  case MIR_CORO_NEW:
    changed |= mir_escape_mark_operands(fn, instr, state->escaped, true);
    break;
  case MIR_CORO_RESET:
    changed |= mir_escape_mark(state->escaped, fn, instr->data.call.callee);
    break;
  case MIR_EXTRACT:
    switch (instr->data.extract.kind) {
    case MIR_EXTRACT_LIST_TAIL:
    case MIR_EXTRACT_ARRAY_SUCC:
    case MIR_EXTRACT_ARRAY_OFFSET:
    case MIR_EXTRACT_CLOSURE_ENV:
      changed |=
          mir_escape_mark(state->escaped, fn, instr->data.extract.value);
      break;
    case MIR_EXTRACT_FIELD:
    case MIR_EXTRACT_VARIANT_PAYLOAD:
    case MIR_EXTRACT_LIST_HEAD:
    case MIR_EXTRACT_ARRAY_AT:
      if (mir_escape_may_be_tracked_type(instr->type)) {
        changed |=
            mir_escape_mark(state->escaped, fn, instr->data.extract.value);
      }
      break;
    case MIR_EXTRACT_VARIANT_TAG:
    case MIR_EXTRACT_CLOSURE_FN:
      break;
    }
    break;
  default:
    break;
  }

  return changed;
}

/* Propagate mutability backward: if an array_set/ptr_offset/array-range/tuple
   result is mutable, its source array/pointer is mutable too. Mutability
   forces heap allocation so the container survives in-place mutation. */
static bool mir_escape_propagate_mutability(MirFunction *fn, MirInstr *instr,
                                            MirEscapeState *state) {
  if (!instr || !state || !mir_escape_value_in_range(fn, instr->result) ||
      !state->mutable[instr->result]) {
    return false;
  }

  switch (instr->kind) {
  case MIR_OP:
    if (instr->data.op.kind == MIR_OP_KIND_ARRAY_SET) {
      return mir_escape_mark(state->mutable, fn, instr->data.op.operands[0]);
    }
    if (instr->data.op.kind == MIR_OP_KIND_PTR_OFFSET) {
      return mir_escape_mark(state->mutable, fn, instr->data.op.operands[0]);
    }
    return false;
  case MIR_CONSTRUCT:
    if (instr->data.construct.kind == MIR_CONSTRUCT_ARRAY_RANGE) {
      return mir_escape_mark(state->mutable, fn,
                             instr->data.construct.operands[2]);
    }
    if (instr->data.construct.kind == MIR_CONSTRUCT_TUPLE &&
        is_array_type(instr->type) && instr->data.construct.items.len > 1) {
      return mir_escape_mark(state->mutable, fn,
                             instr->data.construct.items.items
                                 [instr->data.construct.items.len - 1]);
    }
    return false;
  case MIR_EXTRACT:
    if (instr->data.extract.kind == MIR_EXTRACT_ARRAY_SUCC ||
        instr->data.extract.kind == MIR_EXTRACT_ARRAY_OFFSET ||
        instr->data.extract.kind == MIR_EXTRACT_FIELD) {
      return mir_escape_mark(state->mutable, fn, instr->data.extract.value);
    }
    return false;
  default:
    return false;
  }
}

/* If an array_set/store's array or its result has escaped, then both the
   array and the stored value escape (a mutation through an escaped alias
   means the stored value is observable outside the function). */
static bool mir_escape_propagate_array_store(MirFunction *fn, MirInstr *instr,
                                             MirEscapeState *state) {
  if ((!mir_escape_instr_is_op(instr, MIR_OP_KIND_ARRAY_SET) &&
       !mir_escape_instr_is_op(instr, MIR_OP_KIND_STORE)) ||
      !state) {
    return false;
  }

  bool array_escapes = mir_escape_value_in_range(fn, instr->data.op.operands[0]) &&
                       state->escaped[instr->data.op.operands[0]];
  bool result_escapes = mir_escape_value_in_range(fn, instr->result) &&
                        state->escaped[instr->result];
  if (!array_escapes && !result_escapes) {
    return false;
  }

  bool changed = false;
  changed |= mir_escape_mark(state->escaped, fn, instr->data.op.operands[0]);
  MirValueId stored =
      instr->data.op.kind == MIR_OP_KIND_STORE ? instr->data.op.operands[1]
                                               : instr->data.op.operands[2];
  changed |= mir_escape_mark(state->escaped, fn, stored);
  changed |= mir_escape_mark(state->escaped, fn, instr->result);
  return changed;
}

/* One propagation step: from-result escape, mutability, and array-store
   escape. Applied to every instruction iterated to a fixpoint. */
static bool mir_escape_propagate_instruction(MirFunction *fn, MirInstr *instr,
                                             MirEscapeState *state) {
  bool changed = false;
  changed |= mir_escape_propagate_from_result(fn, instr, state);
  changed |= mir_escape_propagate_mutability(fn, instr, state);
  changed |= mir_escape_propagate_array_store(fn, instr, state);
  return changed;
}

/* Attach the computed classification to each allocation site: EA_HEAP_ALLOC
   if it escaped, EA_STACK_ALLOC if it stayed local, with EA_ATTR_MUTABLE set
   for mutated allocations. Non-allocation-site values get NULL metadata. */
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

static bool mir_escape_state_init(MirFunction *fn, MirEscapeState *state) {
  if (!fn || !state || fn->values.len == 0) {
    return false;
  }

  *state = (MirEscapeState){
      .alloc_site = calloc(fn->values.len, sizeof(bool)),
      .escaped = calloc(fn->values.len, sizeof(bool)),
      .mutable = calloc(fn->values.len, sizeof(bool)),
      .alloc_id = calloc(fn->values.len, sizeof(uint32_t)),
      .next_alloc_id = 0,
  };

  if (!state->alloc_site || !state->escaped || !state->mutable ||
      !state->alloc_id) {
    free(state->alloc_site);
    free(state->escaped);
    free(state->mutable);
    free(state->alloc_id);
    *state = (MirEscapeState){0};
    return false;
  }

  return true;
}

static void mir_escape_state_destroy(MirEscapeState *state) {
  if (!state) {
    return;
  }

  free(state->alloc_site);
  free(state->escaped);
  free(state->mutable);
  free(state->alloc_id);
  *state = (MirEscapeState){0};
}

/* Run the intraprocedural analysis: seed escapes, then propagate to a
   fixpoint. Returns true (state filled) so the caller can attach metadata. */
static bool mir_escape_analyze_function(MirFunction *fn,
                                        MirEscapeProgramCtx *program_ctx,
                                        MirEscapeState *state) {
  if (!fn || fn->is_extern || fn->blocks.len == 0) {
    return false;
  }

  if (!mir_escape_state_init(fn, state)) {
    return false;
  }

  bool changed = false;
  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }
    for (size_t j = 0; j < block->instrs.len; j++) {
      changed |= mir_escape_seed_instruction(fn, &block->instrs.items[j],
                                             state, program_ctx);
    }
    changed |= mir_escape_seed_terminator(fn, &block->term, state);
  }

  do {
    changed = false;
    for (size_t i = 0; i < fn->blocks.len; i++) {
      MirBlock *block = fn->blocks.items[i];
      if (!block) {
        continue;
      }
      for (size_t j = 0; j < block->instrs.len; j++) {
        changed |= mir_escape_propagate_instruction(
            fn, &block->instrs.items[j], state);
      }
    }
  } while (changed);

  return true;
}

/* Standalone single-function escape analysis (no interprocedural summaries):
   used as a fallback when the program-wide summary computation is
   unavailable, and for isolated testing. */
void mir_escape_analysis_function(MirFunction *fn) {
  if (!fn || fn->is_extern || fn->blocks.len == 0) {
    return;
  }

  MirEscapeState state = {0};
  if (!mir_escape_analyze_function(fn, NULL, &state)) {
    return;
  }

  mir_escape_attach_metadata(fn, &state);
  mir_escape_state_destroy(&state);
}

static void mir_escape_program_ctx_destroy(MirEscapeProgramCtx *ctx) {
  if (!ctx) {
    return;
  }

  for (size_t i = 0; i < ctx->functions_len; i++) {
    free(ctx->functions[i].params);
  }
  free(ctx->functions);
  *ctx = (MirEscapeProgramCtx){0};
}

static bool mir_escape_program_ctx_init(MirProgram *program,
                                        MirEscapeProgramCtx *ctx) {
  if (!program || !ctx) {
    return false;
  }

  *ctx = (MirEscapeProgramCtx){
      .program = program,
      .functions = calloc(program->functions.len ? program->functions.len : 1,
                          sizeof(MirEscapeFnSummary)),
      .functions_len = program->functions.len,
  };
  if (!ctx->functions) {
    *ctx = (MirEscapeProgramCtx){0};
    return false;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    MirFunction *fn = program->functions.items[i];
    if (!fn || fn->id >= ctx->functions_len) {
      continue;
    }

    MirEscapeFnSummary *summary = &ctx->functions[fn->id];
    summary->len = fn->params.len;
    summary->params = calloc(fn->params.len ? fn->params.len : 1, sizeof(bool));
    if (!summary->params) {
      mir_escape_program_ctx_destroy(ctx);
      return false;
    }

    if (fn->blocks.len == 0) {
      for (size_t j = 0; j < summary->len; j++) {
        summary->params[j] = true;
      }
    }
  }

  return true;
}

/* Update a function's parameter-escape summary from its analyzed state.
   Returns true if any parameter's escape status changed (drives the
   interprocedural fixpoint). */
static bool mir_escape_update_fn_summary(MirFunction *fn,
                                         MirEscapeFnSummary *summary,
                                         MirEscapeState *state) {
  if (!fn || !summary || !state) {
    return false;
  }

  bool changed = false;
  for (size_t i = 0; i < fn->params.len && i < summary->len; i++) {
    MirParam *param = &fn->params.items[i];
    bool escaped = mir_escape_value_in_range(fn, param->value) &&
                   state->escaped[param->value];
    if (summary->params[i] != escaped) {
      summary->params[i] = escaped;
      changed = true;
    }
  }
  return changed;
}

/* Interprocedural fixpoint: repeatedly analyze every function and update its
   parameter summaries until none change. This makes a caller's escape
   decisions account for what its callees do with passed values. */
static bool mir_escape_compute_program_summaries(MirProgram *program,
                                                 MirEscapeProgramCtx *ctx) {
  if (!program || !ctx) {
    return false;
  }

  bool changed = false;
  do {
    changed = false;
    for (size_t i = 0; i < program->functions.len; i++) {
      MirFunction *fn = program->functions.items[i];
      if (!fn || fn->id >= ctx->functions_len) {
        continue;
      }
      if (fn->is_extern || fn->blocks.len == 0) {
        continue;
      }

      MirEscapeState state = {0};
      if (!mir_escape_analyze_function(fn, ctx, &state)) {
        continue;
      }
      changed |=
          mir_escape_update_fn_summary(fn, &ctx->functions[fn->id], &state);
      mir_escape_state_destroy(&state);
    }
  } while (changed);

  return true;
}

/*
 * Program-wide escape analysis entry point. Computes interprocedural
 * parameter-escape summaries, then runs the intraprocedural analysis for each
 * function with those summaries available and attaches the resulting
 * metadata. Falls back to per-function standalone analysis if summary
 * computation fails.
 */
void mir_escape_analysis(MirProgram *program) {
  if (!program) {
    return;
  }

  MirEscapeProgramCtx ctx = {0};
  if (!mir_escape_program_ctx_init(program, &ctx) ||
      !mir_escape_compute_program_summaries(program, &ctx)) {
    mir_escape_program_ctx_destroy(&ctx);
    for (size_t i = 0; i < program->functions.len; i++) {
      mir_escape_analysis_function(program->functions.items[i]);
    }
    return;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    MirFunction *fn = program->functions.items[i];
    if (!fn || fn->is_extern || fn->blocks.len == 0) {
      continue;
    }

    MirEscapeState state = {0};
    if (!mir_escape_analyze_function(fn, &ctx, &state)) {
      continue;
    }
    mir_escape_attach_metadata(fn, &state);
    mir_escape_state_destroy(&state);
  }

  mir_escape_program_ctx_destroy(&ctx);
}
