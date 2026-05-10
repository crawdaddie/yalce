#include "pattern_coroutine.h"
#include "../../lang/backend_llvm/coroutines/coroutines.h"
#include "../../lang/backend_llvm/strings.h"
#include "llvm-c/Core.h"
#include <ctype.h>
#include <stdio.h>
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

  while (**pp && **pp != ' ' && **pp != '\t' && **pp != '\n' &&
         **pp != '\r' && **pp != '-' && **pp != '.' && !is_key_delim(**pp))
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
