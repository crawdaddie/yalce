#include "backend_llvm/common.h"
#include <stdlib.h>

StackFrame *frame_extend(StackFrame *frame) {
  StackFrame *new_frame = malloc(sizeof(StackFrame));
  new_frame->table = ht_create();
  new_frame->next = frame;
  return new_frame;
}
bool is_top_level_frame(StackFrame *frame) { return frame->next == NULL; }

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
