#ifndef _CTX_H
#define _CTX_H
#include "./config.h"
#include "graph/graph.c"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct UserCtx {
  double main_vol;
  Graph *head;
} UserCtx;

UserCtx ctx = {.main_vol = 0.25};

double **alloc_buses(int num_buses) {
  double **buses;
  buses = calloc(num_buses, sizeof(double *));
  for (int i = 0; i < num_buses; i++) {
    buses[i] = calloc(BUF_SIZE, sizeof(double));
  };
  return buses;
}

void init_user_ctx() { ctx.head = new_graph(); }

void user_ctx_callback(int nframes, double seconds_per_frame) {
  perform_graph(ctx.head, nframes, seconds_per_frame);
}

Graph *ctx_graph_head() { return ctx.head; }

Graph *ctx_set_head(Graph *node) {
  ctx.head = node;
  return ctx.head;
}

Graph *ctx_add_after(Graph *node) {
  ctx.head = node;
  return ctx.head;
}
void graph_set(Graph *graph, char *key, void *val) {}

#endif
