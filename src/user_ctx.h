#ifndef _USER_CTX
#define _USER_CTX
#include <time.h>
#include <stdlib.h>
#include <jack/jack.h>
#include "audio/graph.h"
#include "queue.h"

#define BUS_NUM 8
#define BUS_SIZE 2048

typedef struct UserCtx {
  jack_port_t *input_port;
  jack_port_t **output_ports;
  Graph *graph;
  queue_t *msg_queue;
} UserCtx;

UserCtx *get_user_ctx();
Graph *process_queue(queue_t *queue, Graph *graph);

enum msg_type {
  ADD_BEFORE = 1,
  ADD_AFTER,
};
#endif
