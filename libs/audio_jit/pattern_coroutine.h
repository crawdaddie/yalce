#ifndef PATTERN_COROUTINE_H
#define PATTERN_COROUTINE_H

#include "../../lang/backend_llvm/common.h"
#include "../../lang/backend_llvm/jit.h" // JITLangCtx

// Parse a mini-notation pattern string and emit a looping Coroutine<double>
// that yields each step value in order, cycling forever.
//
// Syntax:
//   "1 2 3"        — yields 1., 2., 3., 1., 2., 3., ...
//   "1 <2 3>"      — yields 1., 2., 1., 3., 1., 2., ...
//                    (<a b> alternates one value per cycle)
//
// Returns the coroutine handle (ptr) or NULL on parse error.
LLVMValueRef emit_pattern_coroutine(const char *pattern_str, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder);

LLVMValueRef pattern_handler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder);

LLVMValueRef emit_key_pattern_coroutine(const char *pattern_str, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder);

LLVMValueRef pattern_key_handler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                 LLVMBuilderRef builder);
#endif
