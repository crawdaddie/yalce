#ifndef _BUFPLAYER_H
#define _BUFPLAYER_H
#include "node.h"

typedef struct {
  double sample_rate_scaling;
  double phase;
} bufplayer_state;

Node *bufplayer_node(const char *filename);

#endif

