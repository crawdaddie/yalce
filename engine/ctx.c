#include "./ctx.h"
#include "./node.h"
#include "audio_graph.h"
#include "audio_routing.h"
#include "ext_lib.h"
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
  ctx.main_vol = 0.5;
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

static void process_msg_pre(int frame_offset, scheduler_msg msg) {

  switch (msg.type) {
  case NODE_ADD: {
    struct NODE_ADD payload = msg.payload.NODE_ADD;
    payload.target->frame_offset = frame_offset;
    if (payload.group) {
      group_add(payload.target, payload.group);
    } else {
      audio_ctx_add(payload.target);
    }

    break;
  }

  case NODE_SET_SCALAR: {
    struct NODE_SET_SCALAR payload = msg.payload.NODE_SET_SCALAR;
    Node *node = payload.target;
    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);
      if (node->state_ptr) {
        g = node->state_ptr;
      }
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      for (int i = frame_offset; i < BUF_SIZE; i++) {
        inlet_data.buf[i] = payload.value;
      }
    }

    break;
  }

  case NODE_SET_INPUT: {
    struct NODE_SET_INPUT payload = msg.payload.NODE_SET_INPUT;
    Node *node = payload.target;
    Node *buf = payload.value;

    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);
      if (node->state_ptr) {
        g = node->state_ptr;
      }
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      inlet_node->output.layout = buf->output.layout;
      inlet_node->output.size = buf->output.size;
      inlet_node->output.buf = buf->output.buf;
    }

    break;
  }

  case NODE_SET_TRIG: {
    struct NODE_SET_TRIG payload = msg.payload.NODE_SET_TRIG;
    Node *node = payload.target;

    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);
      if (node->state_ptr) {
        g = node->state_ptr;
      }
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      inlet_data.buf[frame_offset] = 1.0;
    }

    break;
  }
  default:
    break;
  }
}

static void process_msg_post(int frame_offset, scheduler_msg msg) {
  switch (msg.type) {
  case NODE_ADD: {
    break;
  }

  case GROUP_ADD: {
    break;
  }

  case NODE_SET_SCALAR: {

    struct NODE_SET_SCALAR payload = msg.payload.NODE_SET_SCALAR;
    Node *node = payload.target;

    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);
      if (node->state_ptr) {
        g = node->state_ptr;
      }
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      for (int i = 0; i < frame_offset; i++) {
        inlet_data.buf[i] = payload.value;
      }
    }

    break;
  }

  case NODE_SET_TRIG: {
    struct NODE_SET_TRIG payload = msg.payload.NODE_SET_TRIG;
    Node *node = payload.target;

    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);

      if (node->state_ptr) {
        g = node->state_ptr;
      }
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      inlet_data.buf[frame_offset] = 0.0;
    }
    break;
  }
  default:
    break;
  }
}

int process_msg_queue_pre(uint64_t current_tick, msg_queue *queue) {
  int read_ptr = queue->read_ptr;
  scheduler_msg *msg;
  int consumed = 0;
  int write_ptr = queue->write_ptr;
  int num_moved = 0;
  while (read_ptr != queue->write_ptr) {
    msg = queue->buffer + read_ptr;
    if (msg->tick - current_tick >= 512) {
      // TODO: if msg->tick - current_tick > 512 - push message to write_ptr
      push_msg(&ctx.overflow_queue, *msg, 0);
      num_moved++;
    } else if (msg->tick - current_tick < 0) {
      printf("too late for msg\n");
    } else {
      process_msg_pre(msg->tick - current_tick, *msg);
    }

    read_ptr = (read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
    consumed++;
  }

  return consumed;
}

void process_msg_queue_post(uint64_t current_tick, msg_queue *queue,
                            int consumed) {
  scheduler_msg msg;
  while (consumed--) {
    msg = pop_msg(queue);
    if (msg.tick - current_tick >= 512) {
      // skip
    } else {
      int frame_offset = msg.tick - current_tick;
      process_msg_post(frame_offset, msg);
    }
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
