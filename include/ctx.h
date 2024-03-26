#ifndef _CTX_H
#define _CTX_H
#include "graph.h"
#include "msg_queue.h"
#include "node.h"

typedef struct {
  double main_vol;
  double block_time;
  Node *head;
  Signal dac_buffer;
  int sample_rate;
  msg_queue msg_queue;
  Graph graph;
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

Node *ctx_add_head(Node *node);

int process_msg_queue_pre(msg_queue *msg_queue);
void process_msg_queue_post(msg_queue *msg_queue, int consumed);

void ctx_rm_node(Node *node);

#endif
