#include "mir/mir_internal.h"
#include "config.h"
#include "escape_analysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIR_ARENA_DEFAULT_BLOCK_SIZE 32768
#define MIR_ALIGNOF(T) __alignof__(T)

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

char *mir_arena_strdup(MirArena *arena, const char *str) {
  if (!str) {
    return NULL;
  }
  size_t len = strlen(str);
  char *copy = mir_arena_alloc(arena, len + 1, MIR_ALIGNOF(char));
  if (!copy) {
    return NULL;
  }
  memcpy(copy, str, len + 1);
  return copy;
}

static void *mir_vec_grow(MirArena *arena, void *items, size_t len,
                          size_t *cap, size_t elem_size, size_t align,
                          size_t min_cap) {
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

#define MIR_VEC_PUSH(arena, vec, T, value)                                      \
  do {                                                                         \
    if ((vec)->len == (vec)->cap) {                                             \
      (vec)->items = mir_vec_grow((arena), (vec)->items, (vec)->len,            \
                                  &(vec)->cap, sizeof(T), MIR_ALIGNOF(T),       \
                                  (vec)->len + 1);                              \
    }                                                                          \
    (vec)->items[(vec)->len++] = (value);                                       \
  } while (0)

static const char *ast_tag_name(ast_tag tag) {
  switch (tag) {
  case AST_INT:
    return "AST_INT";
  case AST_FLOAT:
    return "AST_FLOAT";
  case AST_DOUBLE:
    return "AST_DOUBLE";
  case AST_STRING:
    return "AST_STRING";
  case AST_CHAR:
    return "AST_CHAR";
  case AST_BOOL:
    return "AST_BOOL";
  case AST_UINT64:
    return "AST_UINT64";
  case AST_IDENTIFIER:
    return "AST_IDENTIFIER";
  case AST_BODY:
    return "AST_BODY";
  case AST_LET:
    return "AST_LET";
  case AST_BINOP:
    return "AST_BINOP";
  case AST_UNOP:
    return "AST_UNOP";
  case AST_APPLICATION:
    return "AST_APPLICATION";
  case AST_TUPLE:
    return "AST_TUPLE";
  case AST_LAMBDA:
    return "AST_LAMBDA";
  case AST_VOID:
    return "AST_VOID";
  case AST_EXTERN_FN:
    return "AST_EXTERN_FN";
  case AST_LIST:
    return "AST_LIST";
  case AST_EMPTY_CONTAINER:
    return "AST_EMPTY_CONTAINER";
  case AST_ARRAY:
    return "AST_ARRAY";
  case AST_MATCH:
    return "AST_MATCH";
  case AST_PLACEHOLDER_ID:
    return "AST_PLACEHOLDER_ID";
  case AST_IMPORT:
    return "AST_IMPORT";
  case AST_RECORD_ACCESS:
    return "AST_RECORD_ACCESS";
  case AST_FMT_STRING:
    return "AST_FMT_STRING";
  case AST_TYPE_DECL:
    return "AST_TYPE_DECL";
  case AST_ASSOC:
    return "AST_ASSOC";
  case AST_EXTERN_VARIANTS:
    return "AST_EXTERN_VARIANTS";
  case AST_FN_SIGNATURE:
    return "AST_FN_SIGNATURE";
  case AST_MATCH_GUARD_CLAUSE:
    return "AST_MATCH_GUARD_CLAUSE";
  case AST_YIELD:
    return "AST_YIELD";
  case AST_SPREAD_OP:
    return "AST_SPREAD_OP";
  case AST_IMPLEMENTS:
    return "AST_IMPLEMENTS";
  case AST_MODULE:
    return "AST_MODULE";
  case AST_RANGE_EXPRESSION:
    return "AST_RANGE_EXPRESSION";
  case AST_LOOP:
    return "AST_LOOP";
  case AST_GET_ARG:
    return "AST_GET_ARG";
  case AST_TRAIT_IMPL:
    return "AST_TRAIT_IMPL";
  }
  return "AST_UNKNOWN";
}

static const char *type_kind_name(Type *type) {
  if (!type) {
    return "<no-type>";
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
    return "()";
  case T_STRING:
    return "String";
  case T_FN:
    return "Fn";
  case T_CONS:
    return type->data.T_CONS.name ? type->data.T_CONS.name : "Cons";
  case T_SUM:
    return type->data.T_CONS.name ? type->data.T_CONS.name : "Sum";
  case T_VAR:
    return type->data.T_VAR.name ? type->data.T_VAR.name : "TypeVar";
  case T_RECURSIVE_REF:
    return type->data.T_RECURSIVE_REF.name ? type->data.T_RECURSIVE_REF.name
                                           : "RecursiveRef";
  case T_EMPTY_LIST:
    return "EmptyList";
  case T_MODULE:
    return "Module";
  }
  return "Type";
}

static const char *token_name(token_type op) {
  switch (op) {
  case TOKEN_PLUS:
    return "+";
  case TOKEN_MINUS:
    return "-";
  case TOKEN_STAR:
    return "*";
  case TOKEN_SLASH:
    return "/";
  case TOKEN_MODULO:
    return "%";
  case TOKEN_LT:
    return "<";
  case TOKEN_GT:
    return ">";
  case TOKEN_LTE:
    return "<=";
  case TOKEN_GTE:
    return ">=";
  case TOKEN_EQUALITY:
    return "==";
  case TOKEN_NOT_EQUAL:
    return "!=";
  case TOKEN_LOGICAL_AND:
  case TOKEN_DOUBLE_AMP:
    return "&&";
  case TOKEN_LOGICAL_OR:
  case TOKEN_DOUBLE_PIPE:
    return "||";
  case TOKEN_BANG:
    return "!";
  default:
    return "?";
  }
}

static const char *instr_name(MirInstrKind kind) {
  switch (kind) {
  case MIR_CONST_INT:
    return "const_int";
  case MIR_CONST_UINT64:
    return "const_uint64";
  case MIR_CONST_DOUBLE:
    return "const_double";
  case MIR_CONST_CHAR:
    return "const_char";
  case MIR_CONST_BOOL:
    return "const_bool";
  case MIR_CONST_VOID:
    return "const_void";
  case MIR_STRING_LITERAL:
    return "string_literal";
  case MIR_PARAM:
    return "param";
  case MIR_FUNCTION_REF:
    return "function_ref";
  case MIR_LOAD_LOCAL:
    return "load_local";
  case MIR_LOAD_SYMBOL:
    return "load_symbol";
  case MIR_ARRAY_LITERAL:
    return "array_literal";
  case MIR_LIST_LITERAL:
    return "list_literal";
  case MIR_TUPLE:
    return "tuple";
  case MIR_BINOP:
    return "binop";
  case MIR_UNOP:
    return "unop";
  case MIR_CALL:
    return "call";
  case MIR_RECORD_GET:
    return "record_get";
  case MIR_CLOSURE:
    return "closure";
  case MIR_YIELD:
    return "yield";
  case MIR_MOVE:
    return "move";
  case MIR_BORROW:
    return "borrow";
  case MIR_DUP_IF_MANAGED:
    return "dup_if_managed";
  case MIR_DROP_IF_MANAGED:
    return "drop_if_managed";
  case MIR_DECREF_IF_MANAGED:
    return "decref_if_managed";
  case MIR_DROP_REUSE_IF_UNIQUE:
    return "drop_reuse_if_unique";
  case MIR_UNSUPPORTED:
    return "unsupported";
  }
  return "unknown";
}

static const char *placement_name(MirPlacement placement) {
  switch (placement) {
  case MIR_PLACE_UNKNOWN:
    return "unknown";
  case MIR_PLACE_STACK:
    return "stack";
  case MIR_PLACE_HEAP:
    return "heap";
  case MIR_PLACE_CORO_FRAME:
    return "coro_frame";
  }
  return "unknown";
}

static MirPlacement placement_from_escape_meta(Ast *ast) {
  if (!ast || !ast->ea_md) {
    return MIR_PLACE_UNKNOWN;
  }
  if (ast->ea_md->status == EA_STACK_ALLOC) {
    return MIR_PLACE_STACK;
  }
  if (ast->ea_md->status == EA_HEAP_ALLOC) {
    return MIR_PLACE_HEAP;
  }
  return MIR_PLACE_UNKNOWN;
}

static bool mutable_from_escape_meta(Ast *ast) {
  return ast && ast->ea_md && (ast->ea_md->attributes & EA_ATTR_MUTABLE);
}

MirInstr mir_make_instr(MirInstrKind kind, Type *type, Ast *origin) {
  MirInstr instr;
  memset(&instr, 0, sizeof(instr));
  instr.kind = kind;
  instr.result = MIR_NO_VALUE;
  instr.type = type;
  instr.origin = origin;
  instr.alloc_id = MIR_NO_ALLOC;
  instr.op = TOKEN_START;
  instr.member_index = -1;
  return instr;
}

void mir_value_id_vec_push(MirArena *arena, MirValueIdVec *vec,
                           MirValueId value) {
  MIR_VEC_PUSH(arena, vec, MirValueId, value);
}

MirValueId mir_create_value(MirFunction *fn, Type *type, Ast *origin) {
  if (!fn) {
    return MIR_NO_VALUE;
  }
  MirValueId id = (MirValueId)fn->values.len;
  MirValue value = {.id = id, .type = type, .origin = origin};
  MIR_VEC_PUSH(fn->arena, &fn->values, MirValue, value);
  return id;
}

MirAllocId mir_create_alloc_site(MirFunction *fn, Ast *origin) {
  if (!fn) {
    return MIR_NO_ALLOC;
  }
  MirAllocId id = (MirAllocId)fn->allocs.len;
  MirAllocSite site = {.id = id,
                       .placement = placement_from_escape_meta(origin),
                       .is_mutable = mutable_from_escape_meta(origin),
                       .origin = origin};
  MIR_VEC_PUSH(fn->arena, &fn->allocs, MirAllocSite, site);
  return id;
}

static MirValueId prepare_inserted_instr(MirFunction *fn, MirInstr *instr) {
  if (!instr) {
    return MIR_NO_VALUE;
  }
  if (instr->result == MIR_NO_VALUE && instr->type) {
    instr->result = mir_create_value(fn, instr->type, instr->origin);
  }
  return instr->result;
}

MirValueId mir_append_instr(MirFunction *fn, MirBlock *block, MirInstr instr) {
  if (!fn || !block) {
    return MIR_NO_VALUE;
  }
  MirValueId result = prepare_inserted_instr(fn, &instr);
  MIR_VEC_PUSH(fn->arena, &block->instrs, MirInstr, instr);
  return result;
}

MirValueId mir_insert_instr_before(MirFunction *fn, MirBlock *block,
                                   size_t index, MirInstr instr) {
  if (!fn || !block) {
    return MIR_NO_VALUE;
  }
  MirValueId result = prepare_inserted_instr(fn, &instr);
  if (index > block->instrs.len) {
    index = block->instrs.len;
  }
  if (block->instrs.len == block->instrs.cap) {
    block->instrs.items = mir_vec_grow(fn->arena, block->instrs.items,
                                       block->instrs.len, &block->instrs.cap,
                                       sizeof(MirInstr), MIR_ALIGNOF(MirInstr),
                                       block->instrs.len + 1);
  }
  memmove(block->instrs.items + index + 1, block->instrs.items + index,
          sizeof(MirInstr) * (block->instrs.len - index));
  block->instrs.items[index] = instr;
  block->instrs.len++;
  return result;
}

MirValueId mir_insert_instr_after(MirFunction *fn, MirBlock *block, size_t index,
                                  MirInstr instr) {
  if (!block || index >= block->instrs.len) {
    return mir_append_instr(fn, block, instr);
  }
  return mir_insert_instr_before(fn, block, index + 1, instr);
}

void mir_run_function_pass(MirProgram *program, MirFunctionPass pass,
                           void *ctx) {
  if (!program || !pass) {
    return;
  }
  for (size_t i = 0; i < program->functions.len; i++) {
    pass(program->functions.items[i], ctx);
  }
}

static MirFunction *mir_new_function(MirProgram *program, const char *name,
                                     Type *type, Ast *origin) {
  MirFunction *fn =
      mir_arena_alloc(program->arena, sizeof(MirFunction), MIR_ALIGNOF(MirFunction));
  memset(fn, 0, sizeof(MirFunction));
  fn->arena = program->arena;
  fn->name = name && name[0] ? name : "anonymous";
  fn->type = type;
  fn->origin = origin;
  MIR_VEC_PUSH(program->arena, &program->functions, MirFunction *, fn);
  return fn;
}

static MirBlock *mir_append_block(MirFunction *fn, const char *name) {
  MirBlock *block =
      mir_arena_alloc(fn->arena, sizeof(MirBlock), MIR_ALIGNOF(MirBlock));
  memset(block, 0, sizeof(MirBlock));
  block->id = (MirBlockId)fn->blocks.len;
  block->name = name ? name : "bb";
  block->term = (MirTerminator){.kind = MIR_TERM_NONE,
                                .value = MIR_NO_VALUE,
                                .target = 0,
                                .then_target = 0,
                                .else_target = 0};
  MIR_VEC_PUSH(fn->arena, &fn->blocks, MirBlock *, block);
  return block;
}

static void mir_builder_init(MirBuilder *builder, MirProgram *program,
                             MirFunction *fn) {
  memset(builder, 0, sizeof(*builder));
  builder->program = program;
  builder->fn = fn;
  builder->block = mir_append_block(fn, "entry");
  MirScope scope = {0};
  MIR_VEC_PUSH(fn->arena, &builder->scopes, MirScope, scope);
}

static MirValueId mir_new_alloc_site(MirBuilder *builder, Ast *origin) {
  return mir_create_alloc_site(builder->fn, origin);
}

static MirValueId mir_builder_append_instr(MirBuilder *builder, MirInstr instr) {
  if (!builder->block || builder->block->term.kind != MIR_TERM_NONE) {
    return instr.result;
  }
  return mir_append_instr(builder->fn, builder->block, instr);
}

static void mir_set_return(MirBuilder *builder, MirValueId value) {
  if (!builder->block || builder->block->term.kind != MIR_TERM_NONE) {
    return;
  }
  builder->block->term = (MirTerminator){.kind = MIR_TERM_RETURN,
                                         .value = value,
                                         .target = 0,
                                         .then_target = 0,
                                         .else_target = 0};
}

static void mir_push_scope(MirBuilder *builder) {
  MirScope scope = {0};
  MIR_VEC_PUSH(builder->fn->arena, &builder->scopes, MirScope, scope);
}

static void mir_pop_scope(MirBuilder *builder) {
  if (builder->scopes.len > 1) {
    builder->scopes.len--;
  }
}

static void mir_bind(MirBuilder *builder, const char *name, MirValueId value) {
  if (!name || value == MIR_NO_VALUE || builder->scopes.len == 0) {
    return;
  }
  MirScope *scope = &builder->scopes.items[builder->scopes.len - 1];
  MirBinding binding = {.name = name, .value = value};
  MIR_VEC_PUSH(builder->fn->arena, &scope->bindings, MirBinding, binding);
}

static bool mir_lookup(MirBuilder *builder, const char *name, MirValueId *out) {
  if (!name) {
    return false;
  }
  for (size_t s = builder->scopes.len; s > 0; s--) {
    MirScope *scope = &builder->scopes.items[s - 1];
    for (size_t i = scope->bindings.len; i > 0; i--) {
      MirBinding *binding = &scope->bindings.items[i - 1];
      if (binding->name && strcmp(binding->name, name) == 0) {
        *out = binding->value;
        return true;
      }
    }
  }
  return false;
}

static const char *lambda_name(Ast *ast, const char *fallback) {
  if (ast && (ast->tag == AST_LAMBDA || ast->tag == AST_MODULE) &&
      ast->data.AST_LAMBDA.fn_name.chars) {
    return ast->data.AST_LAMBDA.fn_name.chars;
  }
  return fallback ? fallback : "anonymous";
}

static MirValueId mir_emit_expr(MirBuilder *builder, Ast *ast);

static MirValueId mir_emit_unsupported(MirBuilder *builder, Ast *ast) {
  MirInstr instr =
      mir_make_instr(MIR_UNSUPPORTED, ast ? ast->type : NULL, ast);
  instr.name = ast ? ast_tag_name(ast->tag) : "<null>";
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_emit_lambda_function(MirProgram *program, Ast *ast,
                                           const char *name) {
  MirFunction *fn =
      mir_new_function(program, lambda_name(ast, name), ast->type, ast);
  MirBuilder fn_builder;
  mir_builder_init(&fn_builder, program, fn);

  Type *fn_type_cursor = ast->type;
  AST_LIST_ITER(ast->data.AST_LAMBDA.params, ({
                  Ast *param_ast = l->ast;
                  Type *param_type =
                      fn_type_cursor && fn_type_cursor->kind == T_FN
                          ? fn_type_cursor->data.T_FN.from
                          : param_ast->type;
                  const char *param_name =
                      param_ast->tag == AST_IDENTIFIER
                          ? param_ast->data.AST_IDENTIFIER.value
                          : "param";
                  MirInstr instr = mir_make_instr(MIR_PARAM, param_type, param_ast);
                  instr.name = param_name;
                  instr.member_index = i;
                  MirValueId param = mir_builder_append_instr(&fn_builder, instr);
                  mir_bind(&fn_builder, param_name, param);
                  if (fn_type_cursor && fn_type_cursor->kind == T_FN) {
                    fn_type_cursor = fn_type_cursor->data.T_FN.to;
                  }
                }));

  MirValueId body = mir_emit_expr(&fn_builder, ast->data.AST_LAMBDA.body);
  mir_set_return(&fn_builder, body);
  return MIR_NO_VALUE;
}

static MirValueId mir_emit_lambda_value(MirBuilder *builder, Ast *ast,
                                        const char *name) {
  mir_emit_lambda_function(builder->program, ast, name);
  MirInstr instr = mir_make_instr(MIR_FUNCTION_REF, ast->type, ast);
  instr.name = lambda_name(ast, name);
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_emit_body(MirBuilder *builder, Ast *ast) {
  MirValueId last = MIR_NO_VALUE;
  for (AstList *l = ast->data.AST_BODY.stmts; l != NULL; l = l->next) {
    last = mir_emit_expr(builder, l->ast);
  }
  if (last == MIR_NO_VALUE) {
    return mir_builder_append_instr(
        builder, mir_make_instr(MIR_CONST_VOID, ast->type, ast));
  }
  return last;
}

static MirValueId mir_emit_let(MirBuilder *builder, Ast *ast) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  const char *binding_name =
      binding && binding->tag == AST_IDENTIFIER ? binding->data.AST_IDENTIFIER.value
                                                : NULL;

  MirValueId value = MIR_NO_VALUE;
  if (expr && (expr->tag == AST_LAMBDA || expr->tag == AST_MODULE)) {
    value = mir_emit_lambda_value(builder, expr, binding_name);
  } else {
    value = mir_emit_expr(builder, expr);
  }

  if (binding_name) {
    mir_bind(builder, binding_name, value);
  }

  if (ast->data.AST_LET.in_expr) {
    mir_push_scope(builder);
    if (binding_name) {
      mir_bind(builder, binding_name, value);
    }
    MirValueId in_value = mir_emit_expr(builder, ast->data.AST_LET.in_expr);
    mir_pop_scope(builder);
    return in_value;
  }

  return value;
}

static MirValueId mir_emit_sequence_literal(MirBuilder *builder, Ast *ast,
                                            MirInstrKind kind) {
  MirValueIdVec items = {0};
  for (size_t i = 0; i < ast->data.AST_LIST.len; i++) {
    mir_value_id_vec_push(builder->fn->arena, &items,
                          mir_emit_expr(builder, ast->data.AST_LIST.items + i));
  }

  MirInstr instr = mir_make_instr(kind, ast->type, ast);
  instr.operands = items;
  if (kind != MIR_TUPLE) {
    instr.alloc_id = mir_new_alloc_site(builder, ast);
  }
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_emit_application(MirBuilder *builder, Ast *ast) {
  MirValueIdVec operands = {0};
  mir_value_id_vec_push(builder->fn->arena, &operands,
                        mir_emit_expr(builder,
                                      ast->data.AST_APPLICATION.function));
  for (size_t i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    mir_value_id_vec_push(builder->fn->arena, &operands,
                          mir_emit_expr(builder,
                                        ast->data.AST_APPLICATION.args + i));
  }

  MirInstr instr = mir_make_instr(MIR_CALL, ast->type, ast);
  instr.operands = operands;
  return mir_builder_append_instr(builder, instr);
}

static MirValueId mir_emit_expr(MirBuilder *builder, Ast *ast) {
  if (!ast) {
    return MIR_NO_VALUE;
  }

  switch (ast->tag) {
  case AST_BODY:
    return mir_emit_body(builder, ast);
  case AST_LET:
  case AST_LOOP:
    return mir_emit_let(builder, ast);
  case AST_INT: {
    MirInstr instr = mir_make_instr(MIR_CONST_INT, ast->type, ast);
    instr.imm.int_value = ast->data.AST_INT.value;
    return mir_builder_append_instr(builder, instr);
  }
  case AST_UINT64: {
    MirInstr instr = mir_make_instr(MIR_CONST_UINT64, ast->type, ast);
    instr.imm.uint64_value = ast->data.AST_UINT64.value;
    return mir_builder_append_instr(builder, instr);
  }
  case AST_DOUBLE: {
    MirInstr instr = mir_make_instr(MIR_CONST_DOUBLE, ast->type, ast);
    instr.imm.double_value = ast->data.AST_DOUBLE.value;
    return mir_builder_append_instr(builder, instr);
  }
  case AST_CHAR: {
    MirInstr instr = mir_make_instr(MIR_CONST_CHAR, ast->type, ast);
    instr.imm.char_value = ast->data.AST_CHAR.value;
    return mir_builder_append_instr(builder, instr);
  }
  case AST_BOOL: {
    MirInstr instr = mir_make_instr(MIR_CONST_BOOL, ast->type, ast);
    instr.imm.bool_value = ast->data.AST_BOOL.value;
    return mir_builder_append_instr(builder, instr);
  }
  case AST_VOID:
    return mir_builder_append_instr(
        builder, mir_make_instr(MIR_CONST_VOID, ast->type, ast));
  case AST_STRING: {
    MirInstr instr = mir_make_instr(MIR_STRING_LITERAL, ast->type, ast);
    instr.alloc_id = mir_new_alloc_site(builder, ast);
    instr.name = ast->data.AST_STRING.value;
    instr.member_index = (int)ast->data.AST_STRING.length;
    return mir_builder_append_instr(builder, instr);
  }
  case AST_IDENTIFIER: {
    MirValueId source = MIR_NO_VALUE;
    const char *name = ast->data.AST_IDENTIFIER.value;
    if (mir_lookup(builder, name, &source)) {
      MirInstr instr = mir_make_instr(MIR_LOAD_LOCAL, ast->type, ast);
      instr.name = name;
      mir_value_id_vec_push(builder->fn->arena, &instr.operands, source);
      return mir_builder_append_instr(builder, instr);
    }
    MirInstr instr = mir_make_instr(MIR_LOAD_SYMBOL, ast->type, ast);
    instr.name = name;
    return mir_builder_append_instr(builder, instr);
  }
  case AST_ARRAY:
    return mir_emit_sequence_literal(builder, ast, MIR_ARRAY_LITERAL);
  case AST_LIST:
    return mir_emit_sequence_literal(builder, ast, MIR_LIST_LITERAL);
  case AST_TUPLE:
    return mir_emit_sequence_literal(builder, ast, MIR_TUPLE);
  case AST_BINOP: {
    MirInstr instr = mir_make_instr(MIR_BINOP, ast->type, ast);
    instr.op = ast->data.AST_BINOP.op;
    mir_value_id_vec_push(builder->fn->arena, &instr.operands,
                          mir_emit_expr(builder, ast->data.AST_BINOP.left));
    mir_value_id_vec_push(builder->fn->arena, &instr.operands,
                          mir_emit_expr(builder, ast->data.AST_BINOP.right));
    return mir_builder_append_instr(builder, instr);
  }
  case AST_UNOP: {
    MirInstr instr = mir_make_instr(MIR_UNOP, ast->type, ast);
    instr.op = ast->data.AST_UNOP.op;
    mir_value_id_vec_push(builder->fn->arena, &instr.operands,
                          mir_emit_expr(builder, ast->data.AST_UNOP.expr));
    return mir_builder_append_instr(builder, instr);
  }
  case AST_APPLICATION:
    return mir_emit_application(builder, ast);
  case AST_RECORD_ACCESS: {
    MirInstr instr = mir_make_instr(MIR_RECORD_GET, ast->type, ast);
    mir_value_id_vec_push(builder->fn->arena, &instr.operands,
                          mir_emit_expr(builder,
                                        ast->data.AST_RECORD_ACCESS.record));
    instr.name = ast->data.AST_RECORD_ACCESS.member &&
                         ast->data.AST_RECORD_ACCESS.member->tag == AST_IDENTIFIER
                     ? ast->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value
                     : NULL;
    instr.member_index = ast->data.AST_RECORD_ACCESS.index;
    return mir_builder_append_instr(builder, instr);
  }
  case AST_LAMBDA:
  case AST_MODULE:
    return mir_emit_lambda_value(builder, ast, NULL);
  case AST_YIELD: {
    MirInstr instr = mir_make_instr(MIR_YIELD, ast->type, ast);
    mir_value_id_vec_push(builder->fn->arena, &instr.operands,
                          mir_emit_expr(builder, ast->data.AST_YIELD.expr));
    return mir_builder_append_instr(builder, instr);
  }
  default:
    return mir_emit_unsupported(builder, ast);
  }
}

static void dump_value(FILE *stream, MirValueId value) {
  if (value == MIR_NO_VALUE) {
    fprintf(stream, "<none>");
  } else {
    fprintf(stream, "%%%u", value);
  }
}

static void dump_operands(FILE *stream, const MirValueIdVec *operands) {
  for (size_t i = 0; i < operands->len; i++) {
    if (i > 0) {
      fprintf(stream, ", ");
    }
    dump_value(stream, operands->items[i]);
  }
}

static const MirAllocSite *find_alloc(const MirFunction *fn, MirAllocId id) {
  if (id == MIR_NO_ALLOC || id >= fn->allocs.len) {
    return NULL;
  }
  return fn->allocs.items + id;
}

static void dump_instr(FILE *stream, const MirFunction *fn,
                       const MirInstr *instr) {
  fprintf(stream, "    ");
  if (instr->result != MIR_NO_VALUE) {
    dump_value(stream, instr->result);
    fprintf(stream, " = ");
  }

  fprintf(stream, "%s", instr_name(instr->kind));
  switch (instr->kind) {
  case MIR_CONST_INT:
    fprintf(stream, " %d", instr->imm.int_value);
    break;
  case MIR_CONST_UINT64:
    fprintf(stream, " %llu", (unsigned long long)instr->imm.uint64_value);
    break;
  case MIR_CONST_DOUBLE:
    fprintf(stream, " %f", instr->imm.double_value);
    break;
  case MIR_CONST_CHAR:
    fprintf(stream, " '%c'", instr->imm.char_value);
    break;
  case MIR_CONST_BOOL:
    fprintf(stream, " %s", instr->imm.bool_value ? "true" : "false");
    break;
  case MIR_STRING_LITERAL:
    fprintf(stream, " \"%.*s\"", instr->member_index,
            instr->name ? instr->name : "");
    break;
  case MIR_PARAM:
  case MIR_FUNCTION_REF:
  case MIR_LOAD_SYMBOL:
    fprintf(stream, " %s", instr->name ? instr->name : "<unnamed>");
    break;
  case MIR_LOAD_LOCAL:
    fprintf(stream, " %s <- ", instr->name ? instr->name : "<unnamed>");
    dump_operands(stream, &instr->operands);
    break;
  case MIR_BINOP:
  case MIR_UNOP:
    fprintf(stream, " %s ", token_name(instr->op));
    dump_operands(stream, &instr->operands);
    break;
  case MIR_RECORD_GET:
    fprintf(stream, " ");
    dump_operands(stream, &instr->operands);
    fprintf(stream, ".%s", instr->name ? instr->name : "<field>");
    if (instr->member_index >= 0) {
      fprintf(stream, " index %d", instr->member_index);
    }
    break;
  case MIR_UNSUPPORTED:
    fprintf(stream, " %s", instr->name ? instr->name : "<unknown>");
    break;
  default:
    if (instr->operands.len > 0) {
      fprintf(stream, " ");
      dump_operands(stream, &instr->operands);
    }
    break;
  }

  if (instr->alloc_id != MIR_NO_ALLOC) {
    const MirAllocSite *alloc = find_alloc(fn, instr->alloc_id);
    fprintf(stream, " alloc #%u", instr->alloc_id);
    if (alloc) {
      fprintf(stream, " [%s%s]", placement_name(alloc->placement),
              alloc->is_mutable ? ", mutable" : "");
    }
  }

  if (instr->result != MIR_NO_VALUE) {
    fprintf(stream, " : %s", type_kind_name(instr->type));
  }
  fprintf(stream, "\n");
}

static void dump_term(FILE *stream, const MirTerminator *term) {
  switch (term->kind) {
  case MIR_TERM_NONE:
    fprintf(stream, "    <unterminated>\n");
    break;
  case MIR_TERM_RETURN:
    fprintf(stream, "    return ");
    dump_value(stream, term->value);
    fprintf(stream, "\n");
    break;
  case MIR_TERM_BRANCH:
    fprintf(stream, "    br bb%u\n", term->target);
    break;
  case MIR_TERM_COND_BRANCH:
    fprintf(stream, "    cond_br ");
    dump_value(stream, term->value);
    fprintf(stream, ", bb%u, bb%u\n", term->then_target, term->else_target);
    break;
  case MIR_TERM_UNREACHABLE:
    fprintf(stream, "    unreachable\n");
    break;
  }
}

static void dump_function(FILE *stream, const MirFunction *fn) {
  fprintf(stream, "fn %s : %s {\n", fn->name ? fn->name : "<unnamed>",
          type_kind_name(fn->type));
  for (size_t b = 0; b < fn->blocks.len; b++) {
    MirBlock *block = fn->blocks.items[b];
    fprintf(stream, "  bb%u.%s:\n", block->id,
            block->name ? block->name : "<unnamed>");
    for (size_t i = 0; i < block->instrs.len; i++) {
      dump_instr(stream, fn, block->instrs.items + i);
    }
    dump_term(stream, &block->term);
  }
  fprintf(stream, "}\n\n");
}

void mir_dump_program(MirProgram *program, FILE *stream) {
  if (!program || !stream) {
    return;
  }
  fprintf(stream, "# YLC MIR\n");
  for (size_t i = 0; i < program->functions.len; i++) {
    dump_function(stream, program->functions.items[i]);
  }
}

MirProgram *mir_build_program(MirArena *arena, Ast *prog, TypeEnv *type_env) {
  (void)type_env;
  if (!arena || !prog) {
    return NULL;
  }

  MirProgram *program =
      mir_arena_alloc(arena, sizeof(MirProgram), MIR_ALIGNOF(MirProgram));
  memset(program, 0, sizeof(MirProgram));
  program->arena = arena;

  MirFunction *top = mir_new_function(program, "$top", prog->type, prog);
  MirBuilder builder;
  mir_builder_init(&builder, program, top);
  MirValueId result = mir_emit_expr(&builder, prog);
  mir_set_return(&builder, result);
  return program;
}

void mir_run_passes(MirProgram *program) { (void)program; }

int mir(Ast *prog, TypeEnv *type_env) {
  MirArena *arena = mir_arena_create();
  if (!arena) {
    return 0;
  }

  MirProgram *program = mir_build_program(arena, prog, type_env);
  if (!program) {
    mir_arena_destroy(arena);
    return 0;
  }

  mir_run_passes(program);
  if (ylc_config.dump_mir) {
    mir_dump_program(program, stdout);
  }

  mir_arena_destroy(arena);
  return 1;
}
