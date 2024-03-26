#ifndef _NOISE_H
#define _NOISE_H
#include "node.h"
double random_double();

double random_double_range(double min, double max);

int rand_int(int range);

node_perform noise_perform(Node *node, int nframes, double spf);

typedef struct {
  double phase;
  double target;
  double min;
  double max;
  double freq;
} lf_noise_state;

node_perform lf_noise_perform(Node *node, int nframes, double spf);

typedef struct {
  double phase;
  double target;
  double current;
} lf_noise_interp_state;
node_perform lf_noise_interp_perform(Node *node, int nframes, double spf);

Node *lfnoise(double freq, double min, double max);
node_perform lf_noise_interp_perform(Node *node, int nframes, double spf);

typedef struct {
  double freq;
  double phase;
  double target;
  double *choices;
  int size;
} rand_choice_state;

node_perform rand_choice_perform(Node *node, int nframes, double spf);
Node *rand_choice_node(double freq, int size, double *choices);

#endif
