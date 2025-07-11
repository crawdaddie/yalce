#ifndef _LANG_BACKEND_LLVM_COMMON_H
#define _LANG_BACKEND_LLVM_COMMON_H

#include "escape_analysis.h"
#include "ht.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"

#define STACK_MAX 256

#define GENERIC_PTR LLVMPointerType(LLVMInt8Type(), 0)

typedef struct StackFrame {
  ht *table;
  struct StackFrame *next;
} StackFrame;

typedef struct coroutine_ctx_t {
  LLVMValueRef func;
  LLVMTypeRef func_type;
  LLVMTypeRef instance_type;
  int num_branches;
  int current_branch;
  LLVMBasicBlockRef *block_refs;
} coroutine_ctx_t;

typedef struct {
  // ht stack[STACK_MAX];
  int stack_ptr;
  StackFrame *frame;
  TypeEnv *env;
  int *num_globals;
  void **global_storage_array;
  int *global_storage_capacity;
  const char *module_name;
  int num_coroutine_yields;
  int current_yield;
  LLVMBasicBlockRef switch_default;
  LLVMValueRef yield_switch_ref;
} JITLangCtx;

typedef struct SpecificFns {
  Type *arg_types_key;
  LLVMValueRef func;
  struct SpecificFns *next;
} SpecificFns;

typedef struct coroutine_generator_symbol_data_t {
  Ast *ast;
  int stack_ptr;
  Type *ret_option_type;
  LLVMTypeRef llvm_ret_option_type;
  Type *params_obj_type;
  LLVMTypeRef llvm_params_obj_type;
  LLVMTypeRef def_fn_type;
  bool recursive_ref;
} coroutine_generator_symbol_data_t;

typedef enum symbol_type {
  STYPE_TOP_LEVEL_VAR,
  STYPE_LOCAL_VAR,
  STYPE_FUNCTION,
  STYPE_LAZY_EXTERN_FUNCTION,
  STYPE_GENERIC_FUNCTION,
  STYPE_PARTIAL_EVAL_CLOSURE,
  STYPE_MODULE,
  STYPE_VARIANT_TYPE,
  STYPE_COROUTINE_CONSTRUCTOR,
  STYPE_GENERIC_CONSTRUCTOR,
} symbol_type;

typedef LLVMValueRef (*BuiltinHandler)(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);

typedef struct {
  int num_exports;
  int val_map[];
} ModuleTypeMap;

typedef struct {
  symbol_type type;
  LLVMTypeRef llvm_type;
  LLVMValueRef val;
  LLVMValueRef storage;
  union {
    int STYPE_TOP_LEVEL_VAR;
    int STYPE_FN_PARAM;

    struct {
      Type *fn_type;
      bool recursive_ref;
    } STYPE_FUNCTION;

    struct {
      Ast *ast;
    } STYPE_LAZY_EXTERN_FUNCTION;

    struct {
      Ast *ast;
      int stack_ptr;
      StackFrame *stack_frame;
      TypeEnv *type_env;
      SpecificFns *specific_fns;
      BuiltinHandler builtin_handler;
    } STYPE_GENERIC_FUNCTION;

    struct {
      struct JITSymbol *callable_sym;
      LLVMValueRef *args;
      int provided_args_len;
      int original_args_len;
      Type *original_callable_type;
    } STYPE_PARTIAL_EVAL_CLOSURE;

    struct {
      ht generics;
      JITLangCtx *ctx;
      ModuleTypeMap map;
      const char *path;
    } STYPE_MODULE;
    struct {
      LLVMTypeRef llvm_state_type;
      Type *state_type;
      bool recursive_ref;
    } STYPE_COROUTINE_CONSTRUCTOR;
  } symbol_data;

  Type *symbol_type;
} JITSymbol;

typedef struct {
  int stack_level;
  JITSymbol val;
} JITLookupResult;

JITLangCtx ctx_push(JITLangCtx ctx);

JITSymbol *find_in_ctx(const char *name, int name_len, JITLangCtx *ctx);

bool is_top_level_frame(StackFrame *frame);

void destroy_ctx(JITLangCtx *ctx);
#define STACK_ALLOC_CTX_PUSH(_ctx_name, _ctx)                                  \
  JITLangCtx _ctx_name = *_ctx;                                                \
  ht table;                                                                    \
  ht_init(&table);                                                             \
  StackFrame sf = {.table = &table, .next = _ctx_name.frame};                  \
  _ctx_name.frame = &sf;                                                       \
  _ctx_name.stack_ptr = _ctx->stack_ptr + 1;

EscapeStatus find_allocation_strategy(Ast *expr, JITLangCtx *ctx);
#endif
