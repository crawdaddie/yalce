#ifndef _OUT_H
#define _OUT_H
#include "../common.h"
#include "../node.h"
#include "signal.h"

typedef struct {
  Signal *in;
  Signal *out;
} out_data;




Node *replace_out(Signal *in, double *channel_out);
Node *add_out(Signal *in, double *channel_out);

#endif
