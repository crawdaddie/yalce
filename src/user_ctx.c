#include "user_ctx.h"

Node *get_graph(struct SoundIoOutStream *outstream, double *bus) {
  int sample_rate = synth_sample_rate;
  Node *head = alloc_node(NULL, NULL, (t_perform)perform_null, "head", NULL);
  Node *tail = head;

  tail =
      node_add_to_tail(get_delay_node(bus, 750, 1000, 0.3, sample_rate), tail);

  return head;
}
void ctx_play_synth(UserCtx *ctx) {}
double *get_bus(UserCtx *ctx, int bus_num) { return ctx->buses[bus_num]; }

UserCtx *get_user_ctx(struct SoundIoOutStream *outstream) {
  double **buses[16];
  for (int i = 0; i < 16; i++) {
    buses[i] = get_buffer();
  }
  UserCtx *ctx = malloc(sizeof(UserCtx) + sizeof(buses));
  ctx->ctx_play_synth = ctx_play_synth;
  ctx->get_bus = get_bus;

  return ctx;
}
