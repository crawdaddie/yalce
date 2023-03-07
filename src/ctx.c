#include "ctx.h"
#include "./common.h"
#include "graph/graph.h"

static UserCtx ctx = {.main_vol = 0.25};
double **alloc_buses(int num_buses) {
  double **buses;
  buses = calloc(num_buses, sizeof(double *));
  for (int i = 0; i < num_buses; i++) {
    buses[i] = calloc(BUF_SIZE, sizeof(double));
  };
  return buses;
}

void init_user_ctx(UserCtx *ctx) {
  /* ctx->head = new_graph(); */
  ctx->head = NULL;
}

void user_ctx_callback(UserCtx *ctx, int nframes, double seconds_per_frame) {
  perform_graph(ctx->head, nframes, seconds_per_frame);
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

double user_ctx_get_sample(UserCtx *ctx, int channel, int frame) { return 0; };
