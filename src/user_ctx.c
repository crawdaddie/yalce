#include "audio/node.h"
#include "audio/synth.c"
#define BUS_NUM 8
#define BUS_SIZE 512

typedef struct UserCtx {
  double **buses;
  Node *graph;
  /* void (*ctx_play_synth)(struct UserCtx *ctx); */
  /* double *(*get_bus)(struct UserCtx *ctx, int bus_num); */
} UserCtx;

Node *get_graph(double *bus) {
  int sample_rate = 48000;
  Node *head =
      alloc_node(NULL, NULL, NULL, (t_perform)perform_null, "head", NULL);
  Node *tail = head;
  tail = node_add_to_tail(get_synth(220.0, bus), tail);

  tail = node_add_to_tail(get_delay_node(bus, bus, 750, 1000, 0.3, sample_rate),
                          tail);

  return head;
}

void debug_graph(Node *graph) {
  debug_node(graph, NULL);

  if (graph->next) {
    printf("↓\n");
    return debug_graph(graph->next);
  };
  printf("----------\n");
}

void ctx_play_synth(UserCtx *ctx) {}
double *get_bus(UserCtx *ctx, int bus_num) { return ctx->buses[bus_num]; }

UserCtx *get_user_ctx() {
  double **buses;
  buses = calloc(BUS_NUM, sizeof(double *));
  for (int i = 0; i < BUS_NUM; i++) {
    buses[i] = calloc(BUS_SIZE, sizeof(double));
  };

  Node *graph = get_graph(buses[0]);
  UserCtx *ctx = malloc(sizeof(UserCtx) + sizeof(buses) + sizeof(graph));
  ctx->buses = buses;
  ctx->graph = graph;

  return ctx;
}

void debug_ctx(UserCtx *ctx) {
  printf("user_ctx\n");
  printf("user_ctx buses: %#08x\n", ctx->buses);
}
