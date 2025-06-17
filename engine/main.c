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
  float freqs[S] = {150., 300., 450, 200., 175.};
  int note_count = 0;
  int current_freq_idx = 0;

  while (1) {

    float freq = freqs[rand() % S];
    printf("%f\n", freq);
    InValList v = {{0, freq}, NULL};
    Node *ensemble = instantiate_template(template, &v);
    push_msg(&ctx->msg_queue, (scheduler_msg){
                                  NODE_ADD,
                                  get_frame_offset(),
                                  {.NODE_ADD = {.target = ensemble}},
                              });

    AudioGraph *gr = (char *)ensemble + (sizeof(Node));

    // float *trig_buf = audio_graph_inlet(gr, 1)->output.data;

    // audio_ctx_add(ensemble);

    useconds_t tt = 1 << 17;
    usleep(1 << 12); // Set trigger LOW (release phase)
    push_msg(&ctx->msg_queue,
             (scheduler_msg){NODE_SET_SCALAR,
                             get_frame_offset(),
                             {.NODE_SET_SCALAR = {.target = ensemble, 1, 0.}}});
    usleep(tt);
  }

  return 0;
}
