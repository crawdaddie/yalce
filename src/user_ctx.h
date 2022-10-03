#ifndef _USER_CTX
#define _USER_CTX
#include "audio/node.h"
#include "audio/synth.c"
#include <soundio/soundio.h>

typedef struct UserCtx {
  double **buses;
  Node *graph;
  void (*ctx_play_synth)(struct UserCtx *ctx);
  double *(*get_bus)(struct UserCtx *ctx, int bus_num);
} UserCtx;

Node *get_graph(struct SoundIoOutStream *outstream, double *bus);

UserCtx *get_user_ctx(struct SoundIoOutStream *outstream);

double *get_bus(UserCtx *ctx, int bus_num);
#endif
