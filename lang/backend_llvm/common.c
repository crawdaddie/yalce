#include "backend_llvm/common.h"
#include <stdlib.h>

StackFrame *frame_extend(StackFrame *frame) {
  StackFrame *new_frame = malloc(sizeof(StackFrame));
  new_frame->table = ht_create();
  new_frame->next = frame;
  return new_frame;
}

JITLangCtx ctx_push(JITLangCtx ctx) {
  return (JITLangCtx){
      .stack_ptr = ctx.stack_ptr + 1,
      .frame = frame_extend(ctx.frame),
      .env = ctx.env,
      .num_globals = ctx.num_globals,
      .global_storage_array = ctx.global_storage_array,
      .global_storage_capacity = ctx.global_storage_capacity,
  };
}

JITSymbol *find_in_ctx(const char *name, int name_len, JITLangCtx *ctx) {
  uint64_t hash = hash_string(name, name_len);
  StackFrame *frame = ctx->frame;
  while (frame != NULL) {
    JITSymbol *sym = ht_get_hash(&frame->table, name, hash);
    if (sym != NULL) {
      return sym;
    }
    frame = frame->next;
  }
  return NULL;
}
