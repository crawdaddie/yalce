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
  t_queue *msg_queue;

  t_sample **buses;

  struct buf_info **buffers;
} UserCtx;

UserCtx *get_user_ctx(jack_port_t *input_port, jack_port_t **output_ports,
                      t_queue *msg_queue);
Graph *process_queue(t_queue *queue, Graph *graph);

enum msg_type {
  ADD_BEFORE = 1,
  ADD_AFTER,
};

typedef void (*MsgAction)(Graph *graph, int time, void *ref);

typedef struct t_queue_msg {
  char      *msg;
  int       time;
  MsgAction func;
  void      *ref;
  int   num_args;
  void    **args;
} t_queue_msg;

void free_msg(t_queue_msg *msg);

t_queue_msg *msg_init(char *msg_string, t_nframes time, void *func, int num_args);

#endif
