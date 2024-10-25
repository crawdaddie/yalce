#include "audio_loop.h"
#include "clap_node.h"
#include "ctx.h"
#include "lib.h"
#include "node.h"
#include "oscillators.h"
#include <unistd.h>

int main(int argc, char **argv) {

  init_audio();
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

  Node *grains =
      granulator_node(200, buf, &impulse->out, &pos->out, &rate->out);
  audio_ctx_add(grains);
  // Node *grains_amp_mod = sin_node(get_sig_default(1, 0.5));
  // audio_ctx_add(grains_amp_mod);
  // grains = mul2_node(grains, grains_amp_mod);
  // audio_ctx_add(grains);

  Signal *grain_out = &grains->out;
  NodeRef reverb =
      clap_node("/Library/Audio/Plug-Ins/CLAP/RoomReverb.clap", grain_out);

  set_clap_param(reverb, 1, 0.9);
  set_clap_param(reverb, 2, 0.01);
  set_clap_param(reverb, 4, 1.);
  set_clap_param(reverb, 12, 1.);
  set_clap_param(reverb, 13, 1.);
  set_clap_param(reverb, 5, 1.);
  set_clap_param(reverb, 6, 1.);
  set_clap_param(reverb, 17, 1.0);
  // set_param_with_event(reverb->state, "Mode", 1.0);

  audio_ctx_add(reverb);
  add_to_dac(reverb);

  while (1) {
    sleep(1);
  }
  return 0;
}
