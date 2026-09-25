#include "./audio_instructions.h"
#include "ctx.h"
#include "scheduling.h"
#include <stdio.h>
#include <stdlib.h>

#define AUDIO_EVENT_HEAP_MAX MSG_QUEUE_MAX_SIZE

typedef struct {
  audio_instruction msg;
  uint64_t sequence;
} AudioPending;

static AudioPending event_heap[AUDIO_EVENT_HEAP_MAX];
static int event_heap_size;
static AudioPending active_events[AUDIO_EVENT_HEAP_MAX];
static int active_offsets[AUDIO_EVENT_HEAP_MAX];
static int active_event_count;
static uint64_t event_sequence;

typedef struct {
  void *task;
  uint64_t tick;
} AudioCancellation;

static AudioCancellation cancellations[MSG_QUEUE_MAX_SIZE];
static int cancellation_count;

static int audio_event_before(AudioPending left, AudioPending right) {
  if (left.msg.tick != right.msg.tick) {
    return left.msg.tick < right.msg.tick;
  }
  return left.sequence < right.sequence;
}

static void swap_events(AudioPending *left, AudioPending *right) {
  AudioPending tmp = *left;
  *left = *right;
  *right = tmp;
}

static void heap_up(int index) {
  while (index > 0) {
    int parent = (index - 1) / 2;
    if (!audio_event_before(event_heap[index], event_heap[parent])) {
      return;
    }
    swap_events(&event_heap[index], &event_heap[parent]);
    index = parent;
  }
}

static void heap_down(int index) {
  for (;;) {
    int left = index * 2 + 1;
    int right = left + 1;
    int smallest = index;

    if (left < event_heap_size &&
        audio_event_before(event_heap[left], event_heap[smallest])) {
      smallest = left;
    }
    if (right < event_heap_size &&
        audio_event_before(event_heap[right], event_heap[smallest])) {
      smallest = right;
    }
    if (smallest == index) {
      return;
    }
    swap_events(&event_heap[index], &event_heap[smallest]);
    index = smallest;
  }
}

static int heap_push(audio_instruction msg) {
  if (event_heap_size >= AUDIO_EVENT_HEAP_MAX) {
    return 0;
  }

  event_heap[event_heap_size] =
      (AudioPending){.msg = msg, .sequence = event_sequence++};
  heap_up(event_heap_size++);
  return 1;
}

static AudioPending heap_pop(void) {
  AudioPending result = event_heap[0];
  event_heap[0] = event_heap[--event_heap_size];
  if (event_heap_size > 0) {
    heap_down(0);
  }
  return result;
}

static int event_cancelled(audio_instruction msg) {
  if (!msg.task) {
    return 0;
  }

  for (int i = 0; i < cancellation_count; i++) {
    AudioCancellation cancellation = cancellations[i];
    if (cancellation.task == msg.task && msg.tick >= cancellation.tick) {
      return 1;
    }
  }

  return 0;
}

static void record_cancellation(audio_instruction msg) {
  if (!msg.payload.AUDIO_CANCEL_TASK.task) {
    return;
  }

  for (int i = 0; i < cancellation_count; i++) {
    AudioCancellation *cancellation = &cancellations[i];
    if (cancellation->task == msg.payload.AUDIO_CANCEL_TASK.task) {
      if (msg.tick < cancellation->tick) {
        cancellation->tick = msg.tick;
      }
      return;
    }
  }

  if (cancellation_count < MSG_QUEUE_MAX_SIZE) {
    cancellations[cancellation_count++] = (AudioCancellation){
        .task = msg.payload.AUDIO_CANCEL_TASK.task, .tick = msg.tick};
  }
}

void node_connect_input(int idx, NodeRef node, NodeRef input);

static Node *jit_inlet_node(Node *node, int input) {
  if (!node || input < 0 || input >= MAX_INPUTS) {
    return NULL;
  }
  return (Node *)node->connections[input].source_node_index;
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
    audio_ctx_add(payload.target);

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

    Node *inlet_node = jit_inlet_node(node, payload.input);
    if (inlet_node && inlet_node->output.buf) {
      for (int i = frame_offset; i < BUF_SIZE; i++) {
        inlet_node->output.buf[i] = payload.value;
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

    if (payload.input >= 0 && payload.input < MAX_INPUTS) {
      node_connect_input(payload.input, node, buf);
      audio_ctx_mark_dirty();
    }

    break;
  }

  case NODE_PIPE_INPUT: {
    struct NODE_PIPE_INPUT payload = msg.payload.NODE_PIPE_INPUT;
    Node *node = payload.target;
    Node *buf = payload.value;
    if (node_pending_free(node) || node_pending_free(buf)) {
      break;
    }

    if (payload.input >= 0 && payload.input < MAX_INPUTS) {
      buf->frame_offset = frame_offset;
      buf->write_to_output = false;
      node_connect_input(payload.input, node, buf);
      audio_ctx_mark_dirty();
    }

    break;
  }

  case NODE_MIX_INPUT: {
    struct NODE_MIX_INPUT payload = msg.payload.NODE_MIX_INPUT;
    Node *mixer = payload.mixer;
    Node *source = payload.source;
    if (node_pending_free(mixer) || node_pending_free(source)) {
      break;
    }

    source->frame_offset = frame_offset;
    source->write_to_output = false;
    source->mix_next = mixer->mix_head;
    mixer->mix_head = source;
    audio_ctx_mark_dirty();

    break;
  }

  case NODE_SET_TRIG: {
    struct NODE_SET_TRIG payload = msg.payload.NODE_SET_TRIG;
    Node *node = payload.target;
    if (node_pending_free(node)) {
      break;
    }

    Node *inlet_node = jit_inlet_node(node, payload.input);
    if (inlet_node && inlet_node->output.buf) {
      inlet_node->output.buf[frame_offset] = 1.0;
    }

    break;
  }
  case NODE_REMOVE: {
    struct NODE_REMOVE payload = msg.payload.NODE_REMOVE;
    if (payload.target) {
      payload.target->trig_end = true;
      audio_ctx_mark_dirty();
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

  case NODE_SET_SCALAR: {

    struct NODE_SET_SCALAR payload = msg.payload.NODE_SET_SCALAR;
    Node *node = payload.target;
    if (node_pending_free(node)) {
      break;
    }

    Node *inlet_node = jit_inlet_node(node, payload.input);
    if (inlet_node) {
      for (int i = 0; i < frame_offset; i++) {
        inlet_node->output.buf[i] = payload.value;
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

    Node *inlet_node = jit_inlet_node(node, payload.input);
    if (inlet_node && inlet_node->output.buf) {
      inlet_node->output.buf[frame_offset] = 0.0;
    }
    break;
  }
  default:
    break;
  }
}

void process_audio_events_pre(uint64_t current_tick, int frame_count,
                              audio_instructions_queue *queue) {
  active_event_count = 0;

  while (queue->num_msgs > 0) {
    if (!heap_push(pop_msg(queue))) {
      fprintf(stderr, "Audio event heap full\n");
      break;
    }
  }

  uint64_t end_tick = current_tick + (uint64_t)frame_count;
  while (event_heap_size > 0 && event_heap[0].msg.tick < end_tick) {
    AudioPending event = heap_pop();

    if (event.msg.type == AUDIO_CANCEL_TASK) {
      record_cancellation(event.msg);
      continue;
    }

    if (event_cancelled(event.msg)) {
      continue;
    }

    int offset = event.msg.tick < current_tick
                     ? 0
                     : (int)(event.msg.tick - current_tick);
    process_msg_pre(offset, event.msg);

    if (active_event_count < AUDIO_EVENT_HEAP_MAX) {
      active_events[active_event_count++] = event;
      active_offsets[active_event_count - 1] = offset;
    }
  }
}

void process_audio_events_post(void) {
  for (int i = active_event_count - 1; i >= 0; i--) {
    AudioPending event = active_events[i];
    process_msg_post(active_offsets[i], event.msg);
  }
  active_event_count = 0;
}
void print_msg(audio_instruction *msg) {
  printf("[%lu]", (unsigned long)msg->tick);
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
  case NODE_SET_INPUT: {
    printf(" node_set_input %p[%d] <- %p\n",
           msg->payload.NODE_SET_INPUT.target, msg->payload.NODE_SET_INPUT.input,
           msg->payload.NODE_SET_INPUT.value);
    break;
  }
  case NODE_PIPE_INPUT: {
    printf(" node_pipe_input %p[%d] <- %p\n",
           msg->payload.NODE_PIPE_INPUT.target,
           msg->payload.NODE_PIPE_INPUT.input,
           msg->payload.NODE_PIPE_INPUT.value);
    break;
  }
  case NODE_MIX_INPUT: {
    printf(" node_mix_input %p <- %p\n", msg->payload.NODE_MIX_INPUT.mixer,
           msg->payload.NODE_MIX_INPUT.source);
    break;
  }
  case NODE_REMOVE: {
    printf(" node_remove %p\n", msg->payload.NODE_REMOVE.target);
    break;
  }
  }
}

void push_msg(audio_instructions_queue *queue, audio_instruction msg) {
  if (queue->num_msgs == MSG_QUEUE_MAX_SIZE) {
    fprintf(stderr, "Audio Instruction Error: Command FIFO full\n");
    return;
  }

  if (msg.type != AUDIO_CANCEL_TASK && !msg.task) {
    msg.task = get_current_task_token();
  }

  *(queue->buffer + queue->write_ptr) = msg;
  queue->write_ptr = (queue->write_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs++;
}

void cancel_audio_task(audio_instructions_queue *queue, void *task,
                       uint64_t tick) {
  if (!queue || !task) {
    return;
  }

  push_msg(queue, (audio_instruction){
                         .type = AUDIO_CANCEL_TASK,
                         .tick = tick,
                         .payload.AUDIO_CANCEL_TASK = {.task = task}});
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
