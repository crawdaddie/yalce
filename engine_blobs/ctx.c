#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>

Ctx ctx;

void init_ctx() {}

void perform_graph(Node *, int, double, double *, int, int);

void user_ctx_callback(Ctx *ctx, int frame_count, double spf) {
  perform_graph(ctx->head, frame_count, spf, ctx->output_buf, LAYOUT, 0);
}

Ctx *get_audio_ctx() { return &ctx; }
