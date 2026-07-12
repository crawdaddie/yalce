#ifndef _LANG_MIR_INTERNAL_H
#define _LANG_MIR_INTERNAL_H

#include "mir/mir.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef unsigned MirValueId;
typedef unsigned MirBlockId;
typedef unsigned MirAllocId;

#define MIR_NO_VALUE ((MirValueId)-1)
#define MIR_NO_ALLOC ((MirAllocId)-1)

typedef enum {
  MIR_CONST_INT,
  MIR_CONST_UINT64,
  MIR_CONST_DOUBLE,
  MIR_CONST_CHAR,
  MIR_CONST_BOOL,
  MIR_CONST_VOID,
  MIR_STRING_LITERAL,
  MIR_PARAM,
  MIR_FUNCTION_REF,
  MIR_LOAD_LOCAL,
  MIR_LOAD_SYMBOL,
  MIR_ARRAY_LITERAL,
  MIR_LIST_LITERAL,
  MIR_TUPLE,
  MIR_BINOP,
  MIR_UNOP,
  MIR_CALL,
  MIR_RECORD_GET,
  MIR_CLOSURE,
  MIR_YIELD,
  MIR_MOVE,
  MIR_BORROW,
  MIR_DUP_IF_MANAGED,
  MIR_DROP_IF_MANAGED,
  MIR_DECREF_IF_MANAGED,
  MIR_DROP_REUSE_IF_UNIQUE,
  MIR_UNSUPPORTED,
} MirInstrKind;

typedef enum {
  MIR_TERM_NONE,
  MIR_TERM_RETURN,
  MIR_TERM_BRANCH,
  MIR_TERM_COND_BRANCH,
  MIR_TERM_UNREACHABLE,
} MirTermKind;

typedef enum {
  MIR_PLACE_UNKNOWN,
  MIR_PLACE_STACK,
  MIR_PLACE_HEAP,
  MIR_PLACE_CORO_FRAME,
} MirPlacement;

typedef struct MirArenaBlock {
  struct MirArenaBlock *next;
  size_t used;
  size_t capacity;
  unsigned char data[];
} MirArenaBlock;

struct MirArena {
  MirArenaBlock *blocks;
  size_t default_block_size;
};

typedef struct {
  MirValueId *items;
  size_t len;
  size_t cap;
} MirValueIdVec;

typedef struct MirValue {
  MirValueId id;
  Type *type;
  Ast *origin;
} MirValue;

typedef struct {
  MirValue *items;
  size_t len;
  size_t cap;
} MirValueVec;

typedef struct MirAllocSite {
  MirAllocId id;
  MirPlacement placement;
  bool is_mutable;
  Ast *origin;
} MirAllocSite;

typedef struct {
  MirAllocSite *items;
  size_t len;
  size_t cap;
} MirAllocSiteVec;

typedef struct MirInstr {
  MirInstrKind kind;
  MirValueId result;
  Type *type;
  Ast *origin;
  MirValueIdVec operands;
  MirAllocId alloc_id;
  token_type op;
  const char *name;
  int member_index;
  union {
    int int_value;
    uint64_t uint64_value;
    double double_value;
    char char_value;
    bool bool_value;
  } imm;
} MirInstr;

typedef struct {
  MirInstr *items;
  size_t len;
  size_t cap;
} MirInstrVec;

typedef struct MirTerminator {
  MirTermKind kind;
  MirValueId value;
  MirBlockId target;
  MirBlockId then_target;
  MirBlockId else_target;
} MirTerminator;

typedef struct MirBlock {
  MirBlockId id;
  const char *name;
  MirInstrVec instrs;
  MirTerminator term;
} MirBlock;

typedef struct {
  MirBlock **items;
  size_t len;
  size_t cap;
} MirBlockVec;

typedef struct MirFunction {
  MirArena *arena;
  const char *name;
  Type *type;
  Ast *origin;
  MirBlockVec blocks;
  MirValueVec values;
  MirAllocSiteVec allocs;
} MirFunction;

typedef struct {
  MirFunction **items;
  size_t len;
  size_t cap;
} MirFunctionVec;

struct MirProgram {
  MirArena *arena;
  MirFunctionVec functions;
};

typedef struct {
  const char *name;
  MirValueId value;
} MirBinding;

typedef struct {
  MirBinding *items;
  size_t len;
  size_t cap;
} MirBindingVec;

typedef struct {
  MirBindingVec bindings;
} MirScope;

typedef struct {
  MirScope *items;
  size_t len;
  size_t cap;
} MirScopeVec;

typedef struct {
  MirProgram *program;
  MirFunction *fn;
  MirBlock *block;
  MirScopeVec scopes;
} MirBuilder;

typedef void (*MirFunctionPass)(MirFunction *fn, void *ctx);

void *mir_arena_alloc(MirArena *arena, size_t size, size_t align);
char *mir_arena_strdup(MirArena *arena, const char *str);

MirInstr mir_make_instr(MirInstrKind kind, Type *type, Ast *origin);
void mir_value_id_vec_push(MirArena *arena, MirValueIdVec *vec, MirValueId value);
MirValueId mir_create_value(MirFunction *fn, Type *type, Ast *origin);
MirAllocId mir_create_alloc_site(MirFunction *fn, Ast *origin);
MirValueId mir_append_instr(MirFunction *fn, MirBlock *block, MirInstr instr);
MirValueId mir_insert_instr_before(MirFunction *fn, MirBlock *block,
                                   size_t index, MirInstr instr);
MirValueId mir_insert_instr_after(MirFunction *fn, MirBlock *block, size_t index,
                                  MirInstr instr);
void mir_run_function_pass(MirProgram *program, MirFunctionPass pass, void *ctx);

#endif
