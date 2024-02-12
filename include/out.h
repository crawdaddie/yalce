#ifndef _OUT_H
#define _OUT_H
#include "common.h"
#include "ctx.h"
#include "node.h"
#include "signal.h"

typedef struct {
  Signal *in;
  Signal *out_channel;
} out_data;

Node *replace_out(Signal *in, Signal *channel_out);
Node *add_out(Node *in, Signal *channel_out);

#endif
