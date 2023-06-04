#ifndef _SIGNAL_H
#define _SIGNAL_H
#include <stdlib.h>

typedef struct Signal {
  // size of the data array will be size * layout
  // data is interleaved, so sample for frame x, channel 0 will be at index
  // layout * x + 0
  // sample for frame x, channel 1 will be at index layout * x + 1
  double *data;
  int size;   // number of frames
  int layout; // how they are laid out
} Signal;

typedef struct {
  Signal *ins;
  Signal *outs;
  int num_ins;
  int num_outs;
} signals;

#define ALLOC_SIGS(out_enum) calloc(sizeof(Signal), out_enum)

int SAMPLE_IDX(struct Signal sig, int frame, int channel);

Signal new_signal(int size);

Signal *new_signal_heap(int size, int layout);
Signal *new_signal_heap_default(int size, int layout, double def);

void set_signal(Signal signal, double value);

void set_signal_ramp(Signal signal, double value, int time);

double unwrap(Signal sig, int frame);
int sample_idx(Signal sig, int frame, int channel);

void signal_write(Signal *signal, int frame, double value);

void init_signal(Signal *signal, int size, double def);

void init_out_signal(Signal *signal, int size, int layout);

#endif
