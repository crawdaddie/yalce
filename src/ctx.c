#include "ctx.h"
#include "./common.h"
#include <pthread.h>

Ctx ctx;

void init_ctx() {
  ctx.main_vol = 0.25;
  ctx.head = NULL;
  ctx.sys_time = 0;
}

UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame) {
  perform_graph(ctx->head, nframes, seconds_per_frame);
}

// Graph *ctx_graph_head() { return ctx.head; }
//
// Graph *ctx_set_head(Graph *node) {
//   ctx.head = node;
//   return ctx.head;
// }
//
// Graph *ctx_add_after(Graph *node) {
//   ctx.head = node;
//   return ctx.head;
// }

double user_ctx_get_sample(Ctx *ctx, int channel, int frame) { return 0; };

double *get_sys_time() { return &ctx.sys_time; }
