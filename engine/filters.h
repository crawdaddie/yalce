#ifndef _ENGINE_FILTERS_H
#define _ENGINE_FILTERS_H
#include "node.h"

typedef struct {
  double b0, b1, b2; // Feedforward coefficients
  double a1, a2;     // Feedback coefficients
  double x1, x2;     // Input delay elements (x[n-1] and x[n-2])
  double y1, y2;     // Output delay elements (y[n-1] and y[n-2])
} biquad_state;

void init_biquad_filter_state(biquad_state *filter, double b0, double b1,
                              double b2, double a1, double a2);
Node *biquad_node(Signal *in);
Node *biquad_lp_node(Signal *freq, Signal *res, Signal *in);
Node *biquad_hp_node(Signal *freq, Signal *res, Signal *in);
Node *biquad_bp_node(Signal *freq, Signal *res, Signal *in);

// Node *butterworth_hp_dyn_node(double freq, Node *in);
#endif
