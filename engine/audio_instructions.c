#include "./audio_instructions.h"
#include "audio_graph.h"
#include "group.h"
#include <stdio.h>
#include <stdlib.h>

// plug_input_in_graph stores either a raw Node* (when _graph==NULL) or a
// node_index integer (when _graph!=NULL). Resolve back to a Node*.
static Node *jit_inlet_node(Node *node, int input) {
  uint64_t src = node->connections[input].source_node_index;
  if (_graph) {
    return &_graph->nodes[src];
  }
  return (Node *)src;
}

static inline int node_pending_free(Node *node) {
  return !node || node->trig_end;
}

static void process_msg_pre(int frame_offset, audio_instruction msg) {

  switch (msg.type) {
  case NODE_ADD: {
    struct NODE_ADD payload = msg.payload.NODE_ADD;
    if (node_pending_free(payload.target)) {
      break;
    }
    payload.target->frame_offset = frame_offset;
    if (payload.group) {
      if (node_pending_free(payload.group)) {
        break;
      }
      group_add(payload.target, payload.group);
    } else {
      audio_ctx_add(payload.target);
    }

    break;
  }

  case NODE_ADD_BEFORE: {
    struct NODE_ADD_BEFORE payload = msg.payload.NODE_ADD_BEFORE;
    if (node_pending_free(payload.node) || node_pending_free(payload.target)) {
      break;
    }
    payload.node->frame_offset = frame_offset;
    audio_ctx_add_before(payload.target, payload.node);
    break;
  }

  case NODE_SET_SCALAR: {
    struct NODE_SET_SCALAR payload = msg.payload.NODE_SET_SCALAR;
    Node *node = payload.target;
    if (node_pending_free(node)) {
      break;
    }

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
    } else {
      Node *inlet_node = jit_inlet_node(node, payload.input);
      if (inlet_node && inlet_node->output.buf) {
        for (int i = frame_offset; i < BUF_SIZE; i++) {
          inlet_node->output.buf[i] = payload.value;
        }
      }
    }

    break;
  }

  case NODE_SET_INPUT: {
    struct NODE_SET_INPUT payload = msg.payload.NODE_SET_INPUT;
    Node *node = payload.target;
    Node *buf = payload.value;
    if (node_pending_free(node) || node_pending_free(buf)) {
      break;
    }

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
    if (node_pending_free(node)) {
      break;
    }

    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);
      if (node->state_ptr) {
        g = node->state_ptr;
      }
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      inlet_data.buf[frame_offset] = 1.0;
    } else {
      Node *inlet_node = jit_inlet_node(node, payload.input);
      if (inlet_node && inlet_node->output.buf) {
        inlet_node->output.buf[frame_offset] = 1.0;
      }
    }

    break;
  }
  default:
    break;
  }
}

static void process_msg_post(int frame_offset, audio_instruction msg) {
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
    if (node_pending_free(node)) {
      break;
    }

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
    } else {
      Node *inlet_node = jit_inlet_node(node, payload.input);
      if (inlet_node) {
        for (int i = 0; i < frame_offset; i++) {
          inlet_node->output.buf[i] = payload.value;
        }
      }
    }

    break;
  }

  case NODE_SET_TRIG: {
    struct NODE_SET_TRIG payload = msg.payload.NODE_SET_TRIG;
    Node *node = payload.target;
    if (node_pending_free(node)) {
      break;
    }

    if ((char *)node->perform == (char *)perform_audio_graph) {
      AudioGraph *g = (AudioGraph *)((Node *)node + 1);

      if (node->state_ptr) {
        g = node->state_ptr;
      }
      Node *inlet_node = g->nodes + g->inlets[payload.input];
      Signal inlet_data = inlet_node->output;
      inlet_data.buf[frame_offset] = 0.0;
    } else {
      Node *inlet_node = jit_inlet_node(node, payload.input);
      if (inlet_node && inlet_node->output.buf) {
        inlet_node->output.buf[frame_offset] = 0.0;
      }
    }
    break;
  }
  default:
    break;
  }
}
void print_msg(audio_instruction *msg) {
  printf("[%llu]", msg->tick);
  switch (msg->type) {
  case NODE_ADD: {
    printf(" node_add %p", msg->payload.NODE_ADD.target);
    break;
  }
  case NODE_ADD_BEFORE: {
    printf(" node_add_before %p <- %p", msg->payload.NODE_ADD_BEFORE.target,
           msg->payload.NODE_ADD_BEFORE.node);
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

void push_msg(audio_instructions_queue *queue, audio_instruction msg) {
  if (queue->num_msgs == MSG_QUEUE_MAX_SIZE) {
    fprintf(stderr, "Audio Instruction Error: Command FIFO full\n");
    return;
  }

  *(queue->buffer + queue->write_ptr) = msg;
  queue->write_ptr = (queue->write_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs++;
}

audio_instruction pop_msg(audio_instructions_queue *queue) {
  audio_instruction msg = *(queue->buffer + queue->read_ptr);
  queue->read_ptr = (queue->read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs--;
  return msg;
}

audio_instruction *create_bundle(int length) {
  return malloc(sizeof(audio_instruction) * length);
}

// Audio-thread-local deferral buffer — only touched by the audio thread,
// no synchronization needed. Replaces the old cross-thread overflow_queue.
#define DEFERRED_MAX 64
static audio_instruction deferred_msgs[DEFERRED_MAX];
static int num_deferred = 0;

int process_msg_queue_pre(uint64_t current_tick, int frame_count,
                          audio_instructions_queue *queue) {

  // Process deferred messages that are now due
  int still_deferred = 0;
  for (int i = 0; i < num_deferred; i++) {
    audio_instruction *m = &deferred_msgs[i];
    if (m->tick < current_tick) {
      process_msg_pre(0, *m);
    } else if (m->tick - current_tick < frame_count) {
      process_msg_pre(m->tick - current_tick, *m);
    } else {
      deferred_msgs[still_deferred++] = *m;
    }
  }
  num_deferred = still_deferred;

  // Drain the ring buffer
  int read_ptr = queue->read_ptr;
  audio_instruction *msg;
  int consumed = 0;
  while (read_ptr != queue->write_ptr) {
    msg = queue->buffer + read_ptr;

    if (msg->tick < current_tick) {
      process_msg_pre(0, *msg);
    } else if (msg->tick - current_tick >= frame_count) {
      // too early — defer locally
      if (num_deferred < DEFERRED_MAX) {
        deferred_msgs[num_deferred++] = *msg;
      }
    } else {
      process_msg_pre(msg->tick - current_tick, *msg);
    }

    read_ptr = (read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
    consumed++;
  }

  return consumed;
}

void process_msg_queue_post(uint64_t current_tick, int frame_count,
                            audio_instructions_queue *queue, int consumed) {
  audio_instruction msg;
  while (consumed--) {
    msg = pop_msg(queue);
    if (msg.tick < current_tick) {
      process_msg_post(0, msg);
    } else if (msg.tick - current_tick >= frame_count) {
      // was deferred, skip post-processing
    } else {
      int frame_offset = msg.tick - current_tick;
      process_msg_post(frame_offset, msg);
    }
  }
}
