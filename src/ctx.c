#include "ctx.h"
#include "audio/osc.h"
#include "common.h"
#include "log.h"
// #include "midi.h"
#include <stdlib.h>

#include "dbg.h"
#include <pthread.h>

static double output_channel_pool[OUTPUT_CHANNELS][BUF_SIZE * LAYOUT_CHANNELS];
static double channel_vols[OUTPUT_CHANNELS] = {[0 ... OUTPUT_CHANNELS - 1] =
                                                   1.0};

Ctx ctx;
static Node *tail = NULL;
void init_ctx() {
  ctx.main_vol = 0.125;
  ctx.sys_time = 0.0;

  /* for (int i = 0; i < OUTPUT_CHANNELS; i++) { */
  /*   ctx.out_chans[i].data = output_channel_pool[i]; */
  /*   ctx.out_chans[i].size = BUF_SIZE; */
  /*   ctx.out_chans[i].layout = LAYOUT_CHANNELS; */
  /* } */

  /* ctx.channel_vols = channel_vols; */
  ctx.DAC.data = *output_channel_pool;
  ctx.DAC.size = BUF_SIZE;
  ctx.DAC.layout = LAYOUT_CHANNELS;

  ctx.head = NULL;
  tail = ctx.head;
  osc_setup();
  init_queue(&ctx.queue);
  // midi_setup();
}

UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame) {
  if (ctx->head == NULL) {
    return NULL;
  }
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

double user_ctx_get_sample(Ctx *ctx, int channel, int frame) { return 0; };

double *get_sys_time() { return &ctx.sys_time; }

int channel_data_idx(int frame, int layout_channel) {
  return (BUF_SIZE * layout_channel) + frame;
}

void ctx_add_node_out_to_output(Signal *out_ptr, int nframes,
                                double seconds_per_frame) {
  Signal output = ctx.DAC;
  Signal out = *out_ptr;

  int write_idx = 0;
  int read_idx = 0;
  if (out.layout == 1) {
    for (int f = 0; f < nframes; f++) {
      write_idx = SAMPLE_IDX(output, f, 0);
      read_idx = SAMPLE_IDX(out, f, 0);
      double samp = out.data[read_idx];
      output.data[write_idx] += samp;
      output.data[write_idx + 1] += samp;
    }
    return;
  }

  if (out.layout == 2) {
    for (int f = 0; f < nframes; f++) {
      write_idx = SAMPLE_IDX(output, f, 0);
      read_idx = SAMPLE_IDX(out, f, 0);
      output.data[write_idx] += out.data[read_idx];
      output.data[++write_idx] += out.data[read_idx];
    } // TODO: create code for multichannel configurations
    return;
  }
}
/**
 * get number of frames into the block at
 * which to schedule the audible change the message represents
 **/
int get_msg_block_offset(Msg msg, Ctx ctx, double sample_rate) {
  return (int)(msg.timestamp - ctx.block_time) * sample_rate;
}

void dump_graph() { dump_nodes(ctx.head, 0); }
void handle_queue(Ctx *ctx, double sample_rate) {

  for (int i = 0; i < ctx->queue.top; i++) {
    Msg msg = ctx->queue.items[i];
    if (msg.handler != NULL) {
      int block_offset = get_msg_block_offset(msg, *ctx, sample_rate);
      msg.handler(ctx, msg, block_offset);
    }
  }
  ctx->queue.top = 0;
}
