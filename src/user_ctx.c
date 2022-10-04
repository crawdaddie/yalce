#include "user_ctx.h"

Node *get_square_synth_node(double freq, double *bus, long sustain) {

  Node *head = get_sq_detune_node(freq);
  Node *tail = head;
  tail = node_add_to_tail(get_tanh_node(tail->out, 20.0), tail);
  tail =
      node_add_to_tail(get_biquad_lpf(tail->out, 2500, 0.2, 2.0, 48000), tail);
  Node *env = get_env_node(10, 25.0, (double)sustain);

  tail = node_mul(env, tail);

  synth_data *data = malloc(sizeof(synth_data) + sizeof(bus));
  data->graph = head;
  data->bus = bus;

  Node *out_node =
      alloc_node((NodeData *)data, NULL, (t_perform)perform_synth_graph,
                 "synth", (t_free_node)free_synth);
  env_set_on_free(env, out_node, on_env_free);
  return out_node;
}
Node *get_graph(double *bus) {
  int sample_rate = 48000;
  Node *head = alloc_node(NULL, NULL, (t_perform)perform_null, "head", NULL);
  Node *tail = head;

  tail =
      node_add_to_tail(get_delay_node(bus, 750, 1000, 0.3, sample_rate), tail);
  tail->out = bus;

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

void debug_synths(Node *graph) {
  if (graph->name != "head") {
    debug_node(graph, NULL);
  };

  if (graph->next) {
    return debug_synths(graph->next);
  };
  printf("----------\n");
}

double *get_bus(UserCtx *ctx, int bus_num) { return ctx->buses[bus_num]; }
void zero_bus(double *bus, int frame_count, double seconds_per_frame,
              double seconds_offset) {
  for (int i = 0; i < frame_count; i++) {
    bus[i] = 0;
  }
}
Node *ctx_play_synth(UserCtx *ctx, double freq, long sustain) {
  Node *head = ctx->graph;
  Node *next = head->next;
  Node *synth = node_add_to_tail(
      get_square_synth_node(freq, ctx->buses[0], sustain), head);
  synth->next = next;
  return synth;
};

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
  ctx->seconds_offset = 0.0;

  return ctx;
}

void debug_ctx(UserCtx *ctx) {
  printf("user_ctx\n");
  printf("user_ctx buses: %#08x\n", ctx->buses);
}
