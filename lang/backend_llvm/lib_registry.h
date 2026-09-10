#ifndef YLC_LIB_REGISTRY_H
#define YLC_LIB_REGISTRY_H

#include <stdbool.h>
#include <llvm-c/Types.h>

typedef struct JITLangCtx JITLangCtx;
typedef struct MirProgram MirProgram;
typedef struct MirCtx MirCtx;
typedef void (*YlcMirProgramInitFn)(MirProgram *program, MirCtx *ctx);

/*
 * Set immediately before a compiler-triggered dlopen() and cleared after the
 * call returns. A loaded library can inspect these from an
 * __attribute__((constructor)) function to register compiler extensions.
 *
 * During AST-to-MIR construction, ylc_mir_program and ylc_mir_ctx are set.
 * During LLVM lowering, ylc_jit_ctx, ylc_jit_module and ylc_jit_builder are
 * set instead.
 *
 * Usage in a library constructor:
 *
 *   #include "backend_llvm/lib_registry.h"
 *
 *   __attribute__((constructor))
 *   static void ylc_mylib_init(void) {
 *       if (ylc_mir_program) {
 *           // Register MIR builtins / extension ops.
 *       } else if (ylc_jit_ctx) {
 *           // Register legacy LLVM backend builtins.
 *       }
 *   }
 */
extern JITLangCtx *ylc_jit_ctx;
extern LLVMModuleRef ylc_jit_module;
extern LLVMBuilderRef ylc_jit_builder;
extern MirProgram *ylc_mir_program;
extern MirCtx *ylc_mir_ctx;
extern YlcMirProgramInitFn ylc_mir_program_init_fn;
typedef void (*YlcRuntimeLoadFn)(void);

extern YlcRuntimeLoadFn ylc_runtime_load_fn;

bool ylc_link_llvm_bitcode_file(LLVMModuleRef module, const char *path);

#endif
