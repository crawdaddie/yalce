#ifndef _USER_CTX
#define _USER_CTX
#include "audio/node.h"
#include "audio/synth.c"
#include "vec.h"
#include <time.h>

#define BUS_NUM 8
#define BUS_SIZE 2048

typedef struct List {
  Node *value;
  struct List *next;
} List;

typedef struct UserCtx {
  double **buses;
  double seconds_offset;
  double latency;
  List *graphs;
} UserCtx;

UserCtx *get_user_ctx(double latency);
Node *add_graph_to_ctx(UserCtx *ctx);

struct player_ctx {
  UserCtx *ctx;
  Node *group;
  struct timespec initial_time;
};
struct player_ctx *get_player_ctx_ref(UserCtx *ctx,
                                      struct timespec initial_time);
#endif
