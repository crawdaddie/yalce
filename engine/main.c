#include "audio_loop.h"
#include "ctx.h"
#include "node.h"

Node *synth(double freq, double cutoff) {
  Node *group = group_new(0);
  Node *sq1 = sq_node(freq);
  group_add_tail(group, sq1);

  Node *sq2 = sq_node(freq * 1.01);
  group_add_tail(group, sq2);

  Node *summed = sum2_node(sq1, sq2);
  group_add_tail(group, summed);
  add_to_dac(summed);

  return group;
}

int main(int argc, char **argv) {
  init_audio();
  Node *s = synth(50., 500.);

  add_to_dac(s);
  audio_ctx_add(s);

  while (1) {
    // print_graph();
  }
  return 0;
}
