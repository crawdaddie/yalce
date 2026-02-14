#include "./ctx.h"
#include "./node.h"
#include "audio_graph.h"
#include "audio_routing.h"
#include "ext_lib.h"
#include "group.h"
#include <stdio.h>
#include <stdlib.h>

Ctx ctx;

void init_ctx() {

  ctx.num_input_signals = num_signals;
  ctx.input_signals = malloc(sizeof(Signal) * num_signals);

  for (int i = 0; i < num_signals; i++) {
    int layout = signal_info[i].num_channels;

    ctx.input_signals[i].buf = calloc(1, sizeof(double) * BUF_SIZE * layout);
    ctx.input_signals[i].layout = layout;
    ctx.input_signals[i].size = BUF_SIZE;
  }
  ctx.main_vol = 0.25;
}

void audio_ctx_add(Node *node) {
  node->write_to_output = true;
  node_group_state *ctx = &get_audio_ctx()->graph;

  // Add to existing chain
  if (ctx->head == NULL) {
    ctx->head = node;
    ctx->tail = ctx->head;
  } else {
    // Find the end of the chain
    Node *current = ctx->head;
    while (current->next != NULL) {
      current = (Node *)current->next;
    }
    // Append to the end
    current->next = node;
  }
}

Node *add_to_dac(Node *node) {
  // return NULL;
  // node->type = OUTPUT;
  // node->write_to_dac = true;
  return node;
}

Ctx *get_audio_ctx() { return &ctx; }

void push_msg(msg_queue *queue, scheduler_msg msg, int buffer_offset) {
  msg.tick += buffer_offset;

  if (queue->num_msgs == MSG_QUEUE_MAX_SIZE) {
    // printf("Error: Command FIFO full overflow ? %d\n",
    //        queue == &ctx.overflow_queue);
    return;
  }

  *(queue->buffer + queue->write_ptr) = msg;
  queue->write_ptr = (queue->write_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs++;
}

scheduler_msg pop_msg(msg_queue *queue) {
  scheduler_msg msg = *(queue->buffer + queue->read_ptr);
  queue->read_ptr = (queue->read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs--;
  return msg;
}

scheduler_msg *create_bundle(int length) {
  return malloc(sizeof(scheduler_msg) * length);
}

int get_write_ptr() {
  Ctx *ctx = get_audio_ctx();
  return ctx->msg_queue.write_ptr;
}

int ctx_sample_rate() { return ctx.sample_rate; }
double ctx_spf() { return ctx.spf; }

double *ctx_main_out() { return ctx.output_buf; }
void set_main_vol(double vol) { ctx.main_vol = vol; }

void move_overflow() {
  msg_queue *queue = &ctx.overflow_queue;
  scheduler_msg msg;
  while (queue->num_msgs) {
    msg = pop_msg(queue);
    push_msg(&(ctx.msg_queue), msg, 0);
  }
}
