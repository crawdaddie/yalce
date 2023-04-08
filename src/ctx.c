#include "ctx.h"
#include "audio/blip.h"
#include "audio/osc.h"
#include "common.h"
#include "log.h"
#include <stdlib.h>

#include <pthread.h>

static double output_channel_pool[OUTPUT_CHANNELS][BUF_SIZE * LAYOUT_CHANNELS];
static double channel_vols[OUTPUT_CHANNELS] = {[0 ... OUTPUT_CHANNELS - 1] =
                                                   1.0};

Ctx ctx;
static Node *tail = NULL;
void init_ctx() {
  ctx.main_vol = 0.25;
  ctx.sys_time = 0;

  for (int i = 0; i < OUTPUT_CHANNELS; i++) {
    ctx.out_chans[i].data = output_channel_pool[i];
    ctx.out_chans[i].size = BUF_SIZE;
    ctx.out_chans[i].layout = LAYOUT_CHANNELS;
  }
  ctx.channel_vols = channel_vols;

  ctx.head = NULL;
  tail = ctx.head;
  osc_setup();
  blip_setup();
}

UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame) {
  perform_graph(ctx->head, nframes, seconds_per_frame);
}

Node *ctx_graph_head() { return ctx.head; }

Node *ctx_graph_tail() { return tail; }

static void add_after(Node *head, Node *node) {
  head->next = node;
  node->prev = head;
}

void ctx_remove_node(Node *node) {

  Node *before = node->prev;

  Node *after = node->next;

  if (after == NULL && before == NULL) {

    ctx.head = NULL;
    tail = ctx.head;
    free(node);
    return;
  }

  if (node == tail) {
    tail = before;
    tail->next = NULL;
    free(node);
    return;
  }
  before->next = after;
  after->prev = before;
  free(node);
  return;
}

void ctx_add_after_tail(Node *node) {
  if (ctx.head == NULL && tail == NULL) {
    ctx.head = node;

    while (node->next) {
      node = node->next;
    }
    tail = node;
    return;
  }

  add_after(tail, node);
  while (node->next) {
    node = node->next;
  }
  tail = node;
}
// Graph *ctx_set_head(Graph *node) {
//   ctx.head = node;
//   return ctx.head;
// }
//
// Graph *ctx_add_after(Graph *node) {
//   ctx.head = node;
//   return ctx.head;
// }

double user_ctx_get_sample(Ctx *ctx, int channel, int frame) { return 0; };

double *get_sys_time() { return &ctx.sys_time; }

int channel_data_idx(int frame, int layout_channel) {
  return (BUF_SIZE * layout_channel) + frame;
}
