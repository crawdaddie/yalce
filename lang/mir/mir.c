#include "./mir.h"
#include "config.h"
#include "escape_analysis.h"
#include "format_utils.h"
#include "ht.h"
#include "modules.h"
#include "serde.h"
#include "types/builtins.h"
#include "types/inference.h"
#include "types/type_ser.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define MIR_ARENA_DEFAULT_BLOCK_SIZE 32768
#define MIR_ALIGNOF(T) __alignof__(T)

void print_type_to_stream(Type *t, FILE *stream);

static Type *mir_closure_callable_type(MirArena *arena, Type *closure_type);
static MirFunction *mir_program_find_function_by_name(MirProgram *program,
                                                      const char *name);
MirValueId mir_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx);

static void mir_builder_error_at(MirBuilder *builder, Ast *origin,
                                 const char *fmt, ...) {
  if (!builder || !builder->program || builder->program->had_error) {
    return;
  }

  builder->program->had_error = true;
  va_list args;
  va_start(args, fmt);
  fputs("MIR Error: ", stderr);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fputc(' ', stderr);
  if (origin) {
    print_location(origin);
  } else {
    fputc('\n', stderr);
  }
}

static size_t align_forward(size_t value, size_t align) {
  size_t mask = align - 1;
  return (value + mask) & ~mask;
}

static MirArenaBlock *mir_arena_new_block(size_t capacity) {
  MirArenaBlock *block = malloc(sizeof(MirArenaBlock) + capacity);
  if (!block) {
    return NULL;
  }
  block->next = NULL;
  block->used = 0;
  block->capacity = capacity;
  return block;
}

MirArena *mir_arena_create(void) {
  MirArena *arena = malloc(sizeof(MirArena));
  if (!arena) {
    return NULL;
  }
  arena->blocks = NULL;
  arena->default_block_size = MIR_ARENA_DEFAULT_BLOCK_SIZE;
  return arena;
}

void mir_arena_destroy(MirArena *arena) {
  if (!arena) {
    return;
  }

  MirArenaBlock *block = arena->blocks;
  while (block) {
    MirArenaBlock *next = block->next;
    free(block);
    block = next;
  }
  free(arena);
}

void *mir_arena_alloc(MirArena *arena, size_t size, size_t align) {
  if (!arena) {
    return NULL;
  }
  if (size == 0) {
    size = 1;
  }
  if (align < sizeof(void *)) {
    align = sizeof(void *);
  }

  MirArenaBlock *block = arena->blocks;
  if (block) {
    size_t aligned = align_forward(block->used, align);
    if (aligned + size <= block->capacity) {
      void *ptr = block->data + aligned;
      block->used = aligned + size;
      return ptr;
    }
  }

  size_t capacity = arena->default_block_size;
  size_t min_capacity = size + align;
  if (capacity < min_capacity) {
    capacity = align_forward(min_capacity, align);
  }

  block = mir_arena_new_block(capacity);
  if (!block) {
    return NULL;
  }
  block->next = arena->blocks;
  arena->blocks = block;

  size_t aligned = align_forward(block->used, align);
  void *ptr = block->data + aligned;
  block->used = aligned + size;
  return ptr;
}

char *mir_arena_strndup(MirArena *arena, const char *str, size_t len) {
  if (!str) {
    return NULL;
  }

  char *copy = mir_arena_alloc(arena, len + 1, MIR_ALIGNOF(char));
  if (!copy) {
    return NULL;
  }
  memcpy(copy, str, len);
  copy[len] = '\0';
  return copy;
}

char *mir_arena_strdup(MirArena *arena, const char *str) {
  return str ? mir_arena_strndup(arena, str, strlen(str)) : NULL;
}

static char *mir_arena_printf(MirArena *arena, const char *fmt, ...) {
  if (!arena || !fmt) {
    return NULL;
  }

  va_list args;
  va_start(args, fmt);

  va_list measure_args;
  va_copy(measure_args, args);
  int len = vsnprintf(NULL, 0, fmt, measure_args);
  va_end(measure_args);

  if (len < 0) {
    va_end(args);
    return NULL;
  }

  char *str = mir_arena_alloc(arena, (size_t)len + 1, MIR_ALIGNOF(char));
  if (!str) {
    va_end(args);
    return NULL;
  }

  vsnprintf(str, (size_t)len + 1, fmt, args);
  va_end(args);
  return str;
}

static void *mir_vec_grow(MirArena *arena, void *items, size_t len, size_t *cap,
                          size_t elem_size, size_t align, size_t min_cap) {
  size_t new_cap = *cap ? *cap * 2 : 4;
  while (new_cap < min_cap) {
    new_cap *= 2;
  }

  void *new_items = mir_arena_alloc(arena, elem_size * new_cap, align);
  if (!new_items) {
    return NULL;
  }
  if (items && len > 0) {
    memcpy(new_items, items, elem_size * len);
  }

  *cap = new_cap;
  return new_items;
}

#define MIR_VEC_PUSH(arena, vec, T, value)                                     \
  do {                                                                         \
    if ((vec)->len == (vec)->cap) {                                            \
      (vec)->items =                                                           \
          mir_vec_grow((arena), (vec)->items, (vec)->len, &(vec)->cap,         \
                       sizeof(T), MIR_ALIGNOF(T), (vec)->len + 1);             \
    }                                                                          \
    if ((vec)->items) {                                                        \
      (vec)->items[(vec)->len++] = (value);                                    \
    }                                                                          \
  } while (0)

void mir_value_id_vec_push(MirArena *arena, MirValueIdVec *vec,
                           MirValueId value) {
  if (!arena || !vec) {
    return;
  }

  MIR_VEC_PUSH(arena, vec, MirValueId, value);
}

void mir_phi_incoming_vec_push(MirArena *arena, MirPhiIncomingVec *vec,
                               MirPhiIncoming value) {
  if (!arena || !vec) {
    return;
  }

  MIR_VEC_PUSH(arena, vec, MirPhiIncoming, value);
}

void mir_operand_use_vec_push(MirArena *arena, MirOperandUseVec *vec,
                              MirOperandUse value) {
  if (!arena || !vec) {
    return;
  }

  MIR_VEC_PUSH(arena, vec, MirOperandUse, value);
}

static MirResultOwnership mir_default_result_ownership(Type *type) {
  Type *result_type = type && type->kind == T_FN ? fn_return_type(type) : type;
  if (!result_type || result_type->kind == T_VOID) {
    return MIR_RESULT_NONE;
  }
  return MIR_RESULT_OWNED;
}

static MirOperandUse mir_default_param_use(const char *name, Type *type) {
  if ((name && strcmp(name, "$env") == 0) || (type && type->kind == T_VOID)) {
    return MIR_OPERAND_USE_BORROW;
  }
  return MIR_OPERAND_USE_CONSUME;
}

static void mir_fn_summary_init(MirArena *arena, MirFnSummary *summary,
                                Type *type) {
  if (!summary) {
    return;
  }

  *summary = (MirFnSummary){
      .param_uses = {0},
      .result = mir_default_result_ownership(type),
  };

  for (Type *cursor = type; cursor && cursor->kind == T_FN;
       cursor = cursor->data.T_FN.to) {
    mir_operand_use_vec_push(
        arena, &summary->param_uses,
        mir_default_param_use(NULL, cursor->data.T_FN.from));
  }
}

static MirFnSummary *mir_fn_summary_copy(MirArena *arena,
                                         const MirFnSummary *source) {
  if (!arena || !source) {
    return NULL;
  }

  MirFnSummary *copy =
      mir_arena_alloc(arena, sizeof(MirFnSummary), MIR_ALIGNOF(MirFnSummary));
  if (!copy) {
    return NULL;
  }

  *copy = (MirFnSummary){
      .param_uses = {0},
      .result = source->result,
  };
  for (size_t i = 0; i < source->param_uses.len; i++) {
    mir_operand_use_vec_push(arena, &copy->param_uses,
                             source->param_uses.items[i]);
  }
  return copy;
}

static MirFnSummary *mir_fn_summary_from_type(MirArena *arena, Type *type) {
  if (!arena || !type || type->kind != T_FN) {
    return NULL;
  }

  MirFnSummary *summary =
      mir_arena_alloc(arena, sizeof(MirFnSummary), MIR_ALIGNOF(MirFnSummary));
  if (!summary) {
    return NULL;
  }
  mir_fn_summary_init(arena, summary, type);
  return summary;
}

static MirFnSummary *mir_callable_summary_from_type(MirArena *arena,
                                                    Type *type) {
  if (!arena || !type || type->kind != T_FN) {
    return NULL;
  }

  bool closure = is_closure(type) && type->closure_meta;
  Type *callable_type = closure ? mir_closure_callable_type(arena, type) : type;
  MirFnSummary *summary = mir_fn_summary_from_type(arena, callable_type);
  if (closure && summary && summary->param_uses.len > 0 &&
      summary->param_uses.items) {
    summary->param_uses.items[0] = MIR_OPERAND_USE_BORROW;
  }
  return summary;
}

static const MirFnSummary *mir_value_callable_summary(MirFunction *fn,
                                                      MirValueId value) {
  if (!fn || value == MIR_NO_VALUE || value >= fn->values.len) {
    return NULL;
  }
  return fn->values.items[value].callable_summary;
}

static void
mir_function_set_value_callable_summary(MirFunction *fn, MirValueId value,
                                        const MirFnSummary *summary) {
  if (!fn || value == MIR_NO_VALUE || value >= fn->values.len) {
    return;
  }
  fn->values.items[value].callable_summary =
      mir_fn_summary_copy(fn->arena, summary);
}

void mir_instr_vec_push(MirArena *arena, MirInstrVec *vec, MirInstr value) {
  if (!arena || !vec) {
    return;
  }

  MIR_VEC_PUSH(arena, vec, MirInstr, value);
}

static void *mir_ht_alloc(void *ctx, size_t size, size_t align) {
  return mir_arena_alloc(ctx, size, align);
}

void mir_stack_frame_init(MirArena *arena, ht *table, MirStackFrame *frame,
                          MirStackFrame *next) {
  if (!table || !frame) {
    return;
  }

  ht_init_with_allocator(
      table, (ht_allocator){.alloc = mir_ht_alloc, .free = NULL, .ctx = arena});
  *frame = (MirStackFrame){.table = table, .next = next};
}

static MirSymbol *mir_symbol_new(MirArena *arena, MirSymbolKind kind,
                                 Type *type, Ast *origin,
                                 MirModuleId owner_module) {
  if (!arena) {
    return NULL;
  }

  MirSymbol *symbol =
      mir_arena_alloc(arena, sizeof(MirSymbol), MIR_ALIGNOF(MirSymbol));
  if (!symbol) {
    return NULL;
  }
  memset(symbol, 0, sizeof(*symbol));
  symbol->kind = kind;
  symbol->type = type;
  symbol->origin = origin;
  symbol->owner_module = owner_module;
  symbol->as.value = MIR_NO_VALUE;
  return symbol;
}

bool mir_ctx_bind_symbol(MirCtx *ctx, const char *name, MirSymbol *symbol) {
  if (!ctx || !ctx->frame || !ctx->frame->table || !name || !symbol) {
    return false;
  }
  return ht_set(ctx->frame->table, name, symbol) != NULL;
}

bool mir_ctx_bind_value(MirCtx *ctx, const char *name, MirValueId value) {
  if (!ctx || !ctx->frame || !ctx->frame->table || !name ||
      value == MIR_NO_VALUE) {
    return false;
  }

  MirArena *arena = ctx->frame->table->allocator.ctx;
  MirSymbol *symbol =
      mir_symbol_new(arena, MIR_SYMBOL_VALUE, NULL, NULL, ctx->current_module);
  if (!symbol) {
    return false;
  }
  symbol->as.value = value;
  return mir_ctx_bind_symbol(ctx, name, symbol);
}

bool mir_ctx_lookup_value(MirCtx *ctx, const char *name, MirValueId *out) {
  if (!ctx || !name) {
    return false;
  }

  for (MirStackFrame *frame = ctx->frame; frame; frame = frame->next) {
    if (!frame->table) {
      continue;
    }
    MirSymbol *symbol = ht_get(frame->table, name);
    if (symbol && symbol->kind == MIR_SYMBOL_VALUE &&
        symbol->as.value != MIR_NO_VALUE) {
      if (out) {
        *out = symbol->as.value;
      }
      return true;
    }
  }
  return false;
}

static bool mir_bind_identifier(MirCtx *ctx, Ast *binding, MirValueId value) {
  if (!binding || binding->tag != AST_IDENTIFIER ||
      ast_is_placeholder_id(binding)) {
    return false;
  }
  return mir_ctx_bind_value(ctx, binding->data.AST_IDENTIFIER.value, value);
}

static Type *mir_tuple_field_type(Type *type, size_t index) {
  if (!type || !(type->kind == T_CONS || type->kind == T_SUM) ||
      !type->data.T_CONS.args || index >= (size_t)type->data.T_CONS.num_args) {
    return NULL;
  }
  return type->data.T_CONS.args[index];
}

static Type *mir_record_access_type_view(Type *type) {
  if (type && type->kind == T_RECURSIVE_REF &&
      type->data.T_RECURSIVE_REF.decl &&
      type->data.T_RECURSIVE_REF.decl->type) {
    return type->data.T_RECURSIVE_REF.decl->type;
  }
  return type;
}

static Type *mir_sum_constructor_by_name(Type *sum_type, const char *name,
                                         int *idx) {
  if (idx) {
    *idx = -1;
  }
  if (!sum_type || sum_type->kind != T_SUM || !name ||
      !sum_type->data.T_CONS.args) {
    return NULL;
  }

  for (int i = 0; i < sum_type->data.T_CONS.num_args; i++) {
    Type *constructor = sum_type->data.T_CONS.args[i];
    if (constructor &&
        (constructor->kind == T_CONS || constructor->kind == T_SUM) &&
        constructor->data.T_CONS.name &&
        CHARS_EQ(constructor->data.T_CONS.name, name)) {
      if (idx) {
        *idx = i;
      }
      return constructor;
    }
  }

  return NULL;
}

static bool mir_bind_pattern(MirBuilder *builder, MirCtx *ctx, Ast *pattern,
                             MirValueId value, Type *value_type) {
  if (!ctx || !pattern) {
    return false;
  }

  if (pattern->tag == AST_LET) {
    pattern = pattern->data.AST_LET.binding;
  }

  switch (pattern->tag) {
  case AST_PLACEHOLDER_ID:
    return true;
  case AST_INT:
  case AST_UINT64:
  case AST_FLOAT:
  case AST_DOUBLE:
  case AST_STRING:
  case AST_CHAR:
  case AST_BOOL:
  case AST_VOID:
    return true;
  case AST_IDENTIFIER:
    if (ast_is_placeholder_id(pattern)) {
      return true;
    }
    return mir_bind_identifier(ctx, pattern, value);
  case AST_MATCH_GUARD_CLAUSE:
    return mir_bind_pattern(builder, ctx,
                            pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr,
                            value, value_type);
  case AST_TUPLE:
    if (!builder || value == MIR_NO_VALUE) {
      return false;
    }
    for (size_t i = 0; i < pattern->data.AST_LIST.len; i++) {
      Ast *item = pattern->data.AST_LIST.items + i;
      Type *item_type = mir_tuple_field_type(value_type, i);
      if (!item_type) {
        item_type = item->type;
      }
      MirValueId item_value = mir_tuple_get(builder, item_type, item, value, i);
      if (item_value == MIR_NO_VALUE ||
          !mir_bind_pattern(builder, ctx, item, item_value, item_type)) {
        return false;
      }
    }
    return true;
  default:
    return false;
  }
}

static const char *mir_obj_name(MirArena *arena, ObjString name,
                                const char *fallback) {
  if (name.chars && name.length > 0) {
    return mir_arena_strndup(arena, name.chars, (size_t)name.length);
  }
  return mir_arena_strdup(arena, fallback ? fallback : "<anonymous>");
}

static const char *mir_lambda_name(MirArena *arena, Ast *ast,
                                   const char *fallback) {
  if (!ast || (ast->tag != AST_LAMBDA && ast->tag != AST_MODULE)) {
    return mir_arena_strdup(arena, fallback ? fallback : "<anonymous>");
  }
  return mir_obj_name(arena, ast->data.AST_LAMBDA.fn_name, fallback);
}

static const char *mir_unique_function_name(MirProgram *program,
                                            const char *base_name) {
  if (!program || !program->arena || !base_name) {
    return NULL;
  }

  if (!mir_program_find_function_by_name(program, base_name)) {
    return mir_arena_strdup(program->arena, base_name);
  }

  return mir_arena_printf(program->arena, "%s.%zu", base_name,
                          program->functions.len);
}

MirValueId mir_function_add_value(MirFunction *fn, Type *type, Ast *origin);
static MirFunction *mir_builder_function(MirProgram *program, Ast *fn_ast,
                                         const char *name, MirCtx *ctx);
static bool mir_populate_function_body(MirProgram *program, MirFunction *fn,
                                       Ast *fn_ast, MirCtx *ctx,
                                       const char *self_name);
static MirSymbol *mir_resolve_ast_symbol(MirBuilder *builder, Ast *ast,
                                         MirCtx *ctx);

MirInstr *mir_function_find_def_instr(MirFunction *fn, MirValueId value) {
  if (!fn || value == MIR_NO_VALUE) {
    return NULL;
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }
    for (size_t j = 0; j < block->instrs.len; j++) {
      MirInstr *instr = &block->instrs.items[j];
      if (instr->result == value) {
        return instr;
      }
    }
  }

  return NULL;
}

static MirFunction *mir_program_get_function(MirProgram *program,
                                             MirFunctionId id) {
  if (!program || id == MIR_NO_FUNCTION || id >= program->functions.len) {
    return NULL;
  }
  return program->functions.items[id];
}

static MirFunction *mir_program_find_function_by_name(MirProgram *program,
                                                      const char *name) {
  if (!program || !name) {
    return NULL;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    MirFunction *fn = program->functions.items[i];
    if (fn && fn->name && CHARS_EQ(fn->name, name)) {
      return fn;
    }
  }

  return NULL;
}

static MirFunction *mir_program_find_scoped_function(MirProgram *program,
                                                     MirFunction *scope_fn,
                                                     const char *name) {
  if (!program || !program->arena || !scope_fn || !scope_fn->name || !name) {
    return NULL;
  }

  const char *scope_name = scope_fn->name;
  const char *end = scope_name + strlen(scope_name);
  while (end > scope_name) {
    const char *dot = end;
    while (dot > scope_name && dot[-1] != '.') {
      dot--;
    }
    if (dot == scope_name) {
      break;
    }

    dot--;
    size_t prefix_len = (size_t)(dot - scope_name);
    if (prefix_len == 0) {
      break;
    }

    const char *candidate = mir_arena_printf(program->arena, "%.*s.%s",
                                             (int)prefix_len, scope_name, name);
    MirFunction *fn = mir_program_find_function_by_name(program, candidate);
    if (fn) {
      return fn;
    }
    end = dot;
  }

  return NULL;
}

static MirModule *mir_program_get_module(MirProgram *program, MirModuleId id) {
  if (!program || id == MIR_NO_MODULE || id >= program->modules.len) {
    return NULL;
  }
  return program->modules.items[id];
}

static MirModule *mir_program_add_module(MirProgram *program, const char *name,
                                         Type *type, Ast *origin,
                                         MirModuleId parent) {
  if (!program || !program->arena) {
    return NULL;
  }

  MirModule *module = mir_arena_alloc(program->arena, sizeof(MirModule),
                                      MIR_ALIGNOF(MirModule));
  if (!module) {
    return NULL;
  }
  memset(module, 0, sizeof(*module));
  module->id = (MirModuleId)program->modules.len;
  module->name = name ? mir_arena_strdup(program->arena, name) : NULL;
  module->path = NULL;
  module->type = type;
  module->origin = origin;
  module->parent = parent;
  module->init = MIR_NO_FUNCTION;
  ht_init_with_allocator(&module->exports,
                         (ht_allocator){.alloc = mir_ht_alloc,
                                        .free = NULL,
                                        .ctx = program->arena});

  MIR_VEC_PUSH(program->arena, &program->modules, MirModule *, module);
  return module;
}

static bool mir_module_bind_symbol(MirProgram *program, MirModuleId module_id,
                                   const char *name, MirSymbol *symbol) {
  MirModule *module = mir_program_get_module(program, module_id);
  if (!module || !name || !symbol) {
    return false;
  }
  return ht_set(&module->exports, name, symbol) != NULL;
}

static MirSymbol *mir_module_lookup_symbol(MirProgram *program,
                                           MirModuleId module_id,
                                           const char *name,
                                           bool include_parents) {
  if (!program || module_id == MIR_NO_MODULE || !name) {
    return NULL;
  }

  for (MirModule *module = mir_program_get_module(program, module_id); module;
       module = include_parents
                    ? mir_program_get_module(program, module->parent)
                    : NULL) {
    MirSymbol *symbol = ht_get(&module->exports, name);
    if (symbol) {
      return symbol;
    }
    if (!include_parents) {
      break;
    }
  }
  return NULL;
}

static MirSymbol *mir_ctx_lookup_symbol(MirProgram *program, MirCtx *ctx,
                                        const char *name) {
  if (!name) {
    return NULL;
  }

  if (ctx) {
    for (MirStackFrame *frame = ctx->frame; frame; frame = frame->next) {
      if (!frame->table) {
        continue;
      }
      MirSymbol *symbol = ht_get(frame->table, name);
      if (symbol) {
        return symbol;
      }
    }

    MirSymbol *symbol =
        mir_module_lookup_symbol(program, ctx->current_module, name, true);
    if (symbol) {
      return symbol;
    }
  }

  return NULL;
}

static MirFunction *mir_program_find_specialization(MirProgram *program,
                                                    MirFunctionId source_id,
                                                    Type *type) {
  if (!program || source_id == MIR_NO_FUNCTION || !type) {
    return NULL;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    MirFunction *fn = program->functions.items[i];
    if (fn && fn->specialization_of == source_id && fn->specialization_type &&
        types_equal(fn->specialization_type, type)) {
      return fn;
    }
  }

  return NULL;
}

MirFunction *mir_program_add_function(MirProgram *program, const char *name,
                                      Type *type, Ast *origin) {
  if (!program || !program->arena) {
    return NULL;
  }

  MirFunction *fn = mir_arena_alloc(program->arena, sizeof(MirFunction),
                                    MIR_ALIGNOF(MirFunction));
  if (!fn) {
    return NULL;
  }

  memset(fn, 0, sizeof(MirFunction));
  fn->id = (MirFunctionId)program->functions.len;
  fn->arena = program->arena;
  fn->name = mir_arena_strdup(program->arena, name ? name : "<anonymous>");
  fn->type = type;
  fn->origin = origin;
  fn->specialization_of = MIR_NO_FUNCTION;
  fn->specialization_type = NULL;
  fn->summary.result = mir_default_result_ownership(type);

  MIR_VEC_PUSH(program->arena, &program->functions, MirFunction *, fn);
  return fn;
}

MirValueId mir_function_add_param(MirFunction *fn, const char *name, Type *type,
                                  Ast *origin) {
  if (!fn || !fn->arena) {
    return MIR_NO_VALUE;
  }

  MirValueId value = mir_function_add_value(fn, type, origin);
  if (value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirParam param = {.value = value,
                    .name = mir_arena_strdup(fn->arena, name ? name : "_"),
                    .type = type,
                    .origin = origin};
  MIR_VEC_PUSH(fn->arena, &fn->params, MirParam, param);
  mir_operand_use_vec_push(fn->arena, &fn->summary.param_uses,
                           mir_default_param_use(param.name, type));
  return value;
}

static void mir_function_set_param_use(MirFunction *fn, size_t index,
                                       MirOperandUse use) {
  if (!fn || index >= fn->summary.param_uses.len ||
      !fn->summary.param_uses.items) {
    return;
  }
  fn->summary.param_uses.items[index] = use;
}

MirOperandUse mir_function_param_use(const MirFunction *fn, size_t index) {
  if (!fn || index >= fn->summary.param_uses.len ||
      !fn->summary.param_uses.items) {
    return MIR_OPERAND_USE_CONSUME;
  }
  return fn->summary.param_uses.items[index];
}

MirResultOwnership mir_function_result_ownership(const MirFunction *fn) {
  return fn ? fn->summary.result : MIR_RESULT_NONE;
}

static bool mir_function_add_extern_params(MirFunction *fn, Type *type,
                                           Ast *origin) {
  if (!fn || !type) {
    return false;
  }

  if (type->kind == T_FN && type->data.T_FN.from &&
      type->data.T_FN.from->kind == T_VOID) {
    return true;
  }

  size_t index = 0;
  for (Type *cursor = type; cursor && cursor->kind == T_FN;
       cursor = cursor->data.T_FN.to, index++) {
    const char *name = mir_arena_printf(fn->arena, "arg%zu", index);
    MirValueId param = mir_function_add_param(fn, name ? name : "arg",
                                              cursor->data.T_FN.from, origin);
    if (param == MIR_NO_VALUE) {
      return false;
    }
    mir_function_set_param_use(fn, index, MIR_OPERAND_USE_BORROW);
  }

  return true;
}

static MirFunction *mir_program_add_extern_function(MirProgram *program,
                                                    const char *name,
                                                    Type *type, Ast *origin) {
  if (!program || !name || !type) {
    return NULL;
  }

  MirFunction *existing = mir_program_find_function_by_name(program, name);
  if (existing && existing->is_extern && existing->type &&
      types_equal(existing->type, type)) {
    return existing;
  }

  MirFunction *fn = mir_program_add_function(program, name, type, origin);
  if (!fn) {
    return NULL;
  }
  fn->is_extern = true;
  mir_function_add_extern_params(fn, type, origin);
  return fn;
}

MirBlock *mir_function_add_block(MirFunction *fn, const char *name) {
  if (!fn || !fn->arena) {
    return NULL;
  }

  MirBlock *block =
      mir_arena_alloc(fn->arena, sizeof(MirBlock), MIR_ALIGNOF(MirBlock));
  if (!block) {
    return NULL;
  }

  memset(block, 0, sizeof(MirBlock));
  block->id = (MirBlockId)fn->blocks.len;
  block->name = mir_arena_strdup(fn->arena, name ? name : "bb");
  block->term = (MirTerminator){.kind = MIR_TERM_NONE,
                                .value = MIR_NO_VALUE,
                                .cond = MIR_NO_VALUE,
                                .target = MIR_NO_BLOCK,
                                .then_block = MIR_NO_BLOCK,
                                .else_block = MIR_NO_BLOCK};

  MIR_VEC_PUSH(fn->arena, &fn->blocks, MirBlock *, block);
  return block;
}

void mir_builder_init(MirBuilder *builder, MirProgram *program,
                      MirFunction *fn) {
  if (!builder) {
    return;
  }

  builder->program = program;
  builder->fn = fn;
  builder->block = NULL;
}

void mir_builder_position_at_end(MirBuilder *builder, MirBlock *block) {
  if (!builder) {
    return;
  }
  builder->block = block;
}

void mir_builder_set_return(MirBuilder *builder, MirValueId value) {
  if (!builder || !builder->block) {
    return;
  }
  builder->block->term = (MirTerminator){.kind = MIR_TERM_RETURN,
                                         .value = value,
                                         .cond = MIR_NO_VALUE,
                                         .target = MIR_NO_BLOCK,
                                         .then_block = MIR_NO_BLOCK,
                                         .else_block = MIR_NO_BLOCK};
}

void mir_builder_set_br(MirBuilder *builder, MirBlockId target) {
  if (!builder || !builder->block) {
    return;
  }
  builder->block->term = (MirTerminator){.kind = MIR_TERM_BR,
                                         .value = MIR_NO_VALUE,
                                         .cond = MIR_NO_VALUE,
                                         .target = target,
                                         .then_block = MIR_NO_BLOCK,
                                         .else_block = MIR_NO_BLOCK};
}

void mir_builder_set_cond(MirBuilder *builder, MirValueId cond,
                          MirBlockId then_block, MirBlockId else_block) {
  if (!builder || !builder->block) {
    return;
  }
  builder->block->term = (MirTerminator){.kind = MIR_TERM_COND,
                                         .value = MIR_NO_VALUE,
                                         .cond = cond,
                                         .target = MIR_NO_BLOCK,
                                         .then_block = then_block,
                                         .else_block = else_block};
}

void mir_builder_set_unreachable(MirBuilder *builder) {
  if (!builder || !builder->block) {
    return;
  }
  builder->block->term = (MirTerminator){.kind = MIR_TERM_UNREACHABLE,
                                         .value = MIR_NO_VALUE,
                                         .cond = MIR_NO_VALUE,
                                         .target = MIR_NO_BLOCK,
                                         .then_block = MIR_NO_BLOCK,
                                         .else_block = MIR_NO_BLOCK};
}

static void mir_builder_set_unreachable_if_open(MirBuilder *builder) {
  if (!builder || !builder->block ||
      builder->block->term.kind != MIR_TERM_NONE) {
    return;
  }
  mir_builder_set_unreachable(builder);
}

MirInstr mir_make_instr(MirInstrKind kind, Type *type, Ast *origin) {
  return (MirInstr){
      .kind = kind,
      .result = MIR_NO_VALUE,
      .type = type,
      .origin = origin,
  };
}

MirValueId mir_function_add_value(MirFunction *fn, Type *type, Ast *origin) {
  if (!fn || !fn->arena) {
    return MIR_NO_VALUE;
  }

  MirValue value = {
      .id = (MirValueId)fn->values.len,
      .type = type,
      .origin = origin,
      .ea_md = NULL,
      .callable_summary = mir_callable_summary_from_type(fn->arena, type),
  };
  MIR_VEC_PUSH(fn->arena, &fn->values, MirValue, value);
  return value.id;
}

MirValueId mir_append_instr(MirFunction *fn, MirBlock *block, MirInstr instr) {
  if (!fn || !block) {
    return MIR_NO_VALUE;
  }

  instr.result = mir_function_add_value(fn, instr.type, instr.origin);
  MIR_VEC_PUSH(fn->arena, &block->instrs, MirInstr, instr);
  return instr.result;
}

static void mir_attach_value_callable_summary(MirBuilder *builder,
                                              MirInstr *instr,
                                              MirValueId result) {
  if (!builder || !builder->program || !builder->fn || !instr ||
      result == MIR_NO_VALUE) {
    return;
  }

  switch (instr->kind) {
  case MIR_FN_REF: {
    MirFunction *target =
        mir_program_get_function(builder->program, instr->data.fn_ref.fn);
    if (target) {
      mir_function_set_value_callable_summary(builder->fn, result,
                                              &target->summary);
    }
    break;
  }
  case MIR_CONSTRUCT: {
    if (instr->data.construct.kind != MIR_CONSTRUCT_CLOSURE) {
      break;
    }
    MirFunction *impl = mir_program_get_function(builder->program,
                                                 instr->data.construct.impl_fn);
    if (impl) {
      mir_function_set_value_callable_summary(builder->fn, result,
                                              &impl->summary);
    }
    break;
  }
  case MIR_EXTRACT: {
    if (instr->data.extract.kind != MIR_EXTRACT_CLOSURE_FN) {
      break;
    }
    const MirFnSummary *summary =
        mir_value_callable_summary(builder->fn, instr->data.extract.value);
    mir_function_set_value_callable_summary(builder->fn, result, summary);
    break;
  }
  default:
    break;
  }
}

MirValueId mir_builder_append_instr(MirBuilder *builder, MirInstr instr) {
  if (!builder || !builder->fn || !builder->block ||
      builder->block->term.kind != MIR_TERM_NONE) {
    return MIR_NO_VALUE;
  }
  MirValueId result = mir_append_instr(builder->fn, builder->block, instr);
  mir_attach_value_callable_summary(builder, &instr, result);
  return result;
}

static MirOperand mir_make_operand(MirValueId value, MirOperandRole role,
                                   MirOperandUse use, size_t index) {
  return (MirOperand){
      .value = value,
      .role = role,
      .use = use,
      .index = index,
  };
}

static bool mir_instr_is_call_like(const MirInstr *instr) {
  return instr && (instr->kind == MIR_CALL || instr->kind == MIR_CORO_NEW ||
                   instr->kind == MIR_CORO_NEXT);
}

static MirOperandUse mir_call_operand_use(const MirInstr *instr, size_t index) {
  if (!mir_instr_is_call_like(instr) ||
      index >= instr->data.call.operand_uses.len ||
      !instr->data.call.operand_uses.items) {
    return MIR_OPERAND_USE_CONSUME;
  }
  return instr->data.call.operand_uses.items[index];
}

static MirOperandRole mir_op_operand_role(const MirInstr *instr, size_t index) {
  if (!instr || instr->kind != MIR_OP) {
    return MIR_OPERAND_ROLE_VALUE;
  }

  switch (instr->data.op.kind) {
  case MIR_OP_KIND_TAG_EQ:
    return MIR_OPERAND_ROLE_TAG;
  case MIR_OP_KIND_LIST_IS_EMPTY:
  case MIR_OP_KIND_ARRAY_SIZE:
    return MIR_OPERAND_ROLE_CONTAINER;
  case MIR_OP_KIND_ARRAY_SET:
    if (index == 0) {
      return MIR_OPERAND_ROLE_CONTAINER;
    }
    if (index == 1) {
      return MIR_OPERAND_ROLE_INDEX;
    }
    return MIR_OPERAND_ROLE_ELEMENT;
  case MIR_OP_KIND_PTR_OFFSET:
    return index == 1 ? MIR_OPERAND_ROLE_INDEX : MIR_OPERAND_ROLE_VALUE;
  case MIR_OP_KIND_LOAD:
  case MIR_OP_KIND_LOAD_OWNED:
    return MIR_OPERAND_ROLE_VALUE;
  case MIR_OP_KIND_STORE:
    return index == 1 ? MIR_OPERAND_ROLE_ELEMENT : MIR_OPERAND_ROLE_VALUE;
  case MIR_OP_KIND_GLOBAL_STORE:
    return MIR_OPERAND_ROLE_VALUE;
  default:
    return MIR_OPERAND_ROLE_VALUE;
  }
}

static MirOperandUse mir_op_operand_use(const MirInstr *instr, size_t index) {
  if (!instr || instr->kind != MIR_OP) {
    return MIR_OPERAND_USE_BORROW;
  }

  return ((instr->data.op.kind == MIR_OP_KIND_ARRAY_SET && index == 2) ||
          (instr->data.op.kind == MIR_OP_KIND_STORE && index == 1))
             ? MIR_OPERAND_USE_CONSUME
             : MIR_OPERAND_USE_BORROW;
}

static bool mir_visit_operand(MirInstr *instr, MirOperandVisitor visitor,
                              MirOperand operand, void *ctx) {
  return !visitor || visitor(instr, operand, ctx);
}

static bool mir_extract_for_each_operand(MirInstr *instr,
                                         MirOperandVisitor visitor, void *ctx) {
  if (!instr || instr->kind != MIR_EXTRACT) {
    return false;
  }

  switch (instr->data.extract.kind) {
  case MIR_EXTRACT_ARRAY_AT:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.extract.value,
                                              MIR_OPERAND_ROLE_CONTAINER,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx) &&
           mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.extract.index_value,
                                              MIR_OPERAND_ROLE_INDEX,
                                              MIR_OPERAND_USE_BORROW, 1),
                             ctx);
  case MIR_EXTRACT_ARRAY_OFFSET:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.extract.index_value,
                                              MIR_OPERAND_ROLE_INDEX,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx) &&
           mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.extract.value,
                                              MIR_OPERAND_ROLE_CONTAINER,
                                              MIR_OPERAND_USE_BORROW, 1),
                             ctx);
  case MIR_EXTRACT_VARIANT_TAG:
  case MIR_EXTRACT_CLOSURE_FN:
  case MIR_EXTRACT_CLOSURE_ENV:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.extract.value,
                                              MIR_OPERAND_ROLE_VALUE,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx);
  case MIR_EXTRACT_FIELD:
  case MIR_EXTRACT_VARIANT_PAYLOAD:
  case MIR_EXTRACT_LIST_HEAD:
  case MIR_EXTRACT_LIST_TAIL:
  case MIR_EXTRACT_ARRAY_SUCC:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.extract.value,
                                              MIR_OPERAND_ROLE_CONTAINER,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx);
  }

  return true;
}

static MirOperandUse mir_construct_item_use(const MirInstr *instr) {
  return instr && instr->kind == MIR_CONSTRUCT &&
                 instr->data.construct.kind == MIR_CONSTRUCT_TUPLE &&
                 instr->type && is_array_type(instr->type)
             ? MIR_OPERAND_USE_BORROW
             : MIR_OPERAND_USE_CONSUME;
}

static bool mir_construct_for_each_operand(MirInstr *instr,
                                           MirOperandVisitor visitor,
                                           void *ctx) {
  if (!instr || instr->kind != MIR_CONSTRUCT) {
    return false;
  }

  switch (instr->data.construct.kind) {
  case MIR_CONSTRUCT_TUPLE:
  case MIR_CONSTRUCT_VARIANT:
  case MIR_CONSTRUCT_CLOSURE_ENV:
    for (size_t i = 0; i < instr->data.construct.items.len; i++) {
      if (!mir_visit_operand(
              instr, visitor,
              mir_make_operand(instr->data.construct.items.items[i],
                               MIR_OPERAND_ROLE_FIELD,
                               mir_construct_item_use(instr), i),
              ctx)) {
        return false;
      }
    }
    return true;
  case MIR_CONSTRUCT_ARRAY_LITERAL:
    for (size_t i = 0; i < instr->data.construct.items.len; i++) {
      if (!mir_visit_operand(
              instr, visitor,
              mir_make_operand(instr->data.construct.items.items[i],
                               MIR_OPERAND_ROLE_ELEMENT,
                               MIR_OPERAND_USE_CONSUME, i),
              ctx)) {
        return false;
      }
    }
    return true;
  case MIR_CONSTRUCT_LIST_CONS:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[0],
                                              MIR_OPERAND_ROLE_ELEMENT,
                                              MIR_OPERAND_USE_CONSUME, 0),
                             ctx) &&
           mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[1],
                                              MIR_OPERAND_ROLE_CONTAINER,
                                              MIR_OPERAND_USE_CONSUME, 1),
                             ctx);
  case MIR_CONSTRUCT_ARRAY_FILL_CONST:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[0],
                                              MIR_OPERAND_ROLE_VALUE,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx) &&
           mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[1],
                                              MIR_OPERAND_ROLE_ELEMENT,
                                              MIR_OPERAND_USE_CONSUME, 1),
                             ctx);
  case MIR_CONSTRUCT_ARRAY_FILL:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[0],
                                              MIR_OPERAND_ROLE_VALUE,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx) &&
           mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[1],
                                              MIR_OPERAND_ROLE_FUNCTION,
                                              MIR_OPERAND_USE_BORROW, 1),
                             ctx);
  case MIR_CONSTRUCT_ARRAY_RANGE:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[0],
                                              MIR_OPERAND_ROLE_INDEX,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx) &&
           mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[1],
                                              MIR_OPERAND_ROLE_VALUE,
                                              MIR_OPERAND_USE_BORROW, 1),
                             ctx) &&
           mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[2],
                                              MIR_OPERAND_ROLE_CONTAINER,
                                              MIR_OPERAND_USE_BORROW, 2),
                             ctx);
  case MIR_CONSTRUCT_CLOSURE:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[0],
                                              MIR_OPERAND_ROLE_FUNCTION,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx) &&
           mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.construct.operands[1],
                                              MIR_OPERAND_ROLE_ENV,
                                              MIR_OPERAND_USE_CONSUME, 1),
                             ctx);
  case MIR_CONSTRUCT_LIST_EMPTY:
    return true;
  }

  return true;
}

bool mir_instr_for_each_operand(MirInstr *instr, MirOperandVisitor visitor,
                                void *ctx) {
  if (!instr) {
    return false;
  }

  switch (instr->kind) {
  case MIR_OP:
    for (size_t i = 0; i < instr->data.op.argc; i++) {
      if (!mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.op.operands[i],
                                              mir_op_operand_role(instr, i),
                                              mir_op_operand_use(instr, i), i),
                             ctx)) {
        return false;
      }
    }
    return true;
  case MIR_PHI:
    for (size_t i = 0; i < instr->data.phi.incoming.len; i++) {
      if (!mir_visit_operand(
              instr, visitor,
              mir_make_operand(instr->data.phi.incoming.items[i].value,
                               MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW,
                               i),
              ctx)) {
        return false;
      }
    }
    return true;
  case MIR_CONSTRUCT:
    return mir_construct_for_each_operand(instr, visitor, ctx);
  case MIR_EXTRACT:
    return mir_extract_for_each_operand(instr, visitor, ctx);
  case MIR_CALL:
  case MIR_CORO_NEW:
  case MIR_CORO_NEXT:
    if (!mir_visit_operand(instr, visitor,
                           mir_make_operand(instr->data.call.callee,
                                            MIR_OPERAND_ROLE_CALLEE,
                                            MIR_OPERAND_USE_BORROW, 0),
                           ctx)) {
      return false;
    }
    for (size_t i = 0; i < instr->data.call.operands.len; i++) {
      if (!mir_visit_operand(
              instr, visitor,
              mir_make_operand(instr->data.call.operands.items[i],
                               MIR_OPERAND_ROLE_VALUE,
                               mir_call_operand_use(instr, i), i),
              ctx)) {
        return false;
      }
    }
    return true;
  case MIR_CORO_RESET:
    return mir_visit_operand(instr, visitor,
                             mir_make_operand(instr->data.call.callee,
                                              MIR_OPERAND_ROLE_VALUE,
                                              MIR_OPERAND_USE_CONSUME, 0),
                             ctx);
  default:
    return true;
  }
}

bool mir_term_for_each_operand(MirTerminator *term, MirOperandVisitor visitor,
                               void *ctx) {
  if (!term) {
    return false;
  }

  switch (term->kind) {
  case MIR_TERM_RETURN:
    return mir_visit_operand(NULL, visitor,
                             mir_make_operand(term->value,
                                              MIR_OPERAND_ROLE_RETURN,
                                              MIR_OPERAND_USE_CONSUME, 0),
                             ctx);
  case MIR_TERM_COND:
    return mir_visit_operand(NULL, visitor,
                             mir_make_operand(term->cond,
                                              MIR_OPERAND_ROLE_CONDITION,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx);
  case MIR_TERM_YIELD:
    return mir_visit_operand(NULL, visitor,
                             mir_make_operand(term->value,
                                              MIR_OPERAND_ROLE_VALUE,
                                              MIR_OPERAND_USE_BORROW, 0),
                             ctx);
  case MIR_TERM_CORO_RESTART:
    for (size_t i = 0; i < term->args.len; i++) {
      if (!mir_visit_operand(NULL, visitor,
                             mir_make_operand(term->args.items[i],
                                              MIR_OPERAND_ROLE_VALUE,
                                              MIR_OPERAND_USE_CONSUME, i),
                             ctx)) {
        return false;
      }
    }
    return true;
  default:
    return true;
  }
}

static MirValueId mir_rewrite_operand(MirInstr *instr,
                                      MirOperandRewriter rewriter,
                                      MirOperand operand, void *ctx) {
  return rewriter ? rewriter(instr, operand, ctx) : operand.value;
}

static void mir_rewrite_extract_operands(MirInstr *instr,
                                         MirOperandRewriter rewriter,
                                         void *ctx) {
  if (!instr || instr->kind != MIR_EXTRACT) {
    return;
  }

  switch (instr->data.extract.kind) {
  case MIR_EXTRACT_ARRAY_AT:
    instr->data.extract.value = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.extract.value, MIR_OPERAND_ROLE_CONTAINER,
                         MIR_OPERAND_USE_BORROW, 0),
        ctx);
    instr->data.extract.index_value = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.extract.index_value,
                         MIR_OPERAND_ROLE_INDEX, MIR_OPERAND_USE_BORROW, 1),
        ctx);
    break;
  case MIR_EXTRACT_ARRAY_OFFSET:
    instr->data.extract.index_value = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.extract.index_value,
                         MIR_OPERAND_ROLE_INDEX, MIR_OPERAND_USE_BORROW, 0),
        ctx);
    instr->data.extract.value = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.extract.value, MIR_OPERAND_ROLE_CONTAINER,
                         MIR_OPERAND_USE_BORROW, 1),
        ctx);
    break;
  case MIR_EXTRACT_VARIANT_TAG:
  case MIR_EXTRACT_CLOSURE_FN:
  case MIR_EXTRACT_CLOSURE_ENV:
    instr->data.extract.value = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.extract.value, MIR_OPERAND_ROLE_VALUE,
                         MIR_OPERAND_USE_BORROW, 0),
        ctx);
    break;
  case MIR_EXTRACT_FIELD:
  case MIR_EXTRACT_VARIANT_PAYLOAD:
  case MIR_EXTRACT_LIST_HEAD:
  case MIR_EXTRACT_LIST_TAIL:
  case MIR_EXTRACT_ARRAY_SUCC:
    instr->data.extract.value = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.extract.value, MIR_OPERAND_ROLE_CONTAINER,
                         MIR_OPERAND_USE_BORROW, 0),
        ctx);
    break;
  }
}

static void mir_rewrite_construct_operands(MirInstr *instr,
                                           MirOperandRewriter rewriter,
                                           void *ctx) {
  if (!instr || instr->kind != MIR_CONSTRUCT) {
    return;
  }

  switch (instr->data.construct.kind) {
  case MIR_CONSTRUCT_TUPLE:
  case MIR_CONSTRUCT_VARIANT:
  case MIR_CONSTRUCT_CLOSURE_ENV:
    for (size_t i = 0; i < instr->data.construct.items.len; i++) {
      instr->data.construct.items.items[i] = mir_rewrite_operand(
          instr, rewriter,
          mir_make_operand(instr->data.construct.items.items[i],
                           MIR_OPERAND_ROLE_FIELD,
                           mir_construct_item_use(instr), i),
          ctx);
    }
    break;
  case MIR_CONSTRUCT_ARRAY_LITERAL:
    for (size_t i = 0; i < instr->data.construct.items.len; i++) {
      instr->data.construct.items.items[i] = mir_rewrite_operand(
          instr, rewriter,
          mir_make_operand(instr->data.construct.items.items[i],
                           MIR_OPERAND_ROLE_ELEMENT, MIR_OPERAND_USE_CONSUME,
                           i),
          ctx);
    }
    break;
  case MIR_CONSTRUCT_LIST_CONS:
    instr->data.construct.operands[0] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[0],
                         MIR_OPERAND_ROLE_ELEMENT, MIR_OPERAND_USE_CONSUME, 0),
        ctx);
    instr->data.construct.operands[1] =
        mir_rewrite_operand(instr, rewriter,
                            mir_make_operand(instr->data.construct.operands[1],
                                             MIR_OPERAND_ROLE_CONTAINER,
                                             MIR_OPERAND_USE_CONSUME, 1),
                            ctx);
    break;
  case MIR_CONSTRUCT_ARRAY_FILL_CONST:
    instr->data.construct.operands[0] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[0],
                         MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW, 0),
        ctx);
    instr->data.construct.operands[1] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[1],
                         MIR_OPERAND_ROLE_ELEMENT, MIR_OPERAND_USE_CONSUME, 1),
        ctx);
    break;
  case MIR_CONSTRUCT_ARRAY_FILL:
    instr->data.construct.operands[0] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[0],
                         MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW, 0),
        ctx);
    instr->data.construct.operands[1] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[1],
                         MIR_OPERAND_ROLE_FUNCTION, MIR_OPERAND_USE_BORROW, 1),
        ctx);
    break;
  case MIR_CONSTRUCT_ARRAY_RANGE:
    instr->data.construct.operands[0] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[0],
                         MIR_OPERAND_ROLE_INDEX, MIR_OPERAND_USE_BORROW, 0),
        ctx);
    instr->data.construct.operands[1] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[1],
                         MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW, 1),
        ctx);
    instr->data.construct.operands[2] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[2],
                         MIR_OPERAND_ROLE_CONTAINER, MIR_OPERAND_USE_BORROW, 2),
        ctx);
    break;
  case MIR_CONSTRUCT_CLOSURE:
    instr->data.construct.operands[0] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[0],
                         MIR_OPERAND_ROLE_FUNCTION, MIR_OPERAND_USE_BORROW, 0),
        ctx);
    instr->data.construct.operands[1] = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.construct.operands[1],
                         MIR_OPERAND_ROLE_ENV, MIR_OPERAND_USE_CONSUME, 1),
        ctx);
    break;
  case MIR_CONSTRUCT_LIST_EMPTY:
    break;
  }
}

void mir_instr_rewrite_operands(MirInstr *instr, MirOperandRewriter rewriter,
                                void *ctx) {
  if (!instr || !rewriter) {
    return;
  }

  switch (instr->kind) {
  case MIR_OP:
    for (size_t i = 0; i < instr->data.op.argc; i++) {
      instr->data.op.operands[i] =
          mir_rewrite_operand(instr, rewriter,
                              mir_make_operand(instr->data.op.operands[i],
                                               mir_op_operand_role(instr, i),
                                               mir_op_operand_use(instr, i), i),
                              ctx);
    }
    break;
  case MIR_PHI:
    for (size_t i = 0; i < instr->data.phi.incoming.len; i++) {
      instr->data.phi.incoming.items[i].value = mir_rewrite_operand(
          instr, rewriter,
          mir_make_operand(instr->data.phi.incoming.items[i].value,
                           MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW, i),
          ctx);
    }
    break;
  case MIR_CONSTRUCT:
    mir_rewrite_construct_operands(instr, rewriter, ctx);
    break;
  case MIR_EXTRACT:
    mir_rewrite_extract_operands(instr, rewriter, ctx);
    break;
  case MIR_CALL:
  case MIR_CORO_NEW:
  case MIR_CORO_NEXT:
    instr->data.call.callee = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.call.callee, MIR_OPERAND_ROLE_CALLEE,
                         MIR_OPERAND_USE_BORROW, 0),
        ctx);
    for (size_t i = 0; i < instr->data.call.operands.len; i++) {
      instr->data.call.operands.items[i] = mir_rewrite_operand(
          instr, rewriter,
          mir_make_operand(instr->data.call.operands.items[i],
                           MIR_OPERAND_ROLE_VALUE,
                           mir_call_operand_use(instr, i), i),
          ctx);
    }
    break;
  case MIR_CORO_RESET:
    instr->data.call.callee = mir_rewrite_operand(
        instr, rewriter,
        mir_make_operand(instr->data.call.callee, MIR_OPERAND_ROLE_VALUE,
                         MIR_OPERAND_USE_CONSUME, 0),
        ctx);
    break;
  default:
    break;
  }
}

MirValueId mir_const_int(MirBuilder *builder, Type *type, Ast *origin,
                         int value) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_INT;
  instr.data.const_value.as.int_value = value;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_const_uint64(MirBuilder *builder, Type *type, Ast *origin,
                            uint64_t value) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_UINT64;
  instr.data.const_value.as.uint64_value = value;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_const_float(MirBuilder *builder, Type *type, Ast *origin,
                           float value) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_FLOAT;
  instr.data.const_value.as.float_value = value;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_const_double(MirBuilder *builder, Type *type, Ast *origin,
                            double value) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_DOUBLE;
  instr.data.const_value.as.double_value = value;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_const_char(MirBuilder *builder, Type *type, Ast *origin,
                          char value) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_CHAR;
  instr.data.const_value.as.char_value = value;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_const_bool(MirBuilder *builder, Type *type, Ast *origin,
                          bool value) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_BOOL;
  instr.data.const_value.as.bool_value = value;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_const_string(MirBuilder *builder, Type *type, Ast *origin,
                            const char *chars, size_t len) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_STRING;
  if (!chars) {
    len = 0;
  }
  instr.data.const_value.as.string_value.chars =
      builder && builder->fn && chars
          ? mir_arena_strndup(builder->fn->arena, chars, len)
          : NULL;
  instr.data.const_value.as.string_value.len = len;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_const_void(MirBuilder *builder, Type *type, Ast *origin) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_VOID;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_const_undef(MirBuilder *builder, Type *type, Ast *origin) {
  MirInstr instr = mir_make_instr(MIR_CONST, type, origin);
  instr.data.const_value.kind = MIR_CONST_KIND_UNDEF;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_extract_field(MirBuilder *builder, Type *type,
                                    Ast *origin, MirValueId value, size_t index,
                                    const char *name);

static Type *mir_coroutine_instance_yield_type(Type *type) {
  if (!type || !is_coroutine_type(type) || !type->data.T_CONS.args ||
      type->data.T_CONS.num_args < 1) {
    return NULL;
  }
  return type->data.T_CONS.args[0];
}

MirValueId mir_coro_next(MirBuilder *builder, Ast *origin,
                         MirValueId coroutine, Type *coroutine_type) {
  Type *yield_type = mir_coroutine_instance_yield_type(coroutine_type);
  if (!builder || coroutine == MIR_NO_VALUE || !yield_type) {
    return MIR_NO_VALUE;
  }

  MirInstr instr =
      mir_make_instr(MIR_CORO_NEXT, create_option_type(yield_type), origin);
  instr.data.call.callee = coroutine;
  instr.data.call.callee_type = coroutine_type;
  instr.data.call.specialized_fn = MIR_NO_FUNCTION;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_coro_reset(MirBuilder *builder, Ast *origin,
                          MirValueId coroutine, Type *coroutine_type) {
  if (!builder || coroutine == MIR_NO_VALUE ||
      !mir_coroutine_instance_yield_type(coroutine_type)) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CORO_RESET, coroutine_type, origin);
  instr.data.call.callee = coroutine;
  instr.data.call.callee_type = coroutine_type;
  instr.data.call.specialized_fn = MIR_NO_FUNCTION;
  return mir_builder_append_instr(builder, instr);
}

void mir_builder_set_yield(MirBuilder *builder, MirValueId value,
                           MirBlockId resume) {
  if (!builder || !builder->block) {
    return;
  }

  builder->block->term = (MirTerminator){
      .kind = MIR_TERM_YIELD,
      .value = value,
      .cond = MIR_NO_VALUE,
      .target = resume,
      .then_block = MIR_NO_BLOCK,
      .else_block = MIR_NO_BLOCK,
  };
}

static void mir_builder_set_coro_restart(MirBuilder *builder, MirBlockId target,
                                         MirValueIdVec args) {
  if (!builder || !builder->block) {
    return;
  }

  builder->block->term = (MirTerminator){
      .kind = MIR_TERM_CORO_RESTART,
      .value = MIR_NO_VALUE,
      .cond = MIR_NO_VALUE,
      .target = target,
      .then_block = MIR_NO_BLOCK,
      .else_block = MIR_NO_BLOCK,
      .args = args,
  };
}

void mir_builder_set_coro_done(MirBuilder *builder) {
  if (!builder || !builder->block) {
    return;
  }

  builder->block->term = (MirTerminator){
      .kind = MIR_TERM_CORO_DONE,
      .value = MIR_NO_VALUE,
      .cond = MIR_NO_VALUE,
      .target = MIR_NO_BLOCK,
      .then_block = MIR_NO_BLOCK,
      .else_block = MIR_NO_BLOCK,
  };
}

static void mir_builder_set_coro_done_if_open(MirBuilder *builder) {
  if (!builder || !builder->block ||
      builder->block->term.kind != MIR_TERM_NONE) {
    return;
  }
  mir_builder_set_coro_done(builder);
}

static const char *mir_current_source_function_name(MirBuilder *builder) {
  if (!builder || !builder->fn) {
    return NULL;
  }

  Ast *origin = builder->fn->origin;
  if (origin && origin->tag == AST_LAMBDA &&
      origin->data.AST_LAMBDA.fn_name.chars) {
    return origin->data.AST_LAMBDA.fn_name.chars;
  }

  return builder->fn->name;
}

static bool mir_is_recursive_coro_yield(MirBuilder *builder, Ast *expr) {
  if (!builder || !expr || expr->tag != AST_APPLICATION ||
      !expr->data.AST_APPLICATION.function ||
      expr->data.AST_APPLICATION.function->tag != AST_IDENTIFIER) {
    return false;
  }

  const char *self_name = mir_current_source_function_name(builder);
  const char *callee_name =
      expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;
  return self_name && callee_name && strcmp(self_name, callee_name) == 0;
}

static bool mir_application_is_void_call(Ast *app, Type *fn_type) {
  return app && app->tag == AST_APPLICATION &&
         app->data.AST_APPLICATION.len == 1 && app->data.AST_APPLICATION.args &&
         app->data.AST_APPLICATION.args->tag == AST_VOID && fn_type &&
         is_void_func(fn_type);
}

static bool mir_application_has_only_void_arg(Ast *app) {
  return app && app->tag == AST_APPLICATION &&
         (app->data.AST_APPLICATION.len == 0 ||
          (app->data.AST_APPLICATION.len == 1 &&
           app->data.AST_APPLICATION.args &&
           app->data.AST_APPLICATION.args->tag == AST_VOID));
}

static bool mir_collect_restart_args(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                     MirValueIdVec *args) {
  if (!builder || !builder->fn || !app || app->tag != AST_APPLICATION ||
      !args) {
    return false;
  }

  if (mir_application_is_void_call(app, builder->fn->type)) {
    return true;
  }

  for (size_t i = 0; i < app->data.AST_APPLICATION.len; i++) {
    Ast *arg_ast = app->data.AST_APPLICATION.args + i;
    MirValueId arg = mir_expr(builder, arg_ast, ctx);
    if (arg == MIR_NO_VALUE || !builder->block ||
        builder->block->term.kind != MIR_TERM_NONE) {
      return false;
    }
    mir_value_id_vec_push(builder->fn->arena, args, arg);
  }

  return true;
}

static MirValueId mir_yield_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  if (!builder || !ast || ast->tag != AST_YIELD) {
    return MIR_NO_VALUE;
  }

  Ast *yield_expr = ast->data.AST_YIELD.expr;
  if (is_coroutine_constructor_type(builder->fn ? builder->fn->type : NULL) &&
      mir_is_recursive_coro_yield(builder, yield_expr)) {
    MirValueIdVec args = {0};
    if (!mir_collect_restart_args(builder, yield_expr, ctx, &args)) {
      return MIR_NO_VALUE;
    }
    mir_builder_set_coro_restart(builder, 0, args);
    return MIR_NO_VALUE;
  }

  MirValueId yielded = ast->data.AST_YIELD.expr
                           ? mir_expr(builder, ast->data.AST_YIELD.expr, ctx)
                           : mir_const_void(builder, &t_void, ast);

  if (yielded == MIR_NO_VALUE || !builder->block) {
    return MIR_NO_VALUE;
  }

  Type *yielded_type = yield_expr ? yield_expr->type : NULL;
  if (!is_coroutine_type(yielded_type)) {
    yielded_type = mir_function_value_type(builder->fn, yielded);
  }
  Type *inner_yield_type = mir_coroutine_instance_yield_type(yielded_type);
  if (inner_yield_type) {
    Type *next_type = create_option_type(inner_yield_type);
    Type *some_type = next_type && next_type->data.T_CONS.args
                          ? next_type->data.T_CONS.args[0]
                          : NULL;
    if (!some_type) {
      return MIR_NO_VALUE;
    }

    MirBlock *check = mir_function_add_block(builder->fn, "yield_from.check");
    MirBlock *value = mir_function_add_block(builder->fn, "yield_from.value");
    MirBlock *resume = mir_function_add_block(builder->fn, "yield_from.resume");
    MirBlock *exit = mir_function_add_block(builder->fn, "yield_from.exit");
    if (!check || !value || !resume || !exit) {
      return MIR_NO_VALUE;
    }

    mir_builder_set_br(builder, check->id);

    mir_builder_position_at_end(builder, check);
    MirValueId next = mir_coro_next(builder, yield_expr, yielded, yielded_type);
    MirValueId tag = mir_variant_tag(builder, yield_expr, next);
    MirValueId is_some =
        mir_tag_eq(builder, yield_expr, tag, 0, TYPE_NAME_SOME);
    if (next == MIR_NO_VALUE || tag == MIR_NO_VALUE ||
        is_some == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_builder_set_cond(builder, is_some, value->id, exit->id);

    mir_builder_position_at_end(builder, value);
    MirValueId payload = mir_variant_payload(builder, yield_expr, next,
                                             some_type, 0, TYPE_NAME_SOME);
    MirValueId yielded_value = mir_extract_field(builder, inner_yield_type,
                                                 yield_expr, payload, 0, NULL);
    if (payload == MIR_NO_VALUE || yielded_value == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_builder_set_yield(builder, yielded_value, resume->id);

    mir_builder_position_at_end(builder, resume);
    mir_builder_set_br(builder, check->id);

    mir_builder_position_at_end(builder, exit);
    return mir_const_void(builder, &t_void, ast);
  }

  MirBlock *resume = mir_function_add_block(builder->fn, "yield.resume");
  if (!resume) {
    return MIR_NO_VALUE;
  }

  mir_builder_set_yield(builder, yielded, resume->id);
  mir_builder_position_at_end(builder, resume);

  // Just a sequencing value so mir_body keeps lowering later statements.
  // Do not treat this as the coroutine function's return value.
  return yielded;
}

MirValueId mir_phi(MirBuilder *builder, Type *type, Ast *origin,
                   MirPhiIncomingVec incoming) {
  if (!builder || !type || incoming.len == 0) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_PHI, type, origin);
  instr.data.phi.incoming = incoming;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_tuple(MirBuilder *builder, Type *type, Ast *origin,
                     MirValueIdVec items) {
  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_TUPLE;
  instr.data.construct.items = items;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_tuple_get(MirBuilder *builder, Type *type, Ast *origin,
                         MirValueId tuple, size_t index) {
  if (tuple == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_FIELD;
  instr.data.extract.value = tuple;
  instr.data.extract.index = index;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_variant(MirBuilder *builder, Type *type, Ast *origin,
                       Type *constructor_type, int constructor_index,
                       const char *constructor_name, MirValueIdVec fields) {
  if (!constructor_type || constructor_index < 0 || !constructor_name) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_VARIANT;
  instr.data.construct.constructor_type = constructor_type;
  instr.data.construct.constructor_index = constructor_index;
  instr.data.construct.constructor_name = constructor_name;
  instr.data.construct.items = fields;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_variant_tag(MirBuilder *builder, Ast *origin, MirValueId value) {
  if (value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, &t_char, origin);
  instr.data.extract.kind = MIR_EXTRACT_VARIANT_TAG;
  instr.data.extract.value = value;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_tag_eq(MirBuilder *builder, Ast *origin, MirValueId tag,
                      int constructor_index, const char *constructor_name) {
  if (tag == MIR_NO_VALUE || constructor_index < 0) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, &t_bool, origin);
  instr.data.op.kind = MIR_OP_KIND_TAG_EQ;
  instr.data.op.argc = 1;
  instr.data.op.operands[0] = tag;
  instr.data.op.constructor_index = constructor_index;
  instr.data.op.constructor_name = constructor_name;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_variant_payload(MirBuilder *builder, Ast *origin,
                               MirValueId value, Type *constructor_type,
                               int constructor_index,
                               const char *constructor_name) {
  if (value == MIR_NO_VALUE || !constructor_type || constructor_index < 0) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, constructor_type, origin);
  instr.data.extract.kind = MIR_EXTRACT_VARIANT_PAYLOAD;
  instr.data.extract.value = value;
  instr.data.extract.constructor_type = constructor_type;
  instr.data.extract.constructor_index = constructor_index;
  instr.data.extract.constructor_name = constructor_name;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_list_empty(MirBuilder *builder, Type *type, Ast *origin) {
  if (!type || !is_list_type(type)) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_LIST_EMPTY;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_list_cons(MirBuilder *builder, Type *type, Ast *origin,
                         MirValueId head, MirValueId tail) {
  if (!type || !is_list_type(type) || head == MIR_NO_VALUE ||
      tail == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_LIST_CONS;
  instr.data.construct.operands[0] = head;
  instr.data.construct.operands[1] = tail;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_list_is_empty(MirBuilder *builder, Ast *origin,
                             MirValueId list) {
  if (list == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, &t_bool, origin);
  instr.data.op.kind = MIR_OP_KIND_LIST_IS_EMPTY;
  instr.data.op.argc = 1;
  instr.data.op.operands[0] = list;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_list_head(MirBuilder *builder, Type *type, Ast *origin,
                         MirValueId list) {
  if (list == MIR_NO_VALUE || !type) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_LIST_HEAD;
  instr.data.extract.value = list;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_list_tail(MirBuilder *builder, Type *type, Ast *origin,
                         MirValueId list) {
  if (list == MIR_NO_VALUE || !type || !is_list_type(type)) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_LIST_TAIL;
  instr.data.extract.value = list;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_primitive_cast(MirBuilder *builder, Type *from_type,
                              Type *to_type, Ast *origin, MirValueId value) {
  if (value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, to_type, origin);
  instr.data.op.kind = MIR_OP_KIND_CAST;
  instr.data.op.argc = 1;
  instr.data.op.operands[0] = value;
  instr.data.op.from_type = from_type;
  instr.data.op.to_type = to_type;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_fn_ref(MirBuilder *builder, Type *type, Ast *origin,
                      MirFunction *fn) {
  if (!fn) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_FN_REF, type, origin);
  instr.data.fn_ref.fn = fn->id;
  instr.data.fn_ref.name = fn->name;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_ptr_offset(MirBuilder *builder, Type *ptr_type, Ast *origin,
                          MirValueId ptr, MirValueId index) {
  if (!builder || !ptr_type || !is_pointer_type(ptr_type) ||
      ptr == MIR_NO_VALUE || index == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, ptr_type, origin);
  instr.data.op.kind = MIR_OP_KIND_PTR_OFFSET;
  instr.data.op.argc = 2;
  instr.data.op.operands[0] = ptr;
  instr.data.op.operands[1] = index;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_ptr_load_with_kind(MirBuilder *builder, MirOpKind kind,
                                         Type *type, Ast *origin,
                                         MirValueId ptr) {
  if (!builder || !type || ptr == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, type, origin);
  instr.data.op.kind = kind;
  instr.data.op.argc = 1;
  instr.data.op.operands[0] = ptr;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_ptr_load(MirBuilder *builder, Type *type, Ast *origin,
                        MirValueId ptr) {
  return mir_ptr_load_with_kind(builder, MIR_OP_KIND_LOAD, type, origin, ptr);
}

MirValueId mir_ptr_load_owned(MirBuilder *builder, Type *type, Ast *origin,
                              MirValueId ptr) {
  return mir_ptr_load_with_kind(builder, MIR_OP_KIND_LOAD_OWNED, type, origin,
                                ptr);
}

MirValueId mir_ptr_store(MirBuilder *builder, Ast *origin, MirValueId ptr,
                         MirValueId value) {
  if (!builder || ptr == MIR_NO_VALUE || value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, &t_void, origin);
  instr.data.op.kind = MIR_OP_KIND_STORE;
  instr.data.op.argc = 2;
  instr.data.op.operands[0] = ptr;
  instr.data.op.operands[1] = value;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_closure_env(MirBuilder *builder, Type *type, Ast *origin,
                                  MirValueIdVec fields) {
  if (!builder || !type) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_CLOSURE_ENV;
  instr.data.construct.items = fields;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_closure(MirBuilder *builder, Type *type, Ast *origin,
                              MirValueId fn, MirValueId env,
                              MirFunction *impl_fn) {
  if (!builder || !type || fn == MIR_NO_VALUE || env == MIR_NO_VALUE ||
      !impl_fn) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_CONSTRUCT, type, origin);
  instr.data.construct.kind = MIR_CONSTRUCT_CLOSURE;
  instr.data.construct.operands[0] = fn;
  instr.data.construct.operands[1] = env;
  instr.data.construct.impl_fn = impl_fn->id;
  instr.data.construct.impl_name = impl_fn->name;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_extract_field(MirBuilder *builder, Type *type,
                                    Ast *origin, MirValueId value, size_t index,
                                    const char *name) {
  if (!builder || !type || value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_FIELD;
  instr.data.extract.value = value;
  instr.data.extract.index = index;
  instr.data.extract.name = name;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_closure_fn(MirBuilder *builder, Type *type, Ast *origin,
                                 MirValueId closure) {
  if (!builder || !type || closure == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_CLOSURE_FN;
  instr.data.extract.value = closure;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_closure_get_env(MirBuilder *builder, Type *type,
                                      Ast *origin, MirValueId closure) {
  if (!builder || !type || closure == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_EXTRACT, type, origin);
  instr.data.extract.kind = MIR_EXTRACT_CLOSURE_ENV;
  instr.data.extract.value = closure;
  return mir_builder_append_instr(builder, instr);
}

MirValueId mir_primitive_instr(MirBuilder *builder, MirPrimitiveOp op,
                               Type *type, Ast *origin,
                               const MirValueId *operands, size_t argc) {
  if (!builder || !type || !operands || argc == 0 ||
      argc > sizeof(((MirInstr *)0)->data.op.operands) /
                 sizeof(((MirInstr *)0)->data.op.operands[0])) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, type, origin);
  instr.data.op.kind = MIR_OP_KIND_PRIMITIVE;
  instr.data.op.primitive = op;
  instr.data.op.argc = (uint8_t)argc;
  for (size_t i = 0; i < argc; i++) {
    if (operands[i] == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    instr.data.op.operands[i] = operands[i];
  }
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_primitive_unary(MirBuilder *builder, MirPrimitiveOp op,
                                      Type *type, Ast *origin,
                                      MirValueId value) {
  MirValueId operands[] = {value};
  return mir_primitive_instr(builder, op, type, origin, operands, 1);
}

static MirValueId mir_primitive_binary(MirBuilder *builder, MirPrimitiveOp op,
                                       Type *type, Ast *origin, MirValueId lhs,
                                       MirValueId rhs) {
  MirValueId operands[] = {lhs, rhs};
  return mir_primitive_instr(builder, op, type, origin, operands, 2);
}

MirValueId mir_iadd(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_IADD, type, origin, lhs, rhs);
}

MirValueId mir_uadd(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_UADD, type, origin, lhs, rhs);
}

MirValueId mir_fadd(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_FADD, type, origin, lhs, rhs);
}

MirValueId mir_isub(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_ISUB, type, origin, lhs, rhs);
}

MirValueId mir_usub(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_USUB, type, origin, lhs, rhs);
}

MirValueId mir_fsub(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_FSUB, type, origin, lhs, rhs);
}

MirValueId mir_imul(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_IMUL, type, origin, lhs, rhs);
}

MirValueId mir_umul(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_UMUL, type, origin, lhs, rhs);
}

MirValueId mir_fmul(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_FMUL, type, origin, lhs, rhs);
}

MirValueId mir_idiv(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_IDIV, type, origin, lhs, rhs);
}

MirValueId mir_udiv(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_UDIV, type, origin, lhs, rhs);
}

MirValueId mir_fdiv(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_FDIV, type, origin, lhs, rhs);
}

MirValueId mir_imod(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_IMOD, type, origin, lhs, rhs);
}

MirValueId mir_umod(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_UMOD, type, origin, lhs, rhs);
}

MirValueId mir_fmod(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_FMOD, type, origin, lhs, rhs);
}

MirValueId mir_ieq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_IEQ, &t_bool, origin, lhs, rhs);
}

MirValueId mir_ueq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_UEQ, &t_bool, origin, lhs, rhs);
}

MirValueId mir_feq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_FEQ, &t_bool, origin, lhs, rhs);
}

MirValueId mir_ceq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_CEQ, &t_bool, origin, lhs, rhs);
}

MirValueId mir_beq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs) {
  return mir_primitive_binary(builder, MIR_OP_BEQ, &t_bool, origin, lhs, rhs);
}

MirValueId mir_lnot(MirBuilder *builder, Ast *origin, MirValueId value) {
  return mir_primitive_unary(builder, MIR_OP_LNOT, &t_bool, origin, value);
}

MirBuiltinSymbol *mir_program_register_builtin(MirProgram *program,
                                               const char *name, Type *type,
                                               MirBuiltinHandler handler,
                                               void *data) {
  if (!program || !program->arena || !name || !handler) {
    return NULL;
  }

  MirBuiltinSymbol *symbol = mir_arena_alloc(
      program->arena, sizeof(MirBuiltinSymbol), MIR_ALIGNOF(MirBuiltinSymbol));
  if (!symbol) {
    return NULL;
  }

  *symbol = (MirBuiltinSymbol){.name = mir_arena_strdup(program->arena, name),
                               .type = type,
                               .handler = handler,
                               .data = data,
                               .function = MIR_NO_FUNCTION};
  mir_fn_summary_init(program->arena, &symbol->summary, type);
  ht_set(&program->builtins, name, symbol);
  return symbol;
}

MirBuiltinSymbol *mir_program_lookup_builtin(MirProgram *program,
                                             const char *name) {
  if (!program || !name) {
    return NULL;
  }
  return ht_get(&program->builtins, name);
}

MirValueId mir_constructor_call(MirBuilder *builder, Ast *origin,
                                Type *result_type, const char *constructor_name,
                                Ast *args, size_t len, MirCtx *ctx) {
  if (!builder || !builder->fn || !constructor_name || !result_type ||
      result_type->kind != T_SUM || is_list_type(result_type)) {
    return MIR_NO_VALUE;
  }

  int constructor_index = -1;
  Type *constructor_type = mir_sum_constructor_by_name(
      result_type, constructor_name, &constructor_index);
  if (!constructor_type || constructor_index < 0 ||
      constructor_type->kind != T_CONS ||
      (size_t)constructor_type->data.T_CONS.num_args != len) {
    return MIR_NO_VALUE;
  }

  MirArena *arena = builder->fn->arena;
  MirValueIdVec fields = {0};
  for (size_t i = 0; i < len; i++) {
    MirValueId field = mir_expr(builder, args + i, ctx);
    if (field == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_value_id_vec_push(arena, &fields, field);
  }

  return mir_variant(builder, result_type, origin, constructor_type,
                     constructor_index, constructor_name, fields);
}

static bool mir_record_constructor_matches(Type *type, const char *name) {
  if (!type || type->kind != T_CONS || !name) {
    return false;
  }
  if (type->alias && CHARS_EQ(type->alias, name)) {
    return true;
  }
  return type->data.T_CONS.name && CHARS_EQ(type->data.T_CONS.name, name);
}

static MirValueId mir_record_constructor_call(MirBuilder *builder, Ast *origin,
                                              Type *result_type,
                                              const char *constructor_name,
                                              Ast *args, size_t len,
                                              MirCtx *ctx) {
  if (!builder || !builder->fn || !result_type ||
      !mir_record_constructor_matches(result_type, constructor_name) ||
      is_array_type(result_type) || is_list_type(result_type) ||
      is_pointer_type(result_type) || is_coroutine_type(result_type) ||
      (size_t)result_type->data.T_CONS.num_args != len) {
    return MIR_NO_VALUE;
  }

  MirArena *arena = builder->fn->arena;
  MirValueIdVec fields = {0};
  for (size_t i = 0; i < len; i++) {
    MirValueId field = mir_expr(builder, args + i, ctx);
    if (field == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_value_id_vec_push(arena, &fields, field);
  }

  return mir_tuple(builder, result_type, origin, fields);
}

static bool mir_type_has_type_vars(Type *type) {
  if (!type) {
    return false;
  }

  switch (type->kind) {
  case T_VAR:
    return true;
  case T_FN:
    return mir_type_has_type_vars(type->closure_meta) ||
           mir_type_has_type_vars(type->data.T_FN.from) ||
           mir_type_has_type_vars(type->data.T_FN.to);
  case T_CONS:
  case T_SUM:
    if (mir_type_has_type_vars(type->closure_meta)) {
      return true;
    }
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (type->data.T_CONS.args &&
          mir_type_has_type_vars(type->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  default:
    return false;
  }
}

static const char *mir_symbol_sanitize(MirArena *arena, const char *text,
                                       const char *fallback) {
  if (!arena) {
    return NULL;
  }
  if (!text || !*text) {
    return mir_arena_strdup(arena, fallback ? fallback : "Type");
  }

  size_t len = strlen(text);
  char *out = mir_arena_alloc(arena, len + 1, MIR_ALIGNOF(char));
  if (!out) {
    return NULL;
  }

  size_t used = 0;
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)text[i];
    if (isalnum(c) || c == '_') {
      out[used++] = (char)c;
    } else if (used > 0 && out[used - 1] != '_') {
      out[used++] = '_';
    }
  }

  while (used > 0 && out[used - 1] == '_') {
    used--;
  }

  if (used == 0) {
    return mir_arena_strdup(arena, fallback ? fallback : "Type");
  }

  out[used] = '\0';
  return out;
}

static const char *mir_type_symbol_fragment(MirArena *arena, Type *type) {
  if (!arena || !type) {
    return NULL;
  }

  switch (type->kind) {
  case T_INT:
    return "Int";
  case T_UINT64:
    return "Uint64";
  case T_NUM:
    return "Double";
  case T_CHAR:
    return "Char";
  case T_BOOL:
    return "Bool";
  case T_VOID:
    return "Void";
  case T_STRING:
    return "String";
  case T_EMPTY_LIST:
    return "EmptyList";
  case T_MODULE:
    return "Module";
  case T_VAR:
    if (type->data.T_VAR.name) {
      return mir_symbol_sanitize(arena, type->data.T_VAR.name, "TypeVar");
    }
    return mir_arena_printf(arena, "t%d", type->data.T_VAR.id);
  case T_RECURSIVE_REF:
    return mir_symbol_sanitize(arena, type->data.T_RECURSIVE_REF.name,
                               "RecursiveRef");
  case T_CONS:
  case T_SUM: {
    const char *name = type->alias ? type->alias : type->data.T_CONS.name;
    const char *fragment =
        mir_symbol_sanitize(arena, name, type->kind == T_SUM ? "Sum" : "Cons");
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      Type *arg = type->data.T_CONS.args ? type->data.T_CONS.args[i] : NULL;
      const char *arg_fragment = mir_type_symbol_fragment(arena, arg);
      if (!arg_fragment) {
        return NULL;
      }
      fragment = mir_arena_printf(arena, "%s_%s", fragment, arg_fragment);
    }
    return fragment;
  }
  case T_FN: {
    const char *fragment = "Fn";
    Type *cur = type;
    while (cur && cur->kind == T_FN) {
      const char *arg_fragment =
          mir_type_symbol_fragment(arena, cur->data.T_FN.from);
      if (!arg_fragment) {
        return NULL;
      }
      fragment = mir_arena_printf(arena, "%s_%s", fragment, arg_fragment);
      cur = cur->data.T_FN.to;
    }
    const char *ret_fragment = mir_type_symbol_fragment(arena, cur);
    return ret_fragment
               ? mir_arena_printf(arena, "%s_%s", fragment, ret_fragment)
               : NULL;
  }
  }

  return "Type";
}

static const char *mir_specialized_function_name(MirArena *arena,
                                                 const char *base_name,
                                                 Type *callee_type) {
  if (!arena || !base_name || !callee_type ||
      mir_type_has_type_vars(callee_type)) {
    return NULL;
  }

  const char *name = mir_symbol_sanitize(arena, base_name, "anonymous");
  Type *cur = callee_type;
  while (cur && cur->kind == T_FN) {
    const char *arg_fragment =
        mir_type_symbol_fragment(arena, cur->data.T_FN.from);
    if (!arg_fragment) {
      return NULL;
    }
    name = mir_arena_printf(arena, "%s.%s", name, arg_fragment);
    cur = cur->data.T_FN.to;
  }

  const char *ret_fragment = mir_type_symbol_fragment(arena, cur);
  return ret_fragment ? mir_arena_printf(arena, "%s.%s", name, ret_fragment)
                      : NULL;
}

static MirInstr *mir_call_callee_fn_ref(MirBuilder *builder, MirInstr *call) {
  if (!builder || !builder->fn || !call || !mir_instr_is_call_like(call) ||
      call->data.call.builtin || call->data.call.callee == MIR_NO_VALUE) {
    return NULL;
  }

  MirInstr *callee_def =
      mir_function_find_def_instr(builder->fn, call->data.call.callee);
  if (!callee_def || callee_def->kind != MIR_FN_REF) {
    return NULL;
  }

  return callee_def;
}

static const char *mir_call_specialized_name(MirBuilder *builder,
                                             MirInstr *call) {
  MirInstr *callee_def = mir_call_callee_fn_ref(builder, call);
  if (!callee_def || !callee_def->data.fn_ref.name) {
    return NULL;
  }

  MirFunction *target = builder && builder->program
                            ? mir_program_get_function(
                                  builder->program, callee_def->data.fn_ref.fn)
                            : NULL;
  if (!target || !mir_type_has_type_vars(target->type)) {
    return NULL;
  }

  return mir_specialized_function_name(builder->fn->arena,
                                       callee_def->data.fn_ref.name,
                                       call->data.call.callee_type);
}

static MirFunction *mir_materialize_call_specialization(MirBuilder *builder,
                                                        MirInstr *call);
static MirFunction *mir_clone_specialized_function(MirProgram *program,
                                                   MirFunction *source,
                                                   const char *name,
                                                   Type *specialized_type);
static void mir_call_apply_callee_summary(MirBuilder *builder, MirInstr *call);
static Type *mir_call_type_from_operand_values(MirBuilder *builder,
                                               MirInstr *call,
                                               Type *result_type);

typedef struct MirTypeSubst {
  int id;
  const char *name;
  Type *type;
  struct MirTypeSubst *next;
} MirTypeSubst;

static bool mir_type_var_matches(Type *var, MirTypeSubst *entry) {
  if (!var || var->kind != T_VAR || !entry) {
    return false;
  }
  if (var->data.T_VAR.id >= 0 && entry->id >= 0) {
    return var->data.T_VAR.id == entry->id;
  }
  return var->data.T_VAR.name && entry->name &&
         strcmp(var->data.T_VAR.name, entry->name) == 0;
}

static Type *mir_type_subst_lookup(MirTypeSubst *subst, Type *var) {
  for (MirTypeSubst *entry = subst; entry; entry = entry->next) {
    if (mir_type_var_matches(var, entry)) {
      return entry->type;
    }
  }
  return NULL;
}

static bool mir_type_subst_bind(MirArena *arena, MirTypeSubst **subst,
                                Type *var, Type *concrete) {
  if (!arena || !subst || !var || var->kind != T_VAR || !concrete) {
    return false;
  }

  Type *existing = mir_type_subst_lookup(*subst, var);
  if (existing) {
    return types_equal(existing, concrete);
  }

  MirTypeSubst *entry =
      mir_arena_alloc(arena, sizeof(MirTypeSubst), MIR_ALIGNOF(MirTypeSubst));
  if (!entry) {
    return false;
  }

  *entry = (MirTypeSubst){.id = var->data.T_VAR.id,
                          .name = var->data.T_VAR.name,
                          .type = concrete,
                          .next = *subst};
  *subst = entry;
  return true;
}

static bool mir_collect_type_subst(MirArena *arena, MirTypeSubst **subst,
                                   Type *generic, Type *concrete) {
  if (!generic || !concrete) {
    return generic == concrete;
  }

  if (generic->kind == T_VAR) {
    return mir_type_subst_bind(arena, subst, generic, concrete);
  }

  if (generic->kind != concrete->kind) {
    return false;
  }

  switch (generic->kind) {
  case T_FN:
    return mir_collect_type_subst(arena, subst, generic->closure_meta,
                                  concrete->closure_meta) &&
           mir_collect_type_subst(arena, subst, generic->data.T_FN.from,
                                  concrete->data.T_FN.from) &&
           mir_collect_type_subst(arena, subst, generic->data.T_FN.to,
                                  concrete->data.T_FN.to);
  case T_CONS:
  case T_SUM:
    if (generic->data.T_CONS.num_args != concrete->data.T_CONS.num_args) {
      return false;
    }
    for (int i = 0; i < generic->data.T_CONS.num_args; i++) {
      Type *generic_arg =
          generic->data.T_CONS.args ? generic->data.T_CONS.args[i] : NULL;
      Type *concrete_arg =
          concrete->data.T_CONS.args ? concrete->data.T_CONS.args[i] : NULL;
      if (!mir_collect_type_subst(arena, subst, generic_arg, concrete_arg)) {
        return false;
      }
    }
    return mir_collect_type_subst(arena, subst, generic->closure_meta,
                                  concrete->closure_meta);
  default:
    return types_equal(generic, concrete);
  }
}

static Type *mir_substitute_type(MirArena *arena, MirTypeSubst *subst,
                                 Type *type) {
  if (!arena || !type) {
    return type;
  }

  if (type->kind == T_VAR) {
    Type *replacement = mir_type_subst_lookup(subst, type);
    return replacement ? replacement : type;
  }

  switch (type->kind) {
  case T_FN: {
    Type *copy = mir_arena_alloc(arena, sizeof(Type), MIR_ALIGNOF(Type));
    if (!copy) {
      return type;
    }
    *copy = *type;
    copy->data.T_FN.from =
        mir_substitute_type(arena, subst, type->data.T_FN.from);
    copy->data.T_FN.to = mir_substitute_type(arena, subst, type->data.T_FN.to);
    copy->closure_meta = mir_substitute_type(arena, subst, type->closure_meta);
    return copy;
  }
  case T_CONS:
  case T_SUM: {
    Type *copy = mir_arena_alloc(arena, sizeof(Type), MIR_ALIGNOF(Type));
    if (!copy) {
      return type;
    }
    *copy = *type;
    if (type->data.T_CONS.num_args > 0) {
      Type **args = mir_arena_alloc(
          arena, sizeof(Type *) * (size_t)type->data.T_CONS.num_args,
          MIR_ALIGNOF(Type *));
      if (!args) {
        return type;
      }
      for (int i = 0; i < type->data.T_CONS.num_args; i++) {
        Type *arg = type->data.T_CONS.args ? type->data.T_CONS.args[i] : NULL;
        args[i] = mir_substitute_type(arena, subst, arg);
      }
      copy->data.T_CONS.args = args;
    }
    copy->closure_meta = mir_substitute_type(arena, subst, type->closure_meta);
    return copy;
  }
  default:
    return type;
  }
}

static Type *mir_closure_callable_type(MirArena *arena, Type *closure_type) {
  if (!arena || !closure_type || !is_closure(closure_type) ||
      !closure_type->closure_meta) {
    return closure_type;
  }

  Type *callable_type = mir_substitute_type(arena, NULL, closure_type);
  if (callable_type && callable_type->kind == T_FN) {
    callable_type->closure_meta = NULL;
  }

  if (callable_type && callable_type->kind == T_FN &&
      callable_type->data.T_FN.from &&
      callable_type->data.T_FN.from->kind == T_VOID) {
    callable_type = callable_type->data.T_FN.to;
  }

  Type *impl_type = mir_arena_alloc(arena, sizeof(Type), MIR_ALIGNOF(Type));
  if (!impl_type) {
    return closure_type;
  }
  memset(impl_type, 0, sizeof(*impl_type));
  impl_type->kind = T_FN;
  impl_type->data.T_FN.from = closure_type->closure_meta;
  impl_type->data.T_FN.to = callable_type;
  impl_type->data.T_FN.attributes = closure_type->data.T_FN.attributes;
  return impl_type;
}

static MirFunction *mir_specialize_fn_ref_instr(MirProgram *program,
                                                MirInstr *fn_ref,
                                                Type *specialized_type) {
  if (!program || !fn_ref || fn_ref->kind != MIR_FN_REF ||
      fn_ref->data.fn_ref.fn == MIR_NO_FUNCTION || !specialized_type ||
      mir_type_has_type_vars(specialized_type)) {
    return NULL;
  }

  fn_ref->type = specialized_type;

  MirFunction *target =
      mir_program_get_function(program, fn_ref->data.fn_ref.fn);
  if (!target) {
    return NULL;
  }

  if (!mir_type_has_type_vars(target->type)) {
    return target;
  }

  MirFunction *specialized =
      mir_program_find_specialization(program, target->id, specialized_type);
  if (!specialized) {
    const char *specialized_name = mir_specialized_function_name(
        program->arena,
        fn_ref->data.fn_ref.name ? fn_ref->data.fn_ref.name : target->name,
        specialized_type);
    if (specialized_name) {
      specialized = mir_clone_specialized_function(
          program, target, specialized_name, specialized_type);
    }
  }

  if (specialized) {
    fn_ref->data.fn_ref.fn = specialized->id;
    fn_ref->data.fn_ref.name = specialized->name;
  }
  return specialized;
}

static MirFunction *mir_specialize_closure_impl_fn_ref(MirProgram *program,
                                                       MirInstr *fn_ref,
                                                       Type *closure_type,
                                                       Type **out_impl_type) {
  if (out_impl_type) {
    *out_impl_type = NULL;
  }
  if (!program || !program->arena || !fn_ref || fn_ref->kind != MIR_FN_REF ||
      !closure_type || !is_closure(closure_type) ||
      !closure_type->closure_meta) {
    return NULL;
  }

  Type *impl_type = mir_closure_callable_type(program->arena, closure_type);
  if (out_impl_type) {
    *out_impl_type = impl_type;
  }
  if (!impl_type || mir_type_has_type_vars(impl_type)) {
    return NULL;
  }

  MirFunction *target =
      mir_program_get_function(program, fn_ref->data.fn_ref.fn);
  Type *specialization_type =
      target && target->type && !is_closure(target->type) ? impl_type
                                                          : closure_type;
  return mir_specialize_fn_ref_instr(program, fn_ref, specialization_type);
}

Type *mir_function_value_type(MirFunction *fn, MirValueId value) {
  if (!fn || value == MIR_NO_VALUE || value >= fn->values.len) {
    return NULL;
  }
  return fn->values.items[value].type;
}

static void mir_function_set_value_type(MirFunction *fn, MirValueId value,
                                        Type *type) {
  if (!fn || value == MIR_NO_VALUE || value >= fn->values.len) {
    return;
  }
  fn->values.items[value].type = type;
  fn->values.items[value].callable_summary =
      mir_callable_summary_from_type(fn->arena, type);
}

static void mir_specialize_call_fn_ref_operands(MirBuilder *builder,
                                                MirInstr *call) {
  if (!builder || !builder->program || !builder->fn || !call ||
      !mir_instr_is_call_like(call) || !call->data.call.callee_type) {
    return;
  }

  Type *cursor = call->data.call.callee_type;
  for (size_t i = 0;
       i < call->data.call.operands.len && cursor && cursor->kind == T_FN;
       i++, cursor = cursor->data.T_FN.to) {
    Type *expected_type = cursor->data.T_FN.from;
    if (!expected_type || expected_type->kind != T_FN ||
        is_closure(expected_type) || mir_type_has_type_vars(expected_type)) {
      continue;
    }

    MirValueId operand = call->data.call.operands.items[i];
    MirInstr *fn_ref = mir_function_find_def_instr(builder->fn, operand);
    if (!fn_ref || fn_ref->kind != MIR_FN_REF) {
      continue;
    }

    MirFunction *specialized =
        mir_specialize_fn_ref_instr(builder->program, fn_ref, expected_type);
    mir_function_set_value_type(builder->fn, fn_ref->result, expected_type);
    if (specialized) {
      mir_function_set_value_callable_summary(builder->fn, fn_ref->result,
                                              &specialized->summary);
    }
  }
}

static MirValueId mir_remap_value(MirValueId *value_map, size_t value_map_len,
                                  MirValueId value) {
  if (value == MIR_NO_VALUE || !value_map || value >= value_map_len) {
    return MIR_NO_VALUE;
  }
  return value_map[value];
}

typedef struct {
  MirValueId *value_map;
  size_t value_map_len;
} MirRemapOperandCtx;

static MirValueId mir_remap_operand(MirInstr *instr, MirOperand operand,
                                    void *ctx) {
  (void)instr;
  MirRemapOperandCtx *remap = ctx;
  if (!remap) {
    return MIR_NO_VALUE;
  }
  return mir_remap_value(remap->value_map, remap->value_map_len, operand.value);
}

static MirInstr mir_clone_instr_with_subst(MirArena *arena, MirTypeSubst *subst,
                                           MirValueId *value_map,
                                           size_t value_map_len,
                                           const MirInstr *instr) {
  MirInstr clone = *instr;
  clone.result = MIR_NO_VALUE;
  clone.type = mir_substitute_type(arena, subst, instr->type);

  switch (instr->kind) {
  case MIR_PHI:
    clone.data.phi.incoming = (MirPhiIncomingVec){0};
    for (size_t i = 0; i < instr->data.phi.incoming.len; i++) {
      mir_phi_incoming_vec_push(arena, &clone.data.phi.incoming,
                                instr->data.phi.incoming.items[i]);
    }
    break;
  case MIR_OP:
    clone.data.op.from_type =
        mir_substitute_type(arena, subst, instr->data.op.from_type);
    clone.data.op.to_type =
        mir_substitute_type(arena, subst, instr->data.op.to_type);
    break;
  case MIR_EXTRACT:
    clone.data.extract.constructor_type =
        mir_substitute_type(arena, subst, instr->data.extract.constructor_type);
    break;
  case MIR_CONSTRUCT:
    clone.data.construct.constructor_type = mir_substitute_type(
        arena, subst, instr->data.construct.constructor_type);
    clone.data.construct.items = (MirValueIdVec){0};
    for (size_t i = 0; i < instr->data.construct.items.len; i++) {
      mir_value_id_vec_push(arena, &clone.data.construct.items,
                            instr->data.construct.items.items[i]);
    }
    break;
  case MIR_CALL:
  case MIR_CORO_NEW:
  case MIR_CORO_NEXT:
  case MIR_CORO_RESET:
    clone.data.call.callee_type =
        mir_substitute_type(arena, subst, instr->data.call.callee_type);
    clone.data.call.specialized_name = NULL;
    clone.data.call.specialized_fn = MIR_NO_FUNCTION;
    clone.data.call.operands = (MirValueIdVec){0};
    clone.data.call.operand_uses = (MirOperandUseVec){0};
    for (size_t i = 0; i < instr->data.call.operands.len; i++) {
      mir_value_id_vec_push(arena, &clone.data.call.operands,
                            instr->data.call.operands.items[i]);
      mir_operand_use_vec_push(arena, &clone.data.call.operand_uses,
                               mir_call_operand_use(instr, i));
    }
    break;
  default:
    break;
  }

  if (clone.kind != MIR_PHI) {
    MirRemapOperandCtx remap_ctx = {.value_map = value_map,
                                    .value_map_len = value_map_len};
    mir_instr_rewrite_operands(&clone, mir_remap_operand, &remap_ctx);
  }

  return clone;
}

static Type *mir_fn_type_from_args(MirArena *arena, Type **args, size_t len,
                                   Type *result_type);

static Type *mir_clone_call_type_from_source_operands(MirProgram *program,
                                                      MirTypeSubst *subst,
                                                      MirFunction *source,
                                                      const MirInstr *instr,
                                                      Type *result_type) {
  if (!program || !program->arena || !source || !instr ||
      !mir_instr_is_call_like(instr)) {
    return NULL;
  }

  size_t operand_count = instr->data.call.operands.len;
  Type **operand_types =
      operand_count
          ? mir_arena_alloc(program->arena, sizeof(Type *) * operand_count,
                            MIR_ALIGNOF(Type *))
          : NULL;
  if (operand_count && !operand_types) {
    return NULL;
  }

  for (size_t i = 0; i < operand_count; i++) {
    MirValueId source_value = instr->data.call.operands.items[i];
    Type *source_type = mir_function_value_type(source, source_value);
    if (!source_type) {
      return NULL;
    }
    operand_types[i] = mir_substitute_type(program->arena, subst, source_type);
  }

  return mir_fn_type_from_args(program->arena, operand_types, operand_count,
                               result_type);
}

static void mir_resolve_named_extract_field(MirFunction *fn, MirInstr *instr) {
  if (!fn || !instr || instr->kind != MIR_EXTRACT ||
      instr->data.extract.kind != MIR_EXTRACT_FIELD ||
      !instr->data.extract.name) {
    return;
  }

  Type *record_type = mir_function_value_type(fn, instr->data.extract.value);
  Type *record_view = mir_record_access_type_view(record_type);
  if ((!record_view || record_view->kind != T_CONS ||
       is_generic(record_view)) &&
      instr->origin && instr->origin->tag == AST_RECORD_ACCESS &&
      instr->origin->data.AST_RECORD_ACCESS.record) {
    record_view = mir_record_access_type_view(
        instr->origin->data.AST_RECORD_ACCESS.record->type);
  }
  if (!record_view || record_view->kind != T_CONS) {
    return;
  }

  bool has_named_fields = record_view->data.T_CONS.names != NULL;
  int member_index =
      get_struct_member_idx(instr->data.extract.name, record_view);
  if (member_index < 0 && !has_named_fields && !is_generic(record_view) &&
      instr->origin && instr->origin->tag == AST_RECORD_ACCESS) {
    member_index = instr->origin->data.AST_RECORD_ACCESS.index;
  }
  if (member_index < 0) {
    return;
  }
  if (member_index >= record_view->data.T_CONS.num_args) {
    return;
  }

  instr->data.extract.index = (size_t)member_index;
  if ((!instr->type || is_generic(instr->type)) &&
      record_view->data.T_CONS.args) {
    instr->type = record_view->data.T_CONS.args[member_index];
  }
}

static void mir_resolve_named_extract_fields(MirFunction *fn) {
  if (!fn) {
    return;
  }

  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items ? fn->blocks.items[i] : NULL;
    if (!block) {
      continue;
    }
    for (size_t j = 0; j < block->instrs.len; j++) {
      MirInstr *instr = &block->instrs.items[j];
      if (instr->kind == MIR_EXTRACT &&
          instr->data.extract.kind == MIR_EXTRACT_FIELD) {
        mir_resolve_named_extract_field(fn, instr);
      }
    }
  }
}

static void mir_resolve_named_extract_fields_program(MirProgram *program) {
  if (!program) {
    return;
  }

  for (size_t i = 0; i < program->functions.len; i++) {
    MirFunction *fn =
        program->functions.items ? program->functions.items[i] : NULL;
    mir_resolve_named_extract_fields(fn);
  }
}

static void mir_remap_deferred_phi_incomings(MirFunction *fn,
                                             MirValueId *value_map,
                                             size_t value_map_len) {
  if (!fn) {
    return;
  }

  MirRemapOperandCtx remap_ctx = {.value_map = value_map,
                                  .value_map_len = value_map_len};
  for (size_t i = 0; i < fn->blocks.len; i++) {
    MirBlock *block = fn->blocks.items[i];
    if (!block) {
      continue;
    }

    for (size_t j = 0; j < block->instrs.len; j++) {
      MirInstr *instr = &block->instrs.items[j];
      if (instr->kind == MIR_PHI) {
        mir_instr_rewrite_operands(instr, mir_remap_operand, &remap_ctx);
      }
    }
  }
}

static void mir_predict_clone_value_ids(MirFunction *source, MirFunction *fn,
                                        MirValueId *value_map,
                                        size_t value_map_len) {
  if (!source || !fn || !value_map) {
    return;
  }

  MirValueId next_value = (MirValueId)fn->values.len;
  for (size_t i = 0; i < source->blocks.len; i++) {
    MirBlock *block = source->blocks.items[i];
    if (!block) {
      continue;
    }

    for (size_t j = 0; j < block->instrs.len; j++) {
      MirInstr *instr = &block->instrs.items[j];
      if (instr->result < value_map_len &&
          value_map[instr->result] == MIR_NO_VALUE) {
        value_map[instr->result] = next_value++;
      }
    }
  }
}

static MirFunction *mir_clone_specialized_function(MirProgram *program,
                                                   MirFunction *source,
                                                   const char *name,
                                                   Type *specialized_type) {
  if (!program || !source || !name || !specialized_type) {
    return NULL;
  }

  MirTypeSubst *subst = NULL;
  if (!mir_collect_type_subst(program->arena, &subst, source->type,
                              specialized_type)) {
    return NULL;
  }

  MirFunction *fn =
      mir_program_add_function(program, name, specialized_type, source->origin);
  if (!fn) {
    return NULL;
  }
  fn->is_extern = source->is_extern;
  fn->specialization_of = source->id;
  fn->specialization_type = specialized_type;

  if (source->is_extern) {
    mir_function_add_extern_params(fn, specialized_type, source->origin);
    return fn;
  }

  size_t value_map_len = source->values.len;
  MirValueId *value_map =
      mir_arena_alloc(program->arena, sizeof(MirValueId) * value_map_len,
                      MIR_ALIGNOF(MirValueId));
  if (!value_map && value_map_len > 0) {
    return fn;
  }
  for (size_t i = 0; i < value_map_len; i++) {
    value_map[i] = MIR_NO_VALUE;
  }

  for (size_t i = 0; i < source->params.len; i++) {
    MirParam *param = &source->params.items[i];
    Type *param_type = mir_substitute_type(program->arena, subst, param->type);
    MirValueId value =
        mir_function_add_param(fn, param->name, param_type, param->origin);
    mir_function_set_param_use(fn, i, mir_function_param_use(source, i));
    mir_function_set_value_callable_summary(
        fn, value, mir_value_callable_summary(source, param->value));
    if (param->value < value_map_len) {
      value_map[param->value] = value;
    }
  }

  mir_predict_clone_value_ids(source, fn, value_map, value_map_len);

  for (size_t i = 0; i < source->blocks.len; i++) {
    MirBlock *source_block = source->blocks.items[i];
    MirBlock *block = mir_function_add_block(
        fn, source_block && source_block->name ? source_block->name : "bb");
    if (!source_block || !block) {
      continue;
    }

    MirBuilder builder;
    mir_builder_init(&builder, program, fn);
    mir_builder_position_at_end(&builder, block);

    for (size_t j = 0; j < source_block->instrs.len; j++) {
      MirInstr *instr = &source_block->instrs.items[j];
      MirInstr clone = mir_clone_instr_with_subst(
          program->arena, subst, value_map, value_map_len, instr);

      MirValueId result = MIR_NO_VALUE;
      if (clone.kind == MIR_FN_REF && clone.type &&
          !mir_type_has_type_vars(clone.type)) {
        mir_specialize_fn_ref_instr(program, &clone, clone.type);
      }
      if (clone.kind == MIR_CONSTRUCT &&
          clone.data.construct.kind == MIR_CONSTRUCT_CLOSURE && clone.type &&
          is_closure(clone.type) && clone.type->closure_meta &&
          !mir_type_has_type_vars(clone.type)) {
        Type *impl_type = NULL;
        MirInstr *fn_ref =
            mir_function_find_def_instr(fn, clone.data.construct.operands[0]);
        MirFunction *impl = mir_specialize_closure_impl_fn_ref(
            program, fn_ref, clone.type, &impl_type);
        if (fn_ref && impl_type && !mir_type_has_type_vars(impl_type)) {
          fn_ref->type = impl_type;
          mir_function_set_value_type(fn, fn_ref->result, impl_type);
        }
        if (impl) {
          clone.data.construct.impl_fn = impl->id;
          clone.data.construct.impl_name = impl->name;
        }
      }
      if (clone.kind == MIR_EXTRACT &&
          clone.data.extract.kind == MIR_EXTRACT_FIELD) {
        mir_resolve_named_extract_field(fn, &clone);
      }
      if (mir_instr_is_call_like(&clone) &&
          (!clone.data.call.callee_type ||
           mir_type_has_type_vars(clone.data.call.callee_type))) {
        Type *callee_type =
            mir_call_type_from_operand_values(&builder, &clone, clone.type);
        if (callee_type && !mir_type_has_type_vars(callee_type)) {
          clone.data.call.callee_type = callee_type;
        }
      }
      if (clone.kind == MIR_CALL && clone.data.call.builtin) {
        Type *callee_type = mir_clone_call_type_from_source_operands(
            program, subst, source, instr, clone.type);
        if (callee_type && !mir_type_has_type_vars(callee_type)) {
          clone.data.call.callee_type = callee_type;
        }
        mir_specialize_call_fn_ref_operands(&builder, &clone);
        result = mir_lower_specialized_builtin_call(&builder, &clone);
      }
      if (clone.kind == MIR_CALL && !clone.data.call.builtin) {
        mir_specialize_call_fn_ref_operands(&builder, &clone);
        clone.data.call.specialized_name =
            mir_call_specialized_name(&builder, &clone);
        MirFunction *specialized =
            mir_materialize_call_specialization(&builder, &clone);
        if (specialized) {
          clone.data.call.specialized_name = specialized->name;
          clone.data.call.specialized_fn = specialized->id;
        }
        mir_call_apply_callee_summary(&builder, &clone);
      }
      if (clone.kind == MIR_CORO_NEW) {
        mir_specialize_call_fn_ref_operands(&builder, &clone);
        clone.data.call.specialized_name =
            mir_call_specialized_name(&builder, &clone);
        MirFunction *specialized =
            mir_materialize_call_specialization(&builder, &clone);
        if (specialized) {
          clone.data.call.specialized_name = specialized->name;
          clone.data.call.specialized_fn = specialized->id;
        }
        mir_call_apply_callee_summary(&builder, &clone);
      }
      if (result == MIR_NO_VALUE) {
        result = mir_builder_append_instr(&builder, clone);
      }
      if (instr->result < value_map_len) {
        value_map[instr->result] = result;
      }
    }

    switch (source_block->term.kind) {
    case MIR_TERM_NONE:
      block->term = (MirTerminator){.kind = MIR_TERM_NONE,
                                    .value = MIR_NO_VALUE,
                                    .cond = MIR_NO_VALUE,
                                    .target = MIR_NO_BLOCK,
                                    .then_block = MIR_NO_BLOCK,
                                    .else_block = MIR_NO_BLOCK};
      break;
    case MIR_TERM_RETURN:
      block->term =
          (MirTerminator){.kind = MIR_TERM_RETURN,
                          .value = mir_remap_value(value_map, value_map_len,
                                                   source_block->term.value),
                          .cond = MIR_NO_VALUE,
                          .target = MIR_NO_BLOCK,
                          .then_block = MIR_NO_BLOCK,
                          .else_block = MIR_NO_BLOCK};
      break;
    case MIR_TERM_BR:
      block->term = (MirTerminator){.kind = MIR_TERM_BR,
                                    .value = MIR_NO_VALUE,
                                    .cond = MIR_NO_VALUE,
                                    .target = source_block->term.target,
                                    .then_block = MIR_NO_BLOCK,
                                    .else_block = MIR_NO_BLOCK};
      break;
    case MIR_TERM_COND:
      block->term =
          (MirTerminator){.kind = MIR_TERM_COND,
                          .value = MIR_NO_VALUE,
                          .cond = mir_remap_value(value_map, value_map_len,
                                                  source_block->term.cond),
                          .target = MIR_NO_BLOCK,
                          .then_block = source_block->term.then_block,
                          .else_block = source_block->term.else_block};
      break;
    case MIR_TERM_YIELD:
      block->term =
          (MirTerminator){.kind = MIR_TERM_YIELD,
                          .value = mir_remap_value(value_map, value_map_len,
                                                   source_block->term.value),
                          .cond = MIR_NO_VALUE,
                          .target = source_block->term.target,
                          .then_block = MIR_NO_BLOCK,
                          .else_block = MIR_NO_BLOCK};
      break;
    case MIR_TERM_CORO_RESTART: {
      MirValueIdVec args = {0};
      for (size_t k = 0; k < source_block->term.args.len; k++) {
        mir_value_id_vec_push(
            program->arena, &args,
            mir_remap_value(value_map, value_map_len,
                            source_block->term.args.items[k]));
      }
      block->term = (MirTerminator){.kind = MIR_TERM_CORO_RESTART,
                                    .value = MIR_NO_VALUE,
                                    .cond = MIR_NO_VALUE,
                                    .target = source_block->term.target,
                                    .then_block = MIR_NO_BLOCK,
                                    .else_block = MIR_NO_BLOCK,
                                    .args = args};
      break;
    }
    case MIR_TERM_CORO_DONE:
      block->term = (MirTerminator){.kind = MIR_TERM_CORO_DONE,
                                    .value = MIR_NO_VALUE,
                                    .cond = MIR_NO_VALUE,
                                    .target = MIR_NO_BLOCK,
                                    .then_block = MIR_NO_BLOCK,
                                    .else_block = MIR_NO_BLOCK};
      break;
    case MIR_TERM_UNREACHABLE:
      block->term = (MirTerminator){.kind = MIR_TERM_UNREACHABLE,
                                    .value = MIR_NO_VALUE,
                                    .cond = MIR_NO_VALUE,
                                    .target = MIR_NO_BLOCK,
                                    .then_block = MIR_NO_BLOCK,
                                    .else_block = MIR_NO_BLOCK};
      break;
    }
  }

  mir_remap_deferred_phi_incomings(fn, value_map, value_map_len);
  mir_resolve_named_extract_fields(fn);
  return fn;
}

static MirFunction *mir_materialize_call_specialization(MirBuilder *builder,
                                                        MirInstr *call) {
  if (!builder || !builder->program || !call ||
      !call->data.call.specialized_name) {
    return NULL;
  }

  MirInstr *callee_def = mir_call_callee_fn_ref(builder, call);
  if (!callee_def) {
    return NULL;
  }

  MirFunction *source =
      mir_program_get_function(builder->program, callee_def->data.fn_ref.fn);
  if (!source) {
    return NULL;
  }
  if (!mir_type_has_type_vars(source->type)) {
    return NULL;
  }

  MirFunction *existing = mir_program_find_specialization(
      builder->program, source->id, call->data.call.callee_type);
  if (existing) {
    return existing;
  }

  return mir_clone_specialized_function(builder->program, source,
                                        call->data.call.specialized_name,
                                        call->data.call.callee_type);
}

static Type *mir_application_expected_callee_type(MirArena *arena, Ast *ast,
                                                  Type *function_type) {
  if (!arena || !ast || ast->tag != AST_APPLICATION) {
    return NULL;
  }

  FnAttributes *attrs = NULL;
  if (ast->data.AST_APPLICATION.len > 0) {
    attrs = mir_arena_alloc(
        arena, sizeof(FnAttributes) * ast->data.AST_APPLICATION.len,
        MIR_ALIGNOF(FnAttributes));
    Type *cursor = function_type;
    for (size_t i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      attrs[i] =
          cursor && cursor->kind == T_FN ? cursor->data.T_FN.attributes : 0;
      cursor = cursor && cursor->kind == T_FN ? cursor->data.T_FN.to : NULL;
    }
  }

  Type *type = ast->type;
  for (size_t i = ast->data.AST_APPLICATION.len; i > 0; i--) {
    Ast *arg = ast->data.AST_APPLICATION.args + (i - 1);
    Type *fn_type = mir_arena_alloc(arena, sizeof(Type), MIR_ALIGNOF(Type));
    if (!fn_type) {
      return NULL;
    }
    memset(fn_type, 0, sizeof(*fn_type));
    fn_type->kind = T_FN;
    fn_type->data.T_FN.from = arg->type;
    fn_type->data.T_FN.to = type;
    fn_type->data.T_FN.attributes = attrs ? attrs[i - 1] : 0;
    type = fn_type;
  }
  return type;
}

static void mir_call_push_operand(MirArena *arena, MirInstr *call,
                                  MirValueId operand, MirOperandUse use) {
  if (!arena || !call || !mir_instr_is_call_like(call) ||
      operand == MIR_NO_VALUE) {
    return;
  }
  mir_value_id_vec_push(arena, &call->data.call.operands, operand);
  mir_operand_use_vec_push(arena, &call->data.call.operand_uses, use);
}

static MirFunction *mir_call_summary_function(MirBuilder *builder,
                                              MirInstr *call) {
  if (!builder || !builder->program || !builder->fn || !call ||
      !mir_instr_is_call_like(call) || call->data.call.builtin) {
    return NULL;
  }

  if (call->data.call.specialized_fn != MIR_NO_FUNCTION) {
    MirFunction *specialized = mir_program_get_function(
        builder->program, call->data.call.specialized_fn);
    if (specialized) {
      return specialized;
    }
  }

  MirInstr *callee =
      mir_function_find_def_instr(builder->fn, call->data.call.callee);
  if (!callee || callee->kind != MIR_FN_REF ||
      callee->data.fn_ref.fn == MIR_NO_FUNCTION) {
    return NULL;
  }

  return mir_program_get_function(builder->program, callee->data.fn_ref.fn);
}

static const MirFnSummary *mir_call_summary(MirBuilder *builder,
                                            MirInstr *call) {
  if (!mir_instr_is_call_like(call)) {
    return NULL;
  }

  if (call->data.call.builtin) {
    return &call->data.call.builtin->summary;
  }

  MirFunction *callee = mir_call_summary_function(builder, call);
  if (callee) {
    return &callee->summary;
  }

  return builder && builder->fn
             ? mir_value_callable_summary(builder->fn, call->data.call.callee)
             : NULL;
}

static void mir_call_apply_callee_summary(MirBuilder *builder, MirInstr *call) {
  const MirFnSummary *summary = mir_call_summary(builder, call);
  if (!builder || !builder->program || !call || !summary) {
    return;
  }

  while (call->data.call.operand_uses.len < call->data.call.operands.len) {
    mir_operand_use_vec_push(builder->program->arena,
                             &call->data.call.operand_uses,
                             MIR_OPERAND_USE_CONSUME);
  }

  size_t len = call->data.call.operands.len;
  if (summary->param_uses.len < len) {
    len = summary->param_uses.len;
  }
  for (size_t i = 0; i < len; i++) {
    call->data.call.operand_uses.items[i] = summary->param_uses.items[i];
  }
}

static bool mir_call_push_application_args(MirBuilder *builder, MirInstr *call,
                                           Ast *ast, MirCtx *ctx,
                                           MirArena *arena,
                                           Type *function_type) {
  if (mir_application_is_void_call(ast, function_type) ||
      mir_application_is_void_call(ast,
                                   call ? call->data.call.callee_type : NULL)) {
    return true;
  }

  for (size_t i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    MirValueId arg = mir_expr(builder, ast->data.AST_APPLICATION.args + i, ctx);
    if (arg == MIR_NO_VALUE) {
      return false;
    }
    mir_call_push_operand(arena, call, arg, MIR_OPERAND_USE_CONSUME);
  }
  return true;
}

static bool mir_application_is_partial(Ast *ast) {
  if (!ast || ast->tag != AST_APPLICATION ||
      !ast->data.AST_APPLICATION.function ||
      !ast->data.AST_APPLICATION.function->type) {
    return false;
  }
  return application_is_partial(ast);
}

static Ast *mir_application_root_function(Ast *app) {
  Ast *fn = app;
  while (fn && fn->tag == AST_APPLICATION) {
    fn = fn->data.AST_APPLICATION.function;
  }
  return fn;
}

static size_t mir_application_flat_arg_count(Ast *app) {
  if (!app || app->tag != AST_APPLICATION) {
    return 0;
  }

  size_t count = app->data.AST_APPLICATION.len;
  Ast *fn = app->data.AST_APPLICATION.function;
  if (fn && fn->tag == AST_APPLICATION) {
    count += mir_application_flat_arg_count(fn);
  }
  return count;
}

static size_t mir_application_collect_flat_args(Ast *app, Ast *args,
                                                size_t offset) {
  if (!app || app->tag != AST_APPLICATION) {
    return offset;
  }

  Ast *fn = app->data.AST_APPLICATION.function;
  if (fn && fn->tag == AST_APPLICATION) {
    offset = mir_application_collect_flat_args(fn, args, offset);
  }

  for (size_t i = 0; i < app->data.AST_APPLICATION.len; i++) {
    args[offset++] = app->data.AST_APPLICATION.args[i];
  }
  return offset;
}

static Ast *mir_application_flatten_if_saturated(MirArena *arena, Ast *app) {
  if (!arena || !app || app->tag != AST_APPLICATION ||
      !app->data.AST_APPLICATION.function ||
      app->data.AST_APPLICATION.function->tag != AST_APPLICATION) {
    return app;
  }

  Ast *root = mir_application_root_function(app);
  if (!root || !root->type || root->type->kind != T_FN) {
    return app;
  }

  size_t arg_count = mir_application_flat_arg_count(app);
  int expected = fn_type_args_len(root->type);
  if (expected <= 0 || arg_count > (size_t)expected) {
    return app;
  }

  Ast *flat = mir_arena_alloc(arena, sizeof(Ast), MIR_ALIGNOF(Ast));
  Ast *args = arg_count ? mir_arena_alloc(arena, sizeof(Ast) * arg_count,
                                          MIR_ALIGNOF(Ast))
                        : NULL;
  if (!flat || (arg_count && !args)) {
    return app;
  }

  *flat = *app;
  flat->data.AST_APPLICATION.function = root;
  flat->data.AST_APPLICATION.args = args;
  flat->data.AST_APPLICATION.len = arg_count;
  mir_application_collect_flat_args(app, args, 0);
  return flat;
}

static size_t mir_fn_param_count(Type *type) {
  size_t count = 0;
  for (Type *cursor = type; cursor && cursor->kind == T_FN;
       cursor = cursor->data.T_FN.to) {
    count++;
  }
  return count;
}

static Type *mir_fn_type_from_args(MirArena *arena, Type **args, size_t len,
                                   Type *result_type) {
  if (!arena) {
    return result_type;
  }

  Type *type = result_type;
  for (size_t i = len; i > 0; i--) {
    Type *fn_type = mir_arena_alloc(arena, sizeof(Type), MIR_ALIGNOF(Type));
    if (!fn_type) {
      return result_type;
    }
    memset(fn_type, 0, sizeof(*fn_type));
    fn_type->kind = T_FN;
    fn_type->data.T_FN.from = args[i - 1];
    fn_type->data.T_FN.to = type;
    type = fn_type;
  }
  return type;
}

static const char *mir_builtin_function_name(MirProgram *program,
                                             MirBuiltinSymbol *builtin) {
  if (!program || !program->arena || !builtin || !builtin->name) {
    return NULL;
  }

  const char *op_name = NULL;
  if (strcmp(builtin->name, "+") == 0) {
    op_name = "op_add";
  } else if (strcmp(builtin->name, "-") == 0) {
    op_name = "op_sub";
  } else if (strcmp(builtin->name, "*") == 0) {
    op_name = "op_mul";
  } else if (strcmp(builtin->name, "/") == 0) {
    op_name = "op_div";
  } else if (strcmp(builtin->name, "%") == 0) {
    op_name = "op_mod";
  } else if (strcmp(builtin->name, "==") == 0) {
    op_name = "op_eq";
  } else if (strcmp(builtin->name, "!=") == 0) {
    op_name = "op_neq";
  } else if (strcmp(builtin->name, "<") == 0) {
    op_name = "op_lt";
  } else if (strcmp(builtin->name, "<=") == 0) {
    op_name = "op_lte";
  } else if (strcmp(builtin->name, ">") == 0) {
    op_name = "op_gt";
  } else if (strcmp(builtin->name, ">=") == 0) {
    op_name = "op_gte";
  } else if (strcmp(builtin->name, "&&") == 0) {
    op_name = "op_and";
  } else if (strcmp(builtin->name, "||") == 0) {
    op_name = "op_or";
  } else if (strcmp(builtin->name, "!") == 0) {
    op_name = "op_not";
  } else if (strcmp(builtin->name, "::") == 0) {
    op_name = "op_list_prepend";
  }

  const char *name =
      op_name ? mir_arena_strdup(program->arena, op_name)
              : mir_symbol_sanitize(program->arena, builtin->name, "builtin");
  return name ? mir_arena_printf(program->arena, "$builtin.%s", name) : NULL;
}

static MirFunction *mir_materialize_builtin_function(MirProgram *program,
                                                     MirBuiltinSymbol *builtin,
                                                     Ast *origin) {
  if (!program || !program->arena || !builtin || !builtin->type ||
      builtin->type->kind != T_FN) {
    return NULL;
  }

  if (builtin->function != MIR_NO_FUNCTION) {
    MirFunction *existing =
        mir_program_get_function(program, builtin->function);
    if (existing) {
      return existing;
    }
    builtin->function = MIR_NO_FUNCTION;
  }

  const char *name = mir_builtin_function_name(program, builtin);
  if (!name) {
    return NULL;
  }

  MirFunction *existing = mir_program_find_function_by_name(program, name);
  if (existing) {
    builtin->function = existing->id;
    return existing;
  }

  MirFunction *fn =
      mir_program_add_function(program, name, builtin->type, origin);
  MirBlock *entry = fn ? mir_function_add_block(fn, "entry") : NULL;
  if (!fn || !entry) {
    return NULL;
  }
  fn->summary.result = builtin->summary.result;
  builtin->function = fn->id;

  MirBuilder builder;
  mir_builder_init(&builder, program, fn);
  mir_builder_position_at_end(&builder, entry);

  MirInstr call =
      mir_make_instr(MIR_CALL, fn_return_type(builtin->type), origin);
  call.data.call.callee = MIR_NO_VALUE;
  call.data.call.builtin = builtin;
  call.data.call.callee_type = builtin->type;
  call.data.call.specialized_fn = MIR_NO_FUNCTION;

  size_t index = 0;
  for (Type *cursor = builtin->type; cursor && cursor->kind == T_FN;
       cursor = cursor->data.T_FN.to, index++) {
    Type *param_type = cursor->data.T_FN.from;
    MirValueId param = mir_function_add_param(
        fn, mir_arena_printf(program->arena, "arg%zu", index), param_type,
        origin);
    if (param == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&builder);
      return fn;
    }
    if (index < builtin->summary.param_uses.len) {
      mir_function_set_param_use(fn, index,
                                 builtin->summary.param_uses.items[index]);
    }
    mir_call_push_operand(program->arena, &call, param,
                          mir_function_param_use(fn, index));
  }

  mir_call_apply_callee_summary(&builder, &call);
  MirValueId result = mir_builder_append_instr(&builder, call);
  if (result == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(&builder);
    return fn;
  }
  mir_builder_set_return(&builder, result);
  return fn;
}

static MirFunction *mir_direct_callee_function(MirBuilder *builder,
                                               Ast *function, MirCtx *ctx) {
  if (!builder || !builder->program || !function) {
    return NULL;
  }

  if (function->tag == AST_IDENTIFIER) {
    MirValueId value = MIR_NO_VALUE;
    if (mir_ctx_lookup_value(ctx, function->data.AST_IDENTIFIER.value,
                             &value)) {
      MirInstr *def = mir_function_find_def_instr(builder->fn, value);
      if (def && def->kind == MIR_FN_REF) {
        return mir_program_get_function(builder->program, def->data.fn_ref.fn);
      }
    }

    MirSymbol *symbol = mir_ctx_lookup_symbol(
        builder->program, ctx, function->data.AST_IDENTIFIER.value);
    if (symbol && (symbol->kind == MIR_SYMBOL_FUNCTION ||
                   symbol->kind == MIR_SYMBOL_EXTERN_FUNCTION)) {
      return mir_program_get_function(builder->program, symbol->as.function);
    }

    MirBuiltinSymbol *builtin = mir_program_lookup_builtin(
        builder->program, function->data.AST_IDENTIFIER.value);
    if (builtin) {
      return mir_materialize_builtin_function(builder->program, builtin,
                                              function);
    }

    MirFunction *scoped = mir_program_find_scoped_function(
        builder->program, builder->fn, function->data.AST_IDENTIFIER.value);
    if (scoped) {
      return scoped;
    }

    return mir_program_find_function_by_name(
        builder->program, function->data.AST_IDENTIFIER.value);
  }

  if (function->tag == AST_RECORD_ACCESS) {
    MirSymbol *symbol = mir_resolve_ast_symbol(builder, function, ctx);
    if (symbol && (symbol->kind == MIR_SYMBOL_FUNCTION ||
                   symbol->kind == MIR_SYMBOL_EXTERN_FUNCTION)) {
      return mir_program_get_function(builder->program, symbol->as.function);
    }
  }

  return NULL;
}

static const char *mir_curried_wrapper_name(MirArena *arena,
                                            MirFunction *callee,
                                            bool captures_env) {
  static unsigned counter = 0;
  const char *callee_name = callee && callee->name ? callee->name : "callable";
  return mir_arena_printf(arena, "%s.curried.%s.%u",
                          captures_env ? "closure" : "const", callee_name,
                          counter++);
}

static MirValueId
mir_append_curried_inner_call(MirBuilder *wrapper_builder, Ast *origin,
                              MirFunction *callee, Type *callee_type,
                              MirValueIdVec operands, Type **operand_types,
                              size_t operand_count, Type *result_type) {

  MirValueId callee_ref =
      mir_fn_ref(wrapper_builder, callee_type, origin, callee);
  if (callee_ref == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr call = mir_make_instr(MIR_CALL, result_type, origin);
  call.data.call.callee = callee_ref;
  call.data.call.builtin = NULL;
  call.data.call.specialized_fn = MIR_NO_FUNCTION;
  call.data.call.operands = operands;
  call.data.call.operand_uses = (MirOperandUseVec){0};
  for (size_t i = 0; i < operands.len; i++) {
    mir_operand_use_vec_push(wrapper_builder->program->arena,
                             &call.data.call.operand_uses,
                             MIR_OPERAND_USE_CONSUME);
  }
  call.data.call.callee_type =
      mir_fn_type_from_args(wrapper_builder->program->arena, operand_types,
                            operand_count, result_type);
  mir_specialize_call_fn_ref_operands(wrapper_builder, &call);
  call.data.call.specialized_name =
      mir_call_specialized_name(wrapper_builder, &call);

  MirFunction *specialized =
      mir_materialize_call_specialization(wrapper_builder, &call);
  if (specialized) {
    call.data.call.specialized_name = specialized->name;
    call.data.call.specialized_fn = specialized->id;
  }
  mir_call_apply_callee_summary(wrapper_builder, &call);

  return mir_builder_append_instr(wrapper_builder, call);
}

static Type *mir_call_type_from_operand_values(MirBuilder *builder,
                                               MirInstr *call,
                                               Type *result_type) {
  if (!builder || !builder->program || !builder->fn || !call) {
    return NULL;
  }

  size_t operand_count = call->data.call.operands.len;
  Type **operand_types =
      operand_count
          ? mir_arena_alloc(builder->program->arena,
                            sizeof(Type *) * operand_count, MIR_ALIGNOF(Type *))
          : NULL;
  if (operand_count && !operand_types) {
    return NULL;
  }

  for (size_t i = 0; i < operand_count; i++) {
    operand_types[i] =
        mir_function_value_type(builder->fn, call->data.call.operands.items[i]);
    if (!operand_types[i]) {
      return NULL;
    }
  }

  return mir_fn_type_from_args(builder->program->arena, operand_types,
                               operand_count, result_type);
}

static void mir_update_call_result_type(MirBuilder *builder, MirInstr *call,
                                        Type *result_type) {
  if (!builder || !builder->fn || !call || !mir_instr_is_call_like(call) ||
      !result_type) {
    return;
  }

  call->type = result_type;
  mir_function_set_value_type(builder->fn, call->result, result_type);
  call->data.call.callee_type =
      mir_call_type_from_operand_values(builder, call, result_type);
  mir_specialize_call_fn_ref_operands(builder, call);

  if (call->data.call.builtin) {
    return;
  }

  call->data.call.specialized_name = mir_call_specialized_name(builder, call);
  call->data.call.specialized_fn = MIR_NO_FUNCTION;
  MirFunction *specialized = mir_materialize_call_specialization(builder, call);
  if (specialized) {
    call->data.call.specialized_name = specialized->name;
    call->data.call.specialized_fn = specialized->id;
  }
}

static void mir_update_closure_value_type(MirBuilder *builder,
                                          MirInstr *closure, Type *type) {
  if (!builder || !builder->program || !builder->fn || !closure ||
      closure->kind != MIR_CONSTRUCT ||
      closure->data.construct.kind != MIR_CONSTRUCT_CLOSURE || !type ||
      !is_closure(type) || !type->closure_meta) {
    return;
  }

  closure->type = type;
  mir_function_set_value_type(builder->fn, closure->result, type);

  MirInstr *env = mir_function_find_def_instr(
      builder->fn, closure->data.construct.operands[1]);
  if (env && env->kind == MIR_CONSTRUCT &&
      env->data.construct.kind == MIR_CONSTRUCT_CLOSURE_ENV) {
    env->type = type->closure_meta;
    mir_function_set_value_type(builder->fn, env->result, type->closure_meta);
  }

  MirInstr *fn_ref = mir_function_find_def_instr(
      builder->fn, closure->data.construct.operands[0]);
  if (!fn_ref || fn_ref->kind != MIR_FN_REF) {
    return;
  }

  Type *impl_type = NULL;
  MirFunction *impl = mir_specialize_closure_impl_fn_ref(
      builder->program, fn_ref, type, &impl_type);
  if (!impl_type || mir_type_has_type_vars(impl_type)) {
    return;
  }
  fn_ref->type = impl_type;
  mir_function_set_value_type(builder->fn, fn_ref->result, impl_type);

  if (impl) {
    closure->data.construct.impl_fn = impl->id;
    closure->data.construct.impl_name = impl->name;
    mir_function_set_value_callable_summary(builder->fn, fn_ref->result,
                                            &impl->summary);
    mir_function_set_value_callable_summary(builder->fn, closure->result,
                                            &impl->summary);
  }
}

void mir_prepare_call(MirBuilder *builder, MirInstr *call) {
  if (!builder || !call || !mir_instr_is_call_like(call)) {
    return;
  }

  mir_specialize_call_fn_ref_operands(builder, call);
  if (!call->data.call.builtin) {
    call->data.call.specialized_name = mir_call_specialized_name(builder, call);
    call->data.call.specialized_fn = MIR_NO_FUNCTION;
    MirFunction *specialized =
        mir_materialize_call_specialization(builder, call);
    if (specialized) {
      call->data.call.specialized_name = specialized->name;
      call->data.call.specialized_fn = specialized->id;
    }
  }
  mir_call_apply_callee_summary(builder, call);
}

MirValueId mir_call_value(MirBuilder *builder, Type *type, Ast *origin,
                          MirValueId callee, Type *callee_type,
                          const MirValueId *args, size_t argc) {
  if (!builder || !builder->program || !builder->fn ||
      callee == MIR_NO_VALUE || !callee_type || callee_type->kind != T_FN ||
      (argc > 0 && !args)) {
    return MIR_NO_VALUE;
  }

  MirInstr call = mir_make_instr(MIR_CALL, type, origin);
  MirArena *arena = builder->fn->arena;
  call.data.call.callee = callee;
  call.data.call.builtin = NULL;
  call.data.call.callee_type = callee_type;
  call.data.call.specialized_fn = MIR_NO_FUNCTION;

  if (is_closure(callee_type) && callee_type->closure_meta) {
    MirInstr *callee_def = mir_function_find_def_instr(builder->fn, callee);
    if (callee_def && callee_def->kind == MIR_CALL &&
        callee_def->type != callee_type) {
      mir_update_call_result_type(builder, callee_def, callee_type);
    } else if (callee_def && callee_def->kind == MIR_CONSTRUCT &&
               callee_def->data.construct.kind == MIR_CONSTRUCT_CLOSURE &&
               callee_def->type != callee_type) {
      mir_update_closure_value_type(builder, callee_def, callee_type);
    }

    MirValueId env =
        mir_closure_get_env(builder, callee_type->closure_meta, origin, callee);
    Type *callable_type = mir_closure_callable_type(arena, callee_type);
    MirValueId fn = mir_closure_fn(builder, callable_type, origin, callee);
    if (env == MIR_NO_VALUE || fn == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }

    call.data.call.callee = fn;
    call.data.call.callee_type = callable_type;
    mir_call_push_operand(arena, &call, env, MIR_OPERAND_USE_BORROW);
  }

  for (size_t i = 0; i < argc; i++) {
    if (args[i] == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_call_push_operand(arena, &call, args[i], MIR_OPERAND_USE_CONSUME);
  }

  mir_prepare_call(builder, &call);
  return mir_builder_append_instr(builder, call);
}

static MirFunction *mir_build_const_curried_wrapper(MirBuilder *builder,
                                                    Ast *app, MirCtx *ctx,
                                                    MirFunction *callee) {
  if (!builder || !builder->program || !app || app->tag != AST_APPLICATION ||
      !callee || !app->type) {
    return NULL;
  }

  MirArena *arena = builder->program->arena;
  const char *name = mir_curried_wrapper_name(arena, callee, false);
  MirFunction *wrapper =
      mir_program_add_function(builder->program, name, app->type, app);
  MirBlock *entry = wrapper ? mir_function_add_block(wrapper, "entry") : NULL;
  if (!wrapper || !entry) {
    return NULL;
  }

  MirBuilder wrapper_builder;
  mir_builder_init(&wrapper_builder, builder->program, wrapper);
  mir_builder_position_at_end(&wrapper_builder, entry);

  MirCtx wrapper_ctx = {
      .env = ctx ? ctx->env : builder->program->type_env,
      .frame = NULL,
      .current_module =
          ctx ? ctx->current_module : builder->program->root_module,
      .export_bindings = false,
  };
  ht table;
  MirStackFrame frame;
  mir_stack_frame_init(arena, &table, &frame, NULL);
  wrapper_ctx.frame = &frame;

  size_t captured_count = app->data.AST_APPLICATION.len;
  size_t remaining_count = mir_fn_param_count(app->type);
  size_t operand_count = captured_count + remaining_count;
  Type **operand_types =
      operand_count ? mir_arena_alloc(arena, sizeof(Type *) * operand_count,
                                      MIR_ALIGNOF(Type *))
                    : NULL;
  if (operand_count && !operand_types) {
    return NULL;
  }

  MirValueIdVec operands = {0};
  for (size_t i = 0; i < captured_count; i++) {
    Ast *arg = app->data.AST_APPLICATION.args + i;
    MirValueId value = mir_expr(&wrapper_builder, arg, &wrapper_ctx);
    if (value == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&wrapper_builder);
      return wrapper;
    }
    operand_types[i] = arg->type;
    mir_value_id_vec_push(arena, &operands, value);
  }

  Type *cursor = app->type;
  for (size_t i = 0; i < remaining_count && cursor && cursor->kind == T_FN;
       i++, cursor = cursor->data.T_FN.to) {
    Type *param_type = cursor->data.T_FN.from;
    MirValueId param = mir_function_add_param(
        wrapper, mir_arena_printf(arena, "arg%zu", i), param_type, app);
    operand_types[captured_count + i] = param_type;
    mir_value_id_vec_push(arena, &operands, param);
  }

  Type *result_type = cursor ? cursor : fn_return_type(app->type);
  MirValueId result = mir_append_curried_inner_call(
      &wrapper_builder, app, callee, app->data.AST_APPLICATION.function->type,
      operands, operand_types, operand_count, result_type);
  if (result != MIR_NO_VALUE) {
    mir_builder_set_return(&wrapper_builder, result);
  } else {
    mir_builder_set_unreachable_if_open(&wrapper_builder);
  }

  return wrapper;
}

static MirFunction *mir_build_env_curried_wrapper(MirBuilder *builder, Ast *app,
                                                  MirCtx *ctx,
                                                  MirFunction *callee,
                                                  Type *impl_type) {
  if (!builder || !builder->program || !app || app->tag != AST_APPLICATION ||
      !callee || !app->type || !app->type->closure_meta || !impl_type) {
    return NULL;
  }

  MirArena *arena = builder->program->arena;
  const char *name = mir_curried_wrapper_name(arena, callee, true);
  MirFunction *wrapper =
      mir_program_add_function(builder->program, name, impl_type, app);
  MirBlock *entry = wrapper ? mir_function_add_block(wrapper, "entry") : NULL;
  if (!wrapper || !entry) {
    return NULL;
  }

  MirBuilder wrapper_builder;
  mir_builder_init(&wrapper_builder, builder->program, wrapper);
  mir_builder_position_at_end(&wrapper_builder, entry);

  MirCtx wrapper_ctx = {
      .env = ctx ? ctx->env : builder->program->type_env,
      .frame = NULL,
      .current_module =
          ctx ? ctx->current_module : builder->program->root_module,
      .export_bindings = false,
  };
  ht table;
  MirStackFrame frame;
  mir_stack_frame_init(arena, &table, &frame, NULL);
  wrapper_ctx.frame = &frame;

  Type *env_type = app->type->closure_meta;
  MirValueId env_param = mir_function_add_param(wrapper, "$env", env_type, app);
  if (env_param == MIR_NO_VALUE) {
    return wrapper;
  }

  Type *remaining_type = impl_type && impl_type->kind == T_FN
                             ? impl_type->data.T_FN.to
                             : app->type;
  size_t captured_count = app->data.AST_APPLICATION.len;
  size_t remaining_count = mir_fn_param_count(remaining_type);
  size_t operand_count = captured_count + remaining_count;
  Type **operand_types =
      operand_count ? mir_arena_alloc(arena, sizeof(Type *) * operand_count,
                                      MIR_ALIGNOF(Type *))
                    : NULL;
  if (operand_count && !operand_types) {
    return NULL;
  }

  MirValueIdVec operands = {0};
  for (size_t i = 0; i < captured_count; i++) {
    Type *field_type = NULL;
    if (env_type->kind == T_CONS && env_type->data.T_CONS.args &&
        i < (size_t)env_type->data.T_CONS.num_args) {
      field_type = env_type->data.T_CONS.args[i];
    }
    if (!field_type) {
      field_type = app->data.AST_APPLICATION.args[i].type;
    }

    MirValueId captured = mir_extract_field(&wrapper_builder, field_type,
                                            app->data.AST_APPLICATION.args + i,
                                            env_param, i, NULL);
    if (captured == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&wrapper_builder);
      return wrapper;
    }
    operand_types[i] = field_type;
    mir_value_id_vec_push(arena, &operands, captured);
  }

  Type *cursor = remaining_type;
  for (size_t i = 0; i < remaining_count && cursor && cursor->kind == T_FN;
       i++, cursor = cursor->data.T_FN.to) {
    Type *param_type = cursor->data.T_FN.from;
    MirValueId param = mir_function_add_param(
        wrapper, mir_arena_printf(arena, "arg%zu", i), param_type, app);
    operand_types[captured_count + i] = param_type;
    mir_value_id_vec_push(arena, &operands, param);
  }

  Type *result_type = cursor ? cursor : fn_return_type(remaining_type);
  MirValueId result = mir_append_curried_inner_call(
      &wrapper_builder, app, callee, app->data.AST_APPLICATION.function->type,
      operands, operand_types, operand_count, result_type);
  if (result != MIR_NO_VALUE) {
    mir_builder_set_return(&wrapper_builder, result);
  } else {
    mir_builder_set_unreachable_if_open(&wrapper_builder);
  }

  return wrapper;
}

static MirValueId mir_partial_application(MirBuilder *builder, Type *type,
                                          Ast *app, MirCtx *ctx) {
  if (!builder || !builder->program || !app || app->tag != AST_APPLICATION) {
    return MIR_NO_VALUE;
  }

  MirFunction *callee = mir_direct_callee_function(
      builder, app->data.AST_APPLICATION.function, ctx);
  if (!callee) {
    fprintf(stderr, "MIR lowering only supports partial application of direct "
                    "functions for now\n");
    print_ast_err(app);
    return MIR_NO_VALUE;
  }

  MirArena *arena = builder->program->arena;
  if (app->data.AST_APPLICATION.is_curried_with_constants && type &&
      type->kind == T_FN && !is_closure(type)) {
    MirFunction *wrapper =
        mir_build_const_curried_wrapper(builder, app, ctx, callee);
    return wrapper ? mir_fn_ref(builder, type, app, wrapper) : MIR_NO_VALUE;
  }

  if (!type || !is_closure(type) || !type->closure_meta) {
    fprintf(stderr,
            "MIR lowering expected partial application to produce a closure\n");
    return MIR_NO_VALUE;
  }

  Type *impl_type = mir_closure_callable_type(arena, type);
  MirFunction *wrapper =
      mir_build_env_curried_wrapper(builder, app, ctx, callee, impl_type);
  if (!wrapper) {
    return MIR_NO_VALUE;
  }

  MirValueIdVec fields = {0};
  for (size_t i = 0; i < app->data.AST_APPLICATION.len; i++) {
    MirValueId value =
        mir_expr(builder, app->data.AST_APPLICATION.args + i, ctx);
    if (value == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_value_id_vec_push(arena, &fields, value);
  }

  MirValueId fn = mir_fn_ref(builder, impl_type, app, wrapper);
  MirValueId env = mir_closure_env(builder, type->closure_meta, app, fields);
  return mir_closure(builder, type, app, fn, env, wrapper);
}

MirValueId mir_application(MirBuilder *builder, Type *type, Ast *ast,
                           MirCtx *ctx) {
  Ast *flat = mir_application_flatten_if_saturated(
      builder && builder->program ? builder->program->arena : NULL, ast);
  if (flat != ast) {
    ast = flat;
    type = ast->type;
  }

  Ast *function = ast->data.AST_APPLICATION.function;
  Type *function_type = function ? function->type : NULL;
  MirBuiltinSymbol *builtin = NULL;
  if (function && function->tag == AST_IDENTIFIER) {
    builtin = mir_program_lookup_builtin(builder->program,
                                         function->data.AST_IDENTIFIER.value);
    if (builtin && builtin->handler) {
      MirValueId value = builtin->handler(builder, ast, ctx, builtin);
      if (value != MIR_NO_VALUE) {
        return value;
      }
    }

    MirValueId constructor = mir_constructor_call(
        builder, ast, ast->type, function->data.AST_IDENTIFIER.value,
        ast->data.AST_APPLICATION.args, ast->data.AST_APPLICATION.len, ctx);
    if (constructor != MIR_NO_VALUE) {
      return constructor;
    }

    constructor = mir_record_constructor_call(
        builder, ast, ast->type, function->data.AST_IDENTIFIER.value,
        ast->data.AST_APPLICATION.args, ast->data.AST_APPLICATION.len, ctx);
    if (constructor != MIR_NO_VALUE) {
      return constructor;
    }
  }

  if (mir_application_is_partial(ast)) {
    return mir_partial_application(builder, type, ast, ctx);
  }

  MirInstr call = mir_make_instr(MIR_CALL, type, ast);
  MirArena *arena = builder->fn->arena;

  call.data.call.callee = MIR_NO_VALUE;
  call.data.call.builtin = builtin;
  call.data.call.specialized_fn = MIR_NO_FUNCTION;
  call.data.call.callee_type =
      mir_application_expected_callee_type(arena, ast, function_type);
  if (!builtin) {
    MirValueId callee_value =
        mir_expr(builder, ast->data.AST_APPLICATION.function, ctx);
    if (callee_value == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    if (is_coroutine_constructor_type(function_type)) {
      call.kind = MIR_CORO_NEW;
      if (is_closure(function_type) && function_type->closure_meta) {
        MirInstr *callee_def =
            mir_function_find_def_instr(builder->fn, callee_value);
        if (callee_def && callee_def->kind == MIR_CALL &&
            callee_def->type != function_type) {
          mir_update_call_result_type(builder, callee_def, function_type);
        } else if (callee_def && callee_def->kind == MIR_CONSTRUCT &&
                   callee_def->data.construct.kind == MIR_CONSTRUCT_CLOSURE &&
                   callee_def->type != function_type) {
          mir_update_closure_value_type(builder, callee_def, function_type);
        }

        MirValueId env = mir_closure_get_env(
            builder, function_type->closure_meta, function, callee_value);
        Type *callable_type = mir_closure_callable_type(arena, function_type);
        MirValueId fn =
            mir_closure_fn(builder, callable_type, function, callee_value);
        if (env == MIR_NO_VALUE || fn == MIR_NO_VALUE) {
          return MIR_NO_VALUE;
        }

        call.data.call.callee = fn;
        call.data.call.callee_type = callable_type;
        mir_call_push_operand(arena, &call, env, MIR_OPERAND_USE_BORROW);

        if (!mir_call_push_application_args(builder, &call, ast, ctx, arena,
                                            function_type)) {
          return MIR_NO_VALUE;
        }

        mir_specialize_call_fn_ref_operands(builder, &call);
        mir_call_apply_callee_summary(builder, &call);
        return mir_builder_append_instr(builder, call);
      }

      call.data.call.callee = callee_value;
      if (!mir_call_push_application_args(builder, &call, ast, ctx, arena,
                                          function_type)) {
        return MIR_NO_VALUE;
      }
      mir_specialize_call_fn_ref_operands(builder, &call);
      call.data.call.specialized_name =
          mir_call_specialized_name(builder, &call);
      MirFunction *specialized =
          mir_materialize_call_specialization(builder, &call);
      if (specialized) {
        call.data.call.specialized_name = specialized->name;
        call.data.call.specialized_fn = specialized->id;
      }
      mir_call_apply_callee_summary(builder, &call);
      return mir_builder_append_instr(builder, call);
    }
    if (is_coroutine_type(function_type)) {
      if (!mir_application_has_only_void_arg(ast)) {
        fprintf(stderr,
                "MIR lowering only supports nullary coroutine resume calls\n");
        return MIR_NO_VALUE;
      }
      call.kind = MIR_CORO_NEXT;
      call.data.call.callee = callee_value;
      call.data.call.callee_type = function_type;
      return mir_builder_append_instr(builder, call);
    }
    if (is_closure(function_type) && function_type->closure_meta) {
      MirInstr *callee_def =
          mir_function_find_def_instr(builder->fn, callee_value);
      if (callee_def && callee_def->kind == MIR_CALL &&
          callee_def->type != function_type) {
        mir_update_call_result_type(builder, callee_def, function_type);
      } else if (callee_def && callee_def->kind == MIR_CONSTRUCT &&
                 callee_def->data.construct.kind == MIR_CONSTRUCT_CLOSURE &&
                 callee_def->type != function_type) {
        mir_update_closure_value_type(builder, callee_def, function_type);
      }

      MirValueId env = mir_closure_get_env(builder, function_type->closure_meta,
                                           function, callee_value);
      Type *callable_type = mir_closure_callable_type(arena, function_type);
      MirValueId fn =
          mir_closure_fn(builder, callable_type, function, callee_value);
      if (env == MIR_NO_VALUE || fn == MIR_NO_VALUE) {
        return MIR_NO_VALUE;
      }

      call.data.call.callee = fn;
      call.data.call.callee_type = callable_type;
      mir_call_push_operand(arena, &call, env, MIR_OPERAND_USE_BORROW);

      if (!mir_call_push_application_args(builder, &call, ast, ctx, arena,
                                          function_type)) {
        return MIR_NO_VALUE;
      }

      mir_specialize_call_fn_ref_operands(builder, &call);
      mir_call_apply_callee_summary(builder, &call);
      return mir_builder_append_instr(builder, call);
    }

    call.data.call.callee = callee_value;
    if (!mir_call_push_application_args(builder, &call, ast, ctx, arena,
                                        function_type)) {
      return MIR_NO_VALUE;
    }
    mir_specialize_call_fn_ref_operands(builder, &call);
    call.data.call.specialized_name = mir_call_specialized_name(builder, &call);
    MirFunction *specialized =
        mir_materialize_call_specialization(builder, &call);
    if (specialized) {
      call.data.call.specialized_name = specialized->name;
      call.data.call.specialized_fn = specialized->id;
    }
    mir_call_apply_callee_summary(builder, &call);
    return mir_builder_append_instr(builder, call);
  }

  if (!mir_call_push_application_args(builder, &call, ast, ctx, arena,
                                      function_type)) {
    return MIR_NO_VALUE;
  }
  mir_specialize_call_fn_ref_operands(builder, &call);
  mir_call_apply_callee_summary(builder, &call);
  return mir_builder_append_instr(builder, call);
}

static MirValueId mir_body(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  MirValueId last = MIR_NO_VALUE;
  for (AstList *item = ast->data.AST_BODY.stmts; item; item = item->next) {
    if (item->ast && item->ast->tag == AST_TYPE_DECL) {
      continue;
    }
    if (builder && builder->block &&
        builder->block->term.kind != MIR_TERM_NONE) {
      break;
    }
    last = mir_expr(builder, item->ast, ctx);
    if (last == MIR_NO_VALUE) {
      break;
    }
  }
  return last;
}

static Ast *mir_get_test_module_ast(Ast *ast) {
  if (!ast) {
    return NULL;
  }

  if (ast->tag == AST_LET && ast->data.AST_LET.binding &&
      ast->data.AST_LET.binding->tag == AST_IDENTIFIER &&
      strcmp(ast->data.AST_LET.binding->data.AST_IDENTIFIER.value, "test") ==
          0) {
    return ast->data.AST_LET.expr;
  }

  if (ast->tag == AST_BODY) {
    for (AstList *item = ast->data.AST_BODY.stmts; item; item = item->next) {
      Ast *stmt = item->ast;
      if (stmt && stmt->tag == AST_LET && stmt->data.AST_LET.binding &&
          stmt->data.AST_LET.binding->tag == AST_IDENTIFIER &&
          strcmp(stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value,
                 "test") == 0) {
        return stmt->data.AST_LET.expr;
      }
    }
  }

  return NULL;
}

static AstList *mir_test_module_stmts(MirArena *arena, Ast *test_module_ast) {
  if (!test_module_ast || test_module_ast->tag != AST_MODULE ||
      !test_module_ast->data.AST_LAMBDA.body) {
    return NULL;
  }

  Ast *body = test_module_ast->data.AST_LAMBDA.body;
  if (body->tag == AST_BODY) {
    return body->data.AST_BODY.stmts;
  }

  AstList *single =
      mir_arena_alloc(arena, sizeof(AstList), MIR_ALIGNOF(AstList));
  if (!single) {
    return NULL;
  }
  *single = (AstList){.ast = body, .next = NULL};
  return single;
}

static bool mir_is_test_binding(Ast *stmt) {
  return stmt && stmt->tag == AST_LET && stmt->data.AST_LET.binding &&
         stmt->data.AST_LET.binding->tag == AST_IDENTIFIER &&
         strncmp(stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value, "test",
                 4) == 0;
}

static MirValueId mir_bool_and_values(MirBuilder *builder, Ast *origin,
                                      MirValueId lhs, MirValueId rhs) {
  if (!builder || !builder->fn || !builder->block || lhs == MIR_NO_VALUE ||
      rhs == MIR_NO_VALUE || builder->block->term.kind != MIR_TERM_NONE) {
    return MIR_NO_VALUE;
  }

  MirBlock *true_block = mir_function_add_block(builder->fn, "test.and.true");
  MirBlock *false_block = mir_function_add_block(builder->fn, "test.and.false");
  MirBlock *cont_block = mir_function_add_block(builder->fn, "test.and.cont");
  if (!true_block || !false_block || !cont_block) {
    return MIR_NO_VALUE;
  }

  mir_builder_set_cond(builder, lhs, true_block->id, false_block->id);

  MirPhiIncomingVec incoming = {0};

  mir_builder_position_at_end(builder, true_block);
  mir_phi_incoming_vec_push(
      builder->fn->arena, &incoming,
      (MirPhiIncoming){.block = true_block->id, .value = rhs});
  mir_builder_set_br(builder, cont_block->id);

  mir_builder_position_at_end(builder, false_block);
  MirValueId false_value = mir_const_bool(builder, &t_bool, origin, false);
  if (false_value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }
  mir_phi_incoming_vec_push(
      builder->fn->arena, &incoming,
      (MirPhiIncoming){.block = false_block->id, .value = false_value});
  mir_builder_set_br(builder, cont_block->id);

  mir_builder_position_at_end(builder, cont_block);
  return mir_phi(builder, &t_bool, origin, incoming);
}

static const char *mir_identifier_name(Ast *ast) {
  if (!ast || ast->tag != AST_IDENTIFIER) {
    return NULL;
  }
  return ast->data.AST_IDENTIFIER.value;
}

static const char *mir_module_name(MirProgram *program, MirModuleId module_id) {
  MirModule *module = mir_program_get_module(program, module_id);
  return module ? module->name : NULL;
}

static const char *mir_qualified_symbol_name(MirProgram *program, MirCtx *ctx,
                                             const char *name) {
  if (!program || !program->arena || !name || !ctx ||
      ctx->current_module == MIR_NO_MODULE ||
      ctx->current_module == program->root_module) {
    return mir_arena_strdup(program ? program->arena : NULL, name);
  }

  const char *module_name = mir_module_name(program, ctx->current_module);
  if (!module_name || module_name[0] == '\0') {
    return mir_arena_strdup(program->arena, name);
  }
  return mir_arena_printf(program->arena, "%s.%s", module_name, name);
}

static const char *mir_scoped_function_name(MirBuilder *builder, MirCtx *ctx,
                                            const char *name) {
  if (!builder || !builder->program || !builder->program->arena || !name) {
    return NULL;
  }

  const char *base = NULL;
  if (ctx && ctx->export_bindings) {
    base = mir_qualified_symbol_name(builder->program, ctx, name);
  } else if (builder->fn && builder->fn->name) {
    base = mir_arena_printf(builder->program->arena, "%s.%s", builder->fn->name,
                            name);
  } else {
    const char *module_name =
        ctx ? mir_module_name(builder->program, ctx->current_module) : NULL;
    base = module_name && module_name[0] != '\0'
               ? mir_arena_printf(builder->program->arena, "%s.%s", module_name,
                                  name)
               : mir_arena_strdup(builder->program->arena, name);
  }

  return mir_unique_function_name(builder->program, base);
}

static const char *mir_global_storage_name(MirBuilder *builder, MirCtx *ctx,
                                           const char *name) {
  if (!builder || !builder->program || !builder->program->arena || !name) {
    return NULL;
  }

  const char *qualified = mir_qualified_symbol_name(builder->program, ctx, name);
  if (!qualified) {
    return NULL;
  }
  return mir_arena_printf(builder->program->arena, "$global.%s", qualified);
}

static MirValueId mir_global_load(MirBuilder *builder, Type *type, Ast *origin,
                                  const char *global_name) {
  if (!builder || !global_name) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, type, origin);
  instr.data.op.kind = MIR_OP_KIND_GLOBAL_LOAD;
  instr.data.op.argc = 0;
  instr.data.op.global_name = global_name;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_global_store(MirBuilder *builder, Type *type,
                                   Ast *origin, const char *global_name,
                                   MirValueId value) {
  if (!builder || !global_name || value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr instr = mir_make_instr(MIR_OP, &t_void, origin);
  instr.data.op.kind = MIR_OP_KIND_GLOBAL_STORE;
  instr.data.op.argc = 1;
  instr.data.op.operands[0] = value;
  instr.data.op.to_type = type;
  instr.data.op.global_name = global_name;
  return mir_builder_append_instr(builder, instr);
}

static const char *mir_scoped_lambda_name(MirBuilder *builder, MirCtx *ctx,
                                          Ast *lambda) {
  if (!builder || !builder->program || !lambda || lambda->tag != AST_LAMBDA) {
    return NULL;
  }

  const char *name =
      mir_lambda_name(builder->program->arena, lambda, "<anonymous>");
  if (!name) {
    return NULL;
  }
  if (strcmp(name, "<anonymous>") != 0) {
    return mir_unique_function_name(builder->program, name);
  }

  return mir_scoped_function_name(builder, ctx, name);
}

static bool mir_ctx_should_export(MirCtx *ctx, Ast *binding) {
  return ctx && ctx->export_bindings && binding &&
         binding->tag == AST_IDENTIFIER && !ast_is_placeholder_id(binding);
}

static bool mir_bind_export_symbol(MirBuilder *builder, MirCtx *ctx,
                                   const char *name, MirSymbol *symbol) {
  if (!builder || !builder->program || !ctx || !name || !symbol) {
    return false;
  }
  symbol->owner_module = ctx->current_module;
  bool ok = true;
  if (ctx->frame) {
    ok = mir_ctx_bind_symbol(ctx, name, symbol);
  }
  if (ctx->current_module != MIR_NO_MODULE) {
    ok = mir_module_bind_symbol(builder->program, ctx->current_module, name,
                                symbol) &&
         ok;
  }
  return ok;
}

static bool mir_bind_function_symbol(MirBuilder *builder, MirCtx *ctx,
                                     const char *name, MirFunction *fn,
                                     Ast *origin, bool export_symbol) {
  if (!builder || !builder->program || !ctx || !name || !fn) {
    return false;
  }

  MirSymbol *symbol = mir_symbol_new(builder->program->arena,
                                     fn->is_extern ? MIR_SYMBOL_EXTERN_FUNCTION
                                                   : MIR_SYMBOL_FUNCTION,
                                     fn->type, origin, ctx->current_module);
  if (!symbol) {
    return false;
  }
  symbol->as.function = fn->id;

  if (export_symbol) {
    return mir_bind_export_symbol(builder, ctx, name, symbol);
  }

  return mir_ctx_bind_symbol(ctx, name, symbol);
}

static MirFunction *mir_value_fn_ref_target(MirProgram *program,
                                            MirFunction *fn, MirValueId value) {
  MirInstr *instr = mir_function_find_def_instr(fn, value);
  if (!instr || instr->kind != MIR_FN_REF ||
      instr->data.fn_ref.fn == MIR_NO_FUNCTION) {
    return NULL;
  }
  return mir_program_get_function(program, instr->data.fn_ref.fn);
}

static bool mir_export_fn_ref_binding(MirBuilder *builder, MirCtx *ctx,
                                      Ast *binding, MirValueId value,
                                      bool is_extern) {
  if (!mir_ctx_should_export(ctx, binding)) {
    return true;
  }

  const char *name = mir_identifier_name(binding);
  MirFunction *target =
      mir_value_fn_ref_target(builder->program, builder->fn, value);
  if (!target) {
    return false;
  }

  MirSymbol *symbol = mir_symbol_new(
      builder->program->arena,
      is_extern ? MIR_SYMBOL_EXTERN_FUNCTION : MIR_SYMBOL_FUNCTION,
      target->type, binding, ctx->current_module);
  if (!symbol) {
    return false;
  }
  symbol->as.function = target->id;
  return mir_bind_export_symbol(builder, ctx, name, symbol);
}

static bool mir_export_expr_binding(MirBuilder *builder, MirCtx *ctx,
                                    Ast *binding, Ast *expr,
                                    MirValueId value) {
  if (!mir_ctx_should_export(ctx, binding) || !expr) {
    return true;
  }

  Type *type = expr->type ? expr->type : binding->type;
  if (!type && builder && builder->fn) {
    type = mir_function_value_type(builder->fn, value);
  }
  if (!type || type->kind == T_VOID) {
    return true;
  }

  const char *name = mir_identifier_name(binding);
  const char *global_name = mir_global_storage_name(builder, ctx, name);
  if (!global_name ||
      mir_global_store(builder, type, expr, global_name, value) ==
          MIR_NO_VALUE) {
    return false;
  }

  MirSymbol *symbol = mir_symbol_new(builder->program->arena, MIR_SYMBOL_GLOBAL,
                                     type, expr, ctx->current_module);
  if (!symbol) {
    return false;
  }
  symbol->as.global_name = global_name;
  return mir_bind_export_symbol(builder, ctx, name, symbol);
}

static MirValueId mir_lambda_value(MirBuilder *builder, Ast *expr, MirCtx *ctx,
                                   const char *fn_name,
                                   const char *binding_name,
                                   bool export_binding) {
  if (!builder || !builder->program || !expr || expr->tag != AST_LAMBDA) {
    return MIR_NO_VALUE;
  }

  if (!fn_name) {
    fn_name = mir_lambda_name(builder->program->arena, expr, "<anonymous>");
  }

  MirFunction *fn =
      mir_program_add_function(builder->program, fn_name, expr->type, expr);
  if (!fn) {
    return MIR_NO_VALUE;
  }
  if (binding_name && ctx) {
    bool should_export = export_binding && ctx->export_bindings;
    if (!mir_bind_function_symbol(builder, ctx, binding_name, fn, expr,
                                  should_export)) {
      return MIR_NO_VALUE;
    }
  }
  mir_populate_function_body(builder->program, fn, expr, ctx, binding_name);

  Type *fn_ref_type =
      is_closure(expr->type)
          ? mir_closure_callable_type(builder->program->arena, expr->type)
          : expr->type;
  MirValueId fn_ref = mir_fn_ref(builder, fn_ref_type, expr, fn);
  if (!is_closure(expr->type) || !expr->type->closure_meta) {
    return fn_ref;
  }

  MirArena *arena = builder->program->arena;
  MirValueIdVec fields = {0};
  for (AstList *closed = expr->data.AST_LAMBDA.closed_vals; closed;
       closed = closed->next) {
    MirValueId value = mir_expr(builder, closed->ast, ctx);
    if (value == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_value_id_vec_push(arena, &fields, value);
  }

  MirValueId env =
      mir_closure_env(builder, expr->type->closure_meta, expr, fields);
  if (env == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }
  return mir_closure(builder, expr->type, expr, fn_ref, env, fn);
}

static MirValueId mir_extern_fn_value(MirBuilder *builder, Ast *expr,
                                      const char *fn_name) {
  if (!builder || !builder->program || !expr || expr->tag != AST_EXTERN_FN) {
    return MIR_NO_VALUE;
  }

  if (!fn_name) {
    fn_name = mir_obj_name(builder->program->arena,
                           expr->data.AST_EXTERN_FN.fn_name, "<extern>");
  }

  MirFunction *fn = mir_program_add_extern_function(builder->program, fn_name,
                                                    expr->type, expr);
  return fn ? mir_fn_ref(builder, expr->type, expr, fn) : MIR_NO_VALUE;
}

static bool mir_compile_module_body(MirBuilder *builder, Ast *module_ast,
                                    MirCtx *module_ctx) {
  if (!builder || !module_ast || module_ast->tag != AST_MODULE ||
      !module_ast->data.AST_LAMBDA.body || !module_ctx) {
    return false;
  }

  Ast *body = module_ast->data.AST_LAMBDA.body;
  MirValueId value = body->tag == AST_BODY
                         ? mir_body(builder, body, module_ctx)
                         : mir_expr(builder, body, module_ctx);
  return value != MIR_NO_VALUE;
}

static Type *mir_module_result_type(Type *type) {
  Type *cur = type;
  while (cur && cur->kind == T_FN) {
    cur = cur->data.T_FN.to;
  }
  return cur && cur->kind == T_MODULE ? cur : NULL;
}

static ModuleTypeMeta *mir_module_type_meta(Type *type) {
  Type *module_type = mir_module_result_type(type);
  return module_type ? (ModuleTypeMeta *)module_type->meta : NULL;
}

static bool mir_module_param_is_type(Ast *param, Ast *annotation) {
  return param && param->tag == AST_IDENTIFIER && annotation == NULL;
}

static bool mir_bind_specialized_module_param(MirBuilder *builder,
                                              MirCtx *parent_ctx,
                                              MirCtx *module_ctx,
                                              Ast *param_ast, Ast *arg_ast) {
  if (!builder || !parent_ctx || !module_ctx || !param_ast || !arg_ast) {
    return false;
  }

  MirValueId value = mir_expr(builder, arg_ast, parent_ctx);
  if (value == MIR_NO_VALUE ||
      !mir_bind_pattern(builder, module_ctx, param_ast, value, arg_ast->type)) {
    return false;
  }

  if (param_ast->tag != AST_IDENTIFIER || ast_is_placeholder_id(param_ast)) {
    return true;
  }

  MirFunction *target =
      mir_value_fn_ref_target(builder->program, builder->fn, value);
  if (!target) {
    return true;
  }

  return mir_bind_function_symbol(builder, module_ctx,
                                  param_ast->data.AST_IDENTIFIER.value, target,
                                  arg_ast, true);
}

static MirValueId
mir_specialized_module_binding_value(MirBuilder *builder, Ast *ast,
                                     MirCtx *ctx) {
  if (!builder || !builder->program || !ast || ast->tag != AST_LET || !ctx ||
      !ast->data.AST_LET.binding ||
      ast->data.AST_LET.binding->tag != AST_IDENTIFIER ||
      !ast->data.AST_LET.expr ||
      ast->data.AST_LET.expr->tag != AST_APPLICATION) {
    return MIR_NO_VALUE;
  }

  Ast *app = mir_application_flatten_if_saturated(builder->program->arena,
                                                  ast->data.AST_LET.expr);
  if (!app || app->tag != AST_APPLICATION) {
    return MIR_NO_VALUE;
  }

  MirSymbol *generic =
      mir_resolve_ast_symbol(builder, app->data.AST_APPLICATION.function, ctx);
  if (!generic || generic->kind != MIR_SYMBOL_GENERIC_MODULE ||
      !generic->as.expr || generic->as.expr->tag != AST_MODULE) {
    return MIR_NO_VALUE;
  }

  const char *local_name =
      ast->data.AST_LET.binding->data.AST_IDENTIFIER.value;
  const char *module_name = mir_qualified_symbol_name(builder->program, ctx,
                                                      local_name);
  Type *module_type = ast->data.AST_LET.binding->type
                          ? ast->data.AST_LET.binding->type
                          : mir_module_result_type(app->type);
  if (!module_type || module_type->kind != T_MODULE) {
    module_type = mir_module_result_type(generic->type);
  }
  if (!module_type || module_type->kind != T_MODULE) {
    return MIR_NO_VALUE;
  }

  MirModule *module =
      mir_program_add_module(builder->program, module_name, module_type,
                             generic->as.expr, ctx->current_module);
  if (!module) {
    return MIR_NO_VALUE;
  }
  module->init = builder->fn ? builder->fn->id : MIR_NO_FUNCTION;

  MirSymbol *module_symbol =
      mir_symbol_new(builder->program->arena, MIR_SYMBOL_MODULE, module_type,
                     generic->as.expr, ctx->current_module);
  if (!module_symbol) {
    return MIR_NO_VALUE;
  }
  module_symbol->as.module = module->id;
  if (!mir_bind_export_symbol(builder, ctx, local_name, module_symbol)) {
    return MIR_NO_VALUE;
  }

  ht module_table;
  MirStackFrame module_frame;
  mir_stack_frame_init(builder->program->arena, &module_table, &module_frame,
                       NULL);
  MirCtx module_ctx = {
      .env = ctx->env,
      .frame = &module_frame,
      .current_module = module->id,
      .export_bindings = true,
  };

  ModuleTypeMeta *meta = mir_module_type_meta(generic->type);
  int arg_i = meta ? meta->num_type_params : 0;
  AstList *param = generic->as.expr->data.AST_LAMBDA.params;
  AstList *annotation = generic->as.expr->data.AST_LAMBDA.type_annotations;
  for (; param; param = param->next) {
    Ast *annotation_ast = annotation ? annotation->ast : NULL;
    if (annotation) {
      annotation = annotation->next;
    }
    if (mir_module_param_is_type(param->ast, annotation_ast)) {
      continue;
    }
    if (arg_i >= app->data.AST_APPLICATION.len) {
      return MIR_NO_VALUE;
    }
    if (!mir_bind_specialized_module_param(
            builder, ctx, &module_ctx, param->ast,
            app->data.AST_APPLICATION.args + arg_i)) {
      return MIR_NO_VALUE;
    }
    arg_i++;
  }

  if (!mir_compile_module_body(builder, generic->as.expr, &module_ctx)) {
    return MIR_NO_VALUE;
  }

  return mir_const_void(builder, &t_void, ast);
}

static bool mir_is_specialized_module_binding_expr(MirBuilder *builder,
                                                   Ast *expr, MirCtx *ctx) {
  if (!builder || !builder->program || !expr ||
      expr->tag != AST_APPLICATION) {
    return false;
  }

  Ast *app = mir_application_flatten_if_saturated(builder->program->arena,
                                                  expr);
  if (!app || app->tag != AST_APPLICATION) {
    return false;
  }

  MirSymbol *generic =
      mir_resolve_ast_symbol(builder, app->data.AST_APPLICATION.function, ctx);
  return generic && generic->kind == MIR_SYMBOL_GENERIC_MODULE;
}

static MirValueId mir_module_binding_value(MirBuilder *builder, Ast *ast,
                                           MirCtx *ctx) {
  if (!builder || !builder->program || !ast || ast->tag != AST_LET || !ctx ||
      !ast->data.AST_LET.binding ||
      ast->data.AST_LET.binding->tag != AST_IDENTIFIER ||
      !ast->data.AST_LET.expr || ast->data.AST_LET.expr->tag != AST_MODULE) {
    return MIR_NO_VALUE;
  }

  Ast *binding = ast->data.AST_LET.binding;
  Ast *module_ast = ast->data.AST_LET.expr;
  const char *local_name = binding->data.AST_IDENTIFIER.value;

  if (module_ast->data.AST_LAMBDA.len > 0) {
    MirSymbol *symbol =
        mir_symbol_new(builder->program->arena, MIR_SYMBOL_GENERIC_MODULE,
                       module_ast->type, module_ast, ctx->current_module);
    if (!symbol) {
      return MIR_NO_VALUE;
    }
    symbol->as.expr = module_ast;
    if (!mir_bind_export_symbol(builder, ctx, local_name, symbol)) {
      return MIR_NO_VALUE;
    }
    return mir_const_void(builder, &t_void, module_ast);
  }

  const char *module_name =
      mir_qualified_symbol_name(builder->program, ctx, local_name);
  MirModule *module =
      mir_program_add_module(builder->program, module_name, module_ast->type,
                             module_ast, ctx->current_module);
  if (!module) {
    return MIR_NO_VALUE;
  }
  module->init = builder->fn ? builder->fn->id : MIR_NO_FUNCTION;

  MirSymbol *symbol =
      mir_symbol_new(builder->program->arena, MIR_SYMBOL_MODULE,
                     module_ast->type, module_ast, ctx->current_module);
  if (!symbol) {
    return MIR_NO_VALUE;
  }
  symbol->as.module = module->id;
  if (!mir_bind_export_symbol(builder, ctx, local_name, symbol)) {
    return MIR_NO_VALUE;
  }

  ht module_table;
  MirStackFrame module_frame;
  mir_stack_frame_init(builder->program->arena, &module_table, &module_frame,
                       NULL);
  MirCtx module_ctx = {
      .env = ctx->env,
      .frame = &module_frame,
      .current_module = module->id,
      .export_bindings = true,
  };

  if (!mir_compile_module_body(builder, module_ast, &module_ctx)) {
    return MIR_NO_VALUE;
  }

  return mir_const_void(builder, &t_void, module_ast);
}

static MirValueId mir_import_value(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  if (!builder || !builder->program || !ast || ast->tag != AST_IMPORT || !ctx ||
      !ast->data.AST_IMPORT.identifier) {
    return MIR_NO_VALUE;
  }

  const char *import_name = ast->data.AST_IMPORT.identifier;
  const char *import_path = ast->data.AST_IMPORT.fully_qualified_name;
  if (!import_path) {
    MirSymbol *symbol =
        mir_ctx_lookup_symbol(builder->program, ctx, import_name);
    if (symbol && symbol->kind == MIR_SYMBOL_MODULE) {
      return mir_const_void(builder, &t_void, ast);
    }
    return MIR_NO_VALUE;
  }

  YLCModule *imported = get_module(import_path);
  if (!imported) {
    return MIR_NO_VALUE;
  }
  if (!imported->ast || !imported->env) {
    imported = init_import(imported);
  }
  if (!imported || !imported->ast || imported->ast->tag != AST_MODULE) {
    return MIR_NO_VALUE;
  }

  const char *module_name =
      mir_qualified_symbol_name(builder->program, ctx, import_name);
  MirModule *module = mir_program_add_module(
      builder->program, module_name, ast->type ? ast->type : imported->type,
      imported->ast, ctx->current_module);
  if (!module) {
    return MIR_NO_VALUE;
  }
  module->path = mir_arena_strdup(builder->program->arena, import_path);
  module->init = builder->fn ? builder->fn->id : MIR_NO_FUNCTION;

  MirSymbol *symbol =
      mir_symbol_new(builder->program->arena, MIR_SYMBOL_MODULE, module->type,
                     imported->ast, ctx->current_module);
  if (!symbol) {
    return MIR_NO_VALUE;
  }
  symbol->as.module = module->id;

  if (!ast->data.AST_IMPORT.import_all &&
      !mir_bind_export_symbol(builder, ctx, import_name, symbol)) {
    return MIR_NO_VALUE;
  }

  ht module_table;
  MirStackFrame module_frame;
  mir_stack_frame_init(builder->program->arena, &module_table, &module_frame,
                       NULL);
  MirCtx module_ctx = {
      .env = imported->env ? imported->env : ctx->env,
      .frame = &module_frame,
      .current_module = module->id,
      .export_bindings = true,
  };

  if (!mir_compile_module_body(builder, imported->ast, &module_ctx)) {
    return MIR_NO_VALUE;
  }

  if (ast->data.AST_IMPORT.import_all) {
    hti it = ht_iterator(&module->exports);
    for (bool cont = ht_next(&it); cont; cont = ht_next(&it)) {
      if (!mir_bind_export_symbol(builder, ctx, it.key, it.value)) {
        return MIR_NO_VALUE;
      }
    }
  }

  return mir_const_void(builder, &t_void, ast);
}

static MirValueId mir_let_value(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  Ast *expr = ast->data.AST_LET.expr;

  if (expr && expr->tag == AST_MODULE) {
    return mir_module_binding_value(builder, ast, ctx);
  } else if (expr && expr->tag == AST_APPLICATION) {
    MirValueId module_value =
        mir_specialized_module_binding_value(builder, ast, ctx);
    if (module_value != MIR_NO_VALUE) {
      return module_value;
    }
    return mir_expr(builder, expr, ctx);
  } else if (expr && expr->tag == AST_LAMBDA) {
    ObjString name = {0};
    const char *fn_name = NULL;
    const char *binding_name = NULL;
    if (get_let_binding_name(ast, &name) == 0) {
      binding_name =
          mir_arena_strndup(builder->program->arena, name.chars, name.length);
      fn_name = mir_scoped_function_name(builder, ctx, binding_name);
    } else {
      fn_name = mir_scoped_lambda_name(builder, ctx, expr);
    }
    return mir_lambda_value(builder, expr, ctx, fn_name, binding_name, true);
  } else if (expr && expr->tag == AST_EXTERN_FN) {
    ObjString name = {0};
    const char *fn_name = NULL;
    if (get_let_binding_name(ast, &name) == 0) {
      fn_name = mir_obj_name(builder->program->arena, name, "<extern>");
    }
    return mir_extern_fn_value(builder, expr, fn_name);
  } else if (expr) {
    return mir_expr(builder, expr, ctx);
  }

  return MIR_NO_VALUE;
}

static MirValueId mir_let(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  bool is_module_binding =
      expr && (expr->tag == AST_MODULE ||
               mir_is_specialized_module_binding_expr(builder, expr, ctx));

  if (ast->data.AST_LET.in_expr && ctx) {
    MIR_STACK_ALLOC_CTX_PUSH(cont_ctx, builder, ctx)
    cont_ctx.export_bindings = false;
    MirValueId value = mir_let_value(builder, ast, &cont_ctx);
    if (value == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(builder);
      return MIR_NO_VALUE;
    }
    if (is_module_binding) {
      return mir_expr(builder, ast->data.AST_LET.in_expr, &cont_ctx);
    }
    if (!mir_bind_pattern(builder, &cont_ctx, binding, value,
                          expr ? expr->type : binding->type)) {
      mir_builder_set_unreachable_if_open(builder);
      return MIR_NO_VALUE;
    }
    return mir_expr(builder, ast->data.AST_LET.in_expr, &cont_ctx);
  }

  MirValueId value = mir_let_value(builder, ast, ctx);
  if (value == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(builder);
    return MIR_NO_VALUE;
  }
  if (is_module_binding) {
    return value;
  }
  if (expr && expr->tag == AST_EXTERN_FN) {
    if (!mir_export_fn_ref_binding(builder, ctx, binding, value, true)) {
      mir_builder_set_unreachable_if_open(builder);
      return MIR_NO_VALUE;
    }
  } else if (expr && expr->tag != AST_LAMBDA) {
    if (!mir_export_expr_binding(builder, ctx, binding, expr, value)) {
      mir_builder_set_unreachable_if_open(builder);
      return MIR_NO_VALUE;
    }
  }
  if (!ast->data.AST_LET.in_expr) {
    if (!mir_bind_pattern(builder, ctx, binding, value,
                          expr ? expr->type : binding->type)) {
      mir_builder_set_unreachable_if_open(builder);
      return MIR_NO_VALUE;
    }
    return value;
  }

  if (!mir_bind_pattern(builder, ctx, binding, value,
                        expr ? expr->type : binding->type)) {
    mir_builder_set_unreachable_if_open(builder);
    return MIR_NO_VALUE;
  }
  return mir_expr(builder, ast->data.AST_LET.in_expr, ctx);
}

static MirValueId mir_symbol_to_value(MirBuilder *builder, Ast *origin,
                                      MirCtx *ctx, MirSymbol *symbol,
                                      Type *expected_type) {
  if (!builder || !builder->program || !symbol) {
    return MIR_NO_VALUE;
  }

  switch (symbol->kind) {
  case MIR_SYMBOL_VALUE:
    return symbol->as.value;
  case MIR_SYMBOL_FUNCTION:
  case MIR_SYMBOL_EXTERN_FUNCTION: {
    MirFunction *fn =
        mir_program_get_function(builder->program, symbol->as.function);
    if (!fn) {
      return MIR_NO_VALUE;
    }
    Type *ref_type = expected_type ? expected_type : symbol->type;
    return mir_fn_ref(builder, ref_type, origin, fn);
  }
  case MIR_SYMBOL_GLOBAL: {
    Type *type = expected_type ? expected_type : symbol->type;
    return mir_global_load(builder, type, origin, symbol->as.global_name);
  }
  case MIR_SYMBOL_EXPR: {
    if (!symbol->as.expr || symbol->rematerializing) {
      return MIR_NO_VALUE;
    }

    ht expr_table;
    MirStackFrame expr_frame;
    mir_stack_frame_init(builder->program->arena, &expr_table, &expr_frame,
                         NULL);
    MirCtx expr_ctx = {
        .env = ctx ? ctx->env : builder->program->type_env,
        .frame = &expr_frame,
        .current_module = symbol->owner_module,
        .export_bindings = false,
    };
    symbol->rematerializing = true;
    MirValueId value = mir_expr(builder, symbol->as.expr, &expr_ctx);
    symbol->rematerializing = false;
    return value;
  }
  case MIR_SYMBOL_MODULE:
  case MIR_SYMBOL_GENERIC_MODULE:
  case MIR_SYMBOL_TYPE:
    return MIR_NO_VALUE;
  }
  return MIR_NO_VALUE;
}

static MirSymbol *mir_resolve_ast_symbol(MirBuilder *builder, Ast *ast,
                                         MirCtx *ctx) {
  if (!builder || !builder->program || !ast) {
    return NULL;
  }

  if (ast->tag == AST_IDENTIFIER) {
    return mir_ctx_lookup_symbol(builder->program, ctx,
                                 ast->data.AST_IDENTIFIER.value);
  }

  if (ast->tag == AST_RECORD_ACCESS && ast->data.AST_RECORD_ACCESS.member &&
      ast->data.AST_RECORD_ACCESS.member->tag == AST_IDENTIFIER) {
    MirSymbol *record = mir_resolve_ast_symbol(
        builder, ast->data.AST_RECORD_ACCESS.record, ctx);
    if (!record || record->kind != MIR_SYMBOL_MODULE) {
      return NULL;
    }
    return mir_module_lookup_symbol(
        builder->program, record->as.module,
        ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value, false);
  }

  return NULL;
}

static MirValueId mir_identifier(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  MirValueId value = MIR_NO_VALUE;

  if (ast && ast->tag == AST_IDENTIFIER &&
      mir_ctx_lookup_value(ctx, ast->data.AST_IDENTIFIER.value, &value)) {
    return value;
  }

  if (ast && ast->tag == AST_IDENTIFIER) {
    MirSymbol *symbol = mir_ctx_lookup_symbol(
        builder ? builder->program : NULL, ctx, ast->data.AST_IDENTIFIER.value);
    MirValueId symbol_value =
        mir_symbol_to_value(builder, ast, ctx, symbol, ast->type);
    if (symbol_value != MIR_NO_VALUE) {
      return symbol_value;
    }

    MirFunction *fn = mir_program_find_scoped_function(
        builder ? builder->program : NULL, builder ? builder->fn : NULL,
        ast->data.AST_IDENTIFIER.value);
    if (fn) {
      return mir_fn_ref(builder, ast->type, ast, fn);
    }

    fn = mir_program_find_function_by_name(builder ? builder->program : NULL,
                                           ast->data.AST_IDENTIFIER.value);
    if (fn) {
      return mir_fn_ref(builder, ast->type, ast, fn);
    }

    MirBuiltinSymbol *builtin = mir_program_lookup_builtin(
        builder ? builder->program : NULL, ast->data.AST_IDENTIFIER.value);
    MirFunction *builtin_fn = mir_materialize_builtin_function(
        builder ? builder->program : NULL, builtin, ast);
    if (builtin_fn) {
      Type *fn_ref_type = ast->type ? ast->type : builtin_fn->type;
      return mir_fn_ref(builder, fn_ref_type, ast, builtin_fn);
    }

    MirValueId constructor = mir_constructor_call(
        builder, ast, ast->type, ast->data.AST_IDENTIFIER.value, NULL, 0, ctx);
    if (constructor != MIR_NO_VALUE) {
      return constructor;
    }

    mir_builder_error_at(builder, ast, "unresolved identifier `%s`",
                         ast->data.AST_IDENTIFIER.value);
  }

  return MIR_NO_VALUE;
}

static MirValueId mir_record_access(MirBuilder *builder, Ast *ast,
                                    MirCtx *ctx) {
  if (!builder || !builder->fn || !ast || ast->tag != AST_RECORD_ACCESS ||
      !ast->data.AST_RECORD_ACCESS.record ||
      !ast->data.AST_RECORD_ACCESS.member ||
      ast->data.AST_RECORD_ACCESS.member->tag != AST_IDENTIFIER) {
    return MIR_NO_VALUE;
  }

  MirSymbol *member = mir_resolve_ast_symbol(builder, ast, ctx);
  MirValueId member_value =
      mir_symbol_to_value(builder, ast, ctx, member, ast->type);
  if (member_value != MIR_NO_VALUE) {
    return member_value;
  }

  Ast *record = ast->data.AST_RECORD_ACCESS.record;
  MirValueId record_value = mir_expr(builder, record, ctx);
  if (record_value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *record_type = record->type;
  Type *value_type = mir_function_value_type(builder->fn, record_value);
  Type *record_view = mir_record_access_type_view(record_type);
  Type *value_view = mir_record_access_type_view(value_type);
  if (value_view && value_view->kind == T_CONS &&
      (!record_view || record_view->kind != T_CONS ||
       is_generic(record_view))) {
    record_view = value_view;
  }
  const char *member_name =
      ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;
  if (!record_view || record_view->kind != T_CONS) {
    if (ast->type && member_name && (!record_view || is_generic(record_view))) {
      size_t unresolved_index = ast->data.AST_RECORD_ACCESS.index >= 0
                                    ? (size_t)ast->data.AST_RECORD_ACCESS.index
                                    : 0;
      return mir_extract_field(builder, ast->type, ast, record_value,
                               unresolved_index, member_name);
    }
    return MIR_NO_VALUE;
  }

  bool has_named_fields = record_view->data.T_CONS.names != NULL;
  int member_index = get_struct_member_idx(member_name, record_view);
  if (member_index < 0 && !has_named_fields && !is_generic(record_view)) {
    member_index = ast->data.AST_RECORD_ACCESS.index;
  }
  if (member_index < 0 || member_index >= record_view->data.T_CONS.num_args) {
    return MIR_NO_VALUE;
  }

  Type *member_type = ast->type;
  if ((!member_type || is_generic(member_type)) &&
      record_view->data.T_CONS.args) {
    member_type = record_view->data.T_CONS.args[member_index];
  }
  return mir_extract_field(builder, member_type, ast, record_value,
                           (size_t)member_index, member_name);
}

static MirValueId mir_tuple_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  if (!builder || !builder->fn || !ast || ast->tag != AST_TUPLE) {
    return MIR_NO_VALUE;
  }

  MirArena *arena = builder->fn->arena;
  MirValueIdVec items = {0};
  for (size_t i = 0; i < ast->data.AST_LIST.len; i++) {
    Ast *item = ast->data.AST_LIST.items + i;
    if (item->tag == AST_SPREAD_OP) {
      return MIR_NO_VALUE;
    }
    MirValueId item_value = mir_expr(builder, item, ctx);
    if (item_value == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_value_id_vec_push(arena, &items, item_value);
  }

  return mir_tuple(builder, ast->type, ast, items);
}

static MirValueId mir_list_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  if (!builder || !builder->fn || !ast || !is_list_type(ast->type)) {
    return MIR_NO_VALUE;
  }

  if (ast->tag == AST_EMPTY_CONTAINER) {
    return mir_list_empty(builder, ast->type, ast);
  }
  if (ast->tag != AST_LIST) {
    return MIR_NO_VALUE;
  }

  MirArena *arena = builder->fn->arena;
  MirValueIdVec items = {0};
  for (size_t i = 0; i < ast->data.AST_LIST.len; i++) {
    Ast *item = ast->data.AST_LIST.items + i;
    MirValueId item_value = mir_expr(builder, item, ctx);
    if (item_value == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_value_id_vec_push(arena, &items, item_value);
  }

  MirValueId list = mir_list_empty(builder, ast->type, ast);
  for (size_t i = items.len; i > 0; i--) {
    list = mir_list_cons(builder, ast->type, ast, items.items[i - 1], list);
    if (list == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
  }
  return list;
}

static MirValueId mir_array_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  if (!builder || !builder->fn || !ast || ast->tag != AST_ARRAY ||
      !is_array_type(ast->type)) {
    return MIR_NO_VALUE;
  }

  MirArena *arena = builder->fn->arena;
  MirValueIdVec items = {0};
  for (size_t i = 0; i < ast->data.AST_LIST.len; i++) {
    Ast *item = ast->data.AST_LIST.items + i;
    MirValueId item_value = mir_expr(builder, item, ctx);
    if (item_value == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    mir_value_id_vec_push(arena, &items, item_value);
  }

  return mir_array_literal(builder, ast->type, ast, items);
}

static Ast *mir_sum_constructor_head(Ast *pattern) {
  if (!pattern) {
    return NULL;
  }

  if (pattern->tag == AST_MATCH_GUARD_CLAUSE) {
    pattern = pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;
  }

  while (pattern && pattern->tag == AST_APPLICATION) {
    Ast *fn = pattern->data.AST_APPLICATION.function;
    if (!fn) {
      return NULL;
    }
    if (fn->tag == AST_IDENTIFIER) {
      return fn;
    }
    if (fn->tag == AST_RECORD_ACCESS) {
      Ast *id = fn;
      while (id->tag == AST_RECORD_ACCESS) {
        id = id->data.AST_RECORD_ACCESS.member;
      }
      return id;
    }
    pattern = fn;
  }

  if (pattern->tag == AST_IDENTIFIER) {
    return pattern;
  }
  if (pattern->tag == AST_RECORD_ACCESS) {
    Ast *id = pattern;
    while (id->tag == AST_RECORD_ACCESS) {
      id = id->data.AST_RECORD_ACCESS.member;
    }
    return id;
  }
  return NULL;
}

static Type *mir_resolve_sum_constructor(Type *sum_type, Ast *pattern,
                                         int *idx) {
  if (idx) {
    *idx = -1;
  }
  if (!sum_type || sum_type->kind != T_SUM || !pattern) {
    return NULL;
  }

  if (pattern->tag == AST_MATCH_GUARD_CLAUSE) {
    pattern = pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr;
  }

  if (pattern->tag == AST_LIST && pattern->data.AST_LIST.len == 0) {
    for (int i = 0; i < sum_type->data.T_CONS.num_args; i++) {
      Type *mem = sum_type->data.T_CONS.args[i];
      if (mem && mem->kind == T_CONS &&
          CHARS_EQ(mem->data.T_CONS.name, TYPE_NAME_EMPTY_LIST)) {
        if (idx) {
          *idx = i;
        }
        return mem;
      }
    }
    return NULL;
  }

  Ast *head = mir_sum_constructor_head(pattern);
  return head ? extract_member_from_sum_type_idx(sum_type, head, idx) : NULL;
}

static const char *mir_constructor_name(Type *constructor_type) {
  if (!constructor_type ||
      !(constructor_type->kind == T_CONS || constructor_type->kind == T_SUM)) {
    return "<constructor>";
  }
  return constructor_type->data.T_CONS.name ? constructor_type->data.T_CONS.name
                                            : "<constructor>";
}

static MirValueId mir_eq_for_type(MirBuilder *builder, Ast *origin, Type *type,
                                  MirValueId lhs, MirValueId rhs) {
  if (!type) {
    return MIR_NO_VALUE;
  }

  switch (type->kind) {
  case T_INT:
    return mir_ieq(builder, origin, lhs, rhs);
  case T_UINT64:
    return mir_ueq(builder, origin, lhs, rhs);
  case T_NUM:
    return mir_feq(builder, origin, lhs, rhs);
  case T_CHAR:
    return mir_ceq(builder, origin, lhs, rhs);
  case T_BOOL:
    return mir_beq(builder, origin, lhs, rhs);
  default:
    return MIR_NO_VALUE;
  }
}

static bool mir_lower_pattern_to_cfg(MirBuilder *builder, MirCtx *ctx,
                                     Ast *pattern, MirValueId value,
                                     Type *value_type, MirBlockId success_block,
                                     MirBlockId fail_block);

static bool mir_lower_tuple_pattern_to_cfg(MirBuilder *builder, MirCtx *ctx,
                                           Ast *pattern, MirValueId value,
                                           Type *value_type, size_t index,
                                           MirBlockId success_block,
                                           MirBlockId fail_block) {
  if (!builder || !pattern || pattern->tag != AST_TUPLE) {
    return false;
  }
  if (index >= pattern->data.AST_LIST.len) {
    mir_builder_set_br(builder, success_block);
    return true;
  }

  Ast *item = pattern->data.AST_LIST.items + index;
  Type *item_type = mir_tuple_field_type(value_type, index);
  if (!item_type) {
    item_type = item->type;
  }
  MirValueId item_value = mir_tuple_get(builder, item_type, item, value, index);
  if (item_value == MIR_NO_VALUE) {
    return false;
  }

  MirBlockId next_success = success_block;
  MirBlock *next_block = NULL;
  if (index + 1 < pattern->data.AST_LIST.len) {
    next_block = mir_function_add_block(
        builder->fn, mir_arena_printf(builder->fn->arena, "match.tuple.%u.%zu",
                                      builder->block->id, index + 1));
    if (!next_block) {
      return false;
    }
    next_success = next_block->id;
  }

  if (!mir_lower_pattern_to_cfg(builder, ctx, item, item_value, item_type,
                                next_success, fail_block)) {
    return false;
  }

  if (next_block) {
    mir_builder_position_at_end(builder, next_block);
    return mir_lower_tuple_pattern_to_cfg(builder, ctx, pattern, value,
                                          value_type, index + 1, success_block,
                                          fail_block);
  }

  return true;
}

static bool mir_is_list_cons_pattern(Ast *pattern) {
  if (!pattern || pattern->tag != AST_APPLICATION ||
      pattern->data.AST_APPLICATION.len != 2 ||
      !pattern->data.AST_APPLICATION.function ||
      pattern->data.AST_APPLICATION.function->tag != AST_IDENTIFIER) {
    return false;
  }

  const char *name =
      pattern->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value;
  return name && CHARS_EQ(name, TYPE_NAME_OP_LIST_PREPEND);
}

static bool mir_lower_list_cons_pattern_to_cfg(
    MirBuilder *builder, MirCtx *ctx, Ast *pattern, MirValueId value,
    Type *value_type, Ast *head_pattern, Ast *tail_pattern,
    MirBlockId success_block, MirBlockId fail_block) {
  if (!builder || !ctx || !pattern || value == MIR_NO_VALUE ||
      !is_list_type(value_type) || !head_pattern || !tail_pattern) {
    return false;
  }

  MirValueId is_empty = mir_list_is_empty(builder, pattern, value);
  if (is_empty == MIR_NO_VALUE) {
    return false;
  }

  MirBlock *cons_block = mir_function_add_block(
      builder->fn, mir_arena_printf(builder->fn->arena, "match.list_cons.bb%u",
                                    builder->block ? builder->block->id : 0));
  MirBlock *tail_block = mir_function_add_block(
      builder->fn, mir_arena_printf(builder->fn->arena, "match.list_tail.bb%u",
                                    builder->block ? builder->block->id : 0));
  if (!cons_block || !tail_block) {
    return false;
  }

  mir_builder_set_cond(builder, is_empty, fail_block, cons_block->id);
  mir_builder_position_at_end(builder, cons_block);

  Type *head_type = type_of_list(value_type);
  if (!head_type) {
    head_type = head_pattern->type;
  }
  MirValueId head = mir_list_head(builder, head_type, head_pattern, value);
  MirValueId tail = mir_list_tail(builder, value_type, tail_pattern, value);
  if (head == MIR_NO_VALUE || tail == MIR_NO_VALUE) {
    return false;
  }

  if (!mir_lower_pattern_to_cfg(builder, ctx, head_pattern, head, head_type,
                                tail_block->id, fail_block)) {
    return false;
  }

  mir_builder_position_at_end(builder, tail_block);
  return mir_lower_pattern_to_cfg(builder, ctx, tail_pattern, tail, value_type,
                                  success_block, fail_block);
}

static bool mir_lower_list_literal_pattern_to_cfg(
    MirBuilder *builder, MirCtx *ctx, Ast *pattern, MirValueId value,
    Type *value_type, size_t index, MirBlockId success_block,
    MirBlockId fail_block) {
  if (!builder || !ctx || !pattern || pattern->tag != AST_LIST ||
      !is_list_type(value_type)) {
    return false;
  }

  if (index >= pattern->data.AST_LIST.len) {
    MirValueId is_empty = mir_list_is_empty(builder, pattern, value);
    if (is_empty == MIR_NO_VALUE) {
      return false;
    }
    mir_builder_set_cond(builder, is_empty, success_block, fail_block);
    return true;
  }

  Ast *head_pattern = pattern->data.AST_LIST.items + index;
  MirValueId is_empty = mir_list_is_empty(builder, head_pattern, value);
  if (is_empty == MIR_NO_VALUE) {
    return false;
  }

  MirBlock *cons_block = mir_function_add_block(
      builder->fn,
      mir_arena_printf(builder->fn->arena, "match.list_item.bb%u.%zu",
                       builder->block ? builder->block->id : 0, index));
  MirBlock *tail_block = mir_function_add_block(
      builder->fn,
      mir_arena_printf(builder->fn->arena, "match.list_next.bb%u.%zu",
                       builder->block ? builder->block->id : 0, index));
  if (!cons_block || !tail_block) {
    return false;
  }

  mir_builder_set_cond(builder, is_empty, fail_block, cons_block->id);
  mir_builder_position_at_end(builder, cons_block);

  Type *head_type = type_of_list(value_type);
  if (!head_type) {
    head_type = head_pattern->type;
  }
  MirValueId head = mir_list_head(builder, head_type, head_pattern, value);
  MirValueId tail = mir_list_tail(builder, value_type, pattern, value);
  if (head == MIR_NO_VALUE || tail == MIR_NO_VALUE) {
    return false;
  }

  if (!mir_lower_pattern_to_cfg(builder, ctx, head_pattern, head, head_type,
                                tail_block->id, fail_block)) {
    return false;
  }

  mir_builder_position_at_end(builder, tail_block);
  return mir_lower_list_literal_pattern_to_cfg(builder, ctx, pattern, tail,
                                               value_type, index + 1,
                                               success_block, fail_block);
}

static MirValueId mir_pattern_array_size(MirBuilder *builder, Ast *origin,
                                         MirValueId array) {
  if (!builder || array == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  return mir_tuple_get(builder, &t_int, origin, array, 0);
}

static MirValueId mir_pattern_array_at(MirBuilder *builder, Type *type,
                                       Ast *origin, MirValueId array,
                                       MirValueId index) {
  if (!builder || !type || array == MIR_NO_VALUE || index == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *array_type = mir_function_value_type(builder->fn, array);
  if (!array_type || !is_array_type(array_type) ||
      !array_type->data.T_CONS.args || array_type->data.T_CONS.num_args < 1) {
    return MIR_NO_VALUE;
  }

  Type *ptr_type = ptr_of_type(array_type->data.T_CONS.args[0]);
  MirValueId data_ptr = mir_tuple_get(builder, ptr_type, origin, array, 2);
  MirValueId element_ptr =
      mir_ptr_offset(builder, ptr_type, origin, data_ptr, index);
  return mir_ptr_load(builder, type, origin, element_ptr);
}

static bool mir_lower_array_pattern_items_to_cfg(
    MirBuilder *builder, MirCtx *ctx, Ast *pattern, MirValueId value,
    Type *element_type, size_t index, MirBlockId success_block,
    MirBlockId fail_block) {
  if (!builder || !ctx || !pattern || pattern->tag != AST_ARRAY ||
      value == MIR_NO_VALUE || !element_type) {
    return false;
  }

  if (index >= pattern->data.AST_LIST.len) {
    mir_builder_set_br(builder, success_block);
    return true;
  }

  Ast *item = pattern->data.AST_LIST.items + index;
  MirValueId index_value = mir_const_int(builder, &t_int, item, (int)index);
  MirValueId item_value =
      mir_pattern_array_at(builder, element_type, item, value, index_value);
  if (item_value == MIR_NO_VALUE) {
    return false;
  }

  MirBlockId next_success = success_block;
  MirBlock *next_block = NULL;
  if (index + 1 < pattern->data.AST_LIST.len) {
    next_block = mir_function_add_block(
        builder->fn,
        mir_arena_printf(builder->fn->arena, "match.array.%u.%zu",
                         builder->block ? builder->block->id : 0, index + 1));
    if (!next_block) {
      return false;
    }
    next_success = next_block->id;
  }

  if (!mir_lower_pattern_to_cfg(builder, ctx, item, item_value, element_type,
                                next_success, fail_block)) {
    return false;
  }

  if (next_block) {
    mir_builder_position_at_end(builder, next_block);
    return mir_lower_array_pattern_items_to_cfg(builder, ctx, pattern, value,
                                                element_type, index + 1,
                                                success_block, fail_block);
  }

  return true;
}

static bool mir_lower_array_pattern_to_cfg(MirBuilder *builder, MirCtx *ctx,
                                           Ast *pattern, MirValueId value,
                                           Type *value_type,
                                           MirBlockId success_block,
                                           MirBlockId fail_block) {
  if (!builder || !ctx || !pattern || pattern->tag != AST_ARRAY ||
      value == MIR_NO_VALUE || !value_type || !is_array_type(value_type) ||
      !value_type->data.T_CONS.args || value_type->data.T_CONS.num_args < 1) {
    return false;
  }

  MirValueId actual_size = mir_pattern_array_size(builder, pattern, value);
  MirValueId expected_size =
      mir_const_int(builder, &t_int, pattern, (int)pattern->data.AST_LIST.len);
  MirValueId size_matches =
      mir_ieq(builder, pattern, actual_size, expected_size);
  if (size_matches == MIR_NO_VALUE) {
    return false;
  }

  if (pattern->data.AST_LIST.len == 0) {
    mir_builder_set_cond(builder, size_matches, success_block, fail_block);
    return true;
  }

  MirBlock *items_block = mir_function_add_block(
      builder->fn, mir_arena_printf(builder->fn->arena, "match.array.bb%u",
                                    builder->block ? builder->block->id : 0));
  if (!items_block) {
    return false;
  }

  mir_builder_set_cond(builder, size_matches, items_block->id, fail_block);
  mir_builder_position_at_end(builder, items_block);
  return mir_lower_array_pattern_items_to_cfg(builder, ctx, pattern, value,
                                              value_type->data.T_CONS.args[0],
                                              0, success_block, fail_block);
}

static bool mir_lower_constructor_pattern_to_cfg(
    MirBuilder *builder, MirCtx *ctx, Ast *pattern, MirValueId value,
    Type *value_type, Type *constructor_type, int constructor_index,
    MirBlockId success_block, MirBlockId fail_block) {
  if (!builder || !ctx || !pattern || !constructor_type ||
      constructor_index < 0) {
    return false;
  }

  const char *constructor_name = mir_constructor_name(constructor_type);
  MirValueId tag = mir_variant_tag(builder, pattern, value);
  MirValueId tags_match =
      mir_tag_eq(builder, pattern, tag, constructor_index, constructor_name);
  if (tags_match == MIR_NO_VALUE) {
    return false;
  }

  if (pattern->tag != AST_APPLICATION) {
    mir_builder_set_cond(builder, tags_match, success_block, fail_block);
    return true;
  }

  if (pattern->data.AST_APPLICATION.len == 0) {
    mir_builder_set_cond(builder, tags_match, success_block, fail_block);
    return true;
  }

  MirBlock *payload_block = mir_function_add_block(
      builder->fn, mir_arena_printf(builder->fn->arena, "match.payload.bb%u",
                                    builder->block ? builder->block->id : 0));
  if (!payload_block) {
    return false;
  }
  mir_builder_set_cond(builder, tags_match, payload_block->id, fail_block);

  mir_builder_position_at_end(builder, payload_block);
  MirValueId payload =
      mir_variant_payload(builder, pattern, value, constructor_type,
                          constructor_index, constructor_name);
  if (payload == MIR_NO_VALUE) {
    return false;
  }

  MirBlockId next_success = success_block;
  for (size_t i = pattern->data.AST_APPLICATION.len; i > 0; i--) {
    size_t arg_index = i - 1;
    MirBlock *arg_block = payload_block;
    if (arg_index > 0) {
      arg_block = mir_function_add_block(
          builder->fn,
          mir_arena_printf(builder->fn->arena, "match.payload.bb%u.%zu",
                           payload_block->id, arg_index));
      if (!arg_block) {
        return false;
      }
    }

    mir_builder_position_at_end(builder, arg_block);
    Ast *arg_pattern = pattern->data.AST_APPLICATION.args + arg_index;
    Type *payload_type = NULL;
    if (constructor_type->kind == T_CONS &&
        constructor_type->data.T_CONS.args &&
        arg_index < (size_t)constructor_type->data.T_CONS.num_args) {
      payload_type = constructor_type->data.T_CONS.args[arg_index];
    }
    if (!payload_type) {
      payload_type = arg_pattern->type;
    }

    MirValueId field =
        mir_tuple_get(builder, payload_type, arg_pattern, payload, arg_index);
    if (field == MIR_NO_VALUE ||
        !mir_lower_pattern_to_cfg(builder, ctx, arg_pattern, field,
                                  payload_type, next_success, fail_block)) {
      return false;
    }

    next_success = arg_block->id;
  }

  return true;
}

static bool mir_lower_pattern_to_cfg(MirBuilder *builder, MirCtx *ctx,
                                     Ast *pattern, MirValueId value,
                                     Type *value_type, MirBlockId success_block,
                                     MirBlockId fail_block) {
  if (!builder || !ctx || !pattern) {
    return false;
  }

  if (pattern->tag == AST_LET) {
    pattern = pattern->data.AST_LET.binding;
  }

  if (pattern->tag == AST_MATCH_GUARD_CLAUSE) {
    MirBlock *guard_block = mir_function_add_block(builder->fn, "match.guard");
    if (!guard_block) {
      return false;
    }
    if (!mir_lower_pattern_to_cfg(
            builder, ctx, pattern->data.AST_MATCH_GUARD_CLAUSE.test_expr, value,
            value_type, guard_block->id, fail_block)) {
      return false;
    }
    mir_builder_position_at_end(builder, guard_block);
    MirValueId guard =
        mir_expr(builder, pattern->data.AST_MATCH_GUARD_CLAUSE.guard_expr, ctx);
    if (guard == MIR_NO_VALUE) {
      return false;
    }
    mir_builder_set_cond(builder, guard, success_block, fail_block);
    return true;
  }

  if (is_list_type(value_type)) {
    if (pattern->tag == AST_LIST) {
      return mir_lower_list_literal_pattern_to_cfg(builder, ctx, pattern, value,
                                                   value_type, 0, success_block,
                                                   fail_block);
    }
    if (mir_is_list_cons_pattern(pattern)) {
      return mir_lower_list_cons_pattern_to_cfg(
          builder, ctx, pattern, value, value_type,
          pattern->data.AST_APPLICATION.args,
          pattern->data.AST_APPLICATION.args + 1, success_block, fail_block);
    }
  }

  if (pattern->tag == AST_ARRAY && value_type && is_array_type(value_type)) {
    return mir_lower_array_pattern_to_cfg(
        builder, ctx, pattern, value, value_type, success_block, fail_block);
  }

  int constructor_index = -1;
  Type *constructor_type =
      mir_resolve_sum_constructor(value_type, pattern, &constructor_index);
  if (constructor_type) {
    return mir_lower_constructor_pattern_to_cfg(
        builder, ctx, pattern, value, value_type, constructor_type,
        constructor_index, success_block, fail_block);
  }

  switch (pattern->tag) {
  case AST_PLACEHOLDER_ID:
    mir_builder_set_br(builder, success_block);
    return true;
  case AST_IDENTIFIER:
    if (!ast_is_placeholder_id(pattern) &&
        !mir_bind_identifier(ctx, pattern, value)) {
      return false;
    }
    mir_builder_set_br(builder, success_block);
    return true;
  case AST_INT:
  case AST_UINT64:
  case AST_FLOAT:
  case AST_DOUBLE:
  case AST_CHAR:
  case AST_BOOL: {
    MirValueId literal = mir_expr(builder, pattern, ctx);
    MirValueId eq =
        mir_eq_for_type(builder, pattern, value_type, value, literal);
    if (eq == MIR_NO_VALUE) {
      return false;
    }
    mir_builder_set_cond(builder, eq, success_block, fail_block);
    return true;
  }
  case AST_VOID: {
    MirValueId literal = mir_expr(builder, pattern, ctx);
    MirValueId eq =
        mir_eq_for_type(builder, pattern, value_type, value, literal);
    if (eq == MIR_NO_VALUE) {
      mir_builder_set_br(builder, success_block);
    } else {
      mir_builder_set_cond(builder, eq, success_block, fail_block);
    }
    return true;
  }
  case AST_TUPLE:
    return mir_lower_tuple_pattern_to_cfg(
        builder, ctx, pattern, value, value_type, 0, success_block, fail_block);
  default:
    return false;
  }
}

static bool mir_bool_literal_pattern(Ast *pattern, bool *out) {
  if (!pattern || pattern->tag != AST_BOOL) {
    return false;
  }
  if (out) {
    *out = pattern->data.AST_BOOL.value;
  }
  return true;
}

static bool mir_match_bool_exhaustive_arms(Ast *ast, size_t *true_index,
                                           size_t *false_index) {
  if (!ast || ast->tag != AST_MATCH || ast->data.AST_MATCH.len != 2 ||
      !ast->data.AST_MATCH.expr || !ast->data.AST_MATCH.expr->type ||
      ast->data.AST_MATCH.expr->type->kind != T_BOOL) {
    return false;
  }

  bool seen_true = false;
  bool seen_false = false;
  for (size_t i = 0; i < ast->data.AST_MATCH.len; i++) {
    Ast *pattern = ast->data.AST_MATCH.branches + (i * 2);
    bool value = false;
    if (!mir_bool_literal_pattern(pattern, &value)) {
      return false;
    }
    if (value) {
      if (seen_true) {
        return false;
      }
      seen_true = true;
      if (true_index) {
        *true_index = i;
      }
    } else {
      if (seen_false) {
        return false;
      }
      seen_false = true;
      if (false_index) {
        *false_index = i;
      }
    }
  }

  return seen_true && seen_false;
}

static bool mir_join_value_to_block(MirBuilder *builder,
                                    MirPhiIncomingVec *incoming,
                                    MirValueId value,
                                    MirBlockId continuation_block,
                                    bool collect_value) {
  if (!builder || !builder->fn || !builder->block ||
      builder->block->term.kind != MIR_TERM_NONE ||
      continuation_block == MIR_NO_BLOCK) {
    return false;
  }

  if (collect_value) {
    if (!incoming || value == MIR_NO_VALUE) {
      return false;
    }
    mir_phi_incoming_vec_push(
        builder->fn->arena, incoming,
        (MirPhiIncoming){.block = builder->block->id, .value = value});
  }

  mir_builder_set_br(builder, continuation_block);
  return true;
}

static MirValueId mir_bool_match_expr(MirBuilder *builder, Ast *ast,
                                      MirCtx *ctx, size_t true_index,
                                      size_t false_index) {
  if (!builder || !builder->fn || !builder->block || !ast || !ctx) {
    return MIR_NO_VALUE;
  }

  MirValueId scrutinee = mir_expr(builder, ast->data.AST_MATCH.expr, ctx);
  if (scrutinee == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirBlock *match_block = builder->block;
  MirBlock *continuation_block =
      mir_function_add_block(builder->fn, "match.cont");
  MirBlock *true_block = mir_function_add_block(builder->fn, "match.true");
  MirBlock *false_block = mir_function_add_block(builder->fn, "match.false");
  if (!continuation_block || !true_block || !false_block) {
    return MIR_NO_VALUE;
  }

  mir_builder_position_at_end(builder, match_block);
  mir_builder_set_cond(builder, scrutinee, true_block->id, false_block->id);

  Ast *true_body = ast->data.AST_MATCH.branches + (true_index * 2) + 1;
  Ast *false_body = ast->data.AST_MATCH.branches + (false_index * 2) + 1;
  bool collect_value = ast->type && ast->type->kind != T_VOID;
  MirPhiIncomingVec incoming = {0};

  MIR_STACK_ALLOC_CTX_PUSH(true_ctx, builder, ctx)
  mir_builder_position_at_end(builder, true_block);
  MirValueId true_value = mir_expr(builder, true_body, &true_ctx);
  if (true_value == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(builder);
  } else {
    mir_join_value_to_block(builder, &incoming, true_value,
                            continuation_block->id, collect_value);
  }

  MIR_STACK_ALLOC_CTX_PUSH(false_ctx, builder, ctx)
  mir_builder_position_at_end(builder, false_block);
  MirValueId false_value = mir_expr(builder, false_body, &false_ctx);
  if (false_value == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(builder);
  } else {
    mir_join_value_to_block(builder, &incoming, false_value,
                            continuation_block->id, collect_value);
  }

  mir_builder_position_at_end(builder, continuation_block);
  if (!collect_value) {
    return mir_const_void(builder, ast->type, ast);
  }
  return mir_phi(builder, ast->type, ast, incoming);
}

static MirValueId mir_match_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  if (!builder || !builder->fn || !builder->block || !ast ||
      ast->tag != AST_MATCH || !ctx) {
    return MIR_NO_VALUE;
  }

  size_t true_index = 0;
  size_t false_index = 0;
  if (mir_match_bool_exhaustive_arms(ast, &true_index, &false_index)) {
    return mir_bool_match_expr(builder, ast, ctx, true_index, false_index);
  }

  MirArena *arena = builder->fn->arena;
  MirValueId scrutinee = mir_expr(builder, ast->data.AST_MATCH.expr, ctx);
  if (scrutinee == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirBlock *match_block = builder->block;
  MirBlock *no_match_block =
      mir_function_add_block(builder->fn, "match.no_match");
  MirBlock *continuation_block =
      mir_function_add_block(builder->fn, "match.cont");
  if (!no_match_block || !continuation_block) {
    return MIR_NO_VALUE;
  }

  MirBlock *first_test_block =
      ast->data.AST_MATCH.len > 0
          ? mir_function_add_block(builder->fn, "match.arm.0.test")
          : NULL;
  if (ast->data.AST_MATCH.len > 0 && !first_test_block) {
    return MIR_NO_VALUE;
  }
  MirBlockId first_test =
      first_test_block ? first_test_block->id : no_match_block->id;

  mir_builder_position_at_end(builder, match_block);
  mir_builder_set_br(builder, first_test);
  bool collect_value = ast->type && ast->type->kind != T_VOID;
  MirPhiIncomingVec incoming = {0};
  MirBlock *test_block = first_test_block;

  for (size_t i = 0; i < ast->data.AST_MATCH.len; i++) {
    Ast *pattern = ast->data.AST_MATCH.branches + (i * 2);
    Ast *body = ast->data.AST_MATCH.branches + (i * 2) + 1;

    MirBlock *body_block = mir_function_add_block(
        builder->fn, mir_arena_printf(arena, "match.arm.%zu.body", i));
    MirBlock *next_test_block =
        i + 1 < ast->data.AST_MATCH.len
            ? mir_function_add_block(
                  builder->fn,
                  mir_arena_printf(arena, "match.arm.%zu.test", i + 1))
            : NULL;
    MirBlockId fail_block =
        next_test_block ? next_test_block->id : no_match_block->id;
    if (!test_block || !body_block) {
      return MIR_NO_VALUE;
    }

    MIR_STACK_ALLOC_CTX_PUSH(branch_ctx, builder, ctx)
    mir_builder_position_at_end(builder, test_block);
    if (!mir_lower_pattern_to_cfg(builder, &branch_ctx, pattern, scrutinee,
                                  ast->data.AST_MATCH.expr->type,
                                  body_block->id, fail_block)) {
      return MIR_NO_VALUE;
    }

    mir_builder_position_at_end(builder, body_block);
    MirValueId branch_value = mir_expr(builder, body, &branch_ctx);
    if (branch_value == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(builder);
    } else {
      mir_join_value_to_block(builder, &incoming, branch_value,
                              continuation_block->id, collect_value);
    }

    test_block = next_test_block;
  }

  mir_builder_position_at_end(builder, no_match_block);
  if (ast->data.AST_MATCH.allow_no_match && !collect_value) {
    mir_builder_set_br(builder, continuation_block->id);
  } else {
    mir_builder_set_unreachable(builder);
  }

  mir_builder_position_at_end(builder, continuation_block);
  if (!collect_value) {
    return mir_const_void(builder, ast->type, ast);
  }
  return mir_phi(builder, ast->type, ast, incoming);
}

static MirValueId mir_loop_range_expr(MirBuilder *builder, Ast *ast,
                                      MirCtx *ctx) {
  if (!builder || !builder->fn || !builder->block || !ast ||
      ast->tag != AST_LOOP || !ctx) {
    return MIR_NO_VALUE;
  }

  Ast *binding = ast->data.AST_LET.binding;
  Ast *range = ast->data.AST_LET.expr;
  Ast *body = ast->data.AST_LET.in_expr;
  if (!binding || !range || range->tag != AST_RANGE_EXPRESSION || !body) {
    return MIR_NO_VALUE;
  }

  MirValueId start =
      mir_expr(builder, range->data.AST_RANGE_EXPRESSION.from, ctx);
  MirValueId end = mir_expr(builder, range->data.AST_RANGE_EXPRESSION.to, ctx);
  if (start == MIR_NO_VALUE || end == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirBlock *preheader = builder->block;
  MirBlock *cond_block = mir_function_add_block(builder->fn, "loop.cond");
  MirBlock *body_block = mir_function_add_block(builder->fn, "loop.body");
  MirBlock *inc_block = mir_function_add_block(builder->fn, "loop.inc");
  MirBlock *after_block = mir_function_add_block(builder->fn, "loop.after");
  if (!preheader || !cond_block || !body_block || !inc_block || !after_block) {
    return MIR_NO_VALUE;
  }

  mir_builder_set_br(builder, cond_block->id);

  mir_builder_position_at_end(builder, cond_block);
  MirPhiIncomingVec incoming = {0};
  mir_phi_incoming_vec_push(
      builder->fn->arena, &incoming,
      (MirPhiIncoming){.block = preheader->id, .value = start});
  MirValueId index = mir_phi(builder, &t_int, binding, incoming);
  if (index == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueId cmp_operands[2] = {index, end};
  MirValueId cond =
      mir_primitive_instr(builder, MIR_OP_ULT, &t_bool, ast, cmp_operands, 2);
  if (cond == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }
  mir_builder_set_cond(builder, cond, body_block->id, after_block->id);

  MIR_STACK_ALLOC_CTX_PUSH(loop_ctx, builder, ctx)
  mir_builder_position_at_end(builder, body_block);
  if (!mir_bind_pattern(builder, &loop_ctx, binding, index, &t_int)) {
    mir_builder_set_unreachable_if_open(builder);
    return MIR_NO_VALUE;
  }

  MirValueId body_value = mir_expr(builder, body, &loop_ctx);
  if (body_value == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(builder);
  }
  if (builder->block && builder->block->term.kind == MIR_TERM_NONE) {
    mir_builder_set_br(builder, inc_block->id);
  }

  mir_builder_position_at_end(builder, inc_block);
  MirValueId one = mir_const_int(builder, &t_int, ast, 1);
  MirValueId next_operands[2] = {index, one};
  MirValueId next =
      mir_primitive_instr(builder, MIR_OP_IADD, &t_int, ast, next_operands, 2);
  if (one == MIR_NO_VALUE || next == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirInstr *phi_instr = mir_function_find_def_instr(builder->fn, index);
  if (!phi_instr || phi_instr->kind != MIR_PHI) {
    return MIR_NO_VALUE;
  }
  mir_phi_incoming_vec_push(
      builder->fn->arena, &phi_instr->data.phi.incoming,
      (MirPhiIncoming){.block = inc_block->id, .value = next});
  mir_builder_set_br(builder, cond_block->id);

  mir_builder_position_at_end(builder, after_block);
  Type *loop_type = ast->type ? ast->type : &t_int;
  if (loop_type->kind == T_VOID) {
    return mir_const_void(builder, loop_type, ast);
  }
  return mir_const_undef(builder, loop_type, ast);
}

static MirValueId mir_loop_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  if (!ast || ast->tag != AST_LOOP) {
    return MIR_NO_VALUE;
  }

  Ast *iter = ast->data.AST_LET.expr;
  if (iter && iter->tag == AST_RANGE_EXPRESSION) {
    return mir_loop_range_expr(builder, ast, ctx);
  }

  return MIR_NO_VALUE;
}

MirValueId mir_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx) {
  if (!builder || !ast) {
    return MIR_NO_VALUE;
  }

  switch (ast->tag) {
  case AST_BODY:
    return mir_body(builder, ast, ctx);
  case AST_LET:
    return mir_let(builder, ast, ctx);
  case AST_IDENTIFIER:
    return mir_identifier(builder, ast, ctx);
  case AST_RECORD_ACCESS:
    return mir_record_access(builder, ast, ctx);
  case AST_INT:
    return mir_const_int(builder, ast->type, ast, ast->data.AST_INT.value);
  case AST_UINT64:
    return mir_const_uint64(builder, ast->type, ast,
                            ast->data.AST_UINT64.value);
  case AST_FLOAT:
    return mir_const_float(builder, ast->type, ast, ast->data.AST_FLOAT.value);
  case AST_DOUBLE:
    return mir_const_double(builder, ast->type, ast,
                            ast->data.AST_DOUBLE.value);
  case AST_CHAR:
    return mir_const_char(builder, ast->type, ast, ast->data.AST_CHAR.value);
  case AST_BOOL:
    return mir_const_bool(builder, ast->type, ast, ast->data.AST_BOOL.value);
  case AST_STRING:
    return mir_const_string(builder, ast->type, ast, ast->data.AST_STRING.value,
                            ast->data.AST_STRING.length);
  case AST_VOID:
    return mir_const_void(builder, ast->type, ast);
  case AST_TYPE_DECL:
    return mir_const_void(builder, ast->type ? ast->type : &t_void, ast);
  case AST_IMPORT:
    return mir_import_value(builder, ast, ctx);
  case AST_TUPLE:
    return mir_tuple_expr(builder, ast, ctx);
  case AST_LIST:
  case AST_EMPTY_CONTAINER:
    return mir_list_expr(builder, ast, ctx);
  case AST_ARRAY:
    return mir_array_expr(builder, ast, ctx);
  case AST_MATCH:
    return mir_match_expr(builder, ast, ctx);
  case AST_LOOP:
    return mir_loop_expr(builder, ast, ctx);
  case AST_APPLICATION:
    return mir_application(builder, ast->type, ast, ctx);
  case AST_LAMBDA:
    return mir_lambda_value(builder, ast, ctx,
                            mir_scoped_lambda_name(builder, ctx, ast), NULL,
                            false);
  case AST_MODULE:
    return mir_const_void(builder, &t_void, ast);
  case AST_EXTERN_FN:
    return mir_extern_fn_value(builder, ast, NULL);
  case AST_YIELD: {
    return mir_yield_expr(builder, ast, ctx);
  }
  default:
    return MIR_NO_VALUE;
  }
}

static bool mir_populate_function_body(MirProgram *program, MirFunction *fn,
                                       Ast *fn_ast, MirCtx *ctx,
                                       const char *self_name) {
  if (!program || !fn || !fn_ast) {
    return false;
  }

  MirBlock *entry = mir_function_add_block(fn, "entry");
  if (!entry) {
    return false;
  }

  MirBuilder builder;
  mir_builder_init(&builder, program, fn);
  mir_builder_position_at_end(&builder, entry);

  MirCtx fn_ctx = {
      .env = ctx && ctx->env ? ctx->env : program->type_env,
      .frame = NULL,
      .current_module = ctx ? ctx->current_module : program->root_module,
      .export_bindings = false,
  };
  ht fn_table;
  MirStackFrame fn_frame;
  mir_stack_frame_init(program->arena, &fn_table, &fn_frame, NULL);
  fn_ctx.frame = &fn_frame;

  if (self_name) {
    MirBuilder self_builder;
    mir_builder_init(&self_builder, program, fn);
    if (!mir_bind_function_symbol(&self_builder, &fn_ctx, self_name, fn, fn_ast,
                                  false)) {
      return false;
    }
  }

  Type *fn_type = fn_ast->type;
  bool is_closure_fn = fn_ast->type && is_closure(fn_ast->type) &&
                       fn_ast->type->closure_meta != NULL;
  if (is_closure_fn) {
    Type *env_type = fn_ast->type->closure_meta;
    MirValueId env_param = mir_function_add_param(fn, "$env", env_type, fn_ast);
    AstList *closed = fn_ast->data.AST_LAMBDA.closed_vals;
    int closed_len = fn_ast->data.AST_LAMBDA.num_closed_vals;
    for (int i = 0; i < closed_len && closed; i++, closed = closed->next) {
      Type *field_type = NULL;
      if (env_type->kind == T_CONS && env_type->data.T_CONS.args &&
          i < env_type->data.T_CONS.num_args) {
        field_type = env_type->data.T_CONS.args[i];
      }
      if (!field_type && closed->ast) {
        field_type = closed->ast->type;
      }

      const char *name = NULL;
      if (closed->ast && closed->ast->tag == AST_IDENTIFIER) {
        name = closed->ast->data.AST_IDENTIFIER.value;
      }

      MirValueId field = mir_extract_field(&builder, field_type, closed->ast,
                                           env_param, (size_t)i, name);
      mir_bind_pattern(&builder, &fn_ctx, closed->ast, field, field_type);
    }
  }

  AstList *param = fn_ast->data.AST_LAMBDA.params;
  for (size_t i = 0; i < fn_ast->data.AST_LAMBDA.len && param;
       i++, param = param->next) {
    Ast *param_ast = param->ast;
    Type *param_type = NULL;
    if (fn_type && fn_type->kind == T_FN) {
      param_type = fn_type->data.T_FN.from;
      fn_type = fn_type->data.T_FN.to;
    }
    if (!param_type && param_ast) {
      param_type = param_ast->type;
    }

    if (is_closure_fn && param_ast && param_ast->tag == AST_VOID) {
      continue;
    }

    const char *param_name = "_";
    if (param_ast && param_ast->tag == AST_IDENTIFIER) {
      param_name = param_ast->data.AST_IDENTIFIER.value;
    }

    MirValueId param_value =
        mir_function_add_param(fn, param_name, param_type, param_ast);
    mir_bind_pattern(&builder, &fn_ctx, param_ast, param_value, param_type);
  }

  Ast *body = fn_ast->data.AST_LAMBDA.body;
  MirValueId result = mir_expr(&builder, body, &fn_ctx);
  if (is_coroutine_constructor_type(fn_ast->type)) {
    mir_builder_set_coro_done_if_open(&builder);
    return true;
  }
  if (result != MIR_NO_VALUE && builder.block &&
      builder.block->term.kind == MIR_TERM_NONE) {
    mir_builder_set_return(&builder, result);
  } else if (result == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(&builder);
  }
  return true;
}

static MirFunction *mir_builder_function(MirProgram *program, Ast *fn_ast,
                                         const char *name, MirCtx *ctx) {
  if (!program || !fn_ast) {
    return NULL;
  }

  MirFunction *fn =
      mir_program_add_function(program, name, fn_ast->type, fn_ast);
  if (!fn) {
    return NULL;
  }
  mir_populate_function_body(program, fn, fn_ast, ctx, NULL);
  return fn;
}

static const char *mir_primitive_op_name(MirPrimitiveOp op) {
  switch (op) {
  case MIR_OP_IEQ:
    return "ieq";
  case MIR_OP_UEQ:
    return "ueq";
  case MIR_OP_FEQ:
    return "feq";
  case MIR_OP_CEQ:
    return "ceq";
  case MIR_OP_BEQ:
    return "beq";
  case MIR_OP_IGT:
    return "igt";
  case MIR_OP_UGT:
    return "ugt";
  case MIR_OP_FGT:
    return "fgt";
  case MIR_OP_CGT:
    return "cgt";
  case MIR_OP_IGTE:
    return "igte";
  case MIR_OP_UGTE:
    return "ugte";
  case MIR_OP_FGTE:
    return "fgte";
  case MIR_OP_CGTE:
    return "cgte";
  case MIR_OP_ILT:
    return "ilt";
  case MIR_OP_ULT:
    return "ult";
  case MIR_OP_FLT:
    return "flt";
  case MIR_OP_CLT:
    return "clt";
  case MIR_OP_ILTE:
    return "ilte";
  case MIR_OP_ULTE:
    return "ulte";
  case MIR_OP_FLTE:
    return "flte";
  case MIR_OP_CLTE:
    return "clte";
  case MIR_OP_LNOT:
    return "lnot";
  case MIR_OP_IADD:
    return "iadd";
  case MIR_OP_UADD:
    return "uadd";
  case MIR_OP_FADD:
    return "fadd";
  case MIR_OP_ISUB:
    return "isub";
  case MIR_OP_USUB:
    return "usub";
  case MIR_OP_FSUB:
    return "fsub";
  case MIR_OP_IMUL:
    return "imul";
  case MIR_OP_UMUL:
    return "umul";
  case MIR_OP_FMUL:
    return "fmul";
  case MIR_OP_IDIV:
    return "idiv";
  case MIR_OP_UDIV:
    return "udiv";
  case MIR_OP_FDIV:
    return "fdiv";
  case MIR_OP_IMOD:
    return "imod";
  case MIR_OP_UMOD:
    return "umod";
  case MIR_OP_FMOD:
    return "fmod";
  }
  return "primitive.unknown";
}

static const char *mir_const_kind_name(MirConstKind kind) {
  switch (kind) {
  case MIR_CONST_KIND_INT:
    return "const.int";
  case MIR_CONST_KIND_UINT64:
    return "const.uint64";
  case MIR_CONST_KIND_FLOAT:
    return "const.float";
  case MIR_CONST_KIND_DOUBLE:
    return "const.double";
  case MIR_CONST_KIND_CHAR:
    return "const.char";
  case MIR_CONST_KIND_BOOL:
    return "const.bool";
  case MIR_CONST_KIND_STRING:
    return "const.string";
  case MIR_CONST_KIND_VOID:
    return "const.void";
  case MIR_CONST_KIND_UNDEF:
    return "const.undef";
  }
  return "const.unknown";
}

static const char *mir_op_kind_name(MirOpKind kind) {
  switch (kind) {
  case MIR_OP_KIND_PRIMITIVE:
    return "primitive";
  case MIR_OP_KIND_CAST:
    return "primitive_cast";
  case MIR_OP_KIND_TAG_EQ:
    return "tag_eq";
  case MIR_OP_KIND_LIST_IS_EMPTY:
    return "list_is_empty";
  case MIR_OP_KIND_ARRAY_SIZE:
    return "array_size";
  case MIR_OP_KIND_ARRAY_SET:
    return "array_set";
  case MIR_OP_KIND_PTR_OFFSET:
    return "ptr_offset";
  case MIR_OP_KIND_LOAD:
    return "load";
  case MIR_OP_KIND_LOAD_OWNED:
    return "load_owned";
  case MIR_OP_KIND_STORE:
    return "store";
  case MIR_OP_KIND_GLOBAL_LOAD:
    return "global_load";
  case MIR_OP_KIND_GLOBAL_STORE:
    return "global_store";
  case MIR_OP_KIND_STR:
    return "str";
  case MIR_OP_KIND_PRINT:
    return "print";
  case MIR_OP_KIND_FPRINT:
    return "fprint";
  case MIR_OP_KIND_FLUSH:
    return "flush";
  case MIR_OP_KIND_CSTR:
    return "cstr";
  case MIR_OP_KIND_SIZEOF:
    return "sizeof";
  case MIR_OP_KIND_DLOPEN:
    return "dlopen";
  case MIR_OP_KIND_AS_BYTES:
    return "asbytes";
  case MIR_OP_KIND_TYPEOF:
    return "typeof";
  case MIR_OP_KIND_DUP:
    return "dup";
  case MIR_OP_KIND_DROP:
    return "drop";
  }
  return "op.unknown";
}

static const char *mir_extract_kind_name(MirExtractKind kind) {
  switch (kind) {
  case MIR_EXTRACT_FIELD:
    return "extract.field";
  case MIR_EXTRACT_VARIANT_TAG:
    return "extract.variant_tag";
  case MIR_EXTRACT_VARIANT_PAYLOAD:
    return "extract.variant_payload";
  case MIR_EXTRACT_LIST_HEAD:
    return "extract.list_head";
  case MIR_EXTRACT_LIST_TAIL:
    return "extract.list_tail";
  case MIR_EXTRACT_ARRAY_AT:
    return "extract.array_at";
  case MIR_EXTRACT_ARRAY_SUCC:
    return "extract.array_succ";
  case MIR_EXTRACT_ARRAY_OFFSET:
    return "extract.array_offset";
  case MIR_EXTRACT_CLOSURE_FN:
    return "extract.closure_fn";
  case MIR_EXTRACT_CLOSURE_ENV:
    return "extract.closure_env";
  }
  return "extract.unknown";
}

static const char *mir_construct_kind_name(MirConstructKind kind) {
  switch (kind) {
  case MIR_CONSTRUCT_TUPLE:
    return "construct.tuple";
  case MIR_CONSTRUCT_VARIANT:
    return "construct.variant";
  case MIR_CONSTRUCT_LIST_EMPTY:
    return "construct.list_empty";
  case MIR_CONSTRUCT_LIST_CONS:
    return "construct.list_cons";
  case MIR_CONSTRUCT_ARRAY_LITERAL:
    return "construct.array_literal";
  case MIR_CONSTRUCT_ARRAY_FILL_CONST:
    return "construct.array_fill_const";
  case MIR_CONSTRUCT_ARRAY_FILL:
    return "construct.array_fill";
  case MIR_CONSTRUCT_ARRAY_RANGE:
    return "construct.array_range";
  case MIR_CONSTRUCT_CLOSURE_ENV:
    return "construct.closure_env";
  case MIR_CONSTRUCT_CLOSURE:
    return "construct.closure";
  }
  return "construct.unknown";
}

static const char *mir_instr_name(MirInstrKind kind) {
  switch (kind) {
  case MIR_CONST:
    return "const";
  case MIR_OP:
    return "op";
  case MIR_PHI:
    return "phi";
  case MIR_EXTRACT:
    return "extract";
  case MIR_CONSTRUCT:
    return "construct";
  case MIR_FN_REF:
    return "fn_ref";
  case MIR_CALL:
    return "call";
  case MIR_CORO_NEW:
    return "coro.new";
  case MIR_CORO_NEXT:
    return "coro.next";
  case MIR_CORO_RESET:
    return "coro.reset";
  }
  return "unknown";
}

static void dump_value(FILE *stream, MirValueId value) {
  if (value == MIR_NO_VALUE) {
    fprintf(stream, "_");
    return;
  }
  fprintf(stream, "%%%u", value);
}

static void dump_escaped_string(FILE *stream, const char *chars, size_t len) {
  if (!chars) {
    len = 0;
  }

  fputc('"', stream);
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)chars[i];
    switch (c) {
    case '\\':
      fputs("\\\\", stream);
      break;
    case '"':
      fputs("\\\"", stream);
      break;
    case '\n':
      fputs("\\n", stream);
      break;
    case '\r':
      fputs("\\r", stream);
      break;
    case '\t':
      fputs("\\t", stream);
      break;
    default:
      if (isprint(c)) {
        fputc(c, stream);
      } else {
        fprintf(stream, "\\x%02x", c);
      }
      break;
    }
  }
  fputc('"', stream);
}

static void dump_char_literal(FILE *stream, char value) {
  fputc('\'', stream);
  switch (value) {
  case '\\':
    fputs("\\\\", stream);
    break;
  case '\'':
    fputs("\\'", stream);
    break;
  case '\n':
    fputs("\\n", stream);
    break;
  case '\r':
    fputs("\\r", stream);
    break;
  case '\t':
    fputs("\\t", stream);
    break;
  default:
    if (isprint((unsigned char)value)) {
      fputc(value, stream);
    } else {
      fprintf(stream, "\\x%02x", (unsigned char)value);
    }
    break;
  }
  fputc('\'', stream);
}

static void dump_value_id_vec(FILE *stream, const MirValueIdVec *values) {
  for (size_t i = 0; i < values->len; i++) {
    if (i > 0) {
      fprintf(stream, ", ");
    }
    dump_value(stream, values->items[i]);
  }
}

static void dump_named_value_id_vec(FILE *stream, const MirValueIdVec *values,
                                    Type *type) {
  const char **names = NULL;
  int name_count = 0;
  if (type && (type->kind == T_CONS || type->kind == T_SUM) &&
      type->data.T_CONS.names) {
    names = type->data.T_CONS.names;
    name_count = type->data.T_CONS.num_args;
  }

  for (size_t i = 0; i < values->len; i++) {
    if (i > 0) {
      fprintf(stream, ", ");
    }
    if (names && i < (size_t)name_count && names[i]) {
      fprintf(stream, "%s: ", names[i]);
    }
    dump_value(stream, values->items[i]);
  }
}

static void dump_call_type(FILE *stream, Type *type) {
  if (!type || type->kind != T_FN) {
    print_type_to_stream(type, stream);
    return;
  }

  fputc('(', stream);
  Type *cur = type;
  while (cur && cur->kind == T_FN) {
    print_type_to_stream(cur->data.T_FN.from, stream);
    fputs(" -> ", stream);
    cur = cur->data.T_FN.to;
  }
  print_type_to_stream(cur, stream);
  fputc(')', stream);
}

static void dump_block_ref(FILE *stream, MirBlockId block) {
  if (block == MIR_NO_BLOCK) {
    fputs("bb?", stream);
    return;
  }
  fprintf(stream, "bb%u", block);
}

static void dump_escape_meta(FILE *stream, const MirFunction *fn,
                             MirValueId value) {
  EscapeMeta *meta = mir_value_escape_meta((MirFunction *)fn, value);
  if (!meta) {
    return;
  }

  fputs(STYLE_DIM " ; ea ", stream);
  switch (meta->status) {
  case EA_STACK_ALLOC:
    fputs("stack", stream);
    break;
  case EA_HEAP_ALLOC:
    fputs("heap", stream);
    break;
  }
  fprintf(stream, "#%u", meta->id);
  if (meta->attributes & EA_ATTR_MUTABLE) {
    fputs(" mutable", stream);
  }
  fputs(STYLE_RESET_ALL, stream);
}

static const char *mir_operand_role_name(MirOperandRole role) {
  switch (role) {
  case MIR_OPERAND_ROLE_VALUE:
    return "value";
  case MIR_OPERAND_ROLE_CALLEE:
    return "callee";
  case MIR_OPERAND_ROLE_SCRUTINEE:
    return "scrutinee";
  case MIR_OPERAND_ROLE_RETURN:
    return "return";
  case MIR_OPERAND_ROLE_CONDITION:
    return "condition";
  case MIR_OPERAND_ROLE_TAG:
    return "tag";
  case MIR_OPERAND_ROLE_CONTAINER:
    return "container";
  case MIR_OPERAND_ROLE_INDEX:
    return "index";
  case MIR_OPERAND_ROLE_ELEMENT:
    return "element";
  case MIR_OPERAND_ROLE_FIELD:
    return "field";
  case MIR_OPERAND_ROLE_FUNCTION:
    return "function";
  case MIR_OPERAND_ROLE_ENV:
    return "env";
  }
  return "unknown";
}

static const char *mir_operand_use_name(MirOperandUse use) {
  switch (use) {
  case MIR_OPERAND_USE_BORROW:
    return "borrow";
  case MIR_OPERAND_USE_CONSUME:
    return "consume";
  }
  return "unknown";
}

static const char *mir_result_ownership_name(MirResultOwnership ownership) {
  switch (ownership) {
  case MIR_RESULT_NONE:
    return "none";
  case MIR_RESULT_OWNED:
    return "owned";
  case MIR_RESULT_BORROWED:
    return "borrowed";
  }
  return "unknown";
}

typedef struct {
  FILE *stream;
  bool any;
} MirDumpOperandMetaCtx;

static bool dump_operand_meta_item(MirInstr *instr, MirOperand operand,
                                   void *ctx) {
  (void)instr;
  MirDumpOperandMetaCtx *dump_ctx = ctx;
  if (!dump_ctx || !dump_ctx->stream) {
    return true;
  }

  if (!dump_ctx->any) {
    fputs(STYLE_DIM " ; ops [", dump_ctx->stream);
    dump_ctx->any = true;
  } else {
    fputs(", ", dump_ctx->stream);
  }

  dump_value(dump_ctx->stream, operand.value);
  fprintf(dump_ctx->stream, ":%s/%s#%zu", mir_operand_role_name(operand.role),
          mir_operand_use_name(operand.use), operand.index);
  return true;
}

static void dump_operand_meta(FILE *stream, const MirInstr *instr) {
  MirDumpOperandMetaCtx ctx = {
      .stream = stream,
      .any = false,
  };
  mir_instr_for_each_operand((MirInstr *)instr, dump_operand_meta_item, &ctx);
  if (ctx.any) {
    fputs("]" STYLE_RESET_ALL, stream);
  }
}

static bool dump_term_operand_meta_item(MirInstr *instr, MirOperand operand,
                                        void *ctx) {
  return dump_operand_meta_item(NULL, operand, ctx);
}

static void dump_term_operand_meta(FILE *stream, const MirTerminator *term) {
  MirDumpOperandMetaCtx ctx = {
      .stream = stream,
      .any = false,
  };
  mir_term_for_each_operand((MirTerminator *)term, dump_term_operand_meta_item,
                            &ctx);
  if (ctx.any) {
    fputs("]" STYLE_RESET_ALL, stream);
  }
}

static void dump_param_summary(FILE *stream, const MirFunction *fn,
                               size_t index) {
  fprintf(stream, " @%s",
          mir_operand_use_name(mir_function_param_use(fn, index)));
}

static void dump_result_summary(FILE *stream, const MirFunction *fn) {
  fprintf(stream, " @%s",
          mir_result_ownership_name(mir_function_result_ownership(fn)));
}

static void dump_instr(FILE *stream, const MirFunction *fn,
                       const MirInstr *instr) {
  bool rc_instr =
      instr->kind == MIR_OP && (instr->data.op.kind == MIR_OP_KIND_DUP ||
                                instr->data.op.kind == MIR_OP_KIND_DROP);
  if (rc_instr) {
    fputs(COLOR_MAGENTA, stream);
  }
  fputs("    ", stream);
  dump_value(stream, instr->result);
  const char *name = mir_instr_name(instr->kind);
  if (instr->kind == MIR_OP && instr->data.op.kind == MIR_OP_KIND_PRIMITIVE) {
    name = mir_primitive_op_name(instr->data.op.primitive);
  } else if (instr->kind == MIR_OP) {
    name = mir_op_kind_name(instr->data.op.kind);
  } else if (instr->kind == MIR_EXTRACT) {
    name = mir_extract_kind_name(instr->data.extract.kind);
  } else if (instr->kind == MIR_CONSTRUCT) {
    name = mir_construct_kind_name(instr->data.construct.kind);
  } else if (instr->kind == MIR_CONST) {
    name = mir_const_kind_name(instr->data.const_value.kind);
  }
  fprintf(stream, " = %s", name);

  switch (instr->kind) {
  case MIR_CONST:
    switch (instr->data.const_value.kind) {
    case MIR_CONST_KIND_INT:
      fprintf(stream, " %d", instr->data.const_value.as.int_value);
      break;
    case MIR_CONST_KIND_UINT64:
      fprintf(stream, " %" PRIu64, instr->data.const_value.as.uint64_value);
      break;
    case MIR_CONST_KIND_FLOAT:
      fprintf(stream, " %g", (double)instr->data.const_value.as.float_value);
      break;
    case MIR_CONST_KIND_DOUBLE:
      fprintf(stream, " %g", instr->data.const_value.as.double_value);
      break;
    case MIR_CONST_KIND_CHAR:
      fputc(' ', stream);
      dump_char_literal(stream, instr->data.const_value.as.char_value);
      break;
    case MIR_CONST_KIND_BOOL:
      fprintf(stream, " %s",
              instr->data.const_value.as.bool_value ? "true" : "false");
      break;
    case MIR_CONST_KIND_STRING:
      fputc(' ', stream);
      dump_escaped_string(stream, instr->data.const_value.as.string_value.chars,
                          instr->data.const_value.as.string_value.len);
      break;
    case MIR_CONST_KIND_VOID:
    case MIR_CONST_KIND_UNDEF:
      break;
    }
    break;
  case MIR_PHI:
    fputs(" [", stream);
    for (size_t i = 0; i < instr->data.phi.incoming.len; i++) {
      if (i > 0) {
        fputs(", ", stream);
      }
      MirPhiIncoming incoming = instr->data.phi.incoming.items[i];
      dump_block_ref(stream, incoming.block);
      fputc(':', stream);
      fputc(' ', stream);
      dump_value(stream, incoming.value);
    }
    fputc(']', stream);
    if (instr->type) {
      fputs(STYLE_DIM " : ", stream);
      print_type_to_stream(instr->type, stream);
      fputs(STYLE_RESET_ALL, stream);
    }
    break;
  case MIR_OP:
    switch (instr->data.op.kind) {
    case MIR_OP_KIND_CAST:
      fputc(' ', stream);
      dump_value(stream, instr->data.op.operands[0]);
      fputs(" from ", stream);
      print_type_to_stream(instr->data.op.from_type, stream);
      fputs(" to ", stream);
      print_type_to_stream(instr->data.op.to_type, stream);
      break;
    case MIR_OP_KIND_TAG_EQ:
      fputc(' ', stream);
      dump_value(stream, instr->data.op.operands[0]);
      fprintf(stream, ", %s#%d",
              instr->data.op.constructor_name ? instr->data.op.constructor_name
                                              : "<constructor>",
              instr->data.op.constructor_index);
      break;
    case MIR_OP_KIND_PRIMITIVE:
      fputc(' ', stream);
      for (size_t i = 0; i < instr->data.op.argc; i++) {
        if (i > 0) {
          fputs(", ", stream);
        }
        dump_value(stream, instr->data.op.operands[i]);
      }
      break;
    case MIR_OP_KIND_GLOBAL_LOAD:
      fprintf(stream, " @%s", instr->data.op.global_name
                                  ? instr->data.op.global_name
                                  : "<global>");
      if (instr->type) {
        fputs(STYLE_DIM " : ", stream);
        print_type_to_stream(instr->type, stream);
        fputs(STYLE_RESET_ALL, stream);
      }
      break;
    case MIR_OP_KIND_GLOBAL_STORE:
      fprintf(stream, " @%s", instr->data.op.global_name
                                  ? instr->data.op.global_name
                                  : "<global>");
      for (size_t i = 0; i < instr->data.op.argc; i++) {
        fputs(", ", stream);
        dump_value(stream, instr->data.op.operands[i]);
      }
      if (instr->data.op.to_type) {
        fputs(STYLE_DIM " : ", stream);
        print_type_to_stream(instr->data.op.to_type, stream);
        fputs(STYLE_RESET_ALL, stream);
      }
      break;
    default:
      fputc(' ', stream);
      for (size_t i = 0; i < instr->data.op.argc; i++) {
        if (i > 0) {
          fputs(", ", stream);
        }
        dump_value(stream, instr->data.op.operands[i]);
      }
      if (instr->type) {
        fputs(STYLE_DIM " : ", stream);
        print_type_to_stream(instr->type, stream);
        fputs(STYLE_RESET_ALL, stream);
      }
      break;
    }
    break;
  case MIR_EXTRACT:
    fputc(' ', stream);
    switch (instr->data.extract.kind) {
    case MIR_EXTRACT_FIELD:
      dump_value(stream, instr->data.extract.value);
      fprintf(stream, ", %zu", instr->data.extract.index);
      if (instr->data.extract.name) {
        fprintf(stream, " ; %s", instr->data.extract.name);
      }
      break;
    case MIR_EXTRACT_VARIANT_PAYLOAD:
      dump_value(stream, instr->data.extract.value);
      fprintf(stream, ", %s#%d",
              instr->data.extract.constructor_name
                  ? instr->data.extract.constructor_name
                  : "<constructor>",
              instr->data.extract.constructor_index);
      break;
    case MIR_EXTRACT_ARRAY_AT:
      dump_value(stream, instr->data.extract.value);
      fputs(", ", stream);
      dump_value(stream, instr->data.extract.index_value);
      break;
    case MIR_EXTRACT_ARRAY_OFFSET:
      dump_value(stream, instr->data.extract.index_value);
      fputs(", ", stream);
      dump_value(stream, instr->data.extract.value);
      break;
    case MIR_EXTRACT_VARIANT_TAG:
    case MIR_EXTRACT_LIST_HEAD:
    case MIR_EXTRACT_LIST_TAIL:
    case MIR_EXTRACT_ARRAY_SUCC:
    case MIR_EXTRACT_CLOSURE_FN:
    case MIR_EXTRACT_CLOSURE_ENV:
      dump_value(stream, instr->data.extract.value);
      break;
    }
    if (instr->type) {
      fputs(STYLE_DIM " : ", stream);
      print_type_to_stream(instr->type, stream);
      fputs(STYLE_RESET_ALL, stream);
    }
    break;
  case MIR_CONSTRUCT:
    fputc(' ', stream);
    switch (instr->data.construct.kind) {
    case MIR_CONSTRUCT_TUPLE:
    case MIR_CONSTRUCT_CLOSURE_ENV:
      fputs("{ ", stream);
      dump_named_value_id_vec(stream, &instr->data.construct.items,
                              instr->type);
      fputs(" }", stream);
      break;
    case MIR_CONSTRUCT_VARIANT:
      fprintf(stream, "%s#%d(",
              instr->data.construct.constructor_name
                  ? instr->data.construct.constructor_name
                  : "<constructor>",
              instr->data.construct.constructor_index);
      dump_value_id_vec(stream, &instr->data.construct.items);
      fputc(')', stream);
      break;
    case MIR_CONSTRUCT_LIST_EMPTY:
      break;
    case MIR_CONSTRUCT_LIST_CONS:
      dump_value(stream, instr->data.construct.operands[0]);
      fputs(", ", stream);
      dump_value(stream, instr->data.construct.operands[1]);
      break;
    case MIR_CONSTRUCT_ARRAY_LITERAL:
      fputs("{ ", stream);
      dump_value_id_vec(stream, &instr->data.construct.items);
      fputs(" }", stream);
      break;
    case MIR_CONSTRUCT_ARRAY_FILL_CONST:
    case MIR_CONSTRUCT_ARRAY_FILL:
      dump_value(stream, instr->data.construct.operands[0]);
      fputs(", ", stream);
      dump_value(stream, instr->data.construct.operands[1]);
      break;
    case MIR_CONSTRUCT_ARRAY_RANGE:
      dump_value(stream, instr->data.construct.operands[0]);
      fputs(", ", stream);
      dump_value(stream, instr->data.construct.operands[1]);
      fputs(", ", stream);
      dump_value(stream, instr->data.construct.operands[2]);
      break;
    case MIR_CONSTRUCT_CLOSURE:
      fputs("{ env ", stream);
      dump_value(stream, instr->data.construct.operands[1]);
      fputs(", fn ", stream);
      dump_value(stream, instr->data.construct.operands[0]);
      fputs(" }", stream);
      if (instr->data.construct.impl_name) {
        fprintf(stream, " as $%s", instr->data.construct.impl_name);
      }
      break;
    }
    if (instr->type) {
      fputs(STYLE_DIM " : ", stream);
      print_type_to_stream(instr->type, stream);
      fputs(STYLE_RESET_ALL, stream);
    }
    break;
  case MIR_FN_REF:
    fprintf(stream, " $%s",
            instr->data.fn_ref.name ? instr->data.fn_ref.name : "<anonymous>");
    break;
  case MIR_CALL:
  case MIR_CORO_NEW:
  case MIR_CORO_NEXT:
    fputc(' ', stream);
    if (instr->data.call.builtin) {
      fprintf(stream, "@%s", instr->data.call.builtin->name);
    } else {
      dump_value(stream, instr->data.call.callee);
    }
    if (instr->data.call.specialized_name) {
      const char *prefix =
          instr->data.call.specialized_name[0] == '$' ? " as " : " as $";
      fprintf(stream, "%s%s", prefix, instr->data.call.specialized_name);
    }
    // if (instr->data.call.callee_type) {
    //   fputs(STYLE_DIM " : ", stream);
    //   dump_call_type(stream, instr->data.call.callee_type);
    //   fputs(STYLE_RESET_ALL, stream);
    // }

    fputc('(', stream);
    dump_value_id_vec(stream, &instr->data.call.operands);
    fputc(')', stream);

    if (instr->type) {
      fputs(STYLE_DIM " : ", stream);
      print_type_to_stream(instr->type, stream);
      fputs(STYLE_RESET_ALL, stream);
    }

    break;
  case MIR_CORO_RESET:
    fputc(' ', stream);
    dump_value(stream, instr->data.call.callee);
    if (instr->type) {
      fputs(STYLE_DIM " : ", stream);
      print_type_to_stream(instr->type, stream);
      fputs(STYLE_RESET_ALL, stream);
    }
    break;
  }

  dump_escape_meta(stream, fn, instr->result);
  dump_operand_meta(stream, instr);
  if (rc_instr) {
    fputs(STYLE_RESET_ALL, stream);
  }
  fputc('\n', stream);
}

static void dump_term(FILE *stream, const MirTerminator *term) {
  switch (term->kind) {
  case MIR_TERM_NONE:
    fprintf(stream, "    <no terminator>\n");
    break;
  case MIR_TERM_RETURN:
    fprintf(stream, "    return ");
    dump_value(stream, term->value);
    dump_term_operand_meta(stream, term);
    fputc('\n', stream);
    break;
  case MIR_TERM_BR:
    fputs("    br ", stream);
    dump_block_ref(stream, term->target);
    fputc('\n', stream);
    break;
  case MIR_TERM_COND:
    fputs("    cond ", stream);
    dump_value(stream, term->cond);
    fputs(", ", stream);
    dump_block_ref(stream, term->then_block);
    fputs(", ", stream);
    dump_block_ref(stream, term->else_block);
    dump_term_operand_meta(stream, term);
    fputc('\n', stream);
    break;
  case MIR_TERM_YIELD:
    fputs("    yield ", stream);
    dump_value(stream, term->value);
    fputs(", ", stream);
    dump_block_ref(stream, term->target);
    dump_term_operand_meta(stream, term);
    fputc('\n', stream);
    break;
  case MIR_TERM_CORO_RESTART:
    fputs("    coro.restart ", stream);
    dump_block_ref(stream, term->target);
    if (term->args.len > 0) {
      fputs("(", stream);
      dump_value_id_vec(stream, &term->args);
      fputs(")", stream);
    }
    dump_term_operand_meta(stream, term);
    fputc('\n', stream);
    break;
  case MIR_TERM_CORO_DONE:
    fputs("    coro.done\n", stream);
    break;
  case MIR_TERM_UNREACHABLE:
    fputs("    unreachable\n", stream);
    break;
  }
}

static Type *mir_dump_function_return_type(const MirFunction *fn) {
  if (!fn || !fn->type) {
    return NULL;
  }

  Type *type = fn->type;
  bool has_env_param = false;
  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (param->name && strcmp(param->name, "$env") == 0) {
      has_env_param = true;
      if (!(type && type->kind == T_FN && type->data.T_FN.from && param->type &&
            types_equal(type->data.T_FN.from, param->type))) {
        continue;
      }
    }
    if (type && type->kind == T_FN) {
      type = type->data.T_FN.to;
    }
  }

  if (has_env_param && type && type->kind == T_FN && type->data.T_FN.from &&
      type->data.T_FN.from->kind == T_VOID) {
    return type->data.T_FN.to;
  }
  return type;
}

void dump_function(FILE *stream, const MirFunction *fn) {
  if (!stream || !fn) {
    return;
  }

  fprintf(stream, "%sfn %s(", fn->is_extern ? "extern " : "",
          fn->name ? fn->name : "<anonymous>");
  for (size_t i = 0; i < fn->params.len; i++) {
    MirParam *param = &fn->params.items[i];
    if (i > 0) {
      fputs(", ", stream);
    }
    dump_value(stream, param->value);
    if (param->name) {
      fprintf(stream, " %s", param->name);
    }
    if (param->type) {
      fputs(STYLE_DIM ": ", stream);
      if (param->name && strcmp(param->name, "$env") == 0) {
        fputc('&', stream);
      }
      print_type_to_stream(param->type, stream);
      dump_param_summary(stream, fn, i);
      fputs(STYLE_RESET_ALL, stream);
    } else {
      fputs(STYLE_DIM, stream);
      dump_param_summary(stream, fn, i);
      fputs(STYLE_RESET_ALL, stream);
    }
  }
  fputc(')', stream);
  Type *return_type = mir_dump_function_return_type(fn);
  if (return_type) {
    fputs(STYLE_DIM " -> ", stream);
    print_type_to_stream(return_type, stream);
    dump_result_summary(stream, fn);
    fputs(STYLE_RESET_ALL, stream);
  }
  if (fn->is_extern) {
    fputs(";\n", stream);
    return;
  }
  fputs(" {\n", stream);
  for (size_t i = 0; i < fn->blocks.len; i++) {
    const MirBlock *block = fn->blocks.items[i];
    fprintf(stream, "  bb%u", block->id);
    if (block->name) {
      fprintf(stream, " %s", block->name);
    }
    fputs(":\n", stream);
    for (size_t j = 0; j < block->instrs.len; j++) {
      dump_instr(stream, fn, &block->instrs.items[j]);
    }
    dump_term(stream, &block->term);
  }
  fputs("}\n", stream);
}

static bool mir_build_test_top(MirProgram *program, Ast *prog,
                               MirCtx *root_ctx) {
  Ast *test_module = mir_get_test_module_ast(prog);
  AstList *stmts =
      mir_test_module_stmts(program ? program->arena : NULL, test_module);
  if (!program || !root_ctx || !test_module || !stmts) {
    return false;
  }

  MirFunction *top = mir_program_add_function(program, "$top", &t_bool, prog);
  MirBlock *entry = mir_function_add_block(top, "entry");
  if (!top || !entry) {
    return false;
  }

  MirBuilder builder;
  mir_builder_init(&builder, program, top);
  mir_builder_position_at_end(&builder, entry);

  ht test_table;
  MirStackFrame test_frame;
  mir_stack_frame_init(program->arena, &test_table, &test_frame, NULL);
  MirModuleId test_module_id = root_ctx->current_module;
  MirSymbol *test_symbol =
      mir_module_lookup_symbol(program, root_ctx->current_module, "test", true);
  if (test_symbol && test_symbol->kind == MIR_SYMBOL_MODULE) {
    test_module_id = test_symbol->as.module;
  }
  MirCtx test_ctx = {
      .env = root_ctx->env,
      .frame = &test_frame,
      .current_module = test_module_id,
      .export_bindings = false,
  };

  MirValueId result = mir_const_bool(&builder, &t_bool, test_module, true);
  if (result == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(&builder);
    return false;
  }

  Type *report_result_type = type_fn(&t_ptr, type_fn(&t_bool, &t_void));
  Type *report_totals_type = type_fn(&t_int, type_fn(&t_int, &t_void));
  MirFunction *report_result_fn = mir_program_add_extern_function(
      program, "_report_test_result", report_result_type, test_module);
  MirFunction *report_totals_fn = mir_program_add_extern_function(
      program, "_report_test_totals", report_totals_type, test_module);
  if (!report_result_fn || !report_totals_fn) {
    mir_builder_set_unreachable_if_open(&builder);
    return false;
  }
  MirValueId report_result_ref =
      mir_fn_ref(&builder, report_result_type, test_module, report_result_fn);
  MirValueId report_totals_ref =
      mir_fn_ref(&builder, report_totals_type, test_module, report_totals_fn);
  MirValueId num_tests = mir_const_int(&builder, &t_int, test_module, 0);
  MirValueId num_passes = mir_const_int(&builder, &t_int, test_module, 0);
  if (report_result_ref == MIR_NO_VALUE || report_totals_ref == MIR_NO_VALUE ||
      num_tests == MIR_NO_VALUE || num_passes == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(&builder);
    return false;
  }

  for (AstList *item = stmts; item; item = item->next) {
    Ast *stmt = item->ast;
    if (!mir_is_test_binding(stmt)) {
      if (stmt && stmt->tag == AST_TYPE_DECL) {
        continue;
      }
      MirValueId setup = mir_expr(&builder, stmt, &test_ctx);
      if (setup == MIR_NO_VALUE) {
        mir_builder_set_unreachable_if_open(&builder);
        return false;
      }
      continue;
    }

    Ast *expr = stmt->data.AST_LET.expr;
    MirValueId value = MIR_NO_VALUE;
    if (expr && expr->tag == AST_LAMBDA) {
      MirValueId fn_ref = mir_let(&builder, stmt, &test_ctx);
      if (fn_ref == MIR_NO_VALUE) {
        mir_builder_set_unreachable_if_open(&builder);
        return false;
      }

      MirInstr call = mir_make_instr(MIR_CALL, &t_bool, stmt);
      call.data.call.callee = fn_ref;
      call.data.call.builtin = NULL;
      call.data.call.specialized_fn = MIR_NO_FUNCTION;
      call.data.call.callee_type = expr->type;
      mir_call_apply_callee_summary(&builder, &call);
      value = mir_builder_append_instr(&builder, call);
    } else if (expr && types_equal(expr->type, &t_bool)) {
      value = mir_expr(&builder, expr, &test_ctx);
      if (value != MIR_NO_VALUE &&
          !mir_bind_pattern(&builder, &test_ctx, stmt->data.AST_LET.binding,
                            value, expr->type)) {
        value = MIR_NO_VALUE;
      }
    }

    if (value == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&builder);
      return false;
    }

    MirValueId one = mir_const_int(&builder, &t_int, stmt, 1);
    MirValueId pass_increment =
        mir_primitive_cast(&builder, &t_bool, &t_int, stmt, value);
    if (one == MIR_NO_VALUE || pass_increment == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&builder);
      return false;
    }
    num_tests = mir_iadd(&builder, &t_int, stmt, num_tests, one);
    num_passes = mir_iadd(&builder, &t_int, stmt, num_passes, pass_increment);
    if (num_tests == MIR_NO_VALUE || num_passes == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&builder);
      return false;
    }

    const char *test_name =
        stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;
    MirValueId name =
        mir_const_string(&builder, &t_ptr, stmt, test_name, strlen(test_name));
    if (name == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&builder);
      return false;
    }

    MirInstr report_call = mir_make_instr(MIR_CALL, &t_void, stmt);
    report_call.data.call.callee = report_result_ref;
    report_call.data.call.builtin = NULL;
    report_call.data.call.specialized_fn = MIR_NO_FUNCTION;
    report_call.data.call.callee_type = report_result_type;
    mir_call_push_operand(program->arena, &report_call, name,
                          MIR_OPERAND_USE_BORROW);
    mir_call_push_operand(program->arena, &report_call, value,
                          MIR_OPERAND_USE_BORROW);
    mir_call_apply_callee_summary(&builder, &report_call);
    if (mir_builder_append_instr(&builder, report_call) == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&builder);
      return false;
    }

    result = mir_bool_and_values(&builder, stmt, result, value);
    if (result == MIR_NO_VALUE) {
      mir_builder_set_unreachable_if_open(&builder);
      return false;
    }
  }

  MirInstr totals_call = mir_make_instr(MIR_CALL, &t_void, test_module);
  totals_call.data.call.callee = report_totals_ref;
  totals_call.data.call.builtin = NULL;
  totals_call.data.call.specialized_fn = MIR_NO_FUNCTION;
  totals_call.data.call.callee_type = report_totals_type;
  mir_call_push_operand(program->arena, &totals_call, num_passes,
                        MIR_OPERAND_USE_BORROW);
  mir_call_push_operand(program->arena, &totals_call, num_tests,
                        MIR_OPERAND_USE_BORROW);
  mir_call_apply_callee_summary(&builder, &totals_call);
  if (mir_builder_append_instr(&builder, totals_call) == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(&builder);
    return false;
  }

  if (builder.block && builder.block->term.kind == MIR_TERM_NONE) {
    mir_builder_set_return(&builder, result);
  }
  return true;
}

MirProgram *mir_build_program(MirArena *arena, Ast *prog, MirCtx *ctx) {
  if (!arena) {
    return NULL;
  }

  MirCtx fallback_ctx = {.env = NULL, .current_module = MIR_NO_MODULE};
  if (!ctx) {
    ctx = &fallback_ctx;
  }

  MirCtx build_ctx = *ctx;
  ht root_table;
  MirStackFrame root_frame;
  if (!build_ctx.frame) {
    mir_stack_frame_init(arena, &root_table, &root_frame, NULL);
    build_ctx.frame = &root_frame;
  }

  MirProgram *program =
      mir_arena_alloc(arena, sizeof(MirProgram), MIR_ALIGNOF(MirProgram));

  if (!program) {
    return NULL;
  }

  memset(program, 0, sizeof(MirProgram));
  program->arena = arena;
  program->type_env = build_ctx.env;
  ht_init_with_allocator(
      &program->builtins,
      (ht_allocator){.alloc = mir_ht_alloc, .free = NULL, .ctx = arena});
  mir_register_core_builtins(program);
  MirModule *root_module = mir_program_add_module(
      program, NULL, prog ? prog->type : NULL, prog, MIR_NO_MODULE);
  program->root_module = root_module ? root_module->id : MIR_NO_MODULE;
  build_ctx.current_module = program->root_module;
  build_ctx.export_bindings = true;

  MirFunction *top = mir_program_add_function(
      program, ylc_config.test_mode ? "$module_init" : "$top",
      ylc_config.test_mode ? &t_void
      : prog               ? prog->type
                           : NULL,
      prog);
  MirBlock *entry = mir_function_add_block(top, "entry");

  if (!top || !entry) {
    return program;
  }

  MirBuilder builder;
  mir_builder_init(&builder, program, top);
  mir_builder_position_at_end(&builder, entry);

  MirValueId result = mir_expr(&builder, prog, &build_ctx);

  if (result != MIR_NO_VALUE && builder.block &&
      builder.block->term.kind == MIR_TERM_NONE) {
    mir_builder_set_return(&builder, result);
  } else if (result == MIR_NO_VALUE) {
    mir_builder_set_unreachable_if_open(&builder);
  }

  if (ylc_config.test_mode) {
    mir_build_test_top(program, prog, &build_ctx);
  }

  return program;
}

bool mir_program_had_error(MirProgram *program) {
  return !program || program->had_error;
}

void mir_program_destroy(MirProgram *program) { (void)program; }

void mir_run_passes(MirProgram *program) {
  if (mir_program_had_error(program)) {
    return;
  }
  mir_resolve_named_extract_fields_program(program);
  mir_escape_analysis(program);
  if (ylc_config.perceus_rc) {
    mir_perceus_instrumentation(program);
  }
}

void mir_dump_program(MirProgram *program, FILE *stream) {
  if (!program || !stream) {
    return;
  }

  fprintf(stream, "# ylc-mir\n");
  for (size_t i = 0; i < program->functions.len; i++) {
    dump_function(stream, program->functions.items[i]);
  }
}

int mir(Ast *prog, TypeEnv *type_env) {
  MirArena *arena = mir_arena_create();
  if (!arena) {
    return 0;
  }
  ht table;
  MirStackFrame initial_stack_frame;
  mir_stack_frame_init(arena, &table, &initial_stack_frame, NULL);
  MirCtx ctx = {.env = type_env, .frame = &initial_stack_frame};

  MirProgram *program = mir_build_program(arena, prog, &ctx);

  if (!program) {
    mir_arena_destroy(arena);
    return 0;
  }
  if (mir_program_had_error(program)) {
    mir_program_destroy(program);
    mir_arena_destroy(arena);
    return 0;
  }

  mir_run_passes(program);
  if (mir_program_had_error(program)) {
    mir_program_destroy(program);
    mir_arena_destroy(arena);
    return 0;
  }
  if (ylc_config.dump_mir) {
    mir_dump_program(program, stdout);
  }

  mir_program_destroy(program);
  mir_arena_destroy(arena);
  return 1;
}
