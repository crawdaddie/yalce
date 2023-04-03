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

Node *ctx_graph_head() { return ctx.head; }
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

double channel_read_destructive(int out_chan, int layout_channel, int frame) {
  int data_idx = (LAYOUT_CHANNELS * frame) + layout_channel;
  double output = ctx.out_chans[out_chan].data[frame + layout_channel];
  ctx.out_chans[out_chan].data[frame + layout_channel] = 0.0;
  return output;
}
