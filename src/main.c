#include "ctx.h"
#include "entry.h"
#include "node.h"
#include "start_audio.h"
#include <unistd.h>

int main(int argc, char **argv) {
  maketable_sin();
  start_audio();
  entry();

  // lf_noise_interp_data noise_d = {.phase = 0.0, .target=60.0, .current
  // = 60.0}; Node *lf_noise = node_new(&noise_d, lf_noise_interp_perform,
  // (Signal){}, get_sig(1)); ctx->head = lf_noise;

  stop_audio();
}
