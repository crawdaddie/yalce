#ifndef _SQ_H
#define _SQ_H
#include "../common.h"
#include "../graph/graph.h"

typedef struct sq_data {
  double ramp;
  double pan;
} sq_data;

Graph *sq_create(double *out, Signal *freq);
#endif
