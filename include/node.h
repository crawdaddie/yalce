#ifndef _NODE_H
#define _NODE_H
#include "signal.h"
#include <stdbool.h>

typedef struct Node (*node_perform)(struct Node *node, int nframes, double spf);

typedef struct Node {
  enum {
    INTERMEDIATE = 0,
    OUTPUT,
  } type;

  bool killed;

  node_perform perform;
  void *state;
  Signal **ins;
  int num_ins;

  Signal *out;

  struct Node *next;
  struct Node *head;
  struct Node *tail;
  int frame_offset;
} Node;

double random_double();

double random_double_range(double min, double max);

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

void init_sig_ptrs();

Signal *get_sig(int layout);
Node *node_new(void *data, node_perform *perform, Signal *ins, Signal *out);
#endif
