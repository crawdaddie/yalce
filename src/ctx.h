#ifndef _CTX_H
#define _CTX_H
#include "graph/graph.h"

typedef struct UserCtx {
  double main_vol;
  Graph *head;
} UserCtx;

double **alloc_buses(int num_buses);

void init_user_ctx();

void user_ctx_callback(int nframes, double seconds_per_frame);

Graph *ctx_graph_head();

Graph *ctx_set_head(Graph *node);

Graph *ctx_add_after(Graph *node);

#endif
