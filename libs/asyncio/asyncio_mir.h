#ifndef YLC_ASYNCIO_MIR_H
#define YLC_ASYNCIO_MIR_H

#include "mir/mir.h"

typedef MirValueId (*YlcAsyncLowerFn)(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirBuiltinSymbol *symbol);

typedef struct {
  YlcAsyncLowerFn async;
  YlcAsyncLowerFn await;
} YlcAsyncLoweringHooks;

/* Install lowering callbacks before registering a MIR program. */
void ylc_async_set_lowering_hooks(const YlcAsyncLoweringHooks *hooks);

/* Register Async and await extension symbols in the current MIR program. */
void ylc_async_register_current_program(void);

/* Extension entry points used by MIR builtin dispatch. */
MirValueId ylc_async_lower_async(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                 MirBuiltinSymbol *symbol);
MirValueId ylc_async_lower_await(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                 MirBuiltinSymbol *symbol);

#endif
