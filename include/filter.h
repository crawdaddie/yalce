#ifndef _FILTER_H
#define _FILTER_H
#include "node.h"
#include "signal.h"

#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif

typedef enum { BIQUAD_SIG_INPUT, BIQUAD_SIG_OUT } biquad_sig_map;

typedef struct {
  double a0, a1, a2, a3, a4;
  double x1, x2, y1, y2;
} biquad_data;
/* filter types */

typedef enum {
  BIQUAD_LPF,   /* low pass filter */
  BIQUAD_HPF,   /* High pass filter */
  BIQUAD_BPF,   /* band pass filter */
  BIQUAD_NOTCH, /* Notch Filter */
  BIQUAD_PEQ,   /* Peaking band EQ filter */
  BIQUAD_LSH,   /* Low shelf filter */
  BIQUAD_HSH    /* High shelf filter */
} biquad_filter_type;

Node *biquad_node(const biquad_filter_type type,
                  double dbGain,          /* gain of filter */
                  const double freq,      /* center frequency */
                  const double bandwidth, /* bandwidth in octaves */
                  Signal *ins);

Node *biquad_lpf_node(                        /* gain of filter */
                      const double freq,      /* center frequency */
                      const double bandwidth, /* bandwidth in octaves */
                      Signal *ins);
#endif
