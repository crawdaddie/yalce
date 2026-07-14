#ifndef _LANG_MIR_H
#define _LANG_MIR_H
#include <stdio.h>

#include "ht.h"
#include "parse.h"
#include "types/type.h"

typedef struct MirProgram MirProgram;
typedef struct MirArena MirArena;

MirArena *mir_arena_create(void);
void mir_arena_destroy(MirArena *arena);
MirProgram *mir_build_program(MirArena *arena, Ast *prog, TypeEnv *type_env);
void mir_run_passes(MirProgram *program);
void mir_dump_program(MirProgram *program, FILE *stream);

int mir(Ast *prog, TypeEnv *type_env);

typedef unsigned MirValueId;
typedef unsigned MirBlockId;
typedef unsigned MirAllocId;
typedef unsigned MirEnvLayoutId;

#define MIR_NO_VALUE ((MirValueId) - 1)
#define MIR_NO_ALLOC ((MirAllocId) - 1)
#define MIR_NO_ENV_LAYOUT ((MirEnvLayoutId) - 1)

typedef enum {
  MIR_CONST_INT,
  MIR_CONST_UINT64,
  MIR_CONST_DOUBLE,
  MIR_CONST_CHAR,
  MIR_CONST_BOOL,
  MIR_CONST_VOID,
  MIR_STRING_LITERAL,
  MIR_PARAM,
  MIR_FN_REF,
  MIR_LOAD_SYMBOL,
  MIR_ARRAY_LITERAL,
  MIR_LIST_LITERAL,
  MIR_TUPLE,
  MIR_BINOP,
  MIR_UNOP,
  MIR_ENV_NEW,
  MIR_ENV_GET,
  MIR_ENV_SET,
  MIR_MAKE_CLOSURE,
  MIR_CLOSURE_FN,
  MIR_CLOSURE_ENV,
  MIR_CALL_DIRECT,
  MIR_CALL_INDIRECT,
  MIR_RECORD_GET,
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
  MIR_TERM_TAIL_CALL_DIRECT,
  MIR_TERM_TAIL_CALL_INDIRECT,
  MIR_TERM_UNREACHABLE,
} MirTermKind;

typedef enum {
  MIR_ABI_YLC,
  MIR_ABI_C,
} MirAbi;

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

typedef enum {
  MIR_VALUE_PLAIN,
  MIR_VALUE_FN_PTR,
  MIR_VALUE_ENV_PTR,
  MIR_VALUE_CLOSURE_OBJ,
} MirValueKind;

typedef struct MirValue {
  MirValueId id;
  Type *type;
  MirValueKind kind;
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
  MirAbi abi;
  MirEnvLayoutId env_layout_id;
  unsigned field_index;
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
  MirValueId callee;
  MirValueIdVec args;
  const char *symbol;
  MirAbi abi;
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
  bool is_closure_body;
  int env_param_index;
  MirBlockVec blocks;
  MirValueVec values;
  MirAllocSiteVec allocs;
} MirFunction;

typedef struct {
  MirFunction **items;
  size_t len;
  size_t cap;
} MirFunctionVec;

typedef struct MirEnvField {
  const char *name;
  Type *type;
} MirEnvField;

typedef struct {
  MirEnvField *items;
  size_t len;
  size_t cap;
} MirEnvFieldVec;

typedef struct MirEnvLayout {
  MirEnvLayoutId id;
  const char *name;
  MirEnvFieldVec fields;
} MirEnvLayout;

typedef struct {
  MirEnvLayout *items;
  size_t len;
  size_t cap;
} MirEnvLayoutVec;

struct MirProgram {
  MirArena *arena;
  MirFunctionVec functions;
  MirEnvLayoutVec env_layouts;
};

typedef struct {
  ht *bindings;
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
void mir_value_id_vec_push(MirArena *arena, MirValueIdVec *vec,
                           MirValueId value);
MirValueId mir_create_value(MirFunction *fn, Type *type, Ast *origin);
MirValueId mir_create_value_kind(MirFunction *fn, Type *type, MirValueKind kind,
                                 Ast *origin);
MirAllocId mir_create_alloc_site(MirFunction *fn, Ast *origin);
MirValueId mir_append_instr(MirFunction *fn, MirBlock *block, MirInstr instr);
MirValueId mir_insert_instr_before(MirFunction *fn, MirBlock *block,
                                   size_t index, MirInstr instr);
MirValueId mir_insert_instr_after(MirFunction *fn, MirBlock *block,
                                  size_t index, MirInstr instr);
void mir_run_function_pass(MirProgram *program, MirFunctionPass pass,
                           void *ctx);

void dump_function(FILE *stream, const MirFunction *fn);
#endif
