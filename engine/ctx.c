#include "./ctx.h"
#include "./node.h"
#include "audio_graph.h"
#include <stdio.h>
#include <stdlib.h>

Ctx ctx;

void init_ctx() {}

// static void write_null_to_output_buf(double *out, int nframes, int layout) {
//   double *dest = out;
//   while (nframes--) {
//     for (int ch = 0; ch < layout; ch++) {
//       *dest = 0.0;
//       dest++;
//     }
//   }
// }

// void print_graph(Node *node) {
//   Node *n = node;
//
//   while (n != NULL) {
//     if (n->node_perform == group_perform) {
//       print_graph(((group_state *)n->state)->head);
//     }
//     n = n->next;
//   }
// }

void audio_ctx_add(Node *ensemble) {
  Ctx *ctx = get_audio_ctx();

  // Add to existing chain
  if (ctx->head == NULL) {
    ctx->head = ensemble;
  } else {
    // Find the end of the chain
    Node *current = ctx->head;
    while (current->next != NULL) {
      current = current->next;
    }
    // Append to the end
    current->next = ensemble;
  }
}
static void process_msg_pre(scheduler_msg msg) {

  switch (msg.type) {
  case NODE_ADD: {
    struct NODE_ADD payload = msg.payload.NODE_ADD;
    int frame_offset = msg.frame_offset;
    payload.target->frame_offset = frame_offset;
    audio_ctx_add(payload.target);
    //
    // payload.target->write_to_dac = true;
    break;
  }

    // case GROUP_ADD: {
    //   struct GROUP_ADD payload = msg.payload.GROUP_ADD;
    //   int frame_offset = msg.frame_offset;
    //   Node *g = payload.group;
    //   g->frame_offset = frame_offset;
    //   g->type = OUTPUT;
    //   g->next = NULL;
    //   audio_ctx_add(g);
    //
    //   break;
    // }

  case NODE_SET_SCALAR: {
    struct NODE_SET_SCALAR payload = msg.payload.NODE_SET_SCALAR;
    Node *node = payload.target;
    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      for (int i = msg.frame_offset; i < BUF_SIZE; i++) {
        inlet_data.data[i] = payload.value;
      }
    }

    // double *target_input_buf = get_node_input_buf(node, payload.input);
    // int target_input_size = get_node_input_size(node, payload.input);
    // for (int i = msg.frame_offset; i < target_input_size; i++) {
    //   *(target_input_buf + i) = payload.value;
    // }
    break;
  }

  case NODE_SET_TRIG: {
    // struct NODE_SET_TRIG payload = msg.payload.NODE_SET_TRIG;
    // Node *node = payload.target;
    // double *target_input_buf = get_node_input_buf(node, payload.input);
    // *(target_input_buf + msg.frame_offset) = 1.0;
    break;
  }
  default:
    break;
  }
}

static void process_msg_post(scheduler_msg msg) {
  switch (msg.type) {
  case NODE_ADD: {
    // struct NODE_ADD payload = msg.body.NODE_ADD;
    // int frame_offset = msg.frame_offset;
    // printf("node add %d\n", frame_offset);
    // unoffset_node_bufs(payload.target, frame_offset);
    // payload.target->frame_offset = 0;
    break;
  }

  case GROUP_ADD: {
    // struct NODE_ADD payload = msg.body.NODE_ADD;
    // int frame_offset = msg.frame_offset;
    // printf("node add %d\n", frame_offset);
    // unoffset_node_bufs(payload.target, frame_offset);
    // payload.target->frame_offset = 0;
    break;
  }

  case NODE_SET_SCALAR: {

    struct NODE_SET_SCALAR payload = msg.payload.NODE_SET_SCALAR;
    Node *node = payload.target;
    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      for (int i = 0; i < msg.frame_offset; i++) {
        inlet_data.data[i] = payload.value;
      }
    }
    break;
  }

  case NODE_SET_TRIG: {
    // struct NODE_SET_TRIG payload = msg.payload.NODE_SET_TRIG;
    // Node *node = payload.target;
    //
    // double *target_input_buf = get_node_input_buf(node, payload.input);
    // *(target_input_buf + msg.frame_offset) = 0.0;
    //
    break;
  }
  default:
    break;
  }
}

int process_msg_queue_pre(msg_queue *queue) {
  int read_ptr = queue->read_ptr;
  scheduler_msg *msg;
  int consumed = 0;
  while (read_ptr != queue->write_ptr) {
    msg = queue->buffer + read_ptr;
    process_msg_pre(*msg);
    read_ptr = (read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
    consumed++;
  }
  return consumed;
}

void process_msg_queue_post(msg_queue *queue, int consumed) {
  scheduler_msg msg;
  while (consumed--) {
    msg = pop_msg(queue);
    process_msg_post(msg);
  }
}

Node *_audio_ctx_add(Node *node) {
  // printf("audio ctx add %p\n", node);
  if (ctx.head == NULL) {
    ctx.head = node;
    ctx.tail = node;
    return node;
  }

  ctx.tail->next = node;
  ctx.tail = node;
  return node;
}

Node *add_to_dac(Node *node) {
  // return NULL;
  // node->type = OUTPUT;
  // node->write_to_dac = true;
  return node;
}

Ctx *get_audio_ctx() { return &ctx; }

void push_msg(msg_queue *queue, scheduler_msg msg) {
  if (queue->num_msgs == MSG_QUEUE_MAX_SIZE) {
    printf("Error: Command FIFO full\n");
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
