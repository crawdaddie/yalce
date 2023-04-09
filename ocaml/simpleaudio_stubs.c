#include "../src/audio/blip.h"
#include "../src/audio/osc.h"
#include "../src/audio/out.h"
#include "../src/ctx.h"
#include "../src/log.h"
#include "../src/node.h"
#include "../src/oscilloscope.h"
#include "../src/start_audio.h"
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <stdio.h>

#include <pthread.h>

static value Val_node(Node *p) { return caml_copy_nativeint((intnat)p); }
static Node *Node_val(value v) { return (Node *)Nativeint_val(v); }
CAMLprim value caml_start_audio() {
  int audio_status = setup_audio();

  /* write_log("%s\n", audio_status == 0 ? "audio started" : "audio failed"); */
  /* fflush(log_stream); */

  return Val_unit;
}

CAMLprim value caml_kill_audio() {
  int audio_status = stop_audio();

  /* write_log("%s\n", audio_status == 0 ? "audio killed" : "kill audio
   * failed"); */
  /* fflush(log_stream); */

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
static void chain_nodes(Node *container, Node *filter) {
  Node *last = container->_sub_tail;
  NUM_INS(container->data) += NUM_INS(filter->data);
}

CAMLprim value caml_chain_nodes(value send_ptr, value recv_ptr) {
  Node *container = Node_val(send_ptr);
  Node *filter = Node_val(recv_ptr);
  chain_nodes(container, filter);

  return Val_unit;
}
typedef struct {
  signals signals;
} container_node_data;

static Node *wrap_chain(Node *head, Node *tail) {
  Node *container = ALLOC_NODE(container_node_data, "Container");
  container->_sub = head;
  container->_sub_tail = tail; // point to the last node in the chain
                               // that's not an add_out or replace_out -
                               // could be the same node as head
  return container;
}

CAMLprim value caml_play_sin(value freq) {
  Signal *sigs = ALLOC_SIGS(SIN_SIG_OUT);
  Node *osc = sin_node(Double_val(freq), sigs);
  Node *out = add_out(osc, &ctx.out_chans[0]);
  osc->next = out;

  Node *container = wrap_chain(osc, osc);
  INS(container->data) = sigs;
  NUM_INS(container->data) += SIN_SIG_OUT;
  ctx_add_after_tail(container);

  return Val_node(container);
}

CAMLprim value caml_play_sq(value freq) {

  Signal *sigs = ALLOC_SIGS(SQ_SIG_OUT);

  Node *osc = sq_node(Double_val(freq), sigs);
  Node *out = add_out(osc, &ctx.out_chans[0]);

  osc->next = out;

  Node *container = wrap_chain(osc, out);
  INS(container->data) = sigs;
  NUM_INS(container->data) += SQ_SIG_OUT;
  ctx_add_after_tail(container);

  return Val_node(container);
}

CAMLprim value caml_play_sq_detune(value freq) {
  Signal *sigs = ALLOC_SIGS(SQ_SIG_OUT);
  Node *osc = sq_detune_node(Double_val(freq), sigs);
  Node *out = add_out(osc, &ctx.out_chans[0]);

  osc->next = out;

  Node *container = wrap_chain(osc, osc);
  INS(container->data) = sigs;
  NUM_INS(container->data) += SQ_SIG_OUT;
  ctx_add_after_tail(container);

  return Val_node(container);
}

CAMLprim value caml_play_impulse(value freq) {

  Signal *sigs = ALLOC_SIGS(IMPULSE_SIG_OUT);

  Node *osc = impulse_node(Double_val(freq), sigs);
  Node *out = add_out(osc, &ctx.out_chans[0]);

  osc->next = out;

  Node *container = wrap_chain(osc, osc);
  INS(container->data) = sigs;
  NUM_INS(container->data) += IMPULSE_SIG_OUT;
  ctx_add_after_tail(container);

  return Val_node(container);
}

CAMLprim value caml_play_poly_saw(value freq) {

  Signal *sigs = ALLOC_SIGS(POLY_SAW_SIG_OUT);

  Node *osc = poly_saw_node(Double_val(freq), sigs);
  Node *out = add_out(osc, &ctx.out_chans[0]);

  osc->next = out;

  Node *container = wrap_chain(osc, osc);
  INS(container->data) = sigs;
  NUM_INS(container->data) += POLY_SAW_SIG_OUT;
  ctx_add_after_tail(container);

  return Val_node(container);
}

CAMLprim value caml_play_pulse(value freq, value pw) {

  Signal *sigs = ALLOC_SIGS(PULSE_SIG_OUT);
  Node *osc = pulse_node(Double_val(freq), Double_val(pw), sigs);
  Node *out = add_out(osc, &ctx.out_chans[0]);

  osc->next = out;

  Node *container = wrap_chain(osc, osc);
  INS(container->data) = sigs;
  NUM_INS(container->data) += PULSE_SIG_OUT;
  ctx_add_after_tail(container);

  return Val_node(container);
}

CAMLprim value caml_play_blip(value freq, value dur_s) {

  Node *osc = sq_blip_node(Double_val(freq), Double_val(dur_s));
  Node *out = add_out(osc, &ctx.out_chans[0]);

  osc->next = out;

  ctx_add_after_tail(osc);

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

  if (sig_idx >= ((signals *)node->data)->num_ins) {
    printf("node has no input %d\n", sig_idx);
    return Val_unit;
  }
  Signal signal = IN(node, sig_idx);
  set_signal(signal, Double_val(d));
  return Val_unit;
}

CAMLprim value caml_kill_node(value ml_ptr) {
  Node *node = Node_val(ml_ptr);
  node->killed = true;
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

#define INDENT(indent) write_log("%.*s", indent, "  ")
static void dump_nodes(Node *head, int indent) {
  if (!head) {
    return;
  }
  INDENT(indent);
  write_log("%s num inputs: %d", head->name, ((signals *)head->data)->num_ins);

  if (head->_sub) {
    write_log("\n");
    dump_nodes(head->_sub, indent + 1);
  }
  if (head->next) {
    write_log("\n");
    dump_nodes(head->next, indent);
  }
  write_log("\n");
  return;
}

CAMLprim value caml_dump_nodes() {
  dump_nodes(ctx.head, 0);
  fflush(log_stream);
  return Val_unit;
}
