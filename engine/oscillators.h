#ifndef _ENGINE_OSCILLATORS_H
#define _ENGINE_OSCILLATORS_H

#include "node.h"
void maketable_sq(void);
typedef struct {
  double phase;
} sq_state;

node_perform sq_perform(Node *node, int nframes, double spf);

typedef struct {
  double phase;
} sin_state;

node_perform sin_perform(Node *node, int nframes, double spf);

void maketable_sin(void);

Node *bufplayer_node(Signal *buf, Signal *rate);
Node *bufplayer_node_of_scalar(Signal *buf, double rate);
Node *bufplayer_node_of_scalar_int(Signal *buf, int rate);

#endif
