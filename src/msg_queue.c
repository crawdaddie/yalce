#include "msg_queue.h"
#include <stdlib.h>
#define MSG_QUEUE_MAX 64

MsgQueue *new_queue() {

  MsgQueue *q = (MsgQueue *)calloc(sizeof(MsgQueue), 1);
  q->items = (void **)calloc(sizeof(void *), MSG_QUEUE_MAX);
  q->bottom = 0;
  q->top = 1;
  q->max = MSG_QUEUE_MAX;
  return q;
};

int q_is_full(MsgQueue *q) {
  if (q->top == q->bottom - 1) {
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
void q_push(MsgQueue *q, void *newitem) {
  if (q_is_full(q)) {
    // extend queue
    q->items = realloc(q->items, q->max + MSG_QUEUE_MAX);
    return;
  }
  q->items[q->top] = newitem;
  q->top = (q->top + 1) % q->max;
};

void *q_pop_left(MsgQueue *q) {
  void *popped_left = q->items[q->bottom];
  q->bottom = (q->bottom + 1) % q->max;
  return popped_left;
};
