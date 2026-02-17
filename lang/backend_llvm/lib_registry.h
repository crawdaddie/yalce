#ifndef YLC_LIB_REGISTRY_H
#define YLC_LIB_REGISTRY_H

#include "backend_llvm/common.h"
#include <llvm-c/Types.h>

/*
 * Set by DlOpenHandler immediately before calling dlopen().
 * Cleared to NULL after the call returns.
 *
 * Libraries loaded via `dlopen` can declare these as extern and read them
 * inside a __attribute__((constructor)) function to register new builtins
 * into the active JIT context — the same way initialize_builtin_funcs() does.
 *
 * Usage in a library:
 *
 *   #include "backend_llvm/lib_registry.h"
 *   #include "backend_llvm/symbols.h"
 *   #include "common.h"
 *
 *   static LLVMValueRef MyHandler(Ast *ast, JITLangCtx *ctx,
 *                                 LLVMModuleRef module, LLVMBuilderRef builder)
 *   { ... }
 *
 *   __attribute__((constructor))
 *   static void ylc_mylib_init(void) {
 *       if (!ylc_jit_ctx) return;
 *       ht *stack = ylc_jit_ctx->frame->table;
 *       JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, NULL, NULL, NULL);
 *       sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = MyHandler;
 *       const char *name = "my_builtin";
 *       ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
 *   }
 */
extern JITLangCtx    *ylc_jit_ctx;
extern LLVMModuleRef  ylc_jit_module;
extern LLVMBuilderRef ylc_jit_builder;

#endif
