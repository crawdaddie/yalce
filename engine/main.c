#include "audio_loop.h"
#include "ctx.h"
#include "lib.h"
#include "node.h"
#include "oscillators.h"
#include "scheduling.h"
#include <stdio.h>

Node *test_synth(double freq, double cutoff) {
  Node *group = group_new(0);
  Node *sq1 = sq_node(get_sig_default(1, freq));
  group_add_tail(group, sq1);

  Node *sq2 = sq_node(get_sig_default(1, freq * 1.01));
  group_add_tail(group, sq2);

  Node *summed = sum2_node(sq1, sq2);
  group_add_tail(group, summed);
  add_to_dac(summed);

  return group;
}

int _main(int argc, char **argv) {
  init_audio();
  Node *s = test_synth(50., 500.);

  add_to_dac(s);
  audio_ctx_add(s);

  while (1) {
    // print_graph();
  }
  return 0;
}

struct userdata_t {
  int a;
};
// Example callback function
void _example_cb(void *user_data, uint64_t t) {
  printf("Callback executed at %llu\n", t);
}

int main(int argc, char **argv) {
  init_audio();
  // Signal *buf = read_buf("../fat_amen_mono_48000.wav");
  //
  // Node *b = bufplayer_node(buf, get_sig_default(1, 0.99),
  //                          get_sig_default(1, 0.75), get_sig_default(1,
  //                          0.0));
  //
  // add_to_dac(b);
  // audio_ctx_add(b);
  schedule_event(_example_cb, &(struct userdata_t){200}, 1.0);

  while (1) {
    // print_graph();
  }
  return 0;
}
