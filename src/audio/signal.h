#ifndef _SIGNAL_H
#define _SIGNAL_H

typedef struct Signal {
  double *data;
  int size;
  int layout;
} Signal;

typedef struct {
  Signal *ins;
  Signal *outs;
  int num_ins;
  int num_outs;
} signals;

#define INS(node_data) ((signals *)node_data)->ins
#define IN(node, enum_name) (INS(node->data)[enum_name])
#define OUTS(node_data) ((signals *)node_data)->outs
#define NUM_INS(node_data) ((signals *)node_data)->num_ins
#define NUM_OUTS(node_data) ((signals *)node_data)->num_outs

#define ALLOC_SIGS(enum_value) calloc(sizeof(Signal), enum_value + 1)

Signal new_signal(int size);

Signal *new_signal_heap(int size, int layout);
Signal *new_signal_heap_default(int size, int layout, double def);

void set_signal(Signal signal, double value);

void set_signal_ramp(Signal signal, double value, int time);

double unwrap(Signal sig, int frame);

void signal_write(Signal *signal, int frame, double value);

void init_signal(Signal *signal, int size, double def);

void init_out_signal(Signal *signal, int size, int layout);

#endif
