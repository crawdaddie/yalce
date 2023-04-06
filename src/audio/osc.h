#ifndef _OSC_H
#define _OSC_H
#include "../common.h"
#include "../node.h"
#include "signal.h"

typedef struct {
  Signal *freq;
  Signal *out;
  double phase;
} sin_data;

typedef struct {
  double phase;
  double freq;
  double pm_freq;
  double pm_index;
  Signal *out;
} pmsin_data;

Node *sin_node(double freq);

Node *pmsin_node(double freq, double pm_index, double pm_freq);

typedef struct {
  Signal *freq;
  Signal *out;
  double ramp;
} sq_data;
/* node_perform sq_perform(Node *node, int nframes, double spf); */
Node *sq_node(double freq);

Node *sq_detune_node(double freq);

typedef struct {
  Signal *freq;
  Signal *out;
  double counter;
} impulse_data;

Node *impulse_node(double freq);

typedef struct {
  Signal *freq;
  Signal *out;
  double phase;
} poly_saw_data;

Node *poly_saw_node(double freq);

typedef struct {
  Signal *freq;
  Signal *out;
  Signal *pw;
  double phase;
} pulse_data;

Node *pulse_node(double freq, double pw);

typedef struct {
  Signal *freq;
  Signal *out;
  double *phase;
  int num_oscs;
  double dt_amt;
} hoover_data;

Node *hoover_node(double freq, int num_oscs, double detune_spread);

void osc_setup();
#endif
