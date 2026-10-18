#include "asyncio_mir.h"

#include "backend_llvm/lib_registry.h"

#include <stdio.h>

static YlcAsyncLoweringHooks lowering_hooks;

void ylc_async_set_lowering_hooks(const YlcAsyncLoweringHooks *hooks) {
  lowering_hooks = hooks ? *hooks : (YlcAsyncLoweringHooks){0};
}

MirValueId ylc_async_lower_async(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                 MirBuiltinSymbol *symbol) {
  if (lowering_hooks.async) {
    return lowering_hooks.async(builder, app, ctx, symbol);
  }

  fprintf(stderr, "asyncio: @Async MIR lowering is not installed\n");
  return MIR_NO_VALUE;
}

MirValueId ylc_async_lower_await(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                 MirBuiltinSymbol *symbol) {
  if (lowering_hooks.await) {
    return lowering_hooks.await(builder, app, ctx, symbol);
  }

  fprintf(stderr, "asyncio: await MIR lowering is not installed\n");
  return MIR_NO_VALUE;
}

static void register_symbol(const char *name, MirBuiltinHandler handler) {
  TypeEnv entry = {.name = name};
  mir_register_builtin(ylc_mir_program, &entry, handler,
                       MIR_BUILTIN_SYMBOL_EXTENSION, NULL, 0, MIR_RESULT_NONE);
}

void ylc_async_register_current_program(void) {
  if (!ylc_mir_program) {
    return;
  }

  register_symbol("Async", ylc_async_lower_async);
  register_symbol("await", ylc_async_lower_await);
}

static void ylc_async_register_program(MirProgram *program, MirCtx *ctx) {
  MirProgram *saved_program = ylc_mir_program;
  MirCtx *saved_ctx = ylc_mir_ctx;

  ylc_mir_program = program;
  ylc_mir_ctx = ctx;
  ylc_async_register_current_program();

  ylc_mir_program = saved_program;
  ylc_mir_ctx = saved_ctx;
}

__attribute__((constructor)) static void ylc_async_init(void) {
  ylc_register_mir_program_init(ylc_async_register_program);
  ylc_async_register_current_program();
}
