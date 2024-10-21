#include "audio_loop.h"
#include "clap_host.h"
#include "ctx.h"
#include "lib.h"
#include "node.h"
#include "oscillators.h"
#include "scheduling.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {

  init_audio();
  // Signal *buf = read_buf("../fat_amen_mono_48000.wav");
  Signal *buf = read_buf_mono(
      "/Users/adam/Desktop/Snakes\ of\ Russia\ -\ Oblations\ Sample\ "
      "Pack/Textures/SOR_OB_Texture_Beginning_Dm.wav");

  Node *impulse = nbl_impulse_node(get_sig_default(1, 90.));
  audio_ctx_add(impulse);

  // Node *pos = ramp_node(get_sig_default(1, 0.2));
  Node *pos = trig_rand_node(&impulse->out);
  audio_ctx_add(pos);

  double sels[] = {1.0, 2.0, 4.0, 0.5, 1.5};
  Signal sel_sig = {sels, 5, 1};
  Node *rate = trig_sel_node(&impulse->out, &sel_sig);
  audio_ctx_add(rate);

  Node *grains = granulator_node(200, buf, &impulse->out, &pos->out, &rate->out
                                 // get_sig_default(1, 0.5)
  );
  audio_ctx_add(grains);

  add_to_dac(grains);

  Signal *grain_out = &grains->out;

  NodeRef rev = clap_node("/Library/Audio/Plug-Ins/CLAP/TAL-Reverb-4.clap/"
                          "Contents/MacOS/TAL-Reverb-4",
                          1, &grain_out);
  while (1) {
    sleep(1);
  }
  return 0;
}
