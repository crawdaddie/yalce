#ifndef _LANG_BACKEND_LLVM_COMMON_H
#define _LANG_BACKEND_LLVM_COMMON_H

#include "./escape_analysis.h"
#include "ht.h"
#include "parse.h"
#include "types/inference.h"
#include "types/type.h"
#include <llvm-c/Types.h>

#define STACK_MAX 256

#define GENERIC_PTR LLVMPointerType(LLVMInt8Type(), 0)

typedef struct StackFrame {
  ht *table;
  struct StackFrame *next;
} StackFrame;

typedef struct {
  // ht stack[STACK_MAX];
  int stack_ptr;
  StackFrame *frame;
  TypeEnv *env;
  int *num_globals;
  void **global_storage_array;
  int *global_storage_capacity;
  const char *module_name;
  void *coro_ctx;
  LLVMValueRef allocator;
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
  STYPE_CLOSURE,
} symbol_type;

typedef LLVMValueRef (*BuiltinHandler)(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);

// typedef struct {
//   int num_exports;
//   int val_map[];
// } ModuleTypeMap;

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
      const char *path;
    } STYPE_MODULE;

    struct {
      LLVMTypeRef llvm_state_type;
      Type *state_type;
      bool recursive_ref;
    } STYPE_COROUTINE_CONSTRUCTOR;

    struct {
    } STYPE_CLOSURE;

  } symbol_data;

  Type *symbol_type;
} JITSymbol;

JITLangCtx ctx_push(JITLangCtx ctx);

JITSymbol *find_in_ctx(const char *name, int name_len, JITLangCtx *ctx);

void destroy_ctx(JITLangCtx *ctx);
#define STACK_ALLOC_CTX_PUSH(_ctx_name, _ctx)                                  \
  JITLangCtx _ctx_name = *_ctx;                                                \
  ht table;                                                                    \
  ht_init(&table);                                                             \
  StackFrame sf = {.table = &table, .next = _ctx_name.frame};                  \
  _ctx_name.frame = &sf;                                                       \
  _ctx_name.stack_ptr = _ctx->stack_ptr + 1;                                   \
  _ctx_name.coro_ctx = _ctx->coro_ctx;

EscapeStatus find_allocation_strategy(Ast *expr, JITLangCtx *ctx);

#define INSERT_PRINTF(num_args, fmt_str, ...)                                  \
  ({                                                                           \
    LLVMValueRef format_str =                                                  \
        LLVMBuildGlobalStringPtr(builder, fmt_str, "format");                  \
    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");           \
    if (!printf_fn) {                                                          \
      LLVMTypeRef printf_type = LLVMFunctionType(                              \
          LLVMInt32Type(),                                                     \
          (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)}, 1, true);       \
      printf_fn = LLVMAddFunction(module, "printf", printf_type);              \
    }                                                                          \
    LLVMBuildCall2(builder, LLVMGlobalGetValueType(printf_fn), printf_fn,      \
                   (LLVMValueRef[]){format_str, __VA_ARGS__}, num_args + 1,    \
                   "");                                                        \
  })
#endif
