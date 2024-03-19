#ifndef _ENVELOPE_H
#define _ENVELOPE_H

#include "node.h"

typedef struct {
  double *levels;
  double *rates;
  int len;
  int state;
  double value;

  bool should_kill;
} env_state;

node_perform env_perform(Node *node, int nframes, double spf);

Node *env_node(int len, double *levels, double *times);

typedef enum { ATTACK, DECAY, SUSTAIN, RELEASE, IDLE } _adsr_state;
typedef struct {
  _adsr_state state;
  double value;
  double target;
  double attack_rate;
  double decay_rate;
  double release_rate;
  double release_time;
  double sustain_level;
  double sustain_time;
  int counter;
} adsr_state;

Node *adsr_node(double attack_time, double decay_time, double sustain_level,
                double sustain_time, double release_time);

#endif
