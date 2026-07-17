#ifndef _LANG_MIR_H
#define _LANG_MIR_H

#include "ht.h"
#include "parse.h"
#include "types/type.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct MirArena MirArena;
typedef struct MirProgram MirProgram;
typedef struct MirBuilder MirBuilder;
typedef struct MirInstrVec MirInstrVec;
typedef struct MirInstr MirInstr;
typedef struct MirBuiltinSymbol MirBuiltinSymbol;
typedef struct MirFnSummary MirFnSummary;

typedef unsigned MirFunctionId;
typedef unsigned MirBlockId;
typedef unsigned MirValueId;

#define MIR_NO_FUNCTION ((MirFunctionId) - 1)
#define MIR_NO_BLOCK ((MirBlockId) - 1)
#define MIR_NO_VALUE ((MirValueId) - 1)

typedef struct MirStackFrame {
  ht *table;
  struct MirStackFrame *next;
} MirStackFrame;

typedef struct MirCtx {
  MirStackFrame *frame;
  TypeEnv *env;
} MirCtx;

typedef enum {
  MIR_CONST_INT,
  MIR_CONST_UINT64,
  MIR_CONST_FLOAT,
  MIR_CONST_DOUBLE,
  MIR_CONST_CHAR,
  MIR_CONST_BOOL,
  MIR_CONST_STRING,
  MIR_CONST_VOID,
  MIR_PRIMITIVE_CAST,
  MIR_TUPLE,
  MIR_TUPLE_GET,
  MIR_MATCH,
  MIR_VARIANT,
  MIR_VARIANT_TAG,
  MIR_TAG_EQ,
  MIR_VARIANT_PAYLOAD,
  MIR_LIST_EMPTY,
  MIR_LIST_CONS,
  MIR_LIST_IS_EMPTY,
  MIR_LIST_HEAD,
  MIR_LIST_TAIL,
  MIR_ARRAY_LITERAL,
  MIR_ARRAY_SIZE,
  MIR_ARRAY_AT,
  MIR_ARRAY_SET,
  MIR_ARRAY_FILL_CONST,
  MIR_ARRAY_FILL,
  MIR_ARRAY_RANGE,
  MIR_ARRAY_SUCC,
  MIR_ARRAY_OFFSET,
  MIR_STR,
  MIR_PRINT,
  MIR_CSTR,
  MIR_SIZEOF,
  MIR_DLOPEN,
  MIR_AS_BYTES,
  MIR_TYPEOF,
  MIR_PRIMITIVE,
  MIR_LOGICAL_AND,
  MIR_LOGICAL_OR,
  MIR_CLOSURE_ENV,
  MIR_CLOSURE,
  MIR_CLOSURE_GET,
  MIR_CLOSURE_FN,
  MIR_CLOSURE_GET_ENV,
  MIR_DUP,
  MIR_DROP,
  MIR_FN_REF,
  MIR_CALL,
} MirInstrKind;

typedef enum {
  MIR_OP_IEQ,
  MIR_OP_UEQ,
  MIR_OP_FEQ,
  MIR_OP_CEQ,
  MIR_OP_BEQ,
  MIR_OP_IGT,
  MIR_OP_UGT,
  MIR_OP_FGT,
  MIR_OP_CGT,
  MIR_OP_IGTE,
  MIR_OP_UGTE,
  MIR_OP_FGTE,
  MIR_OP_CGTE,
  MIR_OP_ILT,
  MIR_OP_ULT,
  MIR_OP_FLT,
  MIR_OP_CLT,
  MIR_OP_ILTE,
  MIR_OP_ULTE,
  MIR_OP_FLTE,
  MIR_OP_CLTE,
  MIR_OP_LNOT,
  MIR_OP_IADD,
  MIR_OP_UADD,
  MIR_OP_FADD,
  MIR_OP_ISUB,
  MIR_OP_USUB,
  MIR_OP_FSUB,
  MIR_OP_IMUL,
  MIR_OP_UMUL,
  MIR_OP_FMUL,
  MIR_OP_IDIV,
  MIR_OP_UDIV,
  MIR_OP_FDIV,
  MIR_OP_IMOD,
  MIR_OP_UMOD,
  MIR_OP_FMOD,
} MirPrimitiveOp;

typedef enum {
  MIR_TERM_NONE,
  MIR_TERM_RETURN,
  MIR_TERM_ARM_RETURN,
  MIR_TERM_BR,
  MIR_TERM_COND,
  MIR_TERM_UNREACHABLE,
} MirTermKind;

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

typedef struct MirValue {
  MirValueId id;
  Type *type;
  Ast *origin;
  EscapeMeta *ea_md;
  MirFnSummary *callable_summary;
} MirValue;

typedef struct {
  MirValue *items;
  size_t len;
  size_t cap;
} MirValueVec;

typedef struct {
  MirValueId *items;
  size_t len;
  size_t cap;
} MirValueIdVec;

typedef struct MirMatchArm {
  Ast *pattern;
  MirBlockId test_block;
  MirBlockId body_block;
} MirMatchArm;

typedef struct {
  MirMatchArm *items;
  size_t len;
  size_t cap;
} MirMatchArmVec;

typedef struct MirParam {
  MirValueId value;
  const char *name;
  Type *type;
  Ast *origin;
} MirParam;

typedef struct {
  MirParam *items;
  size_t len;
  size_t cap;
} MirParamVec;

typedef MirValueId (*MirBuiltinHandler)(MirBuilder *builder, Ast *app,
                                        MirCtx *ctx, MirBuiltinSymbol *symbol);

typedef enum {
  MIR_OPERAND_ROLE_VALUE,
  MIR_OPERAND_ROLE_CALLEE,
  MIR_OPERAND_ROLE_SCRUTINEE,
  MIR_OPERAND_ROLE_RETURN,
  MIR_OPERAND_ROLE_CONDITION,
  MIR_OPERAND_ROLE_TAG,
  MIR_OPERAND_ROLE_CONTAINER,
  MIR_OPERAND_ROLE_INDEX,
  MIR_OPERAND_ROLE_ELEMENT,
  MIR_OPERAND_ROLE_FIELD,
  MIR_OPERAND_ROLE_FUNCTION,
  MIR_OPERAND_ROLE_ENV,
} MirOperandRole;

typedef enum {
  MIR_OPERAND_USE_BORROW,
  MIR_OPERAND_USE_CONSUME,
} MirOperandUse;

typedef enum {
  MIR_RESULT_NONE,
  MIR_RESULT_OWNED,
  MIR_RESULT_BORROWED,
} MirResultOwnership;

typedef struct {
  MirOperandUse *items;
  size_t len;
  size_t cap;
} MirOperandUseVec;

typedef struct {
  MirValueId value;
  MirOperandRole role;
  MirOperandUse use;
  size_t index;
} MirOperand;

typedef bool (*MirOperandVisitor)(MirInstr *instr, MirOperand operand,
                                  void *ctx);
typedef MirValueId (*MirOperandRewriter)(MirInstr *instr, MirOperand operand,
                                         void *ctx);

struct MirFnSummary {
  MirOperandUseVec param_uses;
  MirResultOwnership result;
};

struct MirBuiltinSymbol {
  const char *name;
  Type *type;
  MirBuiltinHandler handler;
  void *data;
  MirFnSummary summary;
};

typedef struct MirInstr {
  MirInstrKind kind;
  MirValueId result;
  Type *type;
  Ast *origin;
  union {
    int int_value;
    uint64_t uint64_value;
    float float_value;
    double double_value;
    char char_value;
    bool bool_value;
    struct {
      const char *chars;
      size_t len;
    } string_value;

    struct {
      MirValueId value;
      Type *from_type;
      Type *to_type;
    } primitive_cast;

    struct {
      MirPrimitiveOp op;
      uint8_t argc;
      MirValueId operands[3];
    } primitive;

    struct {
      MirValueId value;
    } value_op;

    struct {
      MirValueId lhs;
      MirBlockId rhs_block;
      MirBlockId short_block;
      MirBlockId continuation_block;
      bool short_value;
    } logical;

    struct {
      MirValueIdVec items;
    } tuple;

    struct {
      MirValueId tuple;
      size_t index;
    } tuple_get;

    struct {
      MirValueId scrutinee;
      MirMatchArmVec arms;
      MirBlockId first_test_block;
      MirBlockId no_match_block;
      MirBlockId continuation_block;
      bool allow_no_match;
    } match;

    struct {
      Type *constructor_type;
      const char *constructor_name;
      int constructor_index;
      MirValueIdVec fields;
    } variant;

    struct {
      MirValueId value;
    } variant_tag;

    struct {
      MirValueId tag;
      int constructor_index;
      const char *constructor_name;
    } tag_eq;

    struct {
      MirValueId value;
      Type *constructor_type;
      const char *constructor_name;
      int constructor_index;
    } variant_payload;

    struct {
      MirValueId head;
      MirValueId tail;
    } list_cons;

    struct {
      MirValueId list;
    } list_op;

    struct {
      MirValueIdVec items;
    } array_literal;

    struct {
      MirValueId array;
    } array_unop;

    struct {
      MirValueId array;
      MirValueId index;
    } array_at;

    struct {
      MirValueId array;
      MirValueId index;
      MirValueId value;
    } array_set;

    struct {
      MirValueId size;
      MirValueId value;
    } array_fill_const;

    struct {
      MirValueId size;
      MirValueId fill_fn;
    } array_fill;

    struct {
      MirValueId offset;
      MirValueId size;
      MirValueId array;
    } array_range;

    struct {
      MirValueId offset;
      MirValueId array;
    } array_offset;

    struct {
      MirValueIdVec fields;
    } closure_env;

    struct {
      MirValueId fn;
      MirValueId env;
      MirFunctionId impl_fn;
      const char *impl_name;
    } closure;

    struct {
      MirValueId env;
      size_t index;
      const char *name;
    } closure_get;

    struct {
      MirValueId closure;
    } closure_part;

    struct {
      MirFunctionId fn;
      const char *name;
    } fn_ref;

    struct {
      MirValueId callee;
      MirBuiltinSymbol *builtin;
      Type *callee_type;
      const char *specialized_name;
      MirFunctionId specialized_fn;
      MirValueIdVec operands;
      MirOperandUseVec operand_uses;
    } call;

  } data;
} MirInstr;

typedef struct MirInstrVec {
  MirInstr *items;
  size_t len;
  size_t cap;
} MirInstrVec;

typedef struct MirTerminator {
  MirTermKind kind;
  MirValueId value;
  MirValueId cond;
  MirBlockId target;
  MirBlockId then_block;
  MirBlockId else_block;
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
  MirFunctionId id;
  MirArena *arena;
  const char *name;
  Type *type;
  Ast *origin;
  MirFunctionId specialization_of;
  Type *specialization_type;
  MirFnSummary summary;
  MirParamVec params;
  MirBlockVec blocks;
  MirValueVec values;
} MirFunction;

typedef struct {
  MirFunction **items;
  size_t len;
  size_t cap;
} MirFunctionVec;

struct MirProgram {
  MirArena *arena;
  TypeEnv *type_env;
  ht builtins;
  MirFunctionVec functions;
};

struct MirBuilder {
  MirProgram *program;
  MirFunction *fn;
  MirBlock *block;
};

MirArena *mir_arena_create(void);
void mir_arena_destroy(MirArena *arena);
void *mir_arena_alloc(MirArena *arena, size_t size, size_t align);
char *mir_arena_strdup(MirArena *arena, const char *str);
char *mir_arena_strndup(MirArena *arena, const char *str, size_t len);
void mir_value_id_vec_push(MirArena *arena, MirValueIdVec *vec,
                           MirValueId value);
void mir_operand_use_vec_push(MirArena *arena, MirOperandUseVec *vec,
                              MirOperandUse value);
void mir_match_arm_vec_push(MirArena *arena, MirMatchArmVec *vec,
                            MirMatchArm value);
void mir_instr_vec_push(MirArena *arena, MirInstrVec *vec, MirInstr value);

void mir_stack_frame_init(MirArena *arena, ht *table, MirStackFrame *frame,
                          MirStackFrame *next);
bool mir_ctx_bind_value(MirCtx *ctx, const char *name, MirValueId value);
bool mir_ctx_lookup_value(MirCtx *ctx, const char *name, MirValueId *out);

#define MIR_STACK_ALLOC_CTX_PUSH(_ctx_name, _builder, _ctx)                     \
  MirCtx _ctx_name = *(_ctx);                                                    \
  ht _ctx_name##_table;                                                          \
  MirStackFrame _ctx_name##_frame;                                               \
  mir_stack_frame_init((_builder)->fn->arena, &_ctx_name##_table,                \
                       &_ctx_name##_frame, _ctx_name.frame);                     \
  _ctx_name.frame = &_ctx_name##_frame;

MirProgram *mir_build_program(MirArena *arena, Ast *prog, MirCtx *ctx);
void mir_program_destroy(MirProgram *program);
void mir_run_passes(MirProgram *program);
void mir_escape_analysis(MirProgram *program);
void mir_escape_analysis_function(MirFunction *fn);
EscapeMeta *mir_value_escape_meta(MirFunction *fn, MirValueId value);
void mir_perceus_instrumentation(MirProgram *program);
void mir_dump_program(MirProgram *program, FILE *stream);
int mir(Ast *prog, TypeEnv *type_env);

MirFunction *mir_program_add_function(MirProgram *program, const char *name,
                                      Type *type, Ast *origin);
MirValueId mir_function_add_param(MirFunction *fn, const char *name, Type *type,
                                  Ast *origin);
MirBlock *mir_function_add_block(MirFunction *fn, const char *name);

void mir_builder_init(MirBuilder *builder, MirProgram *program,
                      MirFunction *fn);
void mir_builder_position_at_end(MirBuilder *builder, MirBlock *block);
void mir_builder_set_return(MirBuilder *builder, MirValueId value);
void mir_builder_set_arm_return(MirBuilder *builder, MirValueId value,
                                MirBlockId target);
void mir_builder_set_br(MirBuilder *builder, MirBlockId target);
void mir_builder_set_cond(MirBuilder *builder, MirValueId cond,
                          MirBlockId then_block, MirBlockId else_block);
void mir_builder_set_unreachable(MirBuilder *builder);

MirInstr mir_make_instr(MirInstrKind kind, Type *type, Ast *origin);
MirValueId mir_function_add_value(MirFunction *fn, Type *type, Ast *origin);
MirValueId mir_append_instr(MirFunction *fn, MirBlock *block, MirInstr instr);
MirValueId mir_builder_append_instr(MirBuilder *builder, MirInstr instr);
bool mir_instr_for_each_operand(MirInstr *instr, MirOperandVisitor visitor,
                                void *ctx);
void mir_instr_rewrite_operands(MirInstr *instr, MirOperandRewriter rewriter,
                                void *ctx);
bool mir_term_for_each_operand(MirTerminator *term, MirOperandVisitor visitor,
                               void *ctx);
MirOperandUse mir_function_param_use(const MirFunction *fn, size_t index);
MirResultOwnership mir_function_result_ownership(const MirFunction *fn);

MirValueId mir_const_int(MirBuilder *builder, Type *type, Ast *origin,
                         int value);
MirValueId mir_const_uint64(MirBuilder *builder, Type *type, Ast *origin,
                            uint64_t value);
MirValueId mir_const_float(MirBuilder *builder, Type *type, Ast *origin,
                           float value);
MirValueId mir_const_double(MirBuilder *builder, Type *type, Ast *origin,
                            double value);
MirValueId mir_const_char(MirBuilder *builder, Type *type, Ast *origin,
                          char value);
MirValueId mir_const_bool(MirBuilder *builder, Type *type, Ast *origin,
                          bool value);
MirValueId mir_const_string(MirBuilder *builder, Type *type, Ast *origin,
                            const char *chars, size_t len);
MirValueId mir_const_void(MirBuilder *builder, Type *type, Ast *origin);
MirValueId mir_primitive_cast(MirBuilder *builder, Type *from_type,
                              Type *to_type, Ast *origin, MirValueId value);
MirValueId mir_tuple(MirBuilder *builder, Type *type, Ast *origin,
                     MirValueIdVec items);
MirValueId mir_tuple_get(MirBuilder *builder, Type *type, Ast *origin,
                         MirValueId tuple, size_t index);
MirValueId mir_match(MirBuilder *builder, Type *type, Ast *origin,
                     MirValueId scrutinee, MirMatchArmVec arms,
                     MirBlockId first_test_block, MirBlockId no_match_block,
                     MirBlockId continuation_block, bool allow_no_match);
MirValueId mir_variant(MirBuilder *builder, Type *type, Ast *origin,
                       Type *constructor_type, int constructor_index,
                       const char *constructor_name, MirValueIdVec fields);
MirValueId mir_variant_tag(MirBuilder *builder, Ast *origin, MirValueId value);
MirValueId mir_tag_eq(MirBuilder *builder, Ast *origin, MirValueId tag,
                      int constructor_index, const char *constructor_name);
MirValueId mir_variant_payload(MirBuilder *builder, Ast *origin,
                               MirValueId value, Type *constructor_type,
                               int constructor_index,
                               const char *constructor_name);
MirValueId mir_list_empty(MirBuilder *builder, Type *type, Ast *origin);
MirValueId mir_list_cons(MirBuilder *builder, Type *type, Ast *origin,
                         MirValueId head, MirValueId tail);
MirValueId mir_list_is_empty(MirBuilder *builder, Ast *origin, MirValueId list);
MirValueId mir_list_head(MirBuilder *builder, Type *type, Ast *origin,
                         MirValueId list);
MirValueId mir_list_tail(MirBuilder *builder, Type *type, Ast *origin,
                         MirValueId list);
MirValueId mir_array_literal(MirBuilder *builder, Type *type, Ast *origin,
                             MirValueIdVec items);
MirValueId mir_ieq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs);
MirValueId mir_ueq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs);
MirValueId mir_feq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs);
MirValueId mir_ceq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs);
MirValueId mir_beq(MirBuilder *builder, Ast *origin, MirValueId lhs,
                   MirValueId rhs);
MirValueId mir_fn_ref(MirBuilder *builder, Type *type, Ast *origin,
                      MirFunction *fn);
MirValueId mir_primitive_instr(MirBuilder *builder, MirPrimitiveOp op,
                               Type *type, Ast *origin,
                               const MirValueId *operands, size_t argc);
MirValueId mir_lnot(MirBuilder *builder, Ast *origin, MirValueId value);
MirValueId mir_iadd(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_uadd(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_fadd(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_isub(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_usub(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_fsub(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_imul(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_umul(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_fmul(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_idiv(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_udiv(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_fdiv(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_imod(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_umod(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);
MirValueId mir_fmod(MirBuilder *builder, Type *type, Ast *origin,
                    MirValueId lhs, MirValueId rhs);

Type *mir_function_value_type(MirFunction *fn, MirValueId value);
MirInstr *mir_function_find_def_instr(MirFunction *fn, MirValueId value);
MirValueId mir_constructor_call(MirBuilder *builder, Ast *origin,
                                Type *result_type, const char *constructor_name,
                                Ast *args, size_t len, MirCtx *ctx);
MirValueId mir_lower_specialized_builtin_call(MirBuilder *builder,
                                              MirInstr *call);
MirValueId mir_expr(MirBuilder *builder, Ast *ast, MirCtx *ctx);

MirBuiltinSymbol *mir_program_register_builtin(MirProgram *program,
                                               const char *name, Type *type,
                                               MirBuiltinHandler handler,
                                               void *data);
MirBuiltinSymbol *mir_program_lookup_builtin(MirProgram *program,
                                             const char *name);
void mir_register_core_builtins(MirProgram *program);

void dump_function(FILE *stream, const MirFunction *fn);

#endif
