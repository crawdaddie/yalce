// ylc-wasm: browser-facing YLC compiler that lowers MIR straight to a
// WebAssembly module. No LLVM dependency — the entire frontend (lexer,
// parser, type inference, escape analysis, MIR builder + passes) is reused
// from lang/, and a fresh MIR->WASM lowering lives in this file.
//
// Browser contract (exported on the wasm module):
//   ylc_wasm_init()                     -> i32   (status, 0 = ok)
//   ylc_wasm_compile(ptr, len_ptr)       -> i32   (heap ptr to wasm module
//                                                 bytes; size at len_ptr as
//                                                 u32; 0 on failure)
//   ylc_wasm_dump_mir(ptr)              -> i32   (status; writes MIR text to
//                                                 stdout/wasi stderr)
//
// All input strings are UTF-8, NUL-terminated, allocated by the JS host via
// malloc() (see docs/web/wasm.js). Returned module buffers are owned by this
// module and must be freed by the host through free().

#include "config.h"
#include "escape_analysis.h"
#include "ht.h"
#include "mir/mir.h"
#include "modules.h"
#include "parse.h"
#include "types/builtins.h"
#include "types/inference.h"
#include "types/type.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Tiny error helper — front-end passes already print to stderr, we just want
// a single visible prefix so the browser console groups them.
// ---------------------------------------------------------------------------
static void ylc_wasm_error(const char *where) {
  fprintf(stderr, "[ylc-wasm] %s\n", where);
}

// ---------------------------------------------------------------------------
// Persistent REPL session
//
// Each ylc_wasm_compile() call compiles one REPL input as a standalone
// MirProgram (mirrors how the LLVM orc.c REPL works: one module per input,
// not concatenated source). For successive inputs to see prior top-level
// `let` bindings and functions, two pieces of state must outlive a single
// compile:
//
//   1. TypeEnv  — so the next input's typechecker resolves `x` to the
//      type it was given by the prior `let x = ...`. Threaded through
//      ylc_wasm_frontend like `*env` in orc.c:329.
//
//   2. Global slot table — top-level `let` bindings lower to MIR
//      `global_store @$global.<name>` / `global_load @$global.<name>`.
//      Each distinct name is assigned a fixed address in linear memory
//      (a bump pointer starting at GLOBAL_BASE). The mapping is kept in
//      `global_slots` so the next input's `global_load` for the same
//      name reads the slot the prior input wrote. This is the wasm
//      analogue of orc.c's process-wide `global_storage_array` +
//      `num_globals` counter.
//
// Both live in the host wasm module (this file), not in the per-input
// generated modules — generated modules just import linear memory and
// reference globals by their assigned address.
// ---------------------------------------------------------------------------
typedef struct {
  TypeEnv *env;          // persistent type environment across inputs
  ht global_slots;       // MIR global name -> uint32_t linear-mem address
  uint32_t global_bump;  // next free address in the global region
} YlWasmSession;

static YlWasmSession g_session;

// Linear-memory layout for the *host* module. The generated per-input
// modules import this memory and read/write globals at these addresses.
// Keep GLOBAL_BASE past the heap pointer the C runtime would otherwise
// hand out (malloc lives below this), so the two don't collide.
#define YW_GLOBAL_BASE 0x100000u  // 1 MiB
#define YW_GLOBAL_END 0x1000000u  // 16 MiB cap

static void ylc_wasm_session_init(void) {
  g_session.env = NULL;
  ht_init(&g_session.global_slots);
  g_session.global_bump = YW_GLOBAL_BASE;
}

// Look up (or assign) the linear-memory address for a MIR global name.
// Returns 0 if the global region is full. Called by the MIR->wasm
// lowering when it encounters MIR_OP_KIND_GLOBAL_STORE / GLOBAL_LOAD.
static uint32_t ylc_wasm_global_address(const char *global_name) {
  if (!global_name) {
    return 0;
  }
  uint64_t hash = hash_string(global_name, (int)strlen(global_name));
  void *existing = ht_get_hash(&g_session.global_slots, global_name, hash);
  if (existing) {
    return (uint32_t)(uintptr_t)existing;
  }
  if (g_session.global_bump + sizeof(uintptr_t) > YW_GLOBAL_END) {
    ylc_wasm_error("global slot table full");
    return 0;
  }
  uint32_t addr = g_session.global_bump;
  g_session.global_bump += sizeof(uintptr_t); // one slot per global
  ht_set_hash(&g_session.global_slots, global_name, hash,
              (void *)(uintptr_t)addr);
  return addr;
}

// ---------------------------------------------------------------------------
// Single-entry frontend: parse -> typecheck -> escape analysis -> build MIR.
// Returns a fully lowered MirProgram (after passes) or NULL on failure.
// Caller owns the arena and must mir_arena_destroy it.
//
// `session->env` is threaded through: passed into infer() as the starting
// environment and updated to the resulting env on success, so successive
// calls see prior top-level bindings.
// ---------------------------------------------------------------------------
static MirProgram *ylc_wasm_frontend(const char *filename, const char *source,
                                     MirArena **out_arena,
                                     YlWasmSession *session) {
  *out_arena = NULL;

  Ast *prog = parse_input_buffer(filename, source);
  if (!prog) {
    ylc_wasm_error("parse failed");
    return NULL;
  }

  TICtx ti_ctx = {.env = session->env, .scope = 0, .err_stream = stderr};
  if (!infer(prog, &ti_ctx)) {
    ylc_wasm_error("typecheck failed");
    return NULL;
  }
  session->env = ti_ctx.env;

  escape_analysis(prog);

  MirArena *arena = mir_arena_create();
  if (!arena) {
    ylc_wasm_error("mir arena alloc failed");
    return NULL;
  }
  *out_arena = arena;

  ht table;
  MirStackFrame initial_frame;
  mir_stack_frame_init(arena, &table, &initial_frame, NULL);
  MirCtx mir_ctx = {.env = ti_ctx.env, .frame = &initial_frame};

  MirProgram *program = mir_build_program(arena, prog, &mir_ctx);
  if (!program || mir_program_had_error(program)) {
    ylc_wasm_error("mir build failed");
    if (program) {
      mir_program_destroy(program);
    }
    return NULL;
  }

  mir_run_passes(program);
  if (mir_program_had_error(program)) {
    ylc_wasm_error("mir passes failed");
    mir_program_destroy(program);
    return NULL;
  }

  return program;
}

// ===========================================================================
// MIR -> WebAssembly lowering
// ===========================================================================
// This is the scaffold you'll flesh out. The design mirrors the existing
// LLVM lowering (lang/llvm/lowering.c) and the legacy AST->wasm backend
// (lang/backend_wasm/), but walks the MirProgram directly so the browser
// artifact never links against LLVM.
//
// Suggested layout (each section below has a stub):
//   1. Byte buffer + LEB128 writers (lifted from lang/backend_wasm/util.c).
//   2. Type/value/block maps (per-function, mir value id -> wasm operand).
//   3. Section builders (type/import/func/export/memory/code).
//   4. Per-function lowering: declare -> lower body -> lower terminators.
//   5. Top-level driver producing the final wasm binary.

// ---- 1. Byte buffer + LEB128 ---------------------------------------------
typedef struct {
  uint8_t *data;
  size_t size;
  size_t cap;
} WasmBuf;

static void wasm_buf_init(WasmBuf *b) {
  b->cap = 256;
  b->size = 0;
  b->data = (uint8_t *)malloc(b->cap);
}

static void wasm_buf_push(WasmBuf *b, uint8_t v) {
  if (b->size >= b->cap) {
    b->cap *= 2;
    b->data = (uint8_t *)realloc(b->data, b->cap);
  }
  b->data[b->size++] = v;
}

static void wasm_buf_free(WasmBuf *b) {
  free(b->data);
  b->data = NULL;
  b->size = b->cap = 0;
}

static void wasm_buf_leb_u32(WasmBuf *b, uint32_t v) {
  do {
    uint8_t byte = v & 0x7f;
    v >>= 7;
    if (v) {
      byte |= 0x80;
    }
    wasm_buf_push(b, byte);
  } while (v);
}

static void wasm_buf_leb_i32(WasmBuf *b, int32_t v) {
  int more = 1;
  while (more) {
    uint8_t byte = (uint8_t)(v & 0x7f);
    v >>= 7;
    if ((v == 0 && (byte & 0x40) == 0) || (v == -1 && (byte & 0x40) != 0)) {
      more = 0;
    } else {
      byte |= 0x80;
    }
    wasm_buf_push(b, byte);
  }
}

static void wasm_buf_name(WasmBuf *b, const char *s) {
  size_t n = strlen(s);
  wasm_buf_leb_u32(b, (uint32_t)n);
  for (size_t i = 0; i < n; i++) {
    wasm_buf_push(b, (uint8_t)s[i]);
  }
}

// ---- 2. Per-function maps ------------------------------------------------
// MirValueId is dense within a function (see lang/mir/mir.h), so a flat
// array indexed by value id is sufficient. `kind` records how each value
// is materialized in wasm:
//   STACK   - the value is on the wasm value stack right now
//   LOCAL   - the value lives in a local; load it with local.get
//   PENDING - not yet emitted (used to detect unsupported nodes during dev)
typedef enum {
  YWV_PENDING,
  YWV_LOCAL,
  YWV_STACK,
} YwValueKind;

typedef struct {
  YwValueKind kind;
  uint32_t local; // valid when kind == YWV_LOCAL
} YwValueSlot;

typedef struct {
  MirFunction *fn;
  YwValueSlot *slots; // length == fn->values.len
  size_t slots_len;
  uint32_t next_local;
  // Per-block wasm block depth tracking goes here when you implement
  // structured control flow (br/if/loop/block opcodes).
} YwFnCtx;

static void yw_fn_ctx_init(YwFnCtx *c, MirFunction *fn) {
  c->fn = fn;
  c->slots_len = fn->values.len;
  c->slots = c->slots_len ? calloc(c->slots_len, sizeof(YwValueSlot)) : NULL;
  c->next_local = (uint32_t)fn->params.len;
  for (size_t i = 0; i < c->slots_len; i++) {
    c->slots[i].kind = YWV_PENDING;
  }
}

static void yw_fn_ctx_free(YwFnCtx *c) {
  free(c->slots);
  c->slots = NULL;
}

// ---- 3. Section builders -------------------------------------------------
// WASM section ids we emit:
//   0x01 type, 0x02 import, 0x03 function, 0x05 memory, 0x07 export,
//   0x0a code
//
// Sections are accumulated separately then concatenated, mirroring the
// pattern in lang/backend_wasm/wasm.c but kept general for multi-function
// modules.

static const uint8_t WASM_MAGIC[8] = {0x00, 0x61, 0x73, 0x6d,
                                      0x01, 0x00, 0x00, 0x00};

// ---- 4. Per-function lowering --------------------------------------------
// The real work. Each stub here corresponds to a MirInstrKind / terminator
// kind from lang/mir/mir.h. Replace the `fprintf(stderr, ...)` early-outs
// with actual wasm emission as you implement them.

static void yw_lower_const(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  (void)c;
  MirConst *k = &instr->data.const_value;
  switch (k->kind) {
  case MIR_CONST_KIND_INT:
    wasm_buf_push(code, 0x41); // i32.const
    wasm_buf_leb_i32(code, k->as.int_value);
    break;
  case MIR_CONST_KIND_BOOL:
    wasm_buf_push(code, 0x41);
    wasm_buf_leb_i32(code, k->as.bool_value ? 1 : 0);
    break;
  case MIR_CONST_KIND_CHAR:
    wasm_buf_push(code, 0x41);
    wasm_buf_leb_i32(code, k->as.char_value);
    break;
  default:
    fprintf(stderr, "[ylc-wasm] const kind %d not yet implemented\n", k->kind);
    wasm_buf_push(code, 0x41);
    wasm_buf_leb_i32(code, 0);
    break;
  }
}

static void yw_lower_op(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  fprintf(stderr, "[ylc-wasm] MIR_OP lowering not yet implemented\n");
}

static void yw_lower_phi(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  fprintf(stderr, "[ylc-wasm] MIR_PHI lowering not yet implemented\n");
}

static void yw_lower_extract(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  fprintf(stderr, "[ylc-wasm] MIR_EXTRACT lowering not yet implemented\n");
}

static void yw_lower_construct(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  fprintf(stderr, "[ylc-wasm] MIR_CONSTRUCT lowering not yet implemented\n");
}

static void yw_lower_fn_ref(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  fprintf(stderr, "[ylc-wasm] MIR_FN_REF lowering not yet implemented\n");
}

static void yw_lower_call(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  fprintf(stderr, "[ylc-wasm] MIR_CALL lowering not yet implemented\n");
}

static void yw_lower_coro(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  fprintf(stderr, "[ylc-wasm] MIR_CORO_* lowering not yet implemented\n");
}

static void yw_lower_instr(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  switch (instr->kind) {
  case MIR_CONST:
    yw_lower_const(c, instr, code);
    break;
  case MIR_OP:
    yw_lower_op(c, instr, code);
    break;
  case MIR_PHI:
    yw_lower_phi(c, instr, code);
    break;
  case MIR_EXTRACT:
    yw_lower_extract(c, instr, code);
    break;
  case MIR_CONSTRUCT:
    yw_lower_construct(c, instr, code);
    break;
  case MIR_FN_REF:
    yw_lower_fn_ref(c, instr, code);
    break;
  case MIR_CALL:
    yw_lower_call(c, instr, code);
    break;
  case MIR_CORO_NEW:
  case MIR_CORO_NEXT:
  case MIR_CORO_RESET:
    yw_lower_coro(c, instr, code);
    break;
  }
}

static void yw_lower_terminator(YwFnCtx *c, MirTerminator *term,
                                WasmBuf *code) {
  (void)c;
  (void)code;
  switch (term->kind) {
  case MIR_TERM_NONE:
    fprintf(stderr, "[ylc-wasm] unterminated block\n");
    break;
  case MIR_TERM_RETURN:
    fprintf(stderr, "[ylc-wasm] MIR_TERM_RETURN not yet implemented\n");
    break;
  case MIR_TERM_BR:
    fprintf(stderr, "[ylc-wasm] MIR_TERM_BR not yet implemented\n");
    break;
  case MIR_TERM_COND:
    fprintf(stderr, "[ylc-wasm] MIR_TERM_COND not yet implemented\n");
    break;
  case MIR_TERM_UNREACHABLE:
    wasm_buf_push(code, 0x00); // unreachable
    break;
  case MIR_TERM_YIELD:
  case MIR_TERM_CORO_RESTART:
  case MIR_TERM_CORO_DONE:
    fprintf(stderr, "[ylc-wasm] coroutine terminator %d not yet implemented\n",
            term->kind);
    break;
  }
}

// ---- 5. Top-level driver -------------------------------------------------
// Builds a single wasm module from the MirProgram. The current skeleton
// emits just the header; populate the section builders above and thread
// them through here. See lang/backend_wasm/wasm.c for the byte-level
// recipe and lang/llvm/lowering.c for the structural walk over
// program->functions.
static uint8_t *ylc_lower_mir_to_wasm(MirProgram *prog, size_t *out_size) {
  (void)prog;
  WasmBuf out;
  wasm_buf_init(&out);
  for (size_t i = 0; i < sizeof(WASM_MAGIC); i++) {
    wasm_buf_push(&out, WASM_MAGIC[i]);
  }

  // TODO: emit type section (function signatures)
  // TODO: emit import section (host-provided memory + runtime helpers)
  // TODO: emit function section (one entry per non-extern MirFunction)
  // TODO: emit memory section (linear memory for the heap)
  // TODO: emit export section (entry point, e.g. "$top")
  // TODO: emit code section — for each function, walk its blocks and
  //       call yw_lower_instr / yw_lower_terminator. Structured control
  //       flow requires mapping MIR blocks to wasm block/loop/if nests;
  //       see the MIR_COROUTINE_LOWERING_PLAN.md notes for the shape.

  for (size_t i = 0; i < prog->functions.len; i++) {
    MirFunction *fn = prog->functions.items[i];
    if (!fn || fn->is_extern) {
      continue;
    }
    YwFnCtx ctx;
    yw_fn_ctx_init(&ctx, fn);
    WasmBuf body;
    wasm_buf_init(&body);
    for (size_t b = 0; b < fn->blocks.len; b++) {
      MirBlock *blk = fn->blocks.items[b];
      for (size_t j = 0; j < blk->instrs.len; j++) {
        yw_lower_instr(&ctx, &blk->instrs.items[j], &body);
      }
      yw_lower_terminator(&ctx, &blk->term, &body);
    }
    wasm_buf_free(&body);
    yw_fn_ctx_free(&ctx);
  }

  uint8_t *result = (uint8_t *)malloc(out.size ? out.size : 1);
  if (result) {
    memcpy(result, out.data, out.size);
  }
  *out_size = out.size;
  wasm_buf_free(&out);
  return result;
}

// ===========================================================================
// Exported entry points (called from docs/web/wasm.js)
// ===========================================================================

int ylc_wasm_init(void) {
  init_module_registry();
  initialize_builtin_types();
  ylc_wasm_session_init();
  ylc_config = (RTConfig){0};
  ylc_config.opt_level = "default<O0>";
  return 0;
}

// Compile a single YLC source string into a wasm module.
//   src_ptr  : NUL-terminated source, allocated by the host
//   len_ptr  : pointer to a u32 the host reserved; we write the module's
//              byte length there
//   returns  : heap pointer to the module bytes (host frees with free()),
//              or 0 on failure.
uint8_t *ylc_wasm_compile(const char *src_ptr, uint32_t *len_ptr) {
  if (!src_ptr || !len_ptr) {
    ylc_wasm_error("ylc_wasm_compile: null arg");
    return NULL;
  }
  *len_ptr = 0;

  MirArena *arena = NULL;
  MirProgram *prog =
      ylc_wasm_frontend("<repl>", src_ptr, &arena, &g_session);
  if (!prog) {
    return NULL;
  }

  size_t module_size = 0;
  uint8_t *module = ylc_lower_mir_to_wasm(prog, &module_size);

  mir_program_destroy(prog);
  if (arena) {
    mir_arena_destroy(arena);
  }

  if (!module) {
    ylc_wasm_error("lowering produced no module");
    return NULL;
  }
  *len_ptr = (uint32_t)module_size;
  return module;
}

// Diagnostic: dump the MIR for a source string to stdout (wasi stdout).
int ylc_wasm_dump_mir(const char *src_ptr) {
  if (!src_ptr) {
    return 1;
  }
  MirArena *arena = NULL;
  MirProgram *prog =
      ylc_wasm_frontend("<dump>", src_ptr, &arena, &g_session);
  if (!prog) {
    return 1;
  }
  mir_dump_program(prog, stdout);
  mir_program_destroy(prog);
  if (arena) {
    mir_arena_destroy(arena);
  }
  return 0;
}
