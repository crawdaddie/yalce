#ifndef _USER_CTX
#define _USER_CTX
#include "audio/graph.h"
#include "queue.h"
#include "buf_read.h"
#include <jack/jack.h>
#include <stdlib.h>
#include <time.h>

#define BUS_SIZE 2048

typedef struct UserCtx {
  jack_port_t *input_port;
  jack_port_t **output_ports;
  Graph *graph;
  queue_t *msg_queue;

  sample_t **buses;

  struct buf_info **buffers;
} UserCtx;

UserCtx *get_user_ctx(jack_port_t *input_port, jack_port_t **output_ports,
                      queue_t *msg_queue);
Graph *process_queue(queue_t *queue, Graph *graph);

enum msg_type {
  ADD_BEFORE = 1,
  ADD_AFTER,
};

typedef void (*MsgAction)(Graph *graph, int time, void *ref);

typedef struct queue_msg_t {
  char *msg;
  int time;
  MsgAction func;
  void *ref;
  int num_args;
  void **args;
} queue_msg_t;
void free_msg(queue_msg_t *msg);
queue_msg_t *msg_init(char *msg_string, nframes_t time, void *func, int num_args);

#endif
