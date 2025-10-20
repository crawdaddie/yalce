#include "backend_llvm/common.h"
#include "escape_analysis.h"
#include "llvm-c/Core.h"
#include <stdarg.h>
#include <stdlib.h>

StackFrame *frame_extend(StackFrame *frame) {
  StackFrame *new_frame = malloc(sizeof(StackFrame));
  new_frame->table = ht_create();
  new_frame->next = frame;
  return new_frame;
}

JITLangCtx ctx_push(JITLangCtx ctx) {
  JITLangCtx new_ctx = ctx;
  new_ctx.stack_ptr = ctx.stack_ptr + 1;
  new_ctx.frame = frame_extend(ctx.frame);
  return new_ctx;
}

JITSymbol *find_in_ctx(const char *name, int name_len, JITLangCtx *ctx) {
  uint64_t hash = hash_string(name, name_len);
  StackFrame *frame = ctx->frame;
  while (frame != NULL) {
    JITSymbol *sym = ht_get_hash(frame->table, name, hash);
    if (sym != NULL) {
      return sym;
    }
    frame = frame->next;
  }
  return NULL;
}

void destroy_ctx(JITLangCtx *ctx) { free(ctx->frame->table->entries); }

EscapeStatus find_allocation_strategy(Ast *expr, JITLangCtx *ctx) {
  if (expr->ea_md && ctx->stack_ptr != 0) {
    return ((EscapeMeta *)expr->ea_md)->status;
  }
  return EA_HEAP_ALLOC;
}

LLVMValueRef STRUCT(LLVMTypeRef type, LLVMBuilderRef builder, int num_values,
                    ...) {
  // Start with an undef value of the given struct type
  LLVMValueRef result = LLVMGetUndef(type);

  // Initialize varargs
  va_list args;
  va_start(args, num_values);

  // Successively insert each value at the corresponding index
  for (int i = 0; i < num_values; i++) {
    LLVMValueRef value = va_arg(args, LLVMValueRef);
    result = LLVMBuildInsertValue(builder, result, value, i, "");
  }

  va_end(args);
  return result;
}
