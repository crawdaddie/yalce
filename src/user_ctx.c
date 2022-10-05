#include "user_ctx.h"

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
Node *get_tail(Node *node) {
  if (node->next == NULL) {
    return node;
  }
  return get_tail(node->next);
}

UserCtx *get_user_ctx(double latency) {
  double **buses;
  buses = calloc(BUS_NUM, sizeof(double *));
  for (int i = 0; i < BUS_NUM; i++) {
    buses[i] = calloc(BUS_SIZE, sizeof(double));
  };

  UserCtx *ctx = malloc(sizeof(UserCtx) + sizeof(buses));
  ctx->buses = buses;
  ctx->seconds_offset = 0.0;
  ctx->latency = latency;
  ctx->graphs = NULL;

  return ctx;
}

Node *add_graph_to_ctx(UserCtx *ctx) {
  Node *new_graph =
      alloc_node(NULL, NULL, (t_perform)perform_null, "head", NULL);
  if (!ctx->graphs) {
    ctx->graphs = malloc(sizeof(List) + sizeof(new_graph));
    ((List *)ctx->graphs)->value = new_graph;
    return new_graph;
  };
  List *list_el = malloc(sizeof(List) + sizeof(new_graph));
  list_el->value = new_graph;
  List *tail = ctx->graphs;
  while (tail->next) {
    tail = tail->next;
  };
  tail->next = list_el;

  return new_graph;
}

void iterate_list(List *list, void (*cb)(void *list_el)) {
  cb(list->value);
  if (list->next) {
    return iterate_list(list->next, cb);
  }
}

void perform_ctx_group(Node *group, int frame_count, double seconds_per_frame,
                       double seconds_offset) {

  perform_graph(group, frame_count, seconds_per_frame,
                seconds_offset); // compute a block of samples and
                                 // write it to a bus
}

void iterate_groups_perform(List *list, int frame_count,
                            double seconds_per_frame, double seconds_offset) {
  perform_graph(list->value, frame_count, seconds_per_frame, seconds_offset);
  if (list->next) {
    return iterate_groups_perform(list->next, frame_count, seconds_per_frame,
                                  seconds_offset);
  }
}

struct player_ctx *get_player_ctx_ref(UserCtx *ctx) {

  Node *group = add_graph_to_ctx(ctx);
  struct player_ctx *player_ctx = malloc(sizeof(struct player_ctx));
  player_ctx->ctx = ctx;
  player_ctx->group = group;
  return player_ctx;
}
