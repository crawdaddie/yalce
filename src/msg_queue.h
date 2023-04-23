#ifndef _MSG_QUEUE_H
#define _MSG_QUEUE_H

typedef struct MsgQueue {
  void **items;
  int bottom;
  int top;
  int max;

} MsgQueue;

MsgQueue *new_queue();

int q_is_full(MsgQueue *q);
int q_is_empty(MsgQueue *q);
void q_push(MsgQueue *q, void *newitem);
void *q_pop_left(MsgQueue *q);

#endif
