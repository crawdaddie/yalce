#include "backend_llvm/common.h"

JITLangCtx ctx_push(JITLangCtx ctx) {
  return (JITLangCtx){
    ctx.stack,
    ctx.stack_ptr + 1,
    ctx.env,
    ctx.num_globals,
    ctx.global_storage_array,
    ctx.global_storage_capacity,
  };
}
