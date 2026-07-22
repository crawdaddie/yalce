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

#include <stdarg.h>
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
  TypeEnv *env;                  // persistent type environment across inputs
  ht global_slots;               // MIR global name -> uint32_t linear-mem address
  uint32_t global_bump;          // next free address in the global region
  ht mir_root_table;             // persistent MIR top-level symbols
  MirStackFrame mir_root_frame;  // root frame backed by mir_root_table
  MirArena *mir_durable_arena;   // durable top-level function bodies
  ht *mir_durable_builtins;      // durable builtin symbols
  char *import_prelude;          // successful import/open statements
} YlWasmSession;

static YlWasmSession g_session;

// Linear-memory layout for the *host* module. The generated per-input
// modules import this memory and read/write globals at these addresses.
// Keep GLOBAL_BASE past the heap pointer the C runtime would otherwise
// hand out (malloc lives below this), so the two don't collide.
#define YW_GLOBAL_BASE 0x100000u // 1 MiB
#define YW_GLOBAL_END 0x1000000u // 16 MiB cap
#define YW_GLOBAL_SLOT_SIZE 8u
#define YW_WASM_PAGE_SIZE 65536u

static bool ylc_wasm_ensure_memory(uint32_t end_addr) {
#if defined(__wasm__) || defined(__wasm32__)
  uint32_t pages = (uint32_t)__builtin_wasm_memory_size(0);
  uint64_t have = (uint64_t)pages * YW_WASM_PAGE_SIZE;
  if (have >= end_addr) {
    return true;
  }
  uint32_t needed_pages =
      (uint32_t)(((uint64_t)end_addr + YW_WASM_PAGE_SIZE - 1) /
                 YW_WASM_PAGE_SIZE);
  uint32_t grow_by = needed_pages > pages ? needed_pages - pages : 0;
  if (!grow_by) {
    return true;
  }
  return __builtin_wasm_memory_grow(0, grow_by) != (size_t)-1;
#else
  (void)end_addr;
  return true;
#endif
}

static bool ylc_wasm_session_init(void) {
  g_session.env = NULL;
  ht_init(&g_session.global_slots);
  g_session.global_bump = YW_GLOBAL_BASE;
  g_session.import_prelude = NULL;
  mir_stack_frame_init(NULL, &g_session.mir_root_table,
                       &g_session.mir_root_frame, NULL);
  g_session.mir_durable_arena = mir_arena_create();
  g_session.mir_durable_builtins = mir_durable_builtins_create();
  if (!g_session.mir_durable_arena || !g_session.mir_durable_builtins) {
    ylc_wasm_error("persistent MIR session allocation failed");
    return false;
  }
  g_session.mir_root_frame.durable_arena = g_session.mir_durable_arena;
  g_session.mir_root_frame.durable_builtins = g_session.mir_durable_builtins;
  return true;
}

static bool ylc_wasm_is_space(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ||
         ch == '\f' || ch == '\v';
}

static bool ylc_wasm_is_ident_char(char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
         (ch >= '0' && ch <= '9') || ch == '_';
}

static const char *ylc_wasm_skip_ws(const char *src) {
  while (src && ylc_wasm_is_space(*src)) {
    src++;
  }
  return src;
}

static bool ylc_wasm_starts_with_word(const char *src, const char *word) {
  size_t len = strlen(word);
  return strncmp(src, word, len) == 0 && !ylc_wasm_is_ident_char(src[len]);
}

static bool ylc_wasm_source_is_import_only(const char *source,
                                           const char **out_stmt,
                                           size_t *out_stmt_len) {
  const char *stmt = ylc_wasm_skip_ws(source);
  if (!stmt || (!ylc_wasm_starts_with_word(stmt, "import") &&
                !ylc_wasm_starts_with_word(stmt, "open"))) {
    return false;
  }

  const char *semi = strchr(stmt, ';');
  if (!semi) {
    return false;
  }
  const char *tail = ylc_wasm_skip_ws(semi + 1);
  if (*tail != '\0') {
    return false;
  }

  if (out_stmt) {
    *out_stmt = stmt;
  }
  if (out_stmt_len) {
    *out_stmt_len = (size_t)((semi + 1) - stmt);
  }
  return true;
}

static bool ylc_wasm_session_has_import(YlWasmSession *session,
                                        const char *stmt,
                                        size_t stmt_len) {
  if (!session || !session->import_prelude) {
    return false;
  }

  const char *line = session->import_prelude;
  while (*line) {
    const char *line_end = strchr(line, '\n');
    size_t line_len = line_end ? (size_t)(line_end - line) : strlen(line);
    if (line_len == stmt_len && strncmp(line, stmt, stmt_len) == 0) {
      return true;
    }
    if (!line_end) {
      break;
    }
    line = line_end + 1;
  }
  return false;
}

static void ylc_wasm_session_remember_import(YlWasmSession *session,
                                             const char *source) {
  const char *stmt = NULL;
  size_t stmt_len = 0;
  if (!session ||
      !ylc_wasm_source_is_import_only(source, &stmt, &stmt_len) ||
      ylc_wasm_session_has_import(session, stmt, stmt_len)) {
    return;
  }

  size_t old_len = session->import_prelude ? strlen(session->import_prelude) : 0;
  char *next = (char *)malloc(old_len + stmt_len + 2);
  if (!next) {
    ylc_wasm_error("persistent import prelude allocation failed");
    return;
  }
  if (old_len) {
    memcpy(next, session->import_prelude, old_len);
  }
  memcpy(next + old_len, stmt, stmt_len);
  next[old_len + stmt_len] = '\n';
  next[old_len + stmt_len + 1] = '\0';
  free(session->import_prelude);
  session->import_prelude = next;
}

static const char *ylc_wasm_source_with_imports(YlWasmSession *session,
                                                const char *source) {
  if (!session || !session->import_prelude || !session->import_prelude[0]) {
    return source;
  }

  const char *stmt = NULL;
  size_t stmt_len = 0;
  if (ylc_wasm_source_is_import_only(source, &stmt, &stmt_len) &&
      ylc_wasm_session_has_import(session, stmt, stmt_len)) {
    return source;
  }

  size_t prelude_len = strlen(session->import_prelude);
  size_t source_len = strlen(source);
  char *combined = (char *)malloc(prelude_len + source_len + 1);
  if (!combined) {
    ylc_wasm_error("import prelude source allocation failed");
    return source;
  }
  memcpy(combined, session->import_prelude, prelude_len);
  memcpy(combined + prelude_len, source, source_len + 1);
  return combined;
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
  if (g_session.global_bump + YW_GLOBAL_SLOT_SIZE > YW_GLOBAL_END) {
    ylc_wasm_error("global slot table full");
    return 0;
  }
  uint32_t addr = g_session.global_bump;
  if (!ylc_wasm_ensure_memory(addr + YW_GLOBAL_SLOT_SIZE)) {
    ylc_wasm_error("could not grow wasm memory for globals");
    return 0;
  }
  g_session.global_bump += YW_GLOBAL_SLOT_SIZE; // one scalar slot per global
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
                                     YlWasmSession *session,
                                     bool persist_session) {
  *out_arena = NULL;

  const char *frontend_source =
      ylc_wasm_source_with_imports(session, source);
  Ast *prog = parse_input_buffer(filename, frontend_source);
  if (!prog) {
    ylc_wasm_error("parse failed");
    return NULL;
  }

  TICtx ti_ctx = {.env = session ? session->env : NULL,
                  .scope = 0,
                  .err_stream = stderr};
  if (!infer(prog, &ti_ctx)) {
    ylc_wasm_error("typecheck failed");
    return NULL;
  }
  if (persist_session && session) {
    session->env = ti_ctx.env;
  }

  escape_analysis(prog);

  MirArena *arena = mir_arena_create();
  if (!arena) {
    ylc_wasm_error("mir arena alloc failed");
    return NULL;
  }
  *out_arena = arena;

  ht transient_table;
  MirStackFrame transient_frame;
  MirStackFrame *frame = NULL;
  if (session) {
    if (persist_session) {
      frame = &session->mir_root_frame;
    } else {
      mir_stack_frame_init(arena, &transient_table, &transient_frame,
                           &session->mir_root_frame);
      frame = &transient_frame;
    }
  }
  MirCtx mir_ctx = {.env = ti_ctx.env, .frame = frame};

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

static void wasm_buf_append(WasmBuf *dst, const WasmBuf *src) {
  if (!dst || !src) {
    return;
  }
  for (size_t i = 0; i < src->size; i++) {
    wasm_buf_push(dst, src->data[i]);
  }
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

static void wasm_buf_leb_i64(WasmBuf *b, int64_t v) {
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

static void wasm_buf_bytes(WasmBuf *b, const void *src, size_t len) {
  const uint8_t *bytes = (const uint8_t *)src;
  for (size_t i = 0; i < len; i++) {
    wasm_buf_push(b, bytes[i]);
  }
}

static void wasm_buf_f32(WasmBuf *b, float v) {
  uint8_t bytes[sizeof(float)];
  memcpy(bytes, &v, sizeof(bytes));
  wasm_buf_bytes(b, bytes, sizeof(bytes));
}

static void wasm_buf_f64(WasmBuf *b, double v) {
  uint8_t bytes[sizeof(double)];
  memcpy(bytes, &v, sizeof(bytes));
  wasm_buf_bytes(b, bytes, sizeof(bytes));
}

static void wasm_buf_name(WasmBuf *b, const char *s) {
  size_t n = strlen(s);
  wasm_buf_leb_u32(b, (uint32_t)n);
  for (size_t i = 0; i < n; i++) {
    wasm_buf_push(b, (uint8_t)s[i]);
  }
}

static void wasm_buf_section(WasmBuf *out, uint8_t id, const WasmBuf *body) {
  wasm_buf_push(out, id);
  wasm_buf_leb_u32(out, body ? (uint32_t)body->size : 0);
  if (body) {
    wasm_buf_append(out, body);
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
  YWV_VOID,
  YWV_LOCAL,
  YWV_STACK,
} YwValueKind;

typedef struct {
  YwValueKind kind;
  uint32_t local; // valid when kind == YWV_LOCAL
} YwValueSlot;

typedef struct {
  MirFunction *fn;
  struct YwModuleCtx *module;
  YwValueSlot *slots; // length == fn->values.len
  size_t slots_len;
  uint32_t next_local;
  uint32_t param_count;
  uint8_t *local_types; // indexed by wasm local index, params included
  size_t local_types_len;
  size_t local_types_cap;
  bool ok;
} YwFnCtx;

static bool yw_type_to_val(Type *type, uint8_t *out) {
  if (!type || !out) {
    return false;
  }

  switch (type->kind) {
  case T_INT:
  case T_BOOL:
  case T_CHAR:
    *out = 0x7f; // i32
    return true;
  case T_UINT64:
    *out = 0x7e; // i64
    return true;
  case T_NUM:
    *out = 0x7c; // f64
    return true;
  case T_FN:
  case T_CONS:
  case T_SUM:
  case T_STRING:
  case T_EMPTY_LIST:
    *out = 0x7f; // pointer/reference representation for this early backend
    return true;
  case T_VOID:
    return false;
  default:
    return false;
  }
}

static bool yw_type_has_value(Type *type, uint8_t *out) {
  if (!type || type->kind == T_VOID || type->kind == T_MODULE) {
    return false;
  }
  return yw_type_to_val(type, out);
}

static bool yw_type_has_type_vars(Type *type) {
  if (!type) {
    return false;
  }

  switch (type->kind) {
  case T_VAR:
    return true;
  case T_FN:
    return yw_type_has_type_vars(type->closure_meta) ||
           yw_type_has_type_vars(type->data.T_FN.from) ||
           yw_type_has_type_vars(type->data.T_FN.to);
  case T_CONS:
  case T_SUM:
    if (yw_type_has_type_vars(type->closure_meta)) {
      return true;
    }
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (type->data.T_CONS.args &&
          yw_type_has_type_vars(type->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  default:
    return false;
  }
}

static bool yw_param_is_env(MirParam *param) {
  return param && param->name && strcmp(param->name, "$env") == 0;
}

static bool yw_param_is_lowered(MirParam *param, uint8_t *out_type) {
  return param && param->type && param->type->kind != T_VOID &&
         !yw_param_is_env(param) && yw_type_to_val(param->type, out_type);
}

static Type *yw_function_return_type(MirFunction *fn) {
  if (!fn || !fn->type) {
    return &t_void;
  }

  Type *type = fn->type;
  bool has_env_param = false;
  for (size_t i = 0; type && type->kind == T_FN && i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (yw_param_is_env(param)) {
      has_env_param = true;
      if (!(type->data.T_FN.from && param->type &&
            types_equal(type->data.T_FN.from, param->type))) {
        continue;
      }
    }
    type = type->data.T_FN.to;
  }

  if (has_env_param && type && type->kind == T_FN && type->data.T_FN.from &&
      type->data.T_FN.from->kind == T_VOID) {
    return type->data.T_FN.to;
  }

  if (type && type->kind == T_FN && !is_closure(type) && type->data.T_FN.from &&
      type->data.T_FN.from->kind == T_VOID) {
    return type->data.T_FN.to;
  }

  return type ? type : &t_void;
}

static bool yw_function_result_type(MirFunction *fn, uint8_t *out) {
  Type *ret = yw_function_return_type(fn);
  return ret && ret->kind != T_VOID && yw_type_to_val(ret, out);
}

static bool yw_fn_error(YwFnCtx *c, const char *fmt, ...) {
  if (c) {
    c->ok = false;
  }
  fprintf(stderr, "[ylc-wasm] ");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  return false;
}

static bool yw_fn_alloc_local(YwFnCtx *c, uint8_t type, uint32_t *out) {
  if (!c || !out) {
    return false;
  }
  if (c->local_types_len >= c->local_types_cap) {
    size_t next_cap = c->local_types_cap ? c->local_types_cap * 2 : 8;
    uint8_t *next =
        (uint8_t *)realloc(c->local_types, next_cap * sizeof(uint8_t));
    if (!next) {
      return yw_fn_error(c, "local type allocation failed in %s",
                         c->fn && c->fn->name ? c->fn->name : "<anonymous>");
    }
    c->local_types = next;
    c->local_types_cap = next_cap;
  }

  uint32_t index = (uint32_t)c->local_types_len;
  c->local_types[c->local_types_len++] = type;
  c->next_local = (uint32_t)c->local_types_len;
  *out = index;
  return true;
}

static bool yw_fn_ctx_init(YwFnCtx *c, MirFunction *fn,
                           struct YwModuleCtx *module) {
  memset(c, 0, sizeof(*c));
  c->fn = fn;
  c->module = module;
  c->slots_len = fn->values.len;
  c->slots = c->slots_len ? calloc(c->slots_len, sizeof(YwValueSlot)) : NULL;
  c->ok = true;
  if (c->slots_len && !c->slots) {
    return false;
  }
  for (size_t i = 0; i < c->slots_len; i++) {
    c->slots[i].kind = YWV_PENDING;
  }

  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    uint8_t wasm_type = 0;
    if (!yw_param_is_lowered(param, &wasm_type)) {
      if (param && param->value < c->slots_len &&
          ((param->type && param->type->kind == T_VOID) ||
           yw_param_is_env(param))) {
        c->slots[param->value].kind = YWV_VOID;
      }
      continue;
    }
    if (param->value >= c->slots_len) {
      return yw_fn_error(c, "param value %u out of range in %s", param->value,
                         fn && fn->name ? fn->name : "<anonymous>");
    }

    uint32_t local = 0;
    if (!yw_fn_alloc_local(c, wasm_type, &local)) {
      return false;
    }
    c->slots[param->value] = (YwValueSlot){.kind = YWV_LOCAL, .local = local};
    c->param_count++;
  }
  return true;
}

static void yw_fn_ctx_free(YwFnCtx *c) {
  free(c->slots);
  c->slots = NULL;
  free(c->local_types);
  c->local_types = NULL;
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

typedef struct {
  uint8_t *params;
  size_t params_len;
  bool has_result;
  uint8_t result;
} YwFuncType;

typedef struct {
  YwFuncType *items;
  size_t len;
  size_t cap;
} YwFuncTypeVec;

typedef struct {
  MirFunction *fn;
  uint32_t type_index;
  uint32_t func_index;
  bool has_result;
  uint8_t result_type;
} YwFuncDef;

typedef struct {
  YwFuncDef *items;
  size_t len;
  size_t cap;
} YwFuncDefVec;

typedef struct YwModuleCtx {
  YwFuncTypeVec types;
  YwFuncDefVec imports;
  YwFuncDefVec defs;
  uint32_t import_func_count;
  uint32_t print_int_func;
  uint32_t print_string_func;
  uint32_t abort_func;
  uint32_t print_int_type;
  uint32_t print_string_type;
  uint32_t abort_type;
} YwModuleCtx;

static void yw_func_type_vec_free(YwFuncTypeVec *vec) {
  if (!vec) {
    return;
  }
  for (size_t i = 0; i < vec->len; i++) {
    free(vec->items[i].params);
  }
  free(vec->items);
  *vec = (YwFuncTypeVec){0};
}

static void yw_func_def_vec_free(YwFuncDefVec *vec) {
  if (!vec) {
    return;
  }
  free(vec->items);
  *vec = (YwFuncDefVec){0};
}

static bool yw_func_type_eq(const YwFuncType *type, const uint8_t *params,
                            size_t params_len, bool has_result,
                            uint8_t result) {
  if (!type || type->params_len != params_len ||
      type->has_result != has_result) {
    return false;
  }
  if (has_result && type->result != result) {
    return false;
  }
  for (size_t i = 0; i < params_len; i++) {
    if (type->params[i] != params[i]) {
      return false;
    }
  }
  return true;
}

static bool yw_type_table_add(YwModuleCtx *m, const uint8_t *params,
                              size_t params_len, bool has_result,
                              uint8_t result, uint32_t *out_index) {
  if (!m || !out_index) {
    return false;
  }
  for (size_t i = 0; i < m->types.len; i++) {
    if (yw_func_type_eq(&m->types.items[i], params, params_len, has_result,
                        result)) {
      *out_index = (uint32_t)i;
      return true;
    }
  }

  if (m->types.len >= m->types.cap) {
    size_t next_cap = m->types.cap ? m->types.cap * 2 : 8;
    YwFuncType *next =
        (YwFuncType *)realloc(m->types.items, next_cap * sizeof(YwFuncType));
    if (!next) {
      return false;
    }
    m->types.items = next;
    m->types.cap = next_cap;
  }

  uint8_t *params_copy = NULL;
  if (params_len) {
    params_copy = (uint8_t *)malloc(params_len);
    if (!params_copy) {
      return false;
    }
    memcpy(params_copy, params, params_len);
  }

  m->types.items[m->types.len] = (YwFuncType){
      .params = params_copy,
      .params_len = params_len,
      .has_result = has_result,
      .result = result,
  };
  *out_index = (uint32_t)m->types.len;
  m->types.len++;
  return true;
}

static bool yw_func_def_vec_push(YwFuncDefVec *vec, YwFuncDef def) {
  if (!vec) {
    return false;
  }
  if (vec->len >= vec->cap) {
    size_t next_cap = vec->cap ? vec->cap * 2 : 8;
    YwFuncDef *next =
        (YwFuncDef *)realloc(vec->items, next_cap * sizeof(YwFuncDef));
    if (!next) {
      return false;
    }
    vec->items = next;
    vec->cap = next_cap;
  }
  vec->items[vec->len++] = def;
  return true;
}

static bool yw_function_param_types(MirFunction *fn, uint8_t **out_params,
                                    size_t *out_len) {
  if (!fn || !out_params || !out_len) {
    return false;
  }
  *out_params = NULL;
  *out_len = 0;

  size_t count = 0;
  for (size_t i = 0; i < fn->params.len; i++) {
    uint8_t ignored = 0;
    if (yw_param_is_lowered(&fn->params.items[i], &ignored)) {
      count++;
    }
  }
  if (!count) {
    return true;
  }

  uint8_t *params = (uint8_t *)malloc(count);
  if (!params) {
    return false;
  }
  size_t out = 0;
  for (size_t i = 0; i < fn->params.len; i++) {
    uint8_t wasm_type = 0;
    if (yw_param_is_lowered(&fn->params.items[i], &wasm_type)) {
      params[out++] = wasm_type;
    }
  }

  *out_params = params;
  *out_len = count;
  return true;
}

static MirFunction *yw_instr_direct_call_target(MirFunction *fn,
                                                MirInstr *instr) {
  if (!fn || !instr || instr->kind != MIR_CALL || instr->data.call.builtin) {
    return NULL;
  }
  if (instr->data.call.specialized_fn) {
    return instr->data.call.specialized_fn;
  }
  if (instr->data.call.callee == MIR_NO_VALUE) {
    return NULL;
  }
  MirInstr *callee_def =
      mir_function_find_def_instr(fn, instr->data.call.callee);
  if (!callee_def || callee_def->kind != MIR_FN_REF) {
    return NULL;
  }
  return callee_def->data.fn_ref.fn;
}

static bool yw_function_body_supported_for_now_depth(MirFunction *fn,
                                                     unsigned depth) {
  if (!fn || fn->is_extern) {
    return true;
  }
  if (depth > 32 || yw_type_has_type_vars(fn->type)) {
    return false;
  }

  for (size_t b = 0; b < fn->blocks.len; b++) {
    MirBlock *block = fn->blocks.items[b];
    if (!block) {
      continue;
    }
    for (size_t i = 0; i < block->instrs.len; i++) {
      switch (block->instrs.items[i].kind) {
      case MIR_CONST:
      case MIR_OP:
      case MIR_FN_REF:
        break;
      case MIR_CALL: {
        MirFunction *target =
            yw_instr_direct_call_target(fn, &block->instrs.items[i]);
        if (!target || (target != fn && !target->is_extern &&
                        !yw_function_body_supported_for_now_depth(target,
                                                                  depth + 1))) {
          return false;
        }
        break;
      }
      default:
        return false;
      }
    }

    switch (block->term.kind) {
    case MIR_TERM_NONE:
    case MIR_TERM_RETURN:
    case MIR_TERM_UNREACHABLE:
      break;
    default:
      return false;
    }
  }

  return true;
}

static bool yw_function_body_supported_for_now(MirFunction *fn) {
  return yw_function_body_supported_for_now_depth(fn, 0);
}

static bool yw_add_func_def(YwModuleCtx *m, MirFunction *fn,
                            YwFuncDefVec *vec, uint32_t func_index) {
  if (!m || !fn || !vec) {
    return false;
  }

  uint8_t *params = NULL;
  size_t params_len = 0;
  if (!yw_function_param_types(fn, &params, &params_len)) {
    free(params);
    return false;
  }

  uint8_t result = 0;
  bool has_result = yw_function_result_type(fn, &result);
  Type *ret = yw_function_return_type(fn);
  if (ret && ret->kind != T_VOID && ret->kind != T_MODULE && !has_result) {
    fprintf(stderr, "[ylc-wasm] unsupported return type for %s\n",
            fn->name ? fn->name : "<anonymous>");
    free(params);
    return false;
  }

  uint32_t type_index = 0;
  bool ok =
      yw_type_table_add(m, params, params_len, has_result, result, &type_index);
  free(params);
  if (!ok) {
    return false;
  }

  YwFuncDef def = {
      .fn = fn,
      .type_index = type_index,
      .func_index = func_index,
      .has_result = has_result,
      .result_type = result,
  };
  return yw_func_def_vec_push(vec, def);
}

static bool yw_module_ctx_init(YwModuleCtx *m, MirProgram *prog) {
  memset(m, 0, sizeof(*m));

  uint8_t i32_param = 0x7f;
  if (!yw_type_table_add(m, &i32_param, 1, false, 0, &m->print_int_type) ||
      !yw_type_table_add(m, &i32_param, 1, false, 0, &m->print_string_type) ||
      !yw_type_table_add(m, NULL, 0, false, 0, &m->abort_type)) {
    return false;
  }

  m->print_int_func = 0;
  m->print_string_func = 1;
  m->abort_func = 2;

  if (!prog) {
    return false;
  }

  for (size_t i = 0; i < prog->functions.len; i++) {
    MirFunction *fn = prog->functions.items[i];
    if (!fn || !fn->is_extern || yw_type_has_type_vars(fn->type)) {
      continue;
    }

    uint32_t func_index = 3 + (uint32_t)m->imports.len;
    if (!yw_add_func_def(m, fn, &m->imports, func_index)) {
      return false;
    }
  }

  m->import_func_count = 3 + (uint32_t)m->imports.len;

  for (size_t i = 0; i < prog->functions.len; i++) {
    MirFunction *fn = prog->functions.items[i];
    if (!fn || fn->is_extern) {
      continue;
    }
    if (yw_type_has_type_vars(fn->type) &&
        (!fn->name || strcmp(fn->name, "$top") != 0)) {
      continue;
    }
    if ((!fn->name || strcmp(fn->name, "$top") != 0) &&
        !yw_function_body_supported_for_now(fn)) {
      continue;
    }

    uint32_t func_index = m->import_func_count + (uint32_t)m->defs.len;
    if (!yw_add_func_def(m, fn, &m->defs, func_index)) {
      return false;
    }
  }

  return true;
}

static void yw_module_ctx_free(YwModuleCtx *m) {
  if (!m) {
    return;
  }
  yw_func_type_vec_free(&m->types);
  yw_func_def_vec_free(&m->imports);
  yw_func_def_vec_free(&m->defs);
}

static YwFuncDef *yw_module_find_func(YwModuleCtx *m, const char *name) {
  if (!m || !name) {
    return NULL;
  }
  for (size_t i = 0; i < m->imports.len; i++) {
    MirFunction *fn = m->imports.items[i].fn;
    if (fn && fn->name && strcmp(fn->name, name) == 0) {
      return &m->imports.items[i];
    }
  }
  for (size_t i = 0; i < m->defs.len; i++) {
    MirFunction *fn = m->defs.items[i].fn;
    if (fn && fn->name && strcmp(fn->name, name) == 0) {
      return &m->defs.items[i];
    }
  }
  return NULL;
}

static YwFuncDef *yw_module_find_func_by_ptr(YwModuleCtx *m, MirFunction *fn) {
  if (!m || !fn) {
    return NULL;
  }
  for (size_t i = 0; i < m->imports.len; i++) {
    if (m->imports.items[i].fn == fn) {
      return &m->imports.items[i];
    }
  }
  for (size_t i = 0; i < m->defs.len; i++) {
    if (m->defs.items[i].fn == fn) {
      return &m->defs.items[i];
    }
  }
  return NULL;
}

static bool yw_build_type_section(YwModuleCtx *m, WasmBuf *body) {
  if (!m || !body) {
    return false;
  }
  wasm_buf_leb_u32(body, (uint32_t)m->types.len);
  for (size_t i = 0; i < m->types.len; i++) {
    YwFuncType *type = &m->types.items[i];
    wasm_buf_push(body, 0x60);
    wasm_buf_leb_u32(body, (uint32_t)type->params_len);
    for (size_t j = 0; j < type->params_len; j++) {
      wasm_buf_push(body, type->params[j]);
    }
    wasm_buf_leb_u32(body, type->has_result ? 1 : 0);
    if (type->has_result) {
      wasm_buf_push(body, type->result);
    }
  }
  return true;
}

static void yw_build_import_section(YwModuleCtx *m, WasmBuf *body) {
  wasm_buf_leb_u32(body, 4 + (uint32_t)m->imports.len);

  wasm_buf_name(body, "env");
  wasm_buf_name(body, "memory");
  wasm_buf_push(body, 0x02); // memory
  wasm_buf_push(body, 0x00); // limits: min only
  wasm_buf_leb_u32(body, 1);

  wasm_buf_name(body, "env");
  wasm_buf_name(body, "ylc_print_int");
  wasm_buf_push(body, 0x00); // function
  wasm_buf_leb_u32(body, m->print_int_type);

  wasm_buf_name(body, "env");
  wasm_buf_name(body, "ylc_print_string");
  wasm_buf_push(body, 0x00);
  wasm_buf_leb_u32(body, m->print_string_type);

  wasm_buf_name(body, "env");
  wasm_buf_name(body, "ylc_abort");
  wasm_buf_push(body, 0x00);
  wasm_buf_leb_u32(body, m->abort_type);

  for (size_t i = 0; i < m->imports.len; i++) {
    MirFunction *fn = m->imports.items[i].fn;
    wasm_buf_name(body, "env");
    wasm_buf_name(body, fn && fn->name ? fn->name : "<anonymous>");
    wasm_buf_push(body, 0x00);
    wasm_buf_leb_u32(body, m->imports.items[i].type_index);
  }
}

static void yw_build_function_section(YwModuleCtx *m, WasmBuf *body) {
  wasm_buf_leb_u32(body, (uint32_t)m->defs.len);
  for (size_t i = 0; i < m->defs.len; i++) {
    wasm_buf_leb_u32(body, m->defs.items[i].type_index);
  }
}

static bool yw_build_export_section(YwModuleCtx *m, WasmBuf *body) {
  YwFuncDef *entry = yw_module_find_func(m, "$top");
  if (!entry && m->defs.len > 0) {
    entry = &m->defs.items[0];
  }
  if (!entry) {
    fprintf(stderr, "[ylc-wasm] no function available to export\n");
    return false;
  }

  wasm_buf_leb_u32(body, 1);
  wasm_buf_name(body, "ylc_entry");
  wasm_buf_push(body, 0x00); // function
  wasm_buf_leb_u32(body, entry->func_index);
  return true;
}

// ---- 4. Per-function lowering --------------------------------------------
static bool yw_slot_for_result(YwFnCtx *c, MirInstr *instr, YwValueSlot *out) {
  if (!c || !instr || instr->result == MIR_NO_VALUE ||
      instr->result >= c->slots_len || !out) {
    return false;
  }
  *out = c->slots[instr->result];
  return true;
}

static bool yw_local_type(YwFnCtx *c, uint32_t local, uint8_t *out) {
  if (!c || !out || local >= c->local_types_len) {
    return false;
  }
  *out = c->local_types[local];
  return true;
}

static bool yw_value_type(YwFnCtx *c, MirValueId value, uint8_t *out) {
  if (!c || value == MIR_NO_VALUE || value >= c->slots_len || !out) {
    return false;
  }
  YwValueSlot slot = c->slots[value];
  if (slot.kind == YWV_LOCAL) {
    return yw_local_type(c, slot.local, out);
  }
  Type *type = mir_function_value_type(c->fn, value);
  return type && type->kind != T_VOID && yw_type_to_val(type, out);
}

static bool yw_emit_value(YwFnCtx *c, MirValueId value, WasmBuf *code) {
  if (!c || value == MIR_NO_VALUE || value >= c->slots_len) {
    return yw_fn_error(c, "value %u out of range in %s", value,
                       c && c->fn && c->fn->name ? c->fn->name : "<anonymous>");
  }

  YwValueSlot slot = c->slots[value];
  switch (slot.kind) {
  case YWV_LOCAL:
    wasm_buf_push(code, 0x20); // local.get
    wasm_buf_leb_u32(code, slot.local);
    return true;
  case YWV_STACK:
    return true;
  case YWV_VOID:
    return yw_fn_error(c, "attempted to use void value %u in %s", value,
                       c->fn && c->fn->name ? c->fn->name : "<anonymous>");
  case YWV_PENDING:
  default:
    return yw_fn_error(c, "value %u was not materialized in %s", value,
                       c->fn && c->fn->name ? c->fn->name : "<anonymous>");
  }
}

static bool yw_declare_instr_locals(YwFnCtx *c) {
  if (!c || !c->fn) {
    return false;
  }

  for (size_t b = 0; b < c->fn->blocks.len; b++) {
    MirBlock *block = c->fn->blocks.items[b];
    if (!block) {
      continue;
    }
    for (size_t i = 0; i < block->instrs.len; i++) {
      MirInstr *instr = &block->instrs.items[i];
      if (instr->result == MIR_NO_VALUE) {
        continue;
      }
      if (instr->result >= c->slots_len) {
        return yw_fn_error(c, "instruction result %u out of range in %s",
                           instr->result,
                           c->fn->name ? c->fn->name : "<anonymous>");
      }
      if (c->slots[instr->result].kind != YWV_PENDING) {
        continue;
      }

      uint8_t wasm_type = 0;
      if (!yw_type_has_value(instr->type, &wasm_type)) {
        c->slots[instr->result].kind = YWV_VOID;
        continue;
      }

      uint32_t local = 0;
      if (!yw_fn_alloc_local(c, wasm_type, &local)) {
        return false;
      }
      c->slots[instr->result] =
          (YwValueSlot){.kind = YWV_LOCAL, .local = local};
    }
  }
  return true;
}

static void yw_emit_local_decls(YwFnCtx *c, WasmBuf *body) {
  uint32_t locals_len = (uint32_t)c->local_types_len;
  uint32_t decls =
      locals_len >= c->param_count ? locals_len - c->param_count : 0;
  wasm_buf_leb_u32(body, decls);
  for (uint32_t i = c->param_count; i < locals_len; i++) {
    wasm_buf_leb_u32(body, 1);
    wasm_buf_push(body, c->local_types[i]);
  }
}

static bool yw_emit_result_store(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  YwValueSlot slot = {0};
  if (!yw_slot_for_result(c, instr, &slot)) {
    return true;
  }
  switch (slot.kind) {
  case YWV_LOCAL:
    wasm_buf_push(code, 0x21); // local.set
    wasm_buf_leb_u32(code, slot.local);
    return true;
  case YWV_VOID:
    return true;
  case YWV_STACK:
    return true;
  case YWV_PENDING:
  default:
    return yw_fn_error(c, "instruction result %u was not declared in %s",
                       instr->result,
                       c->fn && c->fn->name ? c->fn->name : "<anonymous>");
  }
}

static bool yw_memory_ops_for_val(uint8_t val_type, uint8_t *load,
                                  uint8_t *store, uint32_t *align) {
  switch (val_type) {
  case 0x7f:
    if (load) {
      *load = 0x28; // i32.load
    }
    if (store) {
      *store = 0x36; // i32.store
    }
    if (align) {
      *align = 2;
    }
    return true;
  case 0x7e:
    if (load) {
      *load = 0x29; // i64.load
    }
    if (store) {
      *store = 0x37; // i64.store
    }
    if (align) {
      *align = 3;
    }
    return true;
  case 0x7d:
    if (load) {
      *load = 0x2a; // f32.load
    }
    if (store) {
      *store = 0x38; // f32.store
    }
    if (align) {
      *align = 2;
    }
    return true;
  case 0x7c:
    if (load) {
      *load = 0x2b; // f64.load
    }
    if (store) {
      *store = 0x39; // f64.store
    }
    if (align) {
      *align = 3;
    }
    return true;
  default:
    return false;
  }
}

static bool yw_type_is_string(Type *type) {
  return type && (type->kind == T_STRING ||
                  (type->alias && strcmp(type->alias, TYPE_NAME_STRING) == 0));
}

// The real work. Each function here emits the value for one MIR instruction
// onto the wasm stack; yw_lower_instr then spills it into the predeclared
// result local when the instruction has a value result.

static bool yw_lower_const(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  MirConst *k = &instr->data.const_value;
  YwValueSlot slot = {0};
  uint8_t result_type = 0x7f;
  bool has_result = yw_slot_for_result(c, instr, &slot) &&
                    slot.kind == YWV_LOCAL &&
                    yw_local_type(c, slot.local, &result_type);

  if (!has_result && slot.kind == YWV_VOID) {
    return true;
  }

  switch (k->kind) {
  case MIR_CONST_KIND_INT:
    wasm_buf_push(code, 0x41); // i32.const
    wasm_buf_leb_i32(code, k->as.int_value);
    return true;
  case MIR_CONST_KIND_UINT64:
    if (result_type == 0x7e) {
      wasm_buf_push(code, 0x42); // i64.const
      wasm_buf_leb_i64(code, (int64_t)k->as.uint64_value);
    } else {
      wasm_buf_push(code, 0x41);
      wasm_buf_leb_i32(code, (int32_t)k->as.uint64_value);
    }
    return true;
  case MIR_CONST_KIND_FLOAT:
    if (result_type == 0x7d) {
      wasm_buf_push(code, 0x43); // f32.const
      wasm_buf_f32(code, k->as.float_value);
    } else {
      wasm_buf_push(code, 0x44); // f64.const
      wasm_buf_f64(code, (double)k->as.float_value);
    }
    return true;
  case MIR_CONST_KIND_DOUBLE:
    wasm_buf_push(code, 0x44); // f64.const
    wasm_buf_f64(code, k->as.double_value);
    return true;
  case MIR_CONST_KIND_BOOL:
    wasm_buf_push(code, 0x41);
    wasm_buf_leb_i32(code, k->as.bool_value ? 1 : 0);
    return true;
  case MIR_CONST_KIND_CHAR:
    wasm_buf_push(code, 0x41);
    wasm_buf_leb_i32(code, k->as.char_value);
    return true;
  case MIR_CONST_KIND_VOID:
  case MIR_CONST_KIND_UNDEF:
    return true;
  default:
    return yw_fn_error(c, "const kind %d not yet implemented", k->kind);
  }
}

static bool yw_primitive_opcode(MirPrimitiveOp op, uint8_t operand_type,
                                uint8_t *opcode) {
  bool is_i64 = operand_type == 0x7e;
  bool is_f32 = operand_type == 0x7d;
  bool is_f64 = operand_type == 0x7c;

  switch (op) {
  case MIR_OP_IADD:
  case MIR_OP_UADD:
    *opcode = is_i64 ? 0x7c : 0x6a;
    return true;
  case MIR_OP_ISUB:
  case MIR_OP_USUB:
    *opcode = is_i64 ? 0x7d : 0x6b;
    return true;
  case MIR_OP_IMUL:
  case MIR_OP_UMUL:
    *opcode = is_i64 ? 0x7e : 0x6c;
    return true;
  case MIR_OP_IDIV:
    *opcode = is_i64 ? 0x7f : 0x6d;
    return true;
  case MIR_OP_UDIV:
    *opcode = is_i64 ? 0x80 : 0x6e;
    return true;
  case MIR_OP_IMOD:
    *opcode = is_i64 ? 0x81 : 0x6f;
    return true;
  case MIR_OP_UMOD:
    *opcode = is_i64 ? 0x82 : 0x70;
    return true;

  case MIR_OP_FADD:
    *opcode = is_f32 ? 0x92 : 0xa0;
    return is_f32 || is_f64;
  case MIR_OP_FSUB:
    *opcode = is_f32 ? 0x93 : 0xa1;
    return is_f32 || is_f64;
  case MIR_OP_FMUL:
    *opcode = is_f32 ? 0x94 : 0xa2;
    return is_f32 || is_f64;
  case MIR_OP_FDIV:
    *opcode = is_f32 ? 0x95 : 0xa3;
    return is_f32 || is_f64;

  case MIR_OP_IEQ:
  case MIR_OP_UEQ:
  case MIR_OP_CEQ:
  case MIR_OP_BEQ:
    *opcode = is_i64 ? 0x51 : 0x46;
    return true;
  case MIR_OP_FEQ:
    *opcode = is_f32 ? 0x5b : 0x61;
    return is_f32 || is_f64;
  case MIR_OP_IGT:
  case MIR_OP_CGT:
    *opcode = is_i64 ? 0x55 : 0x4a;
    return true;
  case MIR_OP_UGT:
    *opcode = is_i64 ? 0x56 : 0x4b;
    return true;
  case MIR_OP_FGT:
    *opcode = is_f32 ? 0x5e : 0x64;
    return is_f32 || is_f64;
  case MIR_OP_IGTE:
  case MIR_OP_CGTE:
    *opcode = is_i64 ? 0x59 : 0x4e;
    return true;
  case MIR_OP_UGTE:
    *opcode = is_i64 ? 0x5a : 0x4f;
    return true;
  case MIR_OP_FGTE:
    *opcode = is_f32 ? 0x60 : 0x66;
    return is_f32 || is_f64;
  case MIR_OP_ILT:
  case MIR_OP_CLT:
    *opcode = is_i64 ? 0x53 : 0x48;
    return true;
  case MIR_OP_ULT:
    *opcode = is_i64 ? 0x54 : 0x49;
    return true;
  case MIR_OP_FLT:
    *opcode = is_f32 ? 0x5d : 0x63;
    return is_f32 || is_f64;
  case MIR_OP_ILTE:
  case MIR_OP_CLTE:
    *opcode = is_i64 ? 0x57 : 0x4c;
    return true;
  case MIR_OP_ULTE:
    *opcode = is_i64 ? 0x58 : 0x4d;
    return true;
  case MIR_OP_FLTE:
    *opcode = is_f32 ? 0x5f : 0x65;
    return is_f32 || is_f64;
  default:
    return false;
  }
}

static bool yw_lower_primitive(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  MirOp *op = &instr->data.op;
  if (op->primitive == MIR_OP_LNOT) {
    if (op->argc != 1) {
      return yw_fn_error(c, "logical not expected 1 operand");
    }
    uint8_t operand_type = 0;
    if (!yw_value_type(c, op->operands[0], &operand_type) ||
        !yw_emit_value(c, op->operands[0], code)) {
      return false;
    }
    wasm_buf_push(code, operand_type == 0x7e ? 0x50 : 0x45); // eqz
    return true;
  }

  if (op->argc != 2) {
    return yw_fn_error(c, "primitive op %d expected 2 operands", op->primitive);
  }

  uint8_t operand_type = 0;
  if (!yw_value_type(c, op->operands[0], &operand_type)) {
    return yw_fn_error(c, "could not determine primitive operand type");
  }

  uint8_t opcode = 0;
  if (!yw_primitive_opcode(op->primitive, operand_type, &opcode)) {
    return yw_fn_error(c, "primitive op %d not yet implemented", op->primitive);
  }

  return yw_emit_value(c, op->operands[0], code) &&
         yw_emit_value(c, op->operands[1], code) &&
         (wasm_buf_push(code, opcode), true);
}

static bool yw_lower_global_load(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  MirOp *op = &instr->data.op;
  uint32_t addr = ylc_wasm_global_address(op->global_name);
  if (!addr) {
    return yw_fn_error(c, "could not allocate global %s",
                       op->global_name ? op->global_name : "<anonymous>");
  }

  uint8_t val_type = 0;
  if (!yw_type_has_value(instr->type, &val_type)) {
    return true;
  }
  uint8_t load = 0;
  uint32_t align = 0;
  if (!yw_memory_ops_for_val(val_type, &load, NULL, &align)) {
    return yw_fn_error(c, "unsupported global load type in %s",
                       c->fn && c->fn->name ? c->fn->name : "<anonymous>");
  }

  wasm_buf_push(code, 0x41); // i32.const address
  wasm_buf_leb_i32(code, (int32_t)addr);
  wasm_buf_push(code, load);
  wasm_buf_leb_u32(code, align);
  wasm_buf_leb_u32(code, 0);
  return true;
}

static bool yw_lower_global_store(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  MirOp *op = &instr->data.op;
  if (op->argc != 1) {
    return yw_fn_error(c, "global_store expected 1 operand");
  }

  uint32_t addr = ylc_wasm_global_address(op->global_name);
  if (!addr) {
    return yw_fn_error(c, "could not allocate global %s",
                       op->global_name ? op->global_name : "<anonymous>");
  }

  uint8_t val_type = 0;
  if (!yw_value_type(c, op->operands[0], &val_type)) {
    return yw_fn_error(c, "could not determine global_store operand type");
  }
  uint8_t store = 0;
  uint32_t align = 0;
  if (!yw_memory_ops_for_val(val_type, NULL, &store, &align)) {
    return yw_fn_error(c, "unsupported global store type in %s",
                       c->fn && c->fn->name ? c->fn->name : "<anonymous>");
  }

  wasm_buf_push(code, 0x41); // i32.const address
  wasm_buf_leb_i32(code, (int32_t)addr);
  if (!yw_emit_value(c, op->operands[0], code)) {
    return false;
  }
  wasm_buf_push(code, store);
  wasm_buf_leb_u32(code, align);
  wasm_buf_leb_u32(code, 0);

  return yw_type_has_value(instr->type, &val_type)
             ? yw_emit_value(c, op->operands[0], code)
             : true;
}

static bool yw_lower_cast(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  MirOp *op = &instr->data.op;
  if (op->argc != 1) {
    return yw_fn_error(c, "cast expected 1 operand");
  }
  if (!yw_emit_value(c, op->operands[0], code)) {
    return false;
  }

  uint8_t from = 0;
  uint8_t to = 0;
  if (!yw_type_to_val(op->from_type, &from) ||
      !yw_type_to_val(op->to_type, &to) || from == to) {
    return true;
  }

  if (from == 0x7e && to == 0x7f) {
    wasm_buf_push(code, 0xa7); // i32.wrap_i64
    return true;
  }
  if (from == 0x7f && to == 0x7e) {
    wasm_buf_push(code, 0xac); // i64.extend_i32_s
    return true;
  }
  if (from == 0x7f && to == 0x7c) {
    wasm_buf_push(code, 0xb7); // f64.convert_i32_s
    return true;
  }
  if (from == 0x7e && to == 0x7c) {
    wasm_buf_push(code, 0xb9); // f64.convert_i64_s
    return true;
  }
  if (from == 0x7c && to == 0x7f) {
    wasm_buf_push(code, 0xaa); // i32.trunc_f64_s
    return true;
  }
  if (from == 0x7c && to == 0x7e) {
    wasm_buf_push(code, 0xae); // i64.trunc_f64_s
    return true;
  }

  return yw_fn_error(c, "unsupported cast in %s",
                     c->fn && c->fn->name ? c->fn->name : "<anonymous>");
}

static bool yw_lower_op(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  MirOp *op = &instr->data.op;
  switch (op->kind) {
  case MIR_OP_KIND_PRIMITIVE:
    return yw_lower_primitive(c, instr, code);
  case MIR_OP_KIND_GLOBAL_LOAD:
    return yw_lower_global_load(c, instr, code);
  case MIR_OP_KIND_GLOBAL_STORE:
    return yw_lower_global_store(c, instr, code);
  case MIR_OP_KIND_CAST:
    return yw_lower_cast(c, instr, code);
  case MIR_OP_KIND_DUP:
    if (op->argc != 1) {
      return yw_fn_error(c, "dup expected 1 operand");
    }
    return yw_emit_value(c, op->operands[0], code);
  case MIR_OP_KIND_DROP:
    if (op->argc != 1 || !yw_emit_value(c, op->operands[0], code)) {
      return false;
    }
    wasm_buf_push(code, 0x1a); // drop
    return true;
  case MIR_OP_KIND_PRINT: {
    if (op->argc != 1) {
      return yw_fn_error(c, "print expected 1 operand");
    }
    Type *arg_type = mir_function_value_type(c->fn, op->operands[0]);
    uint8_t wasm_type = 0;
    if (!yw_emit_value(c, op->operands[0], code) ||
        !yw_value_type(c, op->operands[0], &wasm_type)) {
      return false;
    }
    if (yw_type_is_string(arg_type)) {
      wasm_buf_push(code, 0x10); // call
      wasm_buf_leb_u32(code, c->module->print_string_func);
      return true;
    }
    if (wasm_type == 0x7e) {
      wasm_buf_push(code, 0xa7); // i32.wrap_i64 for the v1 print contract
    } else if (wasm_type != 0x7f) {
      return yw_fn_error(c, "print only supports integer/string values");
    }
    wasm_buf_push(code, 0x10);
    wasm_buf_leb_u32(code, c->module->print_int_func);
    return true;
  }
  case MIR_OP_KIND_FLUSH:
    return true;
  default:
    return yw_fn_error(c, "MIR_OP kind %d not yet implemented", op->kind);
  }
}

static bool yw_lower_phi(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  return yw_fn_error(c, "MIR_PHI lowering requires structured control flow");
}

static bool yw_lower_extract(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  return yw_fn_error(c, "MIR_EXTRACT lowering not yet implemented");
}

static bool yw_lower_construct(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  return yw_fn_error(c, "MIR_CONSTRUCT lowering not yet implemented");
}

static bool yw_lower_fn_ref(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  if (!instr || !instr->data.fn_ref.fn) {
    return yw_fn_error(c, "invalid MIR_FN_REF in %s",
                       c->fn && c->fn->name ? c->fn->name : "<anonymous>");
  }

  // Direct calls resolve this instruction through the MIR value map. Until
  // table/call_indirect support exists, function values materialize as an
  // opaque i32 placeholder so top-level function bindings stay stack-balanced.
  wasm_buf_push(code, 0x41); // i32.const
  wasm_buf_leb_i32(code, 0);
  return true;
}

static YwFuncDef *yw_call_target(YwFnCtx *c, MirInstr *instr) {
  if (!c || !instr || !c->module) {
    return NULL;
  }

  if (instr->data.call.specialized_fn) {
    YwFuncDef *def =
        yw_module_find_func_by_ptr(c->module, instr->data.call.specialized_fn);
    if (def) {
      return def;
    }
  }

  if (instr->data.call.specialized_name) {
    YwFuncDef *def =
        yw_module_find_func(c->module, instr->data.call.specialized_name);
    if (def) {
      return def;
    }
  }

  if (instr->data.call.callee == MIR_NO_VALUE) {
    return NULL;
  }

  MirInstr *callee_def =
      mir_function_find_def_instr(c->fn, instr->data.call.callee);
  if (!callee_def || callee_def->kind != MIR_FN_REF ||
      !callee_def->data.fn_ref.fn) {
    return NULL;
  }

  YwFuncDef *def =
      yw_module_find_func_by_ptr(c->module, callee_def->data.fn_ref.fn);
  if (def) {
    return def;
  }
  return yw_module_find_func(c->module, callee_def->data.fn_ref.name);
}

static bool yw_emit_call_operands(YwFnCtx *c, MirInstr *instr, YwFuncDef *target,
                                  WasmBuf *code) {
  if (!c || !instr || !target || !target->fn) {
    return false;
  }

  size_t emitted = 0;
  for (size_t i = 0; i < instr->data.call.operands.len; i++) {
    MirValueId operand = instr->data.call.operands.items[i];
    if (operand == MIR_NO_VALUE || operand >= c->slots_len) {
      return yw_fn_error(c, "invalid call operand %zu in %s", i,
                         c->fn && c->fn->name ? c->fn->name : "<anonymous>");
    }

    if (i < target->fn->params.len &&
        yw_param_is_env(&target->fn->params.items[i])) {
      continue;
    }

    Type *operand_type = mir_function_value_type(c->fn, operand);
    if (operand_type && operand_type->kind == T_VOID) {
      continue;
    }

    if (!yw_emit_value(c, operand, code)) {
      return false;
    }
    emitted++;
  }

  if (target->type_index >= c->module->types.len) {
    return yw_fn_error(c, "call target %s has invalid type index",
                       target->fn->name ? target->fn->name : "<anonymous>");
  }
  size_t expected = c->module->types.items[target->type_index].params_len;
  if (emitted != expected) {
    return yw_fn_error(c, "call to %s emitted %zu args, expected %zu",
                       target->fn->name ? target->fn->name : "<anonymous>",
                       emitted, expected);
  }
  return true;
}

static bool yw_lower_call(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  if (instr->data.call.builtin) {
    return yw_fn_error(c, "builtin call %s was not lowered before wasm",
                       instr->data.call.builtin->name
                           ? instr->data.call.builtin->name
                           : "<anonymous>");
  }

  YwFuncDef *target = yw_call_target(c, instr);
  if (!target) {
    return yw_fn_error(c, "indirect call lowering not yet implemented in %s",
                       c->fn && c->fn->name ? c->fn->name : "<anonymous>");
  }

  if (!yw_emit_call_operands(c, instr, target, code)) {
    return false;
  }
  wasm_buf_push(code, 0x10); // call
  wasm_buf_leb_u32(code, target->func_index);
  return true;
}

static bool yw_lower_coro(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  return yw_fn_error(c, "MIR_CORO_* lowering not yet implemented");
}

static bool yw_lower_instr(YwFnCtx *c, MirInstr *instr, WasmBuf *code) {
  bool ok = false;
  switch (instr->kind) {
  case MIR_CONST:
    ok = yw_lower_const(c, instr, code);
    break;
  case MIR_OP:
    ok = yw_lower_op(c, instr, code);
    break;
  case MIR_PHI:
    ok = yw_lower_phi(c, instr, code);
    break;
  case MIR_EXTRACT:
    ok = yw_lower_extract(c, instr, code);
    break;
  case MIR_CONSTRUCT:
    ok = yw_lower_construct(c, instr, code);
    break;
  case MIR_FN_REF:
    ok = yw_lower_fn_ref(c, instr, code);
    break;
  case MIR_CALL:
    ok = yw_lower_call(c, instr, code);
    break;
  case MIR_CORO_NEW:
  case MIR_CORO_NEXT:
  case MIR_CORO_RESET:
    ok = yw_lower_coro(c, instr, code);
    break;
  }
  return ok && yw_emit_result_store(c, instr, code);
}

static bool yw_lower_terminator(YwFnCtx *c, MirTerminator *term, WasmBuf *code,
                                bool has_result) {
  switch (term->kind) {
  case MIR_TERM_NONE:
    if (has_result) {
      return yw_fn_error(c, "unterminated non-void block in %s",
                         c->fn && c->fn->name ? c->fn->name : "<anonymous>");
    }
    return true;
  case MIR_TERM_RETURN:
    if (has_result && !yw_emit_value(c, term->value, code)) {
      return false;
    }
    wasm_buf_push(code, 0x0f); // return
    return true;
  case MIR_TERM_BR:
    return yw_fn_error(c, "MIR_TERM_BR requires CFG relooping");
  case MIR_TERM_COND:
    return yw_fn_error(c, "MIR_TERM_COND requires CFG relooping");
  case MIR_TERM_UNREACHABLE:
    wasm_buf_push(code, 0x00); // unreachable
    return true;
  case MIR_TERM_YIELD:
  case MIR_TERM_CORO_RESTART:
  case MIR_TERM_CORO_DONE:
    return yw_fn_error(c, "coroutine terminator %d not yet implemented",
                       term->kind);
  }
  return yw_fn_error(c, "unknown terminator kind %d", term->kind);
}

static size_t yw_terminator_successors(MirTerminator *term, MirBlockId out[2]) {
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
  case MIR_TERM_YIELD:
  case MIR_TERM_CORO_RESTART:
    if (term->target == MIR_NO_BLOCK) {
      return 0;
    }
    out[0] = term->target;
    return 1;
  default:
    return 0;
  }
}

static bool yw_lower_block_body(YwFnCtx *c, bool *visited, MirBlockId block_id,
                                WasmBuf *code, bool has_result) {
  if (!c || !c->fn || !visited || block_id == MIR_NO_BLOCK ||
      block_id >= c->fn->blocks.len) {
    return false;
  }
  if (visited[block_id]) {
    return true;
  }
  visited[block_id] = true;

  MirBlock *block = c->fn->blocks.items[block_id];
  if (!block) {
    return yw_fn_error(c, "missing block bb%u in %s", block_id,
                       c->fn->name ? c->fn->name : "<anonymous>");
  }

  for (size_t i = 0; i < block->instrs.len; i++) {
    if (!yw_lower_instr(c, &block->instrs.items[i], code)) {
      return false;
    }
  }

  if (!yw_lower_terminator(c, &block->term, code, has_result)) {
    return false;
  }

  MirBlockId successors[2] = {MIR_NO_BLOCK, MIR_NO_BLOCK};
  size_t successors_len = yw_terminator_successors(&block->term, successors);
  for (size_t i = 0; i < successors_len; i++) {
    if (!yw_lower_block_body(c, visited, successors[i], code, has_result)) {
      return false;
    }
  }
  return true;
}

static bool yw_build_function_body(YwModuleCtx *m, YwFuncDef *def,
                                   WasmBuf *code_section) {
  if (!m || !def || !def->fn || !code_section) {
    return false;
  }

  MirFunction *fn = def->fn;
  YwFnCtx ctx;
  if (!yw_fn_ctx_init(&ctx, fn, m)) {
    return false;
  }

  bool ok = yw_declare_instr_locals(&ctx);
  WasmBuf body;
  wasm_buf_init(&body);
  if (ok) {
    yw_emit_local_decls(&ctx, &body);
  }

  bool *visited = NULL;
  if (ok && fn->blocks.len > 0) {
    visited = (bool *)calloc(fn->blocks.len, sizeof(bool));
    if (!visited) {
      ok = yw_fn_error(&ctx, "block visited allocation failed in %s",
                       fn->name ? fn->name : "<anonymous>");
    }
  }

  if (ok && fn->blocks.len > 0) {
    ok = yw_lower_block_body(&ctx, visited, 0, &body, def->has_result);
  }
  for (size_t i = 0; ok && i < fn->blocks.len; i++) {
    if (!visited[i]) {
      ok = yw_lower_block_body(&ctx, visited, (MirBlockId)i, &body,
                               def->has_result);
    }
  }
  if (ok && fn->blocks.len == 0 && def->has_result) {
    ok = yw_fn_error(&ctx, "non-void function %s has no body",
                     fn->name ? fn->name : "<anonymous>");
  }

  if (ok) {
    wasm_buf_push(&body, 0x0b); // end
    wasm_buf_leb_u32(code_section, (uint32_t)body.size);
    wasm_buf_append(code_section, &body);
  }

  free(visited);
  wasm_buf_free(&body);
  yw_fn_ctx_free(&ctx);
  return ok;
}

static bool yw_build_code_section(YwModuleCtx *m, WasmBuf *body) {
  if (!m || !body) {
    return false;
  }
  wasm_buf_leb_u32(body, (uint32_t)m->defs.len);
  for (size_t i = 0; i < m->defs.len; i++) {
    if (!yw_build_function_body(m, &m->defs.items[i], body)) {
      return false;
    }
  }
  return true;
}

// ---- 5. Top-level driver -------------------------------------------------
// Builds a single wasm module from the MirProgram. The current skeleton
// emits just the header; populate the section builders above and thread
// them through here. See lang/backend_wasm/wasm.c for the byte-level
// recipe and lang/llvm/lowering.c for the structural walk over
// program->functions.
static uint8_t *ylc_lower_mir_to_wasm(MirProgram *prog, size_t *out_size) {
  *out_size = 0;

  YwModuleCtx module;
  if (!yw_module_ctx_init(&module, prog)) {
    ylc_wasm_error("failed to initialize wasm module context");
    return NULL;
  }

  WasmBuf out;
  wasm_buf_init(&out);
  for (size_t i = 0; i < sizeof(WASM_MAGIC); i++) {
    wasm_buf_push(&out, WASM_MAGIC[i]);
  }

  WasmBuf type_body;
  WasmBuf import_body;
  WasmBuf function_body;
  WasmBuf export_body;
  WasmBuf code_body;
  wasm_buf_init(&type_body);
  wasm_buf_init(&import_body);
  wasm_buf_init(&function_body);
  wasm_buf_init(&export_body);
  wasm_buf_init(&code_body);

  bool ok = yw_build_type_section(&module, &type_body);
  if (ok) {
    yw_build_import_section(&module, &import_body);
    yw_build_function_section(&module, &function_body);
    ok = yw_build_export_section(&module, &export_body);
  }
  if (ok) {
    ok = yw_build_code_section(&module, &code_body);
  }

  if (ok) {
    wasm_buf_section(&out, 0x01, &type_body);
    wasm_buf_section(&out, 0x02, &import_body);
    wasm_buf_section(&out, 0x03, &function_body);
    wasm_buf_section(&out, 0x07, &export_body);
    wasm_buf_section(&out, 0x0a, &code_body);
  }

  uint8_t *result = NULL;
  if (ok) {
    result = (uint8_t *)malloc(out.size ? out.size : 1);
  }
  if (result && ok) {
    memcpy(result, out.data, out.size);
    *out_size = out.size;
  }

  wasm_buf_free(&type_body);
  wasm_buf_free(&import_body);
  wasm_buf_free(&function_body);
  wasm_buf_free(&export_body);
  wasm_buf_free(&code_body);
  wasm_buf_free(&out);
  yw_module_ctx_free(&module);
  return result;
}

// ===========================================================================
// Exported entry points (called from docs/web/wasm.js)
// ===========================================================================

int ylc_wasm_init(void) {
  init_module_registry();
  initialize_builtin_types();
  if (!ylc_wasm_session_init()) {
    return 1;
  }
  ylc_config = (RTConfig){0};
  ylc_config.opt_level = "default<O0>";
  ylc_config.base_libs_dir = "ylc://modules";
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
      ylc_wasm_frontend("<repl>", src_ptr, &arena, &g_session, true);
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
  ylc_wasm_session_remember_import(&g_session, src_ptr);
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
      ylc_wasm_frontend("<dump>", src_ptr, &arena, &g_session, false);
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
