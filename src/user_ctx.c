#include "audio/node.h"
#include "audio/synth.c"

typedef struct UserCtx {
  double **buses;
  Node *graph;
  void (*ctx_play_synth)(struct UserCtx *ctx);
  double *(*get_bus)(struct UserCtx *ctx, int bus_num);
} UserCtx;

Node *get_graph(double *bus) {
  int sample_rate = 48000;
  Node *head = alloc_node(NULL, NULL, (t_perform)perform_null, "head", NULL);
  Node *tail = head;

  tail =
      node_add_to_tail(get_delay_node(bus, 750, 1000, 0.3, sample_rate), tail);

  return head;
}
void ctx_play_synth(UserCtx *ctx) {}
double *get_bus(UserCtx *ctx, int bus_num) { return ctx->buses[bus_num]; }

UserCtx *get_user_ctx() {
  double **buses;
  buses = calloc(16, sizeof(double *));
  for (int i = 0; i < 16; i++) {
    buses[i] = calloc(2048, sizeof(double));
  };

  Node *graph = get_graph(buses[0]);
  UserCtx *ctx = malloc(sizeof(UserCtx) + sizeof(buses));
  ctx->buses = buses;
  ctx->ctx_play_synth = ctx_play_synth;
  ctx->get_bus = get_bus;

  return ctx;
}

void debug_ctx(UserCtx *ctx) {
  printf("user_ctx\n");
  printf("user_ctx buses: %#08x\n", ctx->buses);
}
