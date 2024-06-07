#ifndef _ENGINE_OSCILLATORS_H
#define _ENGINE_OSCILLATORS_H

#include "node.h"
void maketable_sq(void);
typedef struct {
  double phase;
} sq_state;

void sq_perform(void *state, double *out, int num_ins, double **inputs,
                int nframes, double spf);

typedef struct {
  double phase;
} sin_state;

void sin_perform(void *state, double *out, int num_ins, double **inputs,
                 int nframes, double spf);

void maketable_sin(void);

#endif
