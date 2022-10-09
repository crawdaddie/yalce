#ifndef _USER_CTX
#define _USER_CTX
#include <time.h>
#include <stdlib.h>
#include <jack/jack.h>
#include "audio/graph.h"

#define BUS_NUM 8
#define BUS_SIZE 2048

typedef struct UserCtx {
  jack_port_t *input_port;
  jack_port_t **output_ports;
  Graph *graph;
} UserCtx;

UserCtx *get_user_ctx();
#endif
