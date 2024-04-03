#ifndef _BUFPLAYER_H
#define _BUFPLAYER_H
#include "node.h"

typedef struct {
  double sample_rate_scaling;
  double phase;
} bufplayer_state;

Node *bufplayer_node(Signal *input_buf_sig);

typedef struct {
  double sample_rate_scaling;
  double phase;
  double rate;
  double start_pos;
  double *buf;
  int frames;
  int layout;
} bufplayer_autotrig_state;
// Node *bufplayer_autotrig_node(int buf_slot, double rate, double start_pos);
Node *bufplayer_autotrig_node(Signal *buf, double rate, double start_pos);

typedef struct {
  Signal signal;
  double sample_rate_scaling;
} SharedBuf;

int buf_alloc(const char *filename);
Signal *buf_alloc_to_sig(const char *filename);
#endif
