#include "./audio_graph.h"
#include "./audio_loop.h"
#include "./ctx.h"
#include "./lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "./scheduling.h"

int main(int argc, char **argv) {

  AudioGraph *template = sin_ensemble();

  init_audio();

  Ctx *ctx = get_audio_ctx();

#define S 5
  double freqs[S] = {150., 300., 450, 200., 175.};
  int note_count = 0;
  int current_freq_idx = 0;

  return 0;
}
