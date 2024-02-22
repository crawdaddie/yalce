#ifndef _CTX_H
#define _CTX_H
#include "node.h"

typedef struct {
  double main_vol;
  double block_time;
  Node *head;
  Signal dac_buffer;
  int SR;
} Ctx;

double **alloc_buses(int num_buses);

void init_ctx();
typedef void (*UserCtxCb)(Ctx *ctx, int nframes, double spf);

UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame);

extern Ctx ctx;

Node *perform_graph(Node *head, int nframes, double seconds_per_frame,
                    Signal *dac_buffer, int output_num);

Ctx *get_audio_ctx();
int ctx_sample_rate();

Node *ctx_get_tail();
Node *ctx_add(Node *node);
#endif
