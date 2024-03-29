#ifndef _OSCILLATOR_H
#define _OSCILLATOR_H

#include "node.h"

void maketable_sin(void);

typedef struct {
  double phase;
} sin_state;

node_perform sine_perform(Node *node, int nframes, double spf);
static inline double scale_val_2(double env_val, // 0-1
                                 double min, double max) {
  return min + env_val * (max - min);
}

typedef struct {
  double phase;
} sq_state;

node_perform sq_perform(Node *node, int nframes, double spf);
void maketable_sin(void);

node_perform sine_perform(Node *node, int nframes, double spf);

Node *sine(double freq);

// ----------------------------- SQUARE WAVE OSCILLATORS
//
//
void maketable_sq(void);
double sq_sample(double phase);
node_perform sq_perform(Node *node, int nframes, double spf);

Node *sq_node(double freq);

typedef struct {

  unsigned int num_harmonics;
  unsigned int m;
  double rate;
  double phase;
  double p;
  double C2;
  double a;
  double state;
} blsaw_state;
Node *blsaw_node(double freq, int num_harmonics);

typedef struct {

  unsigned int num_harmonics;
  unsigned int m;
  double rate;
  double phase;
  double p;
  double C2;
  double a;
  double state;

  double base_phase;
} sawsinc_state;
Node *sawsinc_node(double base_freq, double freq, int num_harmonics);

#endif
