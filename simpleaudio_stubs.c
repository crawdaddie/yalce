#include "src/start_audio.h"
#include <caml/mlvalues.h>
#include <caml/memory.h>
#include "src/audio/sin.h"
#include "src/audio/out.h"
#include "src/node.h"
#include "src/ctx.h"
#include <stdio.h>

CAMLprim value caml_start_audio() {
  int audio_status = setup_audio();

  printf("%s\n", audio_status == 0 ? "audio started" : "audio failed");

  return Val_int(audio_status);
}

CAMLprim value caml_kill_audio() {
  int audio_status = stop_audio();

  printf("%s\n", audio_status == 0 ? "audio killed" : "kill audio failed");

  return Val_int(audio_status);
}

CAMLprim value caml_add_node() {
  printf("add node\n");
  return Val_int(0);
}

CAMLprim value caml_sq() {
  printf("sq node\n");
  return Val_int(0);
}

CAMLprim value caml_chain_nodes() {
  printf("chain nodes\n");
  return Val_int(0);
}

CAMLprim value caml_play_sin(value freq) {

  Node *sin = sin_node(Double_val(freq));
  Node *out = replace_out(
      NODE_DATA(sin_data, sin)->out,
      ctx.out_chans[0].data
    );

  ctx.head = sin;
  sin->next = out;

  return Val_int(0);
}
