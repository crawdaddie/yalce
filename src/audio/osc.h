#ifndef _OSC_H
#define _OSC_H
#include "../common.h"
#include "../node.h"
#include "signal.h"

#define INS(node_data) ((signals *)node_data)->ins
#define IN(node, enum_name) (INS(node->data)[enum_name])
#define OUTS(node_data) ((signals *)node_data)->outs
#define NUM_INS(node_data) ((signals *)node_data)->num_ins
#define NUM_OUTS(node_data) ((signals *)node_data)->num_outs

typedef struct {
  Signal *ins;
  Signal *outs;
  int num_ins;
  int num_outs;
} signals;

// EXPORT SIGNAL ENUM
typedef enum {
  SIN_SIG_FREQ,
} sin_sig_map;

typedef struct {
  signals signals;
  double phase;
} sin_data;

Node *sin_node(double freq);

// EXPORT SIGNAL ENUM
typedef enum {
  SQ_SIG_FREQ,
} sq_sig_map;

typedef struct {
  signals signals;
  double phase;
} sq_data;

Node *sq_node(double freq);
Node *sq_detune_node(double freq);

// EXPORT SIGNAL ENUM
typedef enum {
  IMPULSE_SIG_FREQ,
} impulse_sig_map;

typedef struct {
  signals signals;
  double counter;
} impulse_data;

Node *impulse_node(double freq);

// EXPORT SIGNAL ENUM
typedef enum {
  POLY_SAW_SIG_FREQ,
} poly_saw_sig_map;

typedef struct {
  signals signals;
  double phase;
} poly_saw_data;

Node *poly_saw_node(double freq);

// EXPORT SIGNAL ENUM
typedef enum {
  PULSE_SIG_FREQ,
  PULSE_SIG_PW,
} pulse_sig_map;

typedef struct {
  signals signals;
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
