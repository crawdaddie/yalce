#include "entry.h"
#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

Node *sq(double freq) {
  sq_data *dsq = malloc(sizeof(sq_data));
  dsq->phase = 0.0;
  Signal in = get_sig(1);
  int n = in.size;
  // double *dptr = in.data;
  for (int i = 0; i < n; i++) {
    in.data[i] = freq;
  }

  Node *s = node_new(dsq, sq_perform, in, get_sig(1));
  s->ins = in;
  return s;
}

Node *lfnoise(double freq, double min, double max) {
  lf_noise_data *noise_d = malloc(sizeof(lf_noise_data));
  noise_d->phase = 0.0;
  noise_d->target = random_double_range(min, max);
  noise_d->min = min;
  noise_d->max = max;
  return node_new(noise_d, lf_noise_perform, (Signal){}, get_sig(1));
}
void pipe_output(Node *send, Node *recv) { recv->ins.data = send->out.data; }
void add_to_dac(Node *node) { node->type = OUTPUT; }

int entry() {
  Ctx *ctx = get_audio_ctx();
  Node *noise = lfnoise(20.0, 20.0, 1000.0);
  ctx_add(noise);

  Node *nsq = sq(100.0);
  ctx_add(nsq);
  pipe_output(noise, nsq);
  add_to_dac(nsq);
  sleep(10);
}
