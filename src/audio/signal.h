#ifndef _SIGNAL_H
#define _SIGNAL_H

typedef enum {
  SIGNAL_AUDIO,
  SIGNAL_CTL,
} signal_type;

typedef struct Signal {
  double *data;
  int size;
  int layout;
  signal_type type;
} Signal;

Signal new_signal(int size);

Signal *new_signal_heap(int size, int layout);
Signal *new_signal_heap_default(int size, int layout, double def);

void set_signal(Signal signal, double value);

void set_signal_ramp(Signal signal, double value, int time);

double unwrap(Signal sig, int frame);

void write(Signal *signal, int frame, double value);

void init_signal(Signal *signal, int size, double def);

#endif
