#ifndef _SIGNAL_H
#define _SIGNAL_H

typedef struct Signal {
  double *data;
  int size;
  int num_chans;
} Signal;

Signal new_signal(double def, int size);

Signal *new_signal_heap(double def, int size);

void set_signal(Signal signal, double value);

void set_signal_ramp(Signal signal, double value, int time);

double unwrap(Signal sig, int frame);

#endif
