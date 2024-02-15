#ifndef _OSC_H
#define _OSC_H
#include "common.h"
#include "node.h"
#include "signal.h"

// EXPORT SIGNAL ENUM
typedef enum {
  SIN_SIG_FREQ,
  SIN_SIG_OUT,
} sin_sig_map;

typedef struct {
  double phase;
} sin_data;

Node *sin_node(double freq);

// EXPORT SIGNAL ENUM
typedef enum {
  SQ_SIG_FREQ,
  SQ_SIG_OUT,
} sq_sig_map;

typedef struct {
  double phase;
} sq_data;

Node *sq_node(double freq);
Node *sq_detune_node(double freq);

// EXPORT SIGNAL ENUM
typedef enum {
  IMPULSE_SIG_FREQ,
  IMPULSE_SIG_OUT,
} impulse_sig_map;

typedef struct {
  double counter;
} impulse_data;

Node *impulse_node(double freq);

// EXPORT SIGNAL ENUM
typedef enum {
  POLY_SAW_SIG_FREQ,
  POLY_SAW_SIG_OUT,
} poly_saw_sig_map;

typedef struct {
  double phase;
} poly_saw_data;

Node *poly_saw_node(double freq);

// EXPORT SIGNAL ENUM
typedef enum {
  PULSE_SIG_FREQ,
  PULSE_SIG_PW,
  PULSE_SIG_OUT,
} pulse_sig_map;

typedef struct {
  double phase;
} pulse_data;

Node *pulse_node(double freq, double pw);

void osc_setup();
#endif
