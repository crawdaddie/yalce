#include "./pattern_coroutine.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/coroutines/coroutines.h"
#include "../../lang/backend_llvm/strings.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/serde.h"
#include "../../lang/types/type_ser.h"
#include "llvm-c/Core.h"
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Pattern parser
//
// Grammar:
//   pattern  ::= step+
//   step     ::= number | '<' slot+ '>' | '{' slot+ '}'
//   slot     ::= number | '[' number+ ']' | '{' number+ '}'
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

#define MAX_STEPS 256
#define MAX_SLOTS 64
#define MAX_SEQ_VALS 64

typedef enum {
  STEP_SEQ,  // single fixed value
  STEP_ALT,  // cycling alternation  <>
  STEP_RAND, // random selection     {}
} PatternStepKind;

typedef struct {
  PatternStepKind kind; // STEP_SEQ = sequence/scalar, STEP_RAND = random pick
  double *vals;         // malloced; count=1 for scalar, N for [a b c] or {a b}
  int count;
} Slot;

typedef struct {
  PatternStepKind kind;
  // STEP_SEQ:
  double fixed_val;
  // STEP_ALT / STEP_RAND:
  Slot *slots; // malloced array of slots
  int num_slots;
  int counter_idx; // alt_counter alloca index (-1 for STEP_RAND)
} PatternStep;

typedef struct {
  PatternStep *steps; // malloced
  int num_steps;
  int num_alt_groups;
} ParsedPattern;

static void free_parsed(ParsedPattern *p) {
  for (int i = 0; i < p->num_steps; i++) {
    PatternStep *s = &p->steps[i];
    if (s->kind != STEP_SEQ) {
      for (int j = 0; j < s->num_slots; j++)
        free(s->slots[j].vals);
      free(s->slots);
    }
  }
  free(p->steps);
}

static void skip_ws(const char **pp) {
  while (**pp == ' ' || **pp == '\t' || **pp == '\n' || **pp == '\r')
    (*pp)++;
}

static bool parse_num_list(const char **pp, char close, double *tmp, int *n) {
  while (**pp && **pp != close) {
    skip_ws(pp);
    if (**pp == close)
      break;
    char *end;
    double v = strtod(*pp, &end);
    if (end == *pp)
      return false;
    if (*n >= MAX_SEQ_VALS)
      return false;
    tmp[(*n)++] = v;
    *pp = end;
  }
  if (**pp == close)
    (*pp)++;
  return *n > 0;
}

static bool parse_slot(const char **pp, Slot *out) {
  skip_ws(pp);
  if (**pp == '[' || **pp == '{') {
    char close = **pp == '[' ? ']' : '}';
    out->kind = **pp == '[' ? STEP_SEQ : STEP_RAND;
    (*pp)++;
    double tmp[MAX_SEQ_VALS];
    int n = 0;
    if (!parse_num_list(pp, close, tmp, &n))
      return false;
    out->count = n;
    out->vals = malloc(sizeof(double) * (size_t)n);
    memcpy(out->vals, tmp, sizeof(double) * (size_t)n);
    return true;
  } else {
    char *end;
    double v = strtod(*pp, &end);
    if (end == *pp)
      return false;
    *pp = end;
    out->kind = STEP_SEQ;
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
      step->kind = STEP_ALT;
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
      step->kind = STEP_RAND;
      step->fixed_val = 0.0;
      step->num_slots = num_slots;
      step->slots = malloc(sizeof(Slot) * num_slots);
      memcpy(step->slots, tmp_slots, sizeof(Slot) * num_slots);
      step->counter_idx = -1;
    } else {
      char *end;
      double v = strtod(p, &end);
      if (end == p)
        return false;
      p = end;
      PatternStep *step = &out->steps[out->num_steps++];
      step->kind = STEP_SEQ;
      step->fixed_val = v;
      step->slots = NULL;
      step->num_slots = 0;
      step->counter_idx = -1;
    }
  }
  return out->num_steps > 0;
}

// ============================================================================
// Key pattern parser (string tokens)
//
// Grammar (same grouping rules as numeric parser):
//   pattern  ::= step+
//   step     ::= token | '<' slot+ '>' | '{' slot+ '}'
//   slot     ::= token | '[' token+ ']' | '{' token+ '}'
//
// Tokens are whitespace-delimited and cannot include grouping delimiters
// (< > { } [ ]).
// ============================================================================

typedef struct {
  PatternStepKind kind; // STEP_SEQ = sequence/scalar, STEP_RAND = random pick
  char **vals;          // malloced token strings
  int count;
} KeySlot;

typedef struct {
  PatternStepKind kind;
  // STEP_SEQ:
  char *fixed_key; // malloced token string
  // STEP_ALT / STEP_RAND:
  KeySlot *slots; // malloced array of slots
  int num_slots;
  int counter_idx; // alt_counter alloca index (-1 for STEP_RAND)
} KeyPatternStep;

typedef struct {
  KeyPatternStep *steps; // malloced
  int num_steps;
  int num_alt_groups;
} ParsedKeyPattern;

static void free_parsed_key(ParsedKeyPattern *p) {
  for (int i = 0; i < p->num_steps; i++) {
    KeyPatternStep *s = &p->steps[i];
    if (s->kind != STEP_SEQ) {
      for (int j = 0; j < s->num_slots; j++) {
        for (int k = 0; k < s->slots[j].count; k++) {
          free(s->slots[j].vals[k]);
        }
        free(s->slots[j].vals);
      }
      free(s->slots);
    } else {
      free(s->fixed_key);
    }
  }
  free(p->steps);
}

static bool is_key_delim(char c) {
  return c == '<' || c == '>' || c == '{' || c == '}' || c == '[' || c == ']';
}

static bool parse_key_token(const char **pp, char **out) {
  skip_ws(pp);
  const char *start = *pp;
  if (!*start || is_key_delim(*start))
    return false;

  if (*start == '-' || *start == '.') {
    char *tok = malloc(2);
    tok[0] = *start;
    tok[1] = '\0';
    *out = tok;
    (*pp)++;
    return true;
  }

  if (isdigit((unsigned char)*start)) {
    char *tok = malloc(2);
    tok[0] = *start;
    tok[1] = '\0';
    *out = tok;
    (*pp)++;
    return true;
  }

  if (isalpha((unsigned char)*start)) {
    (*pp)++;
    while (**pp == '#' || **pp == '_' || isdigit((unsigned char)**pp))
      (*pp)++;

    int len = (int)(*pp - start);
    char *tok = malloc((size_t)len + 1);
    memcpy(tok, start, (size_t)len);
    tok[len] = '\0';
    *out = tok;
    return true;
  }

  while (**pp && **pp != ' ' && **pp != '\t' && **pp != '\n' && **pp != '\r' &&
         **pp != '-' && **pp != '.' && !is_key_delim(**pp))
    (*pp)++;

  int len = (int)(*pp - start);
  if (len <= 0)
    return false;

  char *tok = malloc((size_t)len + 1);
  memcpy(tok, start, (size_t)len);
  tok[len] = '\0';
  *out = tok;
  return true;
}

static bool parse_key_char_token(const char **pp, char **out) {
  skip_ws(pp);
  const char c = **pp;
  if (!c || is_key_delim(c))
    return false;

  char *tok = malloc(2);
  tok[0] = c;
  tok[1] = '\0';
  *out = tok;
  (*pp)++;
  return true;
}

static bool parse_key_token_list(const char **pp, char close, char **tmp,
                                 int *n) {
  while (**pp && **pp != close) {
    skip_ws(pp);
    if (**pp == close)
      break;
    if (*n >= MAX_SEQ_VALS)
      return false;
    if (!parse_key_token(pp, &tmp[*n])) {
      for (int i = 0; i < *n; i++)
        free(tmp[i]);
      return false;
    }
    (*n)++;
  }
  if (**pp == close)
    (*pp)++;
  return *n > 0;
}

static bool parse_key_char_token_list(const char **pp, char close, char **tmp,
                                      int *n) {
  while (**pp && **pp != close) {
    skip_ws(pp);
    if (**pp == close)
      break;
    if (*n >= MAX_SEQ_VALS)
      return false;
    if (!parse_key_char_token(pp, &tmp[*n])) {
      for (int i = 0; i < *n; i++)
        free(tmp[i]);
      return false;
    }
    (*n)++;
  }
  if (**pp == close)
    (*pp)++;
  return *n > 0;
}

static bool parse_key_slot(const char **pp, KeySlot *out) {
  skip_ws(pp);
  if (**pp == '[' || **pp == '{') {
    char close = **pp == '[' ? ']' : '}';
    out->kind = **pp == '[' ? STEP_SEQ : STEP_RAND;
    (*pp)++;
    char *tmp[MAX_SEQ_VALS];
    int n = 0;
    if (!parse_key_token_list(pp, close, tmp, &n))
      return false;
    out->count = n;
    out->vals = malloc(sizeof(char *) * (size_t)n);
    memcpy(out->vals, tmp, sizeof(char *) * (size_t)n);
    return true;
  } else {
    char *tok;
    if (!parse_key_token(pp, &tok))
      return false;
    out->kind = STEP_SEQ;
    out->count = 1;
    out->vals = malloc(sizeof(char *));
    out->vals[0] = tok;
    return true;
  }
}

static bool parse_key_pattern(const char *src, ParsedKeyPattern *out) {
  out->steps = calloc(MAX_STEPS, sizeof(KeyPatternStep));
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
      KeySlot tmp_slots[MAX_SLOTS];
      int num_slots = 0;
      while (*p && *p != '>') {
        skip_ws(&p);
        if (*p == '>')
          break;
        if (num_slots >= MAX_SLOTS)
          return false;
        if (!parse_key_slot(&p, &tmp_slots[num_slots++]))
          return false;
      }
      if (*p == '>')
        p++;
      if (num_slots == 0)
        return false;

      KeyPatternStep *step = &out->steps[out->num_steps++];
      step->kind = STEP_ALT;
      step->fixed_key = NULL;
      step->num_slots = num_slots;
      step->slots = malloc(sizeof(KeySlot) * (size_t)num_slots);
      memcpy(step->slots, tmp_slots, sizeof(KeySlot) * (size_t)num_slots);
      step->counter_idx = out->num_alt_groups++;
    } else if (*p == '{') {
      p++;
      KeySlot tmp_slots[MAX_SLOTS];
      int num_slots = 0;
      while (*p && *p != '}') {
        skip_ws(&p);
        if (*p == '}')
          break;
        if (num_slots >= MAX_SLOTS)
          return false;
        if (!parse_key_slot(&p, &tmp_slots[num_slots++]))
          return false;
      }
      if (*p == '}')
        p++;
      if (num_slots == 0)
        return false;

      KeyPatternStep *step = &out->steps[out->num_steps++];
      step->kind = STEP_RAND;
      step->fixed_key = NULL;
      step->num_slots = num_slots;
      step->slots = malloc(sizeof(KeySlot) * (size_t)num_slots);
      memcpy(step->slots, tmp_slots, sizeof(KeySlot) * (size_t)num_slots);
      step->counter_idx = -1;
    } else {
      char *tok;
      if (!parse_key_token(&p, &tok))
        return false;

      KeyPatternStep *step = &out->steps[out->num_steps++];
      step->kind = STEP_SEQ;
      step->fixed_key = tok;
      step->slots = NULL;
      step->num_slots = 0;
      step->counter_idx = -1;
    }
  }
  return out->num_steps > 0;
}

static bool parse_key_char_slot(const char **pp, KeySlot *out) {
  skip_ws(pp);
  if (**pp == '[' || **pp == '{') {
    char close = **pp == '[' ? ']' : '}';
    out->kind = **pp == '[' ? STEP_SEQ : STEP_RAND;
    (*pp)++;
    char *tmp[MAX_SEQ_VALS];
    int n = 0;
    if (!parse_key_char_token_list(pp, close, tmp, &n))
      return false;
    out->count = n;
    out->vals = malloc(sizeof(char *) * (size_t)n);
    memcpy(out->vals, tmp, sizeof(char *) * (size_t)n);
    return true;
  } else {
    char *tok;
    if (!parse_key_char_token(pp, &tok))
      return false;
    out->kind = STEP_SEQ;
    out->count = 1;
    out->vals = malloc(sizeof(char *));
    out->vals[0] = tok;
    return true;
  }
}

static bool parse_key_char_pattern(const char *src, ParsedKeyPattern *out) {
  out->steps = calloc(MAX_STEPS, sizeof(KeyPatternStep));
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
      KeySlot tmp_slots[MAX_SLOTS];
      int num_slots = 0;
      while (*p && *p != '>') {
        skip_ws(&p);
        if (*p == '>')
          break;
        if (num_slots >= MAX_SLOTS)
          return false;
        if (!parse_key_char_slot(&p, &tmp_slots[num_slots++]))
          return false;
      }
      if (*p == '>')
        p++;
      if (num_slots == 0)
        return false;

      KeyPatternStep *step = &out->steps[out->num_steps++];
      step->kind = STEP_ALT;
      step->fixed_key = NULL;
      step->num_slots = num_slots;
      step->slots = malloc(sizeof(KeySlot) * (size_t)num_slots);
      memcpy(step->slots, tmp_slots, sizeof(KeySlot) * (size_t)num_slots);
      step->counter_idx = out->num_alt_groups++;
    } else if (*p == '{') {
      p++;
      KeySlot tmp_slots[MAX_SLOTS];
      int num_slots = 0;
      while (*p && *p != '}') {
        skip_ws(&p);
        if (*p == '}')
          break;
        if (num_slots >= MAX_SLOTS)
          return false;
        if (!parse_key_char_slot(&p, &tmp_slots[num_slots++]))
          return false;
      }
      if (*p == '}')
        p++;
      if (num_slots == 0)
        return false;

      KeyPatternStep *step = &out->steps[out->num_steps++];
      step->kind = STEP_RAND;
      step->fixed_key = NULL;
      step->num_slots = num_slots;
      step->slots = malloc(sizeof(KeySlot) * (size_t)num_slots);
      memcpy(step->slots, tmp_slots, sizeof(KeySlot) * (size_t)num_slots);
      step->counter_idx = -1;
    } else {
      char *tok;
      if (!parse_key_char_token(&p, &tok))
        return false;

      KeyPatternStep *step = &out->steps[out->num_steps++];
      step->kind = STEP_SEQ;
      step->fixed_key = tok;
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
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0),
                 LLVMBuildStructGEP2(builder, promise_type, promise_alloca, 1,
                                     "is_done_ptr"));
  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

  // Alt-group counters — in entry block so LLVM spills them to the coro frame,
  // keeping values across loop iterations.
  LLVMValueRef *alt_counters =
      pat.num_alt_groups ? malloc(sizeof(LLVMValueRef) * pat.num_alt_groups)
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
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "init.save");
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

    if (step->kind == STEP_SEQ) {
      snprintf(lbl, sizeof(lbl), "s%d", i);
      emit_yield(wrapper_fn, module, builder, handle, promise_alloca,
                 LLVMConstReal(yield_ty, step->fixed_val), cleanup_bb,
                 suspend_bb, lbl);
    } else {
      // Compute which slot to use: cycling counter or random pick.
      LLVMValueRef slot_idx;
      if (step->kind == STEP_RAND) {
        // Call ylc_rand_int(num_slots) at runtime.
        LLVMTypeRef rand_fn_ty = LLVMFunctionType(
            LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
        LLVMValueRef rand_fn = LLVMGetNamedFunction(module, "ylc_rand_int");
        if (!rand_fn) {
          rand_fn = LLVMAddFunction(module, "ylc_rand_int", rand_fn_ty);
          LLVMSetLinkage(rand_fn, LLVMExternalLinkage);
        }
        LLVMValueRef n_val = LLVMConstInt(LLVMInt32Type(), step->num_slots, 0);
        slot_idx = LLVMBuildCall2(builder, rand_fn_ty, rand_fn, &n_val, 1,
                                  "rand.slot");
      } else {
        // Load counter, compute which slot, increment.
        LLVMValueRef cnt =
            LLVMBuildLoad2(builder, LLVMInt32Type(),
                           alt_counters[step->counter_idx], "alt.cnt");
        LLVMValueRef rem = LLVMBuildURem(
            builder, cnt, LLVMConstInt(LLVMInt32Type(), step->num_slots, 0),
            "alt.rem");
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
        if (slot->kind == STEP_RAND) {
          // Random pick within this slot: ylc_rand_int(count) → switch → yield
          // one value.
          LLVMTypeRef rand_fn_ty = LLVMFunctionType(
              LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
          LLVMValueRef rand_fn = LLVMGetNamedFunction(module, "ylc_rand_int");
          if (!rand_fn) {
            rand_fn = LLVMAddFunction(module, "ylc_rand_int", rand_fn_ty);
            LLVMSetLinkage(rand_fn, LLVMExternalLinkage);
          }
          LLVMValueRef n_val = LLVMConstInt(LLVMInt32Type(), slot->count, 0);
          LLVMValueRef rnd = LLVMBuildCall2(builder, rand_fn_ty, rand_fn,
                                            &n_val, 1, "slot.rnd");
          LLVMBasicBlockRef after_rnd_bb =
              LLVMAppendBasicBlock(wrapper_fn, "slot.after_rnd");
          LLVMBasicBlockRef *val_bbs =
              malloc(sizeof(LLVMBasicBlockRef) * (size_t)slot->count);
          for (int k = 0; k < slot->count; k++) {
            snprintf(lbl, sizeof(lbl), "s%d.sl%d.rnd%d", i, j, k);
            val_bbs[k] = LLVMAppendBasicBlock(wrapper_fn, lbl);
          }
          LLVMValueRef rsw = LLVMBuildSwitch(
              builder, rnd, val_bbs[slot->count - 1], slot->count - 1);
          for (int k = 0; k < slot->count - 1; k++)
            LLVMAddCase(rsw, LLVMConstInt(LLVMInt32Type(), k, 0), val_bbs[k]);
          for (int k = 0; k < slot->count; k++) {
            LLVMPositionBuilderAtEnd(builder, val_bbs[k]);
            snprintf(lbl, sizeof(lbl), "s%d.sl%d.rv%d", i, j, k);
            emit_yield(wrapper_fn, module, builder, handle, promise_alloca,
                       LLVMConstReal(yield_ty, slot->vals[k]), cleanup_bb,
                       suspend_bb, lbl);
            LLVMBuildBr(builder, after_rnd_bb);
          }
          free(val_bbs);
          LLVMPositionBuilderAtEnd(builder, after_rnd_bb);
        } else {
          for (int k = 0; k < slot->count; k++) {
            snprintf(lbl, sizeof(lbl), "s%d.sl%d.v%d", i, j, k);
            emit_yield(wrapper_fn, module, builder, handle, promise_alloca,
                       LLVMConstReal(yield_ty, slot->vals[k]), cleanup_bb,
                       suspend_bb, lbl);
          }
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

  LLVMValueRef reset_fn = emit_reset_fn(
      promise_type, wrapper_fn_type, wrapper_fn, module, builder, reset_name);
  LLVMValueRef prom_ptr = GET_PROMISE_PTR(coro_handle, promise_type);
  PROMISE_SET_RESET_FN(prom_ptr, promise_type, reset_fn);
  PROMISE_SET_ARGS_PTR(prom_ptr, promise_type, LLVMConstNull(GENERIC_PTR));

  free(alt_counters);
  free_parsed(&pat);

  return coro_handle;
}

static LLVMValueRef codegen_const_token_string(const char *token,
                                               LLVMTypeRef yield_ty,
                                               LLVMBuilderRef builder) {
  static int lit_uid = 0;
  char global_name[64];
  snprintf(global_name, sizeof(global_name), "pat_key_tok_%d", lit_uid++);

  LLVMValueRef data_ptr = LLVMBuildGlobalStringPtr(builder, token, global_name);
  LLVMValueRef str_val = LLVMGetUndef(yield_ty);
  str_val = LLVMBuildInsertValue(
      builder, str_val, LLVMConstInt(LLVMInt32Type(), strlen(token), 0), 0,
      "pat_key_len");
  str_val = LLVMBuildInsertValue(builder, str_val, data_ptr, 1, "pat_key_data");
  return str_val;
}

LLVMValueRef emit_key_pattern_coroutine(const char *pattern_str,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  (void)ctx;

  ParsedKeyPattern pat;
  if (!parse_key_pattern(pattern_str, &pat)) {
    fprintf(stderr, "emit_key_pattern_coroutine: failed to parse '%s'\n",
            pattern_str);
    return NULL;
  }

  static int uid = 0;
  int my_id = uid++;
  char wrapper_name[64], reset_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "pattern_key_coro_%d", my_id);
  snprintf(reset_name, sizeof(reset_name), "pattern_key_coro_%d.reset", my_id);

  LLVMTypeRef yield_ty = string_struct_type(LLVMPointerType(LLVMInt8Type(), 0));
  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(yield_ty);

  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR, (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0)}, 1, 0);
  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);
  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_bb = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0),
                 LLVMBuildStructGEP2(builder, promise_type, promise_alloca, 1,
                                     "is_done_ptr"));
  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

  LLVMValueRef *alt_counters =
      pat.num_alt_groups
          ? malloc(sizeof(LLVMValueRef) * (size_t)pat.num_alt_groups)
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

  LLVMValueRef init_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "init.save");
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

  LLVMPositionBuilderAtEnd(builder, start_bb);

  for (int i = 0; i < pat.num_steps; i++) {
    KeyPatternStep *step = &pat.steps[i];
    char lbl[64];

    if (step->kind == STEP_SEQ) {
      snprintf(lbl, sizeof(lbl), "k%d", i);
      emit_yield(wrapper_fn, module, builder, handle, promise_alloca,
                 codegen_const_token_string(step->fixed_key, yield_ty, builder),
                 cleanup_bb, suspend_bb, lbl);
    } else {
      LLVMValueRef slot_idx;
      if (step->kind == STEP_RAND) {
        LLVMTypeRef rand_fn_ty = LLVMFunctionType(
            LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
        LLVMValueRef rand_fn = LLVMGetNamedFunction(module, "ylc_rand_int");
        if (!rand_fn) {
          rand_fn = LLVMAddFunction(module, "ylc_rand_int", rand_fn_ty);
          LLVMSetLinkage(rand_fn, LLVMExternalLinkage);
        }
        LLVMValueRef n_val = LLVMConstInt(LLVMInt32Type(), step->num_slots, 0);
        slot_idx = LLVMBuildCall2(builder, rand_fn_ty, rand_fn, &n_val, 1,
                                  "rand.slot");
      } else {
        LLVMValueRef cnt =
            LLVMBuildLoad2(builder, LLVMInt32Type(),
                           alt_counters[step->counter_idx], "alt.cnt");
        LLVMValueRef rem = LLVMBuildURem(
            builder, cnt, LLVMConstInt(LLVMInt32Type(), step->num_slots, 0),
            "alt.rem");
        LLVMBuildStore(builder,
                       LLVMBuildAdd(builder, cnt,
                                    LLVMConstInt(LLVMInt32Type(), 1, 0),
                                    "alt.next"),
                       alt_counters[step->counter_idx]);
        slot_idx = rem;
      }

      LLVMBasicBlockRef after_alt_bb =
          LLVMAppendBasicBlock(wrapper_fn, "after_alt");
      LLVMBasicBlockRef *slot_bbs =
          malloc(sizeof(LLVMBasicBlockRef) * (size_t)step->num_slots);
      for (int j = 0; j < step->num_slots; j++) {
        snprintf(lbl, sizeof(lbl), "k%d.slot%d", i, j);
        slot_bbs[j] = LLVMAppendBasicBlock(wrapper_fn, lbl);
      }

      LLVMValueRef sw =
          LLVMBuildSwitch(builder, slot_idx, slot_bbs[step->num_slots - 1],
                          step->num_slots - 1);
      for (int j = 0; j < step->num_slots - 1; j++)
        LLVMAddCase(sw, LLVMConstInt(LLVMInt32Type(), j, 0), slot_bbs[j]);

      for (int j = 0; j < step->num_slots; j++) {
        LLVMPositionBuilderAtEnd(builder, slot_bbs[j]);
        KeySlot *slot = &step->slots[j];
        if (slot->kind == STEP_RAND) {
          LLVMTypeRef rand_fn_ty = LLVMFunctionType(
              LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
          LLVMValueRef rand_fn = LLVMGetNamedFunction(module, "ylc_rand_int");
          if (!rand_fn) {
            rand_fn = LLVMAddFunction(module, "ylc_rand_int", rand_fn_ty);
            LLVMSetLinkage(rand_fn, LLVMExternalLinkage);
          }
          LLVMValueRef n_val = LLVMConstInt(LLVMInt32Type(), slot->count, 0);
          LLVMValueRef rnd = LLVMBuildCall2(builder, rand_fn_ty, rand_fn,
                                            &n_val, 1, "kslot.rnd");
          LLVMBasicBlockRef after_rnd_bb =
              LLVMAppendBasicBlock(wrapper_fn, "kslot.after_rnd");
          LLVMBasicBlockRef *val_bbs =
              malloc(sizeof(LLVMBasicBlockRef) * (size_t)slot->count);
          for (int k = 0; k < slot->count; k++) {
            snprintf(lbl, sizeof(lbl), "k%d.sl%d.rnd%d", i, j, k);
            val_bbs[k] = LLVMAppendBasicBlock(wrapper_fn, lbl);
          }
          LLVMValueRef rsw = LLVMBuildSwitch(
              builder, rnd, val_bbs[slot->count - 1], slot->count - 1);
          for (int k = 0; k < slot->count - 1; k++)
            LLVMAddCase(rsw, LLVMConstInt(LLVMInt32Type(), k, 0), val_bbs[k]);
          for (int k = 0; k < slot->count; k++) {
            LLVMPositionBuilderAtEnd(builder, val_bbs[k]);
            snprintf(lbl, sizeof(lbl), "k%d.sl%d.rv%d", i, j, k);
            emit_yield(
                wrapper_fn, module, builder, handle, promise_alloca,
                codegen_const_token_string(slot->vals[k], yield_ty, builder),
                cleanup_bb, suspend_bb, lbl);
            LLVMBuildBr(builder, after_rnd_bb);
          }
          free(val_bbs);
          LLVMPositionBuilderAtEnd(builder, after_rnd_bb);
        } else {
          for (int k = 0; k < slot->count; k++) {
            snprintf(lbl, sizeof(lbl), "k%d.sl%d.v%d", i, j, k);
            emit_yield(
                wrapper_fn, module, builder, handle, promise_alloca,
                codegen_const_token_string(slot->vals[k], yield_ty, builder),
                cleanup_bb, suspend_bb, lbl);
          }
        }
        LLVMBuildBr(builder, after_alt_bb);
      }

      free(slot_bbs);
      LLVMPositionBuilderAtEnd(builder, after_alt_bb);
    }
  }

  LLVMBuildBr(builder, start_bb);

  LLVMPositionBuilderAtEnd(builder, cleanup_bb);
  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){coro_id, handle}, 2,
      "coro.mem");
  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_bb);
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");
  LLVMBuildRet(builder, handle);

  LLVMPositionBuilderAtEnd(builder, prev_bb);

  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "pat_key.frame_size.out");
  LLVMValueRef coro_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn, (LLVMValueRef[]){frame_size_alloca},
      1, "pat_key.coro.handle");

  LLVMValueRef reset_fn = emit_reset_fn(
      promise_type, wrapper_fn_type, wrapper_fn, module, builder, reset_name);
  LLVMValueRef prom_ptr = GET_PROMISE_PTR(coro_handle, promise_type);
  PROMISE_SET_RESET_FN(prom_ptr, promise_type, reset_fn);
  PROMISE_SET_ARGS_PTR(prom_ptr, promise_type, LLVMConstNull(GENERIC_PTR));

  free(alt_counters);
  free_parsed_key(&pat);

  return coro_handle;
}

LLVMValueRef emit_key_char_pattern_coroutine(const char *pattern_str,
                                             JITLangCtx *ctx,
                                             LLVMModuleRef module,
                                             LLVMBuilderRef builder) {
  (void)ctx;

  ParsedKeyPattern pat;
  if (!parse_key_char_pattern(pattern_str, &pat)) {
    fprintf(stderr, "emit_key_char_pattern_coroutine: failed to parse '%s'\n",
            pattern_str);
    return NULL;
  }

  static int uid = 0;
  int my_id = uid++;
  char wrapper_name[64], reset_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "pattern_key_chars_coro_%d",
           my_id);
  snprintf(reset_name, sizeof(reset_name), "pattern_key_chars_coro_%d.reset",
           my_id);

  LLVMTypeRef yield_ty = string_struct_type(LLVMPointerType(LLVMInt8Type(), 0));
  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(yield_ty);

  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR, (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0)}, 1, 0);
  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);
  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_bb = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0),
                 LLVMBuildStructGEP2(builder, promise_type, promise_alloca, 1,
                                     "is_done_ptr"));
  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

  LLVMValueRef *alt_counters =
      pat.num_alt_groups
          ? malloc(sizeof(LLVMValueRef) * (size_t)pat.num_alt_groups)
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

  LLVMValueRef init_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "init.save");
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

  LLVMPositionBuilderAtEnd(builder, start_bb);

  for (int i = 0; i < pat.num_steps; i++) {
    KeyPatternStep *step = &pat.steps[i];
    char lbl[64];

    if (step->kind == STEP_SEQ) {
      snprintf(lbl, sizeof(lbl), "kc%d", i);
      emit_yield(wrapper_fn, module, builder, handle, promise_alloca,
                 codegen_const_token_string(step->fixed_key, yield_ty, builder),
                 cleanup_bb, suspend_bb, lbl);
    } else {
      LLVMValueRef slot_idx;
      if (step->kind == STEP_RAND) {
        LLVMTypeRef rand_fn_ty = LLVMFunctionType(
            LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
        LLVMValueRef rand_fn = LLVMGetNamedFunction(module, "ylc_rand_int");
        if (!rand_fn) {
          rand_fn = LLVMAddFunction(module, "ylc_rand_int", rand_fn_ty);
          LLVMSetLinkage(rand_fn, LLVMExternalLinkage);
        }
        LLVMValueRef n_val = LLVMConstInt(LLVMInt32Type(), step->num_slots, 0);
        slot_idx = LLVMBuildCall2(builder, rand_fn_ty, rand_fn, &n_val, 1,
                                  "rand.slot");
      } else {
        LLVMValueRef cnt =
            LLVMBuildLoad2(builder, LLVMInt32Type(),
                           alt_counters[step->counter_idx], "alt.cnt");
        LLVMValueRef rem = LLVMBuildURem(
            builder, cnt, LLVMConstInt(LLVMInt32Type(), step->num_slots, 0),
            "alt.rem");
        LLVMBuildStore(builder,
                       LLVMBuildAdd(builder, cnt,
                                    LLVMConstInt(LLVMInt32Type(), 1, 0),
                                    "alt.next"),
                       alt_counters[step->counter_idx]);
        slot_idx = rem;
      }

      LLVMBasicBlockRef after_alt_bb =
          LLVMAppendBasicBlock(wrapper_fn, "after_alt");
      LLVMBasicBlockRef *slot_bbs =
          malloc(sizeof(LLVMBasicBlockRef) * (size_t)step->num_slots);
      for (int j = 0; j < step->num_slots; j++) {
        snprintf(lbl, sizeof(lbl), "kc%d.slot%d", i, j);
        slot_bbs[j] = LLVMAppendBasicBlock(wrapper_fn, lbl);
      }

      LLVMValueRef sw =
          LLVMBuildSwitch(builder, slot_idx, slot_bbs[step->num_slots - 1],
                          step->num_slots - 1);
      for (int j = 0; j < step->num_slots - 1; j++)
        LLVMAddCase(sw, LLVMConstInt(LLVMInt32Type(), j, 0), slot_bbs[j]);

      for (int j = 0; j < step->num_slots; j++) {
        LLVMPositionBuilderAtEnd(builder, slot_bbs[j]);
        KeySlot *slot = &step->slots[j];
        if (slot->kind == STEP_RAND) {
          LLVMTypeRef rand_fn_ty = LLVMFunctionType(
              LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
          LLVMValueRef rand_fn = LLVMGetNamedFunction(module, "ylc_rand_int");
          if (!rand_fn) {
            rand_fn = LLVMAddFunction(module, "ylc_rand_int", rand_fn_ty);
            LLVMSetLinkage(rand_fn, LLVMExternalLinkage);
          }
          LLVMValueRef n_val = LLVMConstInt(LLVMInt32Type(), slot->count, 0);
          LLVMValueRef rnd = LLVMBuildCall2(builder, rand_fn_ty, rand_fn,
                                            &n_val, 1, "kslot.rnd");
          LLVMBasicBlockRef after_rnd_bb =
              LLVMAppendBasicBlock(wrapper_fn, "kslot.after_rnd");
          LLVMBasicBlockRef *val_bbs =
              malloc(sizeof(LLVMBasicBlockRef) * (size_t)slot->count);
          for (int k = 0; k < slot->count; k++) {
            snprintf(lbl, sizeof(lbl), "kc%d.sl%d.rnd%d", i, j, k);
            val_bbs[k] = LLVMAppendBasicBlock(wrapper_fn, lbl);
          }
          LLVMValueRef rsw = LLVMBuildSwitch(
              builder, rnd, val_bbs[slot->count - 1], slot->count - 1);
          for (int k = 0; k < slot->count - 1; k++)
            LLVMAddCase(rsw, LLVMConstInt(LLVMInt32Type(), k, 0), val_bbs[k]);
          for (int k = 0; k < slot->count; k++) {
            LLVMPositionBuilderAtEnd(builder, val_bbs[k]);
            snprintf(lbl, sizeof(lbl), "kc%d.sl%d.rv%d", i, j, k);
            emit_yield(
                wrapper_fn, module, builder, handle, promise_alloca,
                codegen_const_token_string(slot->vals[k], yield_ty, builder),
                cleanup_bb, suspend_bb, lbl);
            LLVMBuildBr(builder, after_rnd_bb);
          }
          free(val_bbs);
          LLVMPositionBuilderAtEnd(builder, after_rnd_bb);
        } else {
          for (int k = 0; k < slot->count; k++) {
            snprintf(lbl, sizeof(lbl), "kc%d.sl%d.v%d", i, j, k);
            emit_yield(
                wrapper_fn, module, builder, handle, promise_alloca,
                codegen_const_token_string(slot->vals[k], yield_ty, builder),
                cleanup_bb, suspend_bb, lbl);
          }
        }
        LLVMBuildBr(builder, after_alt_bb);
      }

      free(slot_bbs);
      LLVMPositionBuilderAtEnd(builder, after_alt_bb);
    }
  }

  LLVMBuildBr(builder, start_bb);

  LLVMPositionBuilderAtEnd(builder, cleanup_bb);
  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){coro_id, handle}, 2,
      "coro.mem");
  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_bb);
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");
  LLVMBuildRet(builder, handle);

  LLVMPositionBuilderAtEnd(builder, prev_bb);

  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "pat_key_chars.frame_size.out");
  LLVMValueRef coro_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn, (LLVMValueRef[]){frame_size_alloca},
      1, "pat_key_chars.coro.handle");

  LLVMValueRef reset_fn = emit_reset_fn(
      promise_type, wrapper_fn_type, wrapper_fn, module, builder, reset_name);
  LLVMValueRef prom_ptr = GET_PROMISE_PTR(coro_handle, promise_type);
  PROMISE_SET_RESET_FN(prom_ptr, promise_type, reset_fn);
  PROMISE_SET_ARGS_PTR(prom_ptr, promise_type, LLVMConstNull(GENERIC_PTR));

  free(alt_counters);
  free_parsed_key(&pat);

  return coro_handle;
}

// ============================================================================
// Builtin handler — called from ylc via compile_audio_fn or similar
// ============================================================================

static const char *ast_to_pattern_string(Ast *arg) {
  if (!arg)
    return NULL;
  if (arg->tag == AST_STRING)
    return arg->data.AST_STRING.value;
  if (arg->tag == AST_FMT_STRING && arg->data.AST_LIST.len > 0 &&
      arg->data.AST_LIST.items[0].tag == AST_STRING)
    return arg->data.AST_LIST.items[0].data.AST_STRING.value;
  return NULL;
}

LLVMValueRef pattern_handler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  const char *pattern_str =
      ast_to_pattern_string(ast->data.AST_APPLICATION.args);
  if (!pattern_str) {
    fprintf(stderr, "pat: expected a string or format-string literal\n");
    return NULL;
  }
  return emit_pattern_coroutine(pattern_str, ctx, module, builder);
}

LLVMValueRef pattern_key_handler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  const char *pattern_str =
      ast_to_pattern_string(ast->data.AST_APPLICATION.args);
  if (!pattern_str) {
    fprintf(stderr, "pat_key: expected a string or format-string literal\n");
    return NULL;
  }
  return emit_key_pattern_coroutine(pattern_str, ctx, module, builder);
}

LLVMValueRef pattern_key_chars_handler(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  const char *pattern_str =
      ast_to_pattern_string(ast->data.AST_APPLICATION.args);
  if (!pattern_str) {
    fprintf(stderr,
            "pat_key_chars: expected a string or format-string literal\n");
    return NULL;
  }
  return emit_key_char_pattern_coroutine(pattern_str, ctx, module, builder);
}

static LLVMValueRef _cor_stop(Type *cor_type, LLVMValueRef handle_raw,
                              JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef current_fn = LLVMGetBasicBlockParent(current_bb);

  LLVMBasicBlockRef check_resume_bb =
      LLVMAppendBasicBlock(current_fn, "check_resume");
  LLVMBasicBlockRef set_flag_bb =
      LLVMAppendBasicBlock(current_fn, "set_done_flag");
  LLVMBasicBlockRef done_bb = LLVMAppendBasicBlock(current_fn, "cor_stop_done");

  // Check if the handle is an integer or pointer type
  LLVMTypeRef handle_type = LLVMTypeOf(handle_raw);
  LLVMTypeKind type_kind = LLVMGetTypeKind(handle_type);

  LLVMValueRef handle;
  LLVMValueRef is_null_or_zero;

  if (type_kind == LLVMIntegerTypeKind) {
    // Handle is an integer (e.g., i64 from Uint64 0)
    // Check if it's 0
    is_null_or_zero = LLVMBuildICmp(builder, LLVMIntEQ, handle_raw,
                                    LLVMConstInt(handle_type, 0, 0), "is_zero");
    // Cast to pointer for further use
    handle = LLVMBuildIntToPtr(builder, handle_raw, GENERIC_PTR, "handle_ptr");
  } else {
    // Handle is already a pointer type
    handle = handle_raw;
    is_null_or_zero = LLVMBuildIsNull(builder, handle, "handle_is_null");
  }

  // If handle is null/zero, skip to done
  LLVMBuildCondBr(builder, is_null_or_zero, done_bb, check_resume_bb);

  // Check if resume function pointer is null
  LLVMPositionBuilderAtEnd(builder, check_resume_bb);
  LLVMValueRef resume_fn_ptr =
      LLVMBuildLoad2(builder, GENERIC_PTR, handle, "resume_fn");
  LLVMValueRef resume_is_null =
      LLVMBuildIsNull(builder, resume_fn_ptr, "resume_is_null");

  // If resume is null, skip to done, otherwise set the flag
  LLVMBuildCondBr(builder, resume_is_null, done_bb, set_flag_bb);

  // Set the is_done flag
  LLVMPositionBuilderAtEnd(builder, set_flag_bb);
  Type *yield_type = cor_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);
  LLVMTypeRef prom_type = CORO_PROMISE_TYPE(llvm_yield_type);

  LLVMValueRef prom_ptr = GET_PROMISE_PTR(handle, prom_type);
  LLVMValueRef is_done_flag_ptr =
      LLVMBuildStructGEP2(builder, prom_type, prom_ptr, 1, "get_is_done_flag");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 1, 0), is_done_flag_ptr);

  LLVMBuildBr(builder, done_bb);

  // Done
  LLVMPositionBuilderAtEnd(builder, done_bb);

  // LLVMValueRef coro_destroy = get_coro_destroy_intrinsic(module);
  //
  // LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_destroy), coro_destroy,
  //                (LLVMValueRef[]){handle}, 1, "");

  // cor_stop returns unit/void
  return LLVMGetUndef(LLVMVoidType());
}

int STYPE_AUDIO_JIT_LIVE_PATTERN;

typedef struct live_pattern_state_t {
  double quant;
  void *coroutine;
} live_pattern_state_t;

typedef struct live_pattern_state_entry_t {
  char *name;
  live_pattern_state_t *state;
  struct live_pattern_state_entry_t *next;
} live_pattern_state_entry_t;

static live_pattern_state_entry_t *live_pattern_states = NULL;

static LLVMTypeRef live_pattern_struct_type(void) {
  return LLVMStructType(
      (LLVMTypeRef[]){
          LLVMDoubleType(),
          GENERIC_PTR,
      },
      2, 0);
}

static LLVMValueRef ensure_get_current_sample_fn(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "get_current_sample");
  if (!fn) {
    fn = LLVMAddFunction(module, "get_current_sample",
                         LLVMFunctionType(LLVMInt64Type(), NULL, 0, 0));
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }
  return fn;
}

static LLVMValueRef ensure_ctx_sample_rate_fn(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "ctx_sample_rate");
  if (!fn) {
    fn = LLVMAddFunction(module, "ctx_sample_rate",
                         LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0));
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }
  return fn;
}

static LLVMTypeRef scheduler_callback_type(void) {
  return LLVMFunctionType(LLVMVoidType(),
                          (LLVMTypeRef[]){GENERIC_PTR, LLVMInt64Type()}, 2, 0);
}

static LLVMTypeRef defer_quant_callback_type(void) {
  return LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){LLVMInt64Type()}, 1,
                          0);
}

static LLVMTypeRef schedule_event_type(void) {
  return LLVMFunctionType(GENERIC_PTR,
                          (LLVMTypeRef[]){LLVMInt64Type(), LLVMDoubleType(),
                                          GENERIC_PTR, GENERIC_PTR},
                          4, 0);
}

static LLVMValueRef ensure_schedule_event_fn(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "schedule_event");
  if (!fn) {
    fn = LLVMAddFunction(module, "schedule_event", schedule_event_type());
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }
  return fn;
}

static LLVMTypeRef push_event_type(void) {
  return LLVMFunctionType(LLVMVoidType(),
                          (LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR,
                                          LLVMInt64Type(), LLVMInt64Type()},
                          4, 0);
}

static LLVMValueRef ensure_push_event_fn(LLVMModuleRef module) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, "push_event");
  if (!fn) {
    fn = LLVMAddFunction(module, "push_event", push_event_type());
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }
  return fn;
}

static live_pattern_state_t *get_or_create_live_pattern_state(const char *name) {
  for (live_pattern_state_entry_t *entry = live_pattern_states; entry != NULL;
       entry = entry->next) {
    if (strcmp(entry->name, name) == 0) {
      return entry->state;
    }
  }

  live_pattern_state_entry_t *entry = calloc(1, sizeof(*entry));
  entry->name = strdup(name);
  entry->state = calloc(1, sizeof(*entry->state));
  entry->next = live_pattern_states;
  live_pattern_states = entry;
  return entry->state;
}

static LLVMValueRef codegen_live_pattern_state_ptr(live_pattern_state_t *state,
                                                   LLVMModuleRef module) {
  (void)module;
  LLVMTypeRef state_ptr_ty = LLVMPointerType(live_pattern_struct_type(), 0);
  LLVMValueRef addr =
      LLVMConstInt(LLVMInt64Type(), (uintptr_t)state, 0);
  return LLVMConstIntToPtr(addr, state_ptr_ty);
}

static LLVMValueRef emit_defer_quant_ir(LLVMValueRef quant,
                                        LLVMValueRef callback_fn,
                                        LLVMValueRef callback_userdata,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  LLVMValueRef get_current_sample_fn = ensure_get_current_sample_fn(module);
  LLVMValueRef ctx_sample_rate_fn = ensure_ctx_sample_rate_fn(module);
  LLVMValueRef push_event_fn = ensure_push_event_fn(module);

  LLVMValueRef now =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(get_current_sample_fn),
                     get_current_sample_fn, NULL, 0, "defer_quant.now");
  LLVMValueRef sr_raw =
      LLVMBuildCall2(builder, LLVMGlobalGetValueType(ctx_sample_rate_fn),
                     ctx_sample_rate_fn, NULL, 0, "defer_quant.sr.raw");
  LLVMValueRef sr_is_zero = LLVMBuildICmp(builder, LLVMIntEQ, sr_raw,
                                          LLVMConstInt(LLVMInt32Type(), 0, 0),
                                          "defer_quant.sr.is_zero");
  LLVMValueRef sr = LLVMBuildSelect(builder, sr_is_zero,
                                    LLVMConstInt(LLVMInt32Type(), 48000, 0),
                                    sr_raw, "defer_quant.sr");

  LLVMValueRef sr_f =
      LLVMBuildSIToFP(builder, sr, LLVMDoubleType(), "defer_quant.sr.f64");
  LLVMValueRef quant_samps_f =
      LLVMBuildFMul(builder, quant, sr_f, "defer_quant.samps.f64");
  LLVMValueRef quant_samps = LLVMBuildFPToUI(
      builder, quant_samps_f, LLVMInt64Type(), "defer_quant.samps");
  LLVMValueRef offset_in_cycle =
      LLVMBuildURem(builder, now, quant_samps, "defer_quant.offset");
  LLVMValueRef offset_is_zero = LLVMBuildICmp(
      builder, LLVMIntEQ, offset_in_cycle, LLVMConstInt(LLVMInt64Type(), 0, 0),
      "defer_quant.offset_is_zero");
  LLVMValueRef remainder =
      LLVMBuildSelect(builder, offset_is_zero, quant_samps,
                      LLVMBuildSub(builder, quant_samps, offset_in_cycle,
                                   "defer_quant.remainder.sub"),
                      "defer_quant.remainder");

  LLVMValueRef callback_ptr =
      LLVMBuildBitCast(builder, callback_fn, GENERIC_PTR, "defer_quant.cb");
  return LLVMBuildCall2(builder, push_event_type(), push_event_fn,
                        (LLVMValueRef[]){
                            callback_ptr,
                            callback_userdata,
                            remainder,
                            now,
                        },
                        4, "defer_quant.push_event");
}

static LLVMValueRef
ensure_play_pattern_step_wrapper(const char *name, LLVMValueRef pattern_global,
                                 Type *cor_type, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  char wrapper_name[256];
  snprintf(wrapper_name, sizeof(wrapper_name), "__ylc_play_pattern_step_%s",
           name);

  LLVMValueRef func = LLVMGetNamedFunction(module, wrapper_name);
  if (func) {
    return func;
  }

  LLVMTypeRef fn_type = scheduler_callback_type();
  func = LLVMAddFunction(module, wrapper_name, fn_type);
  LLVMSetLinkage(func, LLVMInternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef finished =
      LLVMAppendBasicBlock(func, "coro.is_finished_block");
  LLVMBasicBlockRef clear_current =
      LLVMAppendBasicBlock(func, "coro.clear_current");
  LLVMBasicBlockRef finished_ret =
      LLVMAppendBasicBlock(func, "coro.finished_ret");
  LLVMBasicBlockRef not_finished =
      LLVMAppendBasicBlock(func, "coro.resume_block");

  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef handle = LLVMGetParam(func, 0);
  LLVMValueRef tick = LLVMGetParam(func, 1);
  LLVMValueRef schedule_event_fn = ensure_schedule_event_fn(module);

  Type *cor_yield_type_t = cor_type->data.T_CONS.args[0];
  bool yield_is_tuple =
      (cor_yield_type_t->kind == T_CONS &&
       strcmp(cor_yield_type_t->data.T_CONS.name, TYPE_NAME_TUPLE) == 0);
  LLVMTypeRef yield_type = type_to_llvm_type(cor_yield_type_t, ctx, module);

  LLVMValueRef resume_result =
      codegen_handle_resume(handle, yield_type, ctx, module, builder);
  LLVMValueRef result_tag =
      LLVMBuildExtractValue(builder, resume_result, 0, "tag");
  LLVMValueRef is_done =
      LLVMBuildICmp(builder, LLVMIntEQ, result_tag,
                    LLVMConstInt(LLVMInt8Type(), 1, 0), "tag_eq_1");
  LLVMBuildCondBr(builder, is_done, finished, not_finished);

  LLVMPositionBuilderAtEnd(builder, not_finished);
  LLVMValueRef promise_ptr_raw = GET_PROMISE_PTR_RAW(handle);
  LLVMValueRef yield_ptr = LLVMBuildBitCast(
      builder, promise_ptr_raw, LLVMPointerType(yield_type, 0), "promise.ptr");
  LLVMValueRef yielded_raw =
      LLVMBuildLoad2(builder, yield_type, yield_ptr, "yielded.raw");

  LLVMValueRef dur_value =
      yield_is_tuple ? LLVMBuildExtractValue(builder, yielded_raw, 0, "dur")
                     : yielded_raw;

  LLVMValueRef self_ptr =
      LLVMBuildBitCast(builder, func, GENERIC_PTR, "step.self_ptr");
  LLVMBuildCall2(builder, schedule_event_type(), schedule_event_fn,
                 (LLVMValueRef[]){
                     tick,
                     dur_value,
                     self_ptr,
                     handle,
                 },
                 4, "schedule_next");
  LLVMBuildRetVoid(builder);

  LLVMPositionBuilderAtEnd(builder, finished);
  LLVMTypeRef state_ty = live_pattern_struct_type();
  LLVMValueRef coro_gep =
      LLVMBuildStructGEP2(builder, state_ty, pattern_global, 1, "pattern.coro");
  LLVMValueRef current_handle =
      LLVMBuildLoad2(builder, GENERIC_PTR, coro_gep, "pattern.coro.current");
  LLVMValueRef is_current_handle = LLVMBuildICmp(
      builder, LLVMIntEQ, current_handle, handle, "pattern.coro.is_current");
  LLVMBuildCondBr(builder, is_current_handle, clear_current, finished_ret);

  LLVMPositionBuilderAtEnd(builder, clear_current);
  LLVMBuildStore(builder, LLVMConstNull(GENERIC_PTR), coro_gep);
  LLVMBuildBr(builder, finished_ret);

  LLVMPositionBuilderAtEnd(builder, finished_ret);
  LLVMBuildRetVoid(builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

static LLVMValueRef
ensure_play_pattern_defer_wrapper(const char *name, LLVMValueRef pattern_global,
                                  LLVMValueRef step_wrapper, Type *cor_type,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  char wrapper_name[256];
  snprintf(wrapper_name, sizeof(wrapper_name), "__ylc_play_pattern_defer_%s",
           name);

  LLVMValueRef func = LLVMGetNamedFunction(module, wrapper_name);
  if (func) {
    return func;
  }

  func = LLVMAddFunction(module, wrapper_name, scheduler_callback_type());
  LLVMSetLinkage(func, LLVMInternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef old_handle = LLVMGetParam(func, 0);
  LLVMValueRef tick = LLVMGetParam(func, 1);
  LLVMValueRef schedule_event_fn = ensure_schedule_event_fn(module);
  LLVMTypeRef state_ty = live_pattern_struct_type();
  _cor_stop(cor_type, old_handle, ctx, module, builder);

  LLVMValueRef coro_gep =
      LLVMBuildStructGEP2(builder, state_ty, pattern_global, 1, "pattern.coro");

  LLVMValueRef handle =
      LLVMBuildLoad2(builder, GENERIC_PTR, coro_gep, "pattern.coro.load");

  LLVMValueRef step_ptr =
      LLVMBuildBitCast(builder, step_wrapper, GENERIC_PTR, "step.cb.ptr");

  LLVMBuildCall2(builder, schedule_event_type(), schedule_event_fn,
                 (LLVMValueRef[]){
                     tick,
                     LLVMConstReal(LLVMDoubleType(), 0.0),
                     step_ptr,
                     handle,
                 },
                 4, "schedule_start");
  LLVMBuildRetVoid(builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

static LLVMValueRef ensure_play_pattern_start_wrapper(
    const char *name, LLVMValueRef pattern_global, LLVMValueRef step_wrapper,
    LLVMModuleRef module, LLVMBuilderRef builder) {
  char wrapper_name[256];
  snprintf(wrapper_name, sizeof(wrapper_name), "__ylc_play_pattern_start_%s",
           name);

  LLVMValueRef func = LLVMGetNamedFunction(module, wrapper_name);
  if (func) {
    return func;
  }

  func = LLVMAddFunction(module, wrapper_name, defer_quant_callback_type());
  LLVMSetLinkage(func, LLVMInternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef tick = LLVMGetParam(func, 0);
  LLVMValueRef schedule_event_fn = ensure_schedule_event_fn(module);
  LLVMTypeRef state_ty = live_pattern_struct_type();
  LLVMValueRef coro_gep =
      LLVMBuildStructGEP2(builder, state_ty, pattern_global, 1, "pattern.coro");
  LLVMValueRef handle =
      LLVMBuildLoad2(builder, GENERIC_PTR, coro_gep, "pattern.coro.load");
  LLVMValueRef step_ptr =
      LLVMBuildBitCast(builder, step_wrapper, GENERIC_PTR, "step.cb.ptr");

  LLVMBuildCall2(builder, schedule_event_type(), schedule_event_fn,
                 (LLVMValueRef[]){
                     tick,
                     LLVMConstReal(LLVMDoubleType(), 0.0),
                     step_ptr,
                     handle,
                 },
                 4, "schedule_start");
  LLVMBuildRetVoid(builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

LLVMValueRef play_pattern(Ast *binding, Ast *quant, JITLangCtx *ctx,
                          LLVMModuleRef module, LLVMBuilderRef builder) {
  if (binding->data.AST_LET.binding->tag != AST_IDENTIFIER) {
    fprintf(stderr, "play_pattern: expected identifier binding\n");
    return NULL;
  }

  const char *pattern_name =
      binding->data.AST_LET.binding->data.AST_IDENTIFIER.value;
  LLVMValueRef quantization = codegen(quant, ctx, module, builder);

  LLVMValueRef coroutine =
      codegen(binding->data.AST_LET.expr, ctx, module, builder);
  Type *cor_type = binding->data.AST_LET.expr->type;
  if (!coroutine || !cor_type) {
    return NULL;
  }

  JITSymbol *sym = lookup_id_ast(binding->data.AST_LET.binding, ctx);
  LLVMTypeRef pattern_struct_ty = live_pattern_struct_type();
  live_pattern_state_t *pattern_state;
  if (sym && (int)sym->type == STYPE_AUDIO_JIT_LIVE_PATTERN) {
    pattern_state = sym->symbol_data._USER_DEFINED_SYMBOL;
  } else {
    pattern_state = get_or_create_live_pattern_state(pattern_name);
  }
  LLVMValueRef pattern_global =
      codegen_live_pattern_state_ptr(pattern_state, module);

  LLVMValueRef live_pattern_struct = LLVMGetUndef(pattern_struct_ty);
  live_pattern_struct =
      LLVMBuildInsertValue(builder, live_pattern_struct, quantization, 0,
                           "set_live_pattern_quantization");

  live_pattern_struct = LLVMBuildInsertValue(
      builder, live_pattern_struct, coroutine, 1, "set_live_pattern_coroutine");
  LLVMValueRef old_coroutine_to_stop = LLVMConstNull(GENERIC_PTR);
  if (sym && (int)sym->type == STYPE_AUDIO_JIT_LIVE_PATTERN) {

    LLVMValueRef old_pattern = LLVMBuildLoad2(
        builder, pattern_struct_ty, pattern_global, "live_pattern.old");
    old_coroutine_to_stop =
        LLVMBuildExtractValue(builder, old_pattern, 1, "live_pattern.old.coro");
  }

  LLVMBuildStore(builder, live_pattern_struct, pattern_global);

  LLVMValueRef step_wrapper = ensure_play_pattern_step_wrapper(
      pattern_name, pattern_global, cor_type, ctx, module, builder);
  LLVMValueRef defer_wrapper = ensure_play_pattern_defer_wrapper(
      pattern_name, pattern_global, step_wrapper, cor_type, ctx, module,
      builder);
  LLVMValueRef start_wrapper = ensure_play_pattern_start_wrapper(
      pattern_name, pattern_global, step_wrapper, module, builder);

  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef current_fn = LLVMGetBasicBlockParent(current_bb);
  LLVMBasicBlockRef replace_bb =
      LLVMAppendBasicBlock(current_fn, "play_pattern.replace");
  LLVMBasicBlockRef start_bb =
      LLVMAppendBasicBlock(current_fn, "play_pattern.start");
  LLVMBasicBlockRef merge_bb =
      LLVMAppendBasicBlock(current_fn, "play_pattern.merge");

  LLVMValueRef has_old_coroutine =
      LLVMBuildIsNotNull(builder, old_coroutine_to_stop, "has_old_coroutine");
  LLVMBuildCondBr(builder, has_old_coroutine, replace_bb, start_bb);

  LLVMPositionBuilderAtEnd(builder, replace_bb);
  emit_defer_quant_ir(quantization, defer_wrapper, old_coroutine_to_stop,
                      module, builder);
  LLVMBuildBr(builder, merge_bb);

  LLVMPositionBuilderAtEnd(builder, start_bb);
  emit_defer_quant_ir(quantization, start_wrapper, LLVMConstNull(GENERIC_PTR),
                      module, builder);
  LLVMBuildBr(builder, merge_bb);

  LLVMPositionBuilderAtEnd(builder, merge_bb);

  if (sym && (int)sym->type == STYPE_AUDIO_JIT_LIVE_PATTERN) {
    printf("found pattern symbol %s\n",
           binding->data.AST_LET.binding->data.AST_IDENTIFIER.value);

  } else {
    JITSymbol *pat_sym = new_symbol(STYPE_AUDIO_JIT_LIVE_PATTERN, binding->type,
                                    pattern_global, pattern_struct_ty);
    pat_sym->symbol_data._USER_DEFINED_SYMBOL = pattern_state;
    ht_set_hash(
        ctx->frame->table, pattern_name,
        hash_string(pattern_name,
                    binding->data.AST_LET.binding->data.AST_IDENTIFIER.length),
        pat_sym);
    printf("need new symbol %s\n",
           binding->data.AST_LET.binding->data.AST_IDENTIFIER.value);
  }
  if (sym && (int)sym->type == STYPE_AUDIO_JIT_LIVE_PATTERN) {
    sym->val = pattern_global;
  }
  return coroutine;
}
LLVMValueRef play_pattern_handler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  Ast *quant_arg = ast->data.AST_APPLICATION.args;
  Ast *pattern_record = ast->data.AST_APPLICATION.args + 1;

  if (pattern_record->tag == AST_TUPLE &&
      (pattern_record->data.AST_LIST.items[0].tag == AST_LET)) {
    LLVMValueRef final;
    Ast app = *ast;

    for (size_t i = 0; i < pattern_record->data.AST_LIST.len; i++) {
      app.data.AST_APPLICATION.args[1] = pattern_record->data.AST_LIST.items[i];
      final = play_pattern_handler(&app, ctx, module, builder);
    }
    return final;
  }

  if (pattern_record->tag == AST_LET) {
    return play_pattern(pattern_record, quant_arg, ctx, module, builder);
  }

  fprintf(stderr, "Error: pattern binding type not implemented %d\n",
          pattern_record->tag);
  print_ast_err(pattern_record);
  return NULL;
}
