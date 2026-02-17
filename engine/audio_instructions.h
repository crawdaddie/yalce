#ifndef _ENGINE_AUDIO_INSTRUCTIONS_QUEUE_H
#define _ENGINE_AUDIO_INSTRUCTIONS_QUEUE_H
#include "node.h"
#include <stdint.h>
typedef struct {
  enum {
    NODE_ADD,
    GROUP_ADD,
    NODE_SET_SCALAR,
    NODE_SET_TRIG,
    NODE_REMOVE,
    NODE_SET_INPUT,
  } type;
  // int frame_offset;
  uint64_t tick;

  union {
    struct NODE_ADD {
      Node *target;
      Node *group;
    } NODE_ADD;

    struct GROUP_ADD {
      Node *group;
      Node *tail;
    } GROUP_ADD;

    struct NODE_SET_SCALAR {
      Node *target;
      int input;
      double value;
    } NODE_SET_SCALAR;

    struct NODE_SET_INPUT {
      Node *target;
      int input;
      Node *value;
    } NODE_SET_INPUT;

    struct NODE_SET_TRIG {
      Node *target;
      int input;
    } NODE_SET_TRIG;

    struct NODE_REMOVE {
      Node *target;
    } NODE_REMOVE;

  } payload;
} audio_instruction;

#define MSG_QUEUE_MAX_SIZE 256
// Single-reader single-writer lock-free FIFO queue.
// Scheduler thread = sole writer, audio thread = sole reader.
typedef struct {
  audio_instruction buffer[MSG_QUEUE_MAX_SIZE];
  int read_ptr;
  int write_ptr;
  int num_msgs;
} audio_instructions_queue;

void push_msg(audio_instructions_queue *queue, audio_instruction msg);
audio_instruction pop_msg(audio_instructions_queue *queue);
audio_instruction *create_bundle(int length);
void print_msg(audio_instruction *msg);

int process_msg_queue_pre(uint64_t current_tick, int frame_count,
                          audio_instructions_queue *queue);

void process_msg_queue_post(uint64_t current_tick, int frame_count,
                            audio_instructions_queue *queue, int consumed);
#endif
