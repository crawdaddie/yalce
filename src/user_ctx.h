#ifndef _USER_CTX
#define _USER_CTX
#include "audio/node.h"
#include "audio/synth.c"
#define BUS_NUM 8
#define BUS_SIZE 2048

typedef struct UserCtx {
  double **buses;
  Node *graph;
  /* void (*ctx_play_synth)(struct UserCtx *ctx); */
  /* double *(*get_bus)(struct UserCtx *ctx, int bus_num); */
  double seconds_offset;
} UserCtx;
#endif
