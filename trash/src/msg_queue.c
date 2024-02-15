#include "msg_queue.h"
#include "log.h"
#include <stdlib.h>
#define MSG_QUEUE_MAX 64

void init_queue(MsgQueue *queue) {
  queue->items = (Msg *)calloc(sizeof(void *), MSG_QUEUE_MAX);
  queue->bottom = 0;
  queue->top = 0;
  queue->max = MSG_QUEUE_MAX;
}

void push_message(MsgQueue *queue, double timestamp, msg_handler handler,
                  void *data, int size) {

  if ((queue->top + 1) % queue->max == queue->bottom) {
    write_log("Command FIFO full\n");
    return;
  }

  Msg *msg = queue->items + queue->top;

  msg->timestamp = timestamp;
  msg->handler = handler;
  msg->data = data;
  msg->size = size;

  queue->top = (queue->top + 1) % queue->max;
};

size_t queue_size(MsgQueue queue) {
  if (queue.bottom > queue.top) {
    return queue.max - queue.bottom + queue.top;
  }
  return queue.top - queue.bottom;
}

Msg queue_pop_left(MsgQueue *queue) {
  Msg popped_left = queue->items[queue->bottom];

  queue->bottom = (queue->bottom + 1) % queue->max;
  return popped_left;
};
