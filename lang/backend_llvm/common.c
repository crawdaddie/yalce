#include "backend_llvm/common.h"

JITLangCtx ctx_push(JITLangCtx ctx) {
  return (JITLangCtx){ctx.stack, ctx.stack_ptr + 1, ctx.env};
}
