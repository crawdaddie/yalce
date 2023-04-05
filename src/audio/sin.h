#ifndef _SIN_H
#define _SIN_H
#include "../node.h"
#include "signal.h"

typedef struct {
  double ramp;
  double freq;
  Signal *out;
} sin_data;

Node *sin_node(double freq);

#endif
