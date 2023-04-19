#ifndef _CTX_H
#define _CTX_H
#include "audio/signal.h"
#include "common.h"
#include "node.h"

typedef struct {
  bool mute;
  Node *head;
  double vol;
  /* double data[BUF_SIZE]; */
} Channel;

typedef struct {
  double main_vol;
  double sys_time; /* global time in secs */
  Node *head;
  Signal out_chans[OUTPUT_CHANNELS];
  Signal DAC;
  double *channel_vols;
  /* Channel out_chans[OUTPUT_CHANNELS]; */
} Ctx;

double **alloc_buses(int num_buses);

void init_ctx();
typedef void (*UserCtxCb)(Ctx *ctx, int nframes, double spf);

UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame);

double user_ctx_get_sample(Ctx *ctx, int channel, int frame);

extern Ctx ctx;

Node *ctx_graph_head();

Node *ctx_graph_tail();

void ctx_add_after_tail(Node *node);

void ctx_remove_node(Node *node);

double channel_read_destructive(int out_chan, int stereo_channel, int frame);

int channel_data_idx(int frame, int layout_channel);

void ctx_add_node_out_to_output(Signal *out, int nframes,
                                double seconds_per_frame);
void dump_graph();
#endif
