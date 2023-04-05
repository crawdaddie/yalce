#ifndef _SQ_H
#define _SQ_H
#include "../common.h"
#include "../node.h"
#include "signal.h"

typedef struct {
  double ramp;
  Signal *freq;
  Signal *out;
} sq_data;
/* node_perform sq_perform(Node *node, int nframes, double spf); */
Node *sq_node(double freq);

Node *sq_detune_node(double freq);

#endif
