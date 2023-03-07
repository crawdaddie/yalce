#ifndef _CTX_H
#define _CTX_H
#include "common.h"
#include "graph/graph.h"

typedef struct {
  bool mute;
  double data[BUF_SIZE * LAYOUT_CHANNELS];
} Channel;

typedef struct UserCtx {
  double main_vol;
  double sched_time;
  Graph *head;
  Channel out_chans[OUTPUT_CHANNELS];
} UserCtx;

double **alloc_buses(int num_buses);

void init_ctx();
typedef void (*UserCtxCb)(UserCtx *ctx, int nframes, double spf);
UserCtxCb user_ctx_callback(UserCtx *ctx, int nframes,
                            double seconds_per_frame);
double user_ctx_get_sample(UserCtx *ctx, int channel, int frame);

Graph *ctx_graph_head();

Graph *ctx_set_head(Graph *node);

Graph *ctx_add_after(Graph *node);

/* typedef Channel *(*UserCtxCb)(int nframes, double spf); */
/* Channel *(UserCtxCallback)(int frame_count, double seconds_per_frame); */
/* UserCtxCb user_callback(int frame_count, double seconds_per_frame); */
extern UserCtx ctx;
#endif
