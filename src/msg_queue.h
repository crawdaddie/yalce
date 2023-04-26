#ifndef _MSG_QUEUE_H
#define _MSG_QUEUE_H
struct Msg;

typedef void (*msg_handler)(void *ctx, struct Msg msg, int block_offset);

typedef struct Msg {
  double timestamp;
  msg_handler handler;
  void *data;
  int size;
} Msg;

typedef struct MsgQueue {
  Msg *items;
  int bottom;
  int top;
  int max;

} MsgQueue;

void init_queue(MsgQueue *queue);

int q_is_full(MsgQueue *q);
int q_is_empty(MsgQueue *q);
void q_push(MsgQueue *q, Msg newitem);
Msg q_pop_left(MsgQueue *q);

void push_message(MsgQueue *queue, double timestamp, msg_handler handler,
                  void *data, int size);
#endif
