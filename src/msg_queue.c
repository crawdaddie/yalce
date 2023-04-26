#include "msg_queue.h"
#include <stdlib.h>
#define MSG_QUEUE_MAX 64

void init_queue(MsgQueue *queue) {
  queue->items = (void **)calloc(sizeof(void *), MSG_QUEUE_MAX);
  queue->bottom = 0;
  queue->top = 0;
  queue->max = MSG_QUEUE_MAX;
}

int q_is_full(MsgQueue *q) {
  if (q->top == q->max) {
    return 1;
  }
  return 0;
};

int q_is_empty(MsgQueue *q) {
  if (q->top == q->bottom) {
    return 1;
  }
  return 0;
};

void push_message(MsgQueue *queue, double timestamp, msg_handler handler,
                  void *data, int size) {

  if (q_is_full(queue)) {
    // extend queue
    queue->items = realloc(queue->items, queue->max + MSG_QUEUE_MAX);
  }
  Msg *msg = queue->items + queue->top;
  msg->timestamp = timestamp;
  msg->handler = handler;
  msg->data = data;
  msg->size = size;

  queue->top++;
};

Msg q_pop_left(MsgQueue *q) {
  Msg popped_left = q->items[q->bottom];
  q->bottom = (q->bottom + 1) % q->max;
  return popped_left;
};
