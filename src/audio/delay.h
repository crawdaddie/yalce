#ifndef _DELAY_H
#define _DELAY_H
#include "../node.h"
#include "signal.h"

// EXPORT SIGNAL ENUM
typedef enum {
  DELAY_SIG_INPUT,
  DELAY_SIG_TIME,
  DELAY_SIG_FB,
} delay_sig_map;

typedef struct {
  signals signals;
  int bufsize;
  double read_ptr;
  int write_ptr;
  double *buffer;
} delay_data;

Node *simple_delay_node(double delay_time_s, double delay_fb, int bufsize_s,
                        Signal *ins);

#endif
