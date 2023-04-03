#ifndef _CTX_H
#define _CTX_H
#include "common.h"
#include "node.h"

typedef struct {
  bool mute;
  double data[BUF_SIZE * LAYOUT_CHANNELS];
} Channel;

typedef struct {
  double main_vol;
  double sys_time; /* global time in secs */
  Node *head;
  Channel out_chans[OUTPUT_CHANNELS];
} Ctx;

double **alloc_buses(int num_buses);

void init_ctx();
typedef void (*UserCtxCb)(Ctx *ctx, int nframes, double spf);

UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame);

double user_ctx_get_sample(Ctx *ctx, int channel, int frame);

extern Ctx ctx;

Node *ctx_graph_head();

double channel_read_destructive(int out_chan, int stereo_channel, int frame);
#endif
