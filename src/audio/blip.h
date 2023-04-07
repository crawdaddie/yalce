#ifndef _BLIP_H
#define _BLIP_H
#include "osc.h"

typedef struct {
  signals signals;
  double phase;
  double dur_s;
} blip_data;

Node *sq_blip_node(double freq, double dur_s);

void blip_setup();

#endif
