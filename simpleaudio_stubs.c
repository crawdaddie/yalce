#include "src/audio/blip.h"
#include "src/audio/osc.h"
#include "src/audio/out.h"
#include "src/ctx.h"
#include "src/node.h"
#include "src/oscilloscope.h"
#include "src/start_audio.h"
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <stdio.h>

#include <pthread.h>

static value Val_node(Node *p) { return caml_copy_nativeint((intnat)p); }
static Node *Node_val(value v) { return (Node *)Nativeint_val(v); }
CAMLprim value caml_start_audio() {
  int audio_status = setup_audio();

  printf("%s\n", audio_status == 0 ? "audio started" : "audio failed");

  return Val_unit;
}

CAMLprim value caml_kill_audio() {
  int audio_status = stop_audio();

  printf("%s\n", audio_status == 0 ? "audio killed" : "kill audio failed");

  return Val_unit;
}

CAMLprim value caml_add_node() {
  printf("add node\n");
  return Val_unit;
}

CAMLprim value caml_sq() {
  printf("sq node\n");
  return Val_unit;
}

CAMLprim value caml_chain_nodes() {
  printf("chain nodes\n");
  return Val_unit;
}

CAMLprim value caml_play_sin(value freq) {

  Node *osc = sin_node(Double_val(freq));
  /* Node *out = add_out(OUTS(NODE_DATA(poly_saw_data, osc)),
   * &ctx.out_chans[0]); */

  ctx.out_chans[0].head = osc;
  /* osc->next = out; */

  return Val_node(osc);
}

CAMLprim value caml_play_sq(value freq) {

  Node *osc = sq_node(Double_val(freq));
  Node *out = add_out(OUTS(NODE_DATA(sq_data, osc)), &ctx.out_chans[0]);

  ctx.head = osc;
  osc->next = out;

  return Val_node(osc);
}

CAMLprim value caml_play_sq_detune(value freq) {

  Node *osc = sq_detune_node(Double_val(freq));
  Node *out = add_out(OUTS(NODE_DATA(sq_data, osc)), &ctx.out_chans[0]);

  ctx.head = osc;
  osc->next = out;

  return Val_node(osc);
}

CAMLprim value caml_play_impulse(value freq) {

  Node *osc = impulse_node(Double_val(freq));
  Node *out = add_out(OUTS(NODE_DATA(impulse_data, osc)), &ctx.out_chans[0]);

  ctx.head = osc;
  osc->next = out;

  return Val_node(osc);
}

CAMLprim value caml_play_poly_saw(value freq) {

  Node *osc = poly_saw_node(Double_val(freq));
  Node *out = add_out(OUTS(NODE_DATA(poly_saw_data, osc)), &ctx.out_chans[0]);

  ctx.head = osc;
  osc->next = out;

  return Val_node(osc);
}

CAMLprim value caml_play_hoover(value freq) {

  Node *osc = hoover_node(Double_val(freq), 2, 1.001);
  Node *out = add_out(NODE_DATA(hoover_data, osc)->out, &ctx.out_chans[0]);

  ctx.head = osc;
  osc->next = out;

  return Val_node(osc);
}

CAMLprim value caml_play_pulse(value freq, value pw) {

  Node *osc = pulse_node(Double_val(freq), Double_val(pw));
  Node *out = add_out(OUTS(NODE_DATA(pulse_data, osc)), &ctx.out_chans[0]);

  ctx.head = osc;
  osc->next = out;

  return Val_node(osc);
}

CAMLprim value caml_play_blip(value freq, value dur_s) {

  Node *osc = sq_blip_node(Double_val(freq), Double_val(dur_s));
  Node *out = add_out(OUTS(NODE_DATA(blip_data, osc)), &ctx.out_chans[0]);

  ctx.head = osc;
  osc->next = out;

  return Val_node(osc);
}
CAMLprim value caml_oscilloscope() {
  /* pthread_t oscil_thread; */
  /* pthread_create(&oscil_thread, NULL, oscilloscope, NULL); */
  /* pthread_detach(oscil_thread); */
  oscilloscope();
  return Val_unit;
}

CAMLprim value caml_set_freq(value ml_ptr, value d) {
  Node *node;
  node = Node_val(ml_ptr);
  Signal freq = IN(node, SIN_SIG_FREQ);
  set_signal(freq, Double_val(d));
  return Val_unit;
}

CAMLprim value caml_set_sig(value ml_ptr, value sig, value d) {
  Node *node = Node_val(ml_ptr);
  int sig_idx = Int_val(sig);

  if (sig_idx >= ((signals *)node->data)->num_outs) {
    printf("node has no input %d\n", sig_idx);
    return Val_unit;
  }
  Signal signal = IN(node, sig_idx);
  set_signal(signal, Double_val(d));
  return Val_unit;
}

typedef struct {
  int x;
  char *y;
} obj;

typedef struct _obj_st {
  double d;
  int i;
  char c;
} obj_st;

static obj_st *Objst_val(value v) { return (obj_st *)Nativeint_val(v); }
static value Val_objst(obj_st *p) { return caml_copy_nativeint((intnat)p); }

CAMLprim value wrapping_ptr_ml2c(value d, value i, value c) {
  obj_st *my_obj;
  my_obj = malloc(sizeof(obj_st));
  my_obj->d = Double_val(d);
  my_obj->i = Int_val(i);
  my_obj->c = Int_val(c);
  return Val_objst(my_obj);
}
CAMLprim value set_d(value ml_ptr, value d) {
  obj_st *my_obj;
  my_obj = Objst_val(ml_ptr);
  my_obj->d = Double_val(d);
  printf("set d to %f\n", my_obj->d);
  return Val_unit;
}

CAMLprim value dump_ptr(value ml_ptr) {
  obj_st *my_obj;
  my_obj = Objst_val(ml_ptr);
  printf(" d: %g\n i: %d\n c: %c\n", my_obj->d, my_obj->i, my_obj->c);
  return Val_unit;
}

CAMLprim value free_ptr(value ml_ptr) {
  obj_st *my_obj;
  my_obj = Objst_val(ml_ptr);
  free(my_obj);
  return Val_unit;
}
