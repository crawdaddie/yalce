#ifndef _ENGINE_CTX_H
#define _ENGINE_CTX_H
#include "common.h"
#include "node.h"

typedef struct scheduler_msg {
  enum {
    NODE_ADD,
    GROUP_ADD,
    NODE_SET_SCALAR,
    NODE_SET_TRIG,
    NODE_REMOVE,
    NODE_SET_INPUT,
  } type;
  int frame_offset;
  union {
    struct NODE_ADD {
      Node *target;
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

typedef struct {
  double output_buf[BUF_SIZE * LAYOUT];
  Node *head;
  Node *tail;
  int sample_rate;
  double spf;
  msg_queue msg_queue;
} Ctx;

extern Ctx ctx;

Ctx *get_audio_ctx();

void init_ctx();

void user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame);

void write_to_output(double *src, double *dest, int nframes, int output_num);

Node *_audio_ctx_add(Node *node);
Node *add_to_dac(Node *node);

int ctx_sample_rate();
double ctx_spf();

int process_msg_queue_pre(msg_queue *queue);

void process_msg_queue_post(msg_queue *queue, int consumed);

void audio_ctx_add(Node *ensemble);

#endif
