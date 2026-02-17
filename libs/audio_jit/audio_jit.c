#include "audio_jit.h"
#include "backend_llvm/lib_registry.h"
#include "backend_llvm/symbols.h"
#include "common.h"
#include "ht.h"
#include "serde.h"
#include "types/type_ser.h"
#include <llvm-c/Core.h>
#include <stdio.h>
#include <string.h>

LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  fprintf(stderr, "CompileAudioFnHandler: not yet implemented\n");
  printf("jit this: ");
  Ast *fn = ast->data.AST_APPLICATION.args;
  print_ast(fn);
  print_type(fn->type);
  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

static void register_builtin(ht *stack, const char *name,
                             BuiltinHandler handler) {
  JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, NULL, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = handler;
  ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
}

__attribute__((constructor)) static void ylc_audio_jit_init(void) {
  if (!ylc_jit_ctx) {
    fprintf(stderr, "libaudio_jit: no active JIT context at load time\n");
    return;
  }

  ht *stack = ylc_jit_ctx->frame->table;
  register_builtin(stack, "compile_audio_fn", CompileAudioFnHandler);

  fprintf(stderr, "libaudio_jit: registered compile_audio_fn\n");
}
