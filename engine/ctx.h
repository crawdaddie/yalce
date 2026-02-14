#ifndef _ENGINE_CTX_H
#define _ENGINE_CTX_H
#include "block_queue.h"
#include "common.h"
#include "node.h"
#include <stdint.h>

int get_write_ptr();
void update_bundle(int write_ptr);

typedef struct {
  Node *head;
  Node *tail;
} node_group_state;

typedef struct {
  double output_buf[BUF_SIZE * LAYOUT];
  int num_input_signals;
  Signal *input_signals;

  node_group_state graph;
  int sample_rate;
  double spf;
  msg_queue msg_queue;
  msg_queue overflow_queue;
  int **sig_to_hw_in_map;
  double main_vol;
} Ctx;

extern Ctx ctx;

Ctx *get_audio_ctx();

void init_ctx();

void user_ctx_callback(Ctx *ctx, uint64_t current_tick, int nframes,
                       double seconds_per_frame);

void write_to_output(double *src, double *dest, int nframes, int output_num);

Node *_audio_ctx_add(Node *node);
Node *add_to_dac(Node *node);

int ctx_sample_rate();
double ctx_spf();

int process_msg_queue_pre(uint64_t current_tick, msg_queue *queue);

void process_msg_queue_post(uint64_t current_tick, msg_queue *queue,
                            int consumed);

void audio_ctx_add(Node *ensemble);

void move_overflow();
#endif
