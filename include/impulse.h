#ifndef _IMPULSE_H
#define _IMPULSE_H

#include "node.h"
typedef struct {
  double counter;
} impulse_state;

typedef struct {
  double counter;
  double min_freq;
  double max_freq;
} dust_state;

Node *trig_node(Node *in);
Node *trig_node_const(double freq);
Node *dust_node_const(double min_freq, double max_freq);

typedef struct {
  unsigned int num_harmonics;
  unsigned int m;
  double rate;
  double phase;
  double p;
} blit_state;
Node *blit_node(double freq, int num_harmonics);

typedef struct {
  unsigned int num_harmonics;
  unsigned int m;
  double rate;
  double phase;
  double p;
  double tightness;
} windowed_impulse_state;

Node *windowed_impulse_node(double pulse_freq, int num_harmonics,
                            double tightness);
#endif
