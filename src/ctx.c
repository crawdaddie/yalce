#include "ctx.h"
#include "audio/blip.h"
#include "audio/osc.h"
#include "common.h"

#include <pthread.h>

static double output_channel_pool[OUTPUT_CHANNELS][BUF_SIZE * LAYOUT_CHANNELS];
static double channel_vols[OUTPUT_CHANNELS] = {[0 ... OUTPUT_CHANNELS - 1] =
                                                   1.0};

Ctx ctx;

void init_ctx() {
  ctx.main_vol = 0.25;
  ctx.sys_time = 0;

  for (int i = 0; i < OUTPUT_CHANNELS; i++) {
    /* ctx.out_chans[i].data = output_channel_pool[i]; */
    /* ctx.out_chans[i].size = BUF_SIZE; */
    /* ctx.out_chans[i].layout = LAYOUT_CHANNELS; */
    ctx.out_chans[i].vol = 1.0;
  }
  /* ctx.channel_vols = channel_vols; */

  ctx.head = NULL;
  osc_setup();
  blip_setup();
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

int channel_data_idx(int frame, int layout_channel) {
  return (BUF_SIZE * layout_channel) + frame;
}
