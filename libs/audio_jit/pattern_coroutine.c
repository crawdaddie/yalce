#include "pattern_coroutine.h"
#include "../../lang/backend_llvm/coroutines/coroutines.h"
#include "../../lang/backend_llvm/strings.h"
#include "llvm-c/Core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Pattern parser
//
// Grammar (one level of nesting inside group brackets):
//   pattern  ::= step+
//   step     ::= number | '<' slot+ '>' | '{' slot+ '}'
//   slot     ::= number | '[' number+ ']'
//
// '<a b>'  — cycle through slots in order (counter % n)
// '{a b}'  — pick a slot uniformly at random each time
// '[a b c]' inside either — emit the whole sub-sequence when selected
//
// Examples:
//   "1 2 1 1 <1 3>"          fixed steps + cycling alternation
//   "1 <2 [1 2]> 1"          cycling alt with sub-sequence slot
//   "{1 2 3}"                 random pick among 1, 2, 3
//   "{1 2 [5 6 7]}"           random: scalar 1, scalar 2, or sequence 5 6 7
// ============================================================================

#define MAX_STEPS    256
#define MAX_SLOTS    64
#define MAX_SEQ_VALS 64

typedef struct {
  double *vals; // malloced; count=1 for scalar, N for [a b c]
  int count;
} Slot;

typedef struct {
  bool is_alt;
  bool is_rand;    // true for {}, false for <>  (only when is_alt)
  // STEP_FIXED:
  double fixed_val;
  // STEP_ALT:
  Slot *slots;     // malloced array of slots
  int num_slots;
  int counter_idx; // which alt_counter alloca to use (-1 for rand steps)
} PatternStep;

typedef struct {
  PatternStep *steps; // malloced
  int num_steps;
  int num_alt_groups;
} ParsedPattern;

static void free_parsed(ParsedPattern *p) {
  for (int i = 0; i < p->num_steps; i++) {
    PatternStep *s = &p->steps[i];
    if (s->is_alt) {
      for (int j = 0; j < s->num_slots; j++)
        free(s->slots[j].vals);
      free(s->slots);
    }
  }
  free(p->steps);
}

static void skip_ws(const char **pp) {
  while (**pp == ' ' || **pp == '\t')
    (*pp)++;
}

static bool parse_slot(const char **pp, Slot *out) {
  skip_ws(pp);
  if (**pp == '[') {
    (*pp)++;
    double tmp[MAX_SEQ_VALS];
    int n = 0;
    while (**pp && **pp != ']') {
      skip_ws(pp);
      if (**pp == ']')
        break;
      char *end;
      double v = strtod(*pp, &end);
      if (end == *pp)
        return false;
      if (n >= MAX_SEQ_VALS)
        return false;
      tmp[n++] = v;
      *pp = end;
    }
    if (**pp == ']')
      (*pp)++;
    if (n == 0)
      return false;
    out->count = n;
    out->vals = malloc(sizeof(double) * n);
    memcpy(out->vals, tmp, sizeof(double) * n);
    return true;
  } else {
    char *end;
    double v = strtod(*pp, &end);
    if (end == *pp)
      return false;
    *pp = end;
    out->count = 1;
    out->vals = malloc(sizeof(double));
    out->vals[0] = v;
    return true;
  }
}

static bool parse_pattern(const char *src, ParsedPattern *out) {
  out->steps = calloc(MAX_STEPS, sizeof(PatternStep));
  out->num_steps = 0;
  out->num_alt_groups = 0;

  const char *p = src;
  while (*p) {
    skip_ws(&p);
    if (!*p)
      break;
    if (out->num_steps >= MAX_STEPS)
      return false;

    if (*p == '<') {
      p++;
      Slot tmp_slots[MAX_SLOTS];
      int num_slots = 0;
      while (*p && *p != '>') {
        skip_ws(&p);
        if (*p == '>')
          break;
        if (num_slots >= MAX_SLOTS)
          return false;
        if (!parse_slot(&p, &tmp_slots[num_slots++]))
          return false;
      }
      if (*p == '>')
        p++;
      if (num_slots == 0)
        return false;

      PatternStep *step = &out->steps[out->num_steps++];
      step->is_alt = true;
      step->is_rand = false;
      step->fixed_val = 0.0;
      step->num_slots = num_slots;
      step->slots = malloc(sizeof(Slot) * num_slots);
      memcpy(step->slots, tmp_slots, sizeof(Slot) * num_slots);
      step->counter_idx = out->num_alt_groups++;
    } else if (*p == '{') {
      p++;
      Slot tmp_slots[MAX_SLOTS];
      int num_slots = 0;
      while (*p && *p != '}') {
        skip_ws(&p);
        if (*p == '}')
          break;
        if (num_slots >= MAX_SLOTS)
          return false;
        if (!parse_slot(&p, &tmp_slots[num_slots++]))
          return false;
      }
      if (*p == '}')
        p++;
      if (num_slots == 0)
        return false;

      PatternStep *step = &out->steps[out->num_steps++];
      step->is_alt = true;
      step->is_rand = true;
      step->fixed_val = 0.0;
      step->num_slots = num_slots;
      step->slots = malloc(sizeof(Slot) * num_slots);
      memcpy(step->slots, tmp_slots, sizeof(Slot) * num_slots);
      step->counter_idx = -1; // no cycling counter for rand
    } else {
      char *end;
      double v = strtod(p, &end);
      if (end == p)
        return false;
      p = end;
      PatternStep *step = &out->steps[out->num_steps++];
      step->is_alt = false;
      step->fixed_val = v;
      step->slots = NULL;
      step->num_slots = 0;
      step->counter_idx = -1;
    }
  }
  return out->num_steps > 0;
}

// ============================================================================
// IR helpers
// ============================================================================

// Emit one yield point. Positions builder at the resume block on return.
static void emit_yield(LLVMValueRef wrapper_fn, LLVMModuleRef module,
                       LLVMBuilderRef builder, LLVMValueRef handle,
                       LLVMValueRef promise_alloca, LLVMValueRef val,
                       LLVMBasicBlockRef cleanup_bb,
                       LLVMBasicBlockRef suspend_bb, const char *label) {
  LLVMBuildStore(builder, val, promise_alloca);

  LLVMValueRef save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1, "save");
  LLVMValueRef sus = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "sus");

  char resume_lbl[64], return_lbl[64];
  snprintf(resume_lbl, sizeof(resume_lbl), "%s.resume", label);
  snprintf(return_lbl, sizeof(return_lbl), "%s.return", label);
  LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(wrapper_fn, resume_lbl);
  LLVMBasicBlockRef return_bb = LLVMAppendBasicBlock(wrapper_fn, return_lbl);

  LLVMValueRef sw = LLVMBuildSwitch(builder, sus, return_bb, 2);
  LLVMAddCase(sw, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
  LLVMAddCase(sw, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, resume_bb);
}

// ============================================================================
// Reset function
// ============================================================================

static LLVMValueRef emit_reset_fn(LLVMTypeRef promise_type,
                                  LLVMTypeRef wrapper_fn_type,
                                  LLVMValueRef wrapper_fn, LLVMModuleRef module,
                                  LLVMBuilderRef builder, const char *name) {
  LLVMTypeRef reset_type = CORO_RESET_FN_TYPE;
  LLVMValueRef reset_fn = LLVMAddFunction(module, name, reset_type);
  LLVMSetLinkage(reset_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef prev_bb = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(reset_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMValueRef frame_size_out = LLVMGetParam(reset_fn, 0);
  LLVMValueRef new_handle = LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                                           &frame_size_out, 1, "new.handle");

  LLVMValueRef new_prom = GET_PROMISE_PTR(new_handle, promise_type);
  PROMISE_SET_RESET_FN(new_prom, promise_type, reset_fn);
  PROMISE_SET_ARGS_PTR(new_prom, promise_type, LLVMConstNull(GENERIC_PTR));

  LLVMBuildRet(builder, new_handle);
  LLVMPositionBuilderAtEnd(builder, prev_bb);
  return reset_fn;
}

// ============================================================================
// Main emitter
// ============================================================================

LLVMValueRef emit_pattern_coroutine(const char *pattern_str, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  (void)ctx;

  ParsedPattern pat;
  if (!parse_pattern(pattern_str, &pat)) {
    fprintf(stderr, "emit_pattern_coroutine: failed to parse '%s'\n",
            pattern_str);
    return NULL;
  }

  static int uid = 0;
  int my_id = uid++;
  char wrapper_name[64], reset_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "pattern_coro_%d", my_id);
  snprintf(reset_name, sizeof(reset_name), "pattern_coro_%d.reset", my_id);

  LLVMTypeRef yield_ty = LLVMDoubleType();
  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(yield_ty);

  // wrapper: ptr (ptr frame_size_out)
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR, (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0)}, 1, 0);
  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);
  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_bb = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");
  LLVMBuildStore(
      builder, LLVMConstInt(LLVMInt1Type(), 0, 0),
      LLVMBuildStructGEP2(builder, promise_type, promise_alloca, 1,
                          "is_done_ptr"));
  PROMISE_SET_RESET_FN(promise_alloca, promise_type, LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type, LLVMConstNull(GENERIC_PTR));

  // Alt-group counters — in entry block so LLVM spills them to the coro frame,
  // keeping values across loop iterations.
  LLVMValueRef *alt_counters =
      pat.num_alt_groups
          ? malloc(sizeof(LLVMValueRef) * pat.num_alt_groups)
          : NULL;
  for (int i = 0; i < pat.num_alt_groups; i++) {
    char name[32];
    snprintf(name, sizeof(name), "alt%d.cnt", i);
    alt_counters[i] = LLVMBuildAlloca(builder, LLVMInt32Type(), name);
    LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0),
                   alt_counters[i]);
  }

  LLVMValueRef coro_id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0), promise_alloca,
                       LLVMConstNull(GENERIC_PTR), LLVMConstNull(GENERIC_PTR)},
      4, "coro.id");

  LLVMValueRef coro_size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");
  LLVMBuildStore(builder, coro_size, LLVMGetParam(wrapper_fn, 0));

  LLVMValueRef frame =
      LLVMBuildArrayMalloc(builder, LLVMInt8Type(), coro_size, "coro.frame");
  LLVMValueRef handle = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_begin_intrinsic(module)),
      get_coro_begin_intrinsic(module), (LLVMValueRef[]){coro_id, frame}, 2,
      "coro.handle");

  // Initial suspend
  LLVMValueRef init_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1, "init.save");
  LLVMValueRef init_sus = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){init_save, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "init.suspend");
  LLVMValueRef init_sw =
      LLVMBuildSwitch(builder, init_sus, initial_return_bb, 2);
  LLVMAddCase(init_sw, LLVMConstInt(LLVMInt8Type(), 0, 0), start_bb);
  LLVMAddCase(init_sw, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, initial_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // === START BLOCK — emit one step at a time ===
  LLVMPositionBuilderAtEnd(builder, start_bb);

  for (int i = 0; i < pat.num_steps; i++) {
    PatternStep *step = &pat.steps[i];
    char lbl[64];

    if (!step->is_alt) {
      snprintf(lbl, sizeof(lbl), "s%d", i);
      emit_yield(wrapper_fn, module, builder, handle, promise_alloca,
                 LLVMConstReal(yield_ty, step->fixed_val), cleanup_bb,
                 suspend_bb, lbl);
    } else {
      // Compute which slot to use: cycling counter or random pick.
      LLVMValueRef slot_idx;
      if (step->is_rand) {
        // Call ylc_rand_int(num_slots) at runtime.
        LLVMTypeRef rand_fn_ty = LLVMFunctionType(
            LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
        LLVMValueRef rand_fn = LLVMGetNamedFunction(module, "ylc_rand_int");
        if (!rand_fn) {
          rand_fn = LLVMAddFunction(module, "ylc_rand_int", rand_fn_ty);
          LLVMSetLinkage(rand_fn, LLVMExternalLinkage);
        }
        LLVMValueRef n_val =
            LLVMConstInt(LLVMInt32Type(), step->num_slots, 0);
        slot_idx = LLVMBuildCall2(builder, rand_fn_ty, rand_fn, &n_val, 1,
                                  "rand.slot");
      } else {
        // Load counter, compute which slot, increment.
        LLVMValueRef cnt =
            LLVMBuildLoad2(builder, LLVMInt32Type(),
                           alt_counters[step->counter_idx], "alt.cnt");
        LLVMValueRef rem = LLVMBuildURem(
            builder, cnt,
            LLVMConstInt(LLVMInt32Type(), step->num_slots, 0), "alt.rem");
        LLVMBuildStore(builder,
                       LLVMBuildAdd(builder, cnt,
                                    LLVMConstInt(LLVMInt32Type(), 1, 0),
                                    "alt.next"),
                       alt_counters[step->counter_idx]);
        slot_idx = rem;
      }

      // Pre-create merge block and per-slot entry blocks.
      LLVMBasicBlockRef after_alt_bb =
          LLVMAppendBasicBlock(wrapper_fn, "after_alt");
      LLVMBasicBlockRef *slot_bbs =
          malloc(sizeof(LLVMBasicBlockRef) * step->num_slots);
      for (int j = 0; j < step->num_slots; j++) {
        snprintf(lbl, sizeof(lbl), "s%d.slot%d", i, j);
        slot_bbs[j] = LLVMAppendBasicBlock(wrapper_fn, lbl);
      }

      // Switch: use last slot as default, explicit cases for the rest.
      LLVMValueRef sw =
          LLVMBuildSwitch(builder, slot_idx, slot_bbs[step->num_slots - 1],
                          step->num_slots - 1);
      for (int j = 0; j < step->num_slots - 1; j++)
        LLVMAddCase(sw, LLVMConstInt(LLVMInt32Type(), j, 0), slot_bbs[j]);

      // Emit each slot's yield(s) then branch to after_alt.
      for (int j = 0; j < step->num_slots; j++) {
        LLVMPositionBuilderAtEnd(builder, slot_bbs[j]);
        Slot *slot = &step->slots[j];
        for (int k = 0; k < slot->count; k++) {
          snprintf(lbl, sizeof(lbl), "s%d.sl%d.v%d", i, j, k);
          emit_yield(wrapper_fn, module, builder, handle, promise_alloca,
                     LLVMConstReal(yield_ty, slot->vals[k]), cleanup_bb,
                     suspend_bb, lbl);
        }
        LLVMBuildBr(builder, after_alt_bb);
      }

      free(slot_bbs);
      LLVMPositionBuilderAtEnd(builder, after_alt_bb);
    }
  }

  // After all steps: loop back — alt counters persist in the coro frame.
  LLVMBuildBr(builder, start_bb);

  // === CLEANUP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);
  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){coro_id, handle}, 2,
      "coro.mem");
  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // === SUSPEND BLOCK ===
  LLVMPositionBuilderAtEnd(builder, suspend_bb);
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");
  LLVMBuildRet(builder, handle);

  // Restore caller insertion point and instantiate.
  LLVMPositionBuilderAtEnd(builder, prev_bb);

  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "pat.frame_size.out");
  LLVMValueRef coro_handle =
      LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                     (LLVMValueRef[]){frame_size_alloca}, 1, "pat.coro.handle");

  LLVMValueRef reset_fn = emit_reset_fn(promise_type, wrapper_fn_type,
                                        wrapper_fn, module, builder, reset_name);
  LLVMValueRef prom_ptr = GET_PROMISE_PTR(coro_handle, promise_type);
  PROMISE_SET_RESET_FN(prom_ptr, promise_type, reset_fn);
  PROMISE_SET_ARGS_PTR(prom_ptr, promise_type, LLVMConstNull(GENERIC_PTR));

  free(alt_counters);
  free_parsed(&pat);

  return coro_handle;
}

// ============================================================================
// Builtin handler — called from ylc via compile_audio_fn or similar
// ============================================================================

LLVMValueRef pattern_handler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  Ast *arg = ast->data.AST_APPLICATION.args;
  if (!arg || arg->tag != AST_STRING) {
    fprintf(stderr, "pattern: expected a string literal argument\n");
    return NULL;
  }
  const char *pattern_str = arg->data.AST_STRING.value;
  return emit_pattern_coroutine(pattern_str, ctx, module, builder);
}
