#include "msg_queue.h"
#include <stdio.h>

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
