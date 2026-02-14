#include "./block_queue.h"
#include "audio_graph.h"
#include "group.h"
#include <stdio.h>

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
      // if (payload.value == 0.) {
      //   printf("setting gate off %llu\n", msg.tick);
      // }
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
void print_msg(scheduler_msg *msg) {
  printf("[%llu]", msg->tick);
  switch (msg->type) {
  case NODE_ADD: {
    printf(" node_add %p", msg->payload.NODE_ADD.target);
    break;
  }
  // case GROUP_ADD,
  case NODE_SET_SCALAR: {

    printf(" node_set_scalar %p[%d] %f\n", msg->payload.NODE_SET_SCALAR.target,
           msg->payload.NODE_SET_SCALAR.input,
           msg->payload.NODE_SET_SCALAR.value);
    break;
    // case NODE_REMOVE,
    // case NODE_SET_INPUT,
  }
  case NODE_SET_TRIG: {

    printf(" node_set_trig ");
    break;
  }
  }
}

int process_msg_queue_pre(uint64_t current_tick, int frame_count,
                          msg_queue *queue) {
  int read_ptr = queue->read_ptr;
  scheduler_msg *msg;
  int consumed = 0;
  int write_ptr = queue->write_ptr;
  int num_moved = 0;
  while (read_ptr != queue->write_ptr) {
    msg = queue->buffer + read_ptr;
    // printf("msg tick %d %d %p\n", msg->tick, current_tick, msg);

    if (msg->tick < current_tick) {
      // msg is in the past - process at offset 0 (better late than never)
      process_msg_pre(0, *msg);
    } else if (msg->tick - current_tick >= frame_count) {
      // msg is too early - defer to next block
      push_msg(&ctx.overflow_queue, *msg, 0);
      num_moved++;
    } else {
      process_msg_pre(msg->tick - current_tick, *msg);
    }

    read_ptr = (read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
    consumed++;
  }

  return consumed;
}

void process_msg_queue_post(uint64_t current_tick, int frame_count,
                            msg_queue *queue, int consumed) {
  scheduler_msg msg;
  while (consumed--) {
    msg = pop_msg(queue);
    if (msg.tick < current_tick) {
      // was in the past, processed at offset 0
      process_msg_post(0, msg);
    } else if (msg.tick - current_tick >= frame_count) {
      // was deferred to overflow, skip post-processing
    } else {
      int frame_offset = msg.tick - current_tick;
      process_msg_post(frame_offset, msg);
    }
  }
}
