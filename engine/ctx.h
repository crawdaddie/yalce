#ifndef _ENGINE_CTX_H
#define _ENGINE_CTX_H
#include "audio_instructions.h"
#include "common.h"
#include "node.h"
#include <stdint.h>

typedef struct {
  Node *head;
  Node *tail;
  unsigned generation;
} node_group_state;

typedef struct {
  Node *node;
  void *state;
  Node *inputs[MAX_INPUTS];
} AudioRenderOp;

typedef struct {
  AudioRenderOp *ops;
  int len;
  int capacity;
  unsigned built_generation;
} AudioRenderPlan;

typedef struct {
  double *output_buf;
  int output_buf_capacity;
  int num_input_signals;
  Signal *input_signals;

  node_group_state graph;
  AudioRenderPlan render_plan;
  int sample_rate;
  double spf;
  audio_instructions_queue msg_queue;
  int **sig_to_hw_in_map;
  double main_vol;
} Ctx;

extern Ctx ctx;

Ctx *get_audio_ctx();

void init_ctx();

void user_ctx_callback(Ctx *ctx, uint64_t current_tick, int nframes,
                       double seconds_per_frame);

int ctx_sample_rate();
double ctx_spf();
void set_main_vol(double vol);

void audio_ctx_add(Node *ensemble);
void audio_ctx_add_before(Node *target, Node *node);
void audio_ctx_mark_dirty(void);

#endif
