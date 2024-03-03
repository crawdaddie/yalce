#ifndef _MSG_QUEUE_H
#define _MSG_QUEUE_H
#include "node.h"

typedef struct {
  enum { NODE_ADD, NODE_SET_SCALAR, NODE_SET_TRIG } type;
  int frame_offset;
  union {
    struct NODE_ADD {
    } NODE_ADD;
    struct NODE_SET_SCALAR {
      Node *target;
      int input;
      double value;
    } NODE_SET_SCALAR;

    struct NODE_SET_TRIG {
      Node *target;
      int input;
    } NODE_SET_TRIG;
  } body;
} scheduler_msg;

#define MSG_QUEUE_MAX_SIZE 100
// single reader-single writer lockfree FIFO queue
//
// implemented as a ringbuffer of scheduler_msg s
typedef struct {
  scheduler_msg buffer[MSG_QUEUE_MAX_SIZE];
  int read_ptr;
  int write_ptr;
  int num_msgs;
} msg_queue;

void push_msg(msg_queue *queue, scheduler_msg msg);

scheduler_msg pop_msg(msg_queue *queue);
int get_write_ptr();
void update_bundle(int write_ptr);

#endif
