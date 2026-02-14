#ifndef _ENGINE_BLOCK_QUEUE_H
#define _ENGINE_BLOCK_QUEUE_H
#include "node.h"
#include <stdint.h>
typedef struct scheduler_msg {
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
} scheduler_msg;

#define MSG_QUEUE_MAX_SIZE 256
// single reader-single writer lockfree FIFO queue
// implemented as a ringbuffer of scheduler_msg s
typedef struct {
  scheduler_msg buffer[MSG_QUEUE_MAX_SIZE];
  int read_ptr;
  int write_ptr;
  int num_msgs;
} msg_queue;

void push_msg(msg_queue *queue, scheduler_msg msg, int buffer_offset);

scheduler_msg pop_msg(msg_queue *queue);

void print_msg(scheduler_msg *msg);
#endif
