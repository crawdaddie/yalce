#ifndef _SIGNAL_H
#define _SIGNAL_H

typedef struct Signal {
  // size of the data array will be size * layout
  // data is interleaved, so sample for frame x, channel 0 will be at index
  // layout * x + 0
  // sample for frame x, channel 1 will be at index layout * x + 1
  double *buf;
  int size;   // number of frames
  int layout; // how they are laid out
} Signal;

typedef struct SignalFloat {
  // size of the data array will be size * layout
  // data is interleaved, so sample for frame x, channel 0 will be at index
  // layout * x + 0
  // sample for frame x, channel 1 will be at index layout * x + 1
  float *buf;
  int size;   // number of frames
  int layout; // how they are laid out
} SignalFloat;

typedef struct SignalFloatDeinterleaved {
  // size of the data array will be size * layout
  // data is interleaved, so sample for frame x, channel 0 will be at index
  // layout * x + 0
  // sample for frame x, channel 1 will be at index layout * x + 1
  float *buf;
  int size;   // number of frames
  int layout; // how they are laid out
} SignalFloatDeinterleaved;

Signal *get_sig_default(int layout, double value);

void init_sig_ptrs();

Signal *get_sig(int layout);

Signal *get_sig_float(int layout);

#endif
