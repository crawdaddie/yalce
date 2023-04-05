#include "src/audio/out.h"
#include "src/audio/sin.h"
#include "src/audio/sq.h"
#include "src/ctx.h"
#include "src/node.h"
#include "src/start_audio.h"
#include <caml/memory.h>
#include <caml/mlvalues.h>
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
  Node *out = replace_out(NODE_DATA(sin_data, sin)->out, &ctx.out_chans[0]);

  ctx.head = sin;
  sin->next = out;

  return Val_int(0);
}

CAMLprim value caml_play_sq(value freq) {

  Node *sq = sq_node(Double_val(freq));
  Node *out = add_out(NODE_DATA(sq_data, sq)->out, &ctx.out_chans[0]);

  ctx.head = sq;
  sq->next = out;

  return Val_int(0);
}

CAMLprim value caml_play_sq_detune(value freq) {

  Node *sq = sq_detune_node(Double_val(freq));
  Node *out = add_out(NODE_DATA(sq_data, sq)->out, &ctx.out_chans[0]);

  ctx.head = sq;
  sq->next = out;

  return Val_int(0);
}
