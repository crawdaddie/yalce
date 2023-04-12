#include "../src/audio/delay.h"
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
  return Val_unit;
}

CAMLprim value caml_kill_audio() {
  int audio_status = stop_audio();
  return Val_unit;
}

CAMLprim value caml_chain_nodes(value source_ptr, value dest_ptr, value idx) {
  Node *source = Node_val(source_ptr);
  Node *dest = Node_val(dest_ptr);
  chain_nodes(source, dest, Int_val(idx));
  return Val_node(source);
}

static Node *wrap_chain(Node *head, Node *tail) {
  Node *container = ALLOC_NODE(container_node_data, "Container");
  container->_sub = head;
  container->_sub_tail = tail; // point to the last node in the chain
                               // that's not an add_out or replace_out -
                               // could be the same node as head
  return container;
}

CAMLprim value caml_sin(value freq) {
  Node *osc = sin_node(Double_val(freq), NULL);
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */
  return Val_node(osc);
}

CAMLprim value caml_sq(value freq) {
  Node *osc = sq_node(Double_val(freq), NULL);
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */
  return Val_node(osc);
}

CAMLprim value caml_sq_detune(value freq) {
  Node *osc = sq_detune_node(Double_val(freq), NULL);
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */
  return Val_node(osc);
}

CAMLprim value caml_impulse(value freq) {

  Node *osc = impulse_node(Double_val(freq), NULL);
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */
  return Val_node(osc);
}

CAMLprim value caml_poly_saw(value freq) {

  Node *osc = poly_saw_node(Double_val(freq), NULL);

  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */

  return Val_node(osc);
}

CAMLprim value caml_pulse(value freq, value pw) {

  Node *osc = pulse_node(Double_val(freq), Double_val(pw), NULL);
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */

  return Val_node(osc);
}

CAMLprim value caml_simple_delay(value time, value fb) {
  Node *osc = simple_delay_node(Double_val(time), Double_val(fb), 1.0, NULL);
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

CAMLprim value caml_set_sig_float(value ml_ptr, value sig, value d) {
  Node *node = Node_val(ml_ptr);
  int sig_idx = Int_val(sig);

  if (sig_idx >= node->num_ins) {
    printf("node has no input %d\n", sig_idx);
    return Val_unit;
  }
  Signal signal = IN(node, sig_idx);
  set_signal(signal, Double_val(d));
  return Val_unit;
}

CAMLprim value caml_set_sig_node(value ml_ptr, value sig_num, value d) {
  Node *node = Node_val(ml_ptr);
  int sig_idx = Int_val(sig_num);
  Node *src = Node_val(d);

  if (sig_idx >= node->num_ins) {
    printf("node has no input %d\n", sig_idx);
    return Val_unit;
  }

  Signal input = IN(node, sig_idx);

  input.data = src->out.data;
  input.size = src->out.size;

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

CAMLprim value caml_set_add(value ml_ptr, value sig) {
  Node *node = Node_val(ml_ptr);
  double val = Double_val(sig);

  set_signal(node->add, val);
  return Val_unit;
}

CAMLprim value caml_set_mul(value ml_ptr, value sig) {
  Node *node = Node_val(ml_ptr);
  double val = Double_val(sig);

  set_signal(node->mul, val);
  return Val_unit;
}

CAMLprim value caml_set_add_node(value ml_ptr, value sig) {
  Node *node = Node_val(ml_ptr);
  Node *src = Node_val(sig);

  node->add.data = src->out.data;
  node->add.size = src->out.size;

  return Val_unit
}

CAMLprim value caml_set_mul_node(value ml_ptr, value sig) {
  Node *node = Node_val(ml_ptr);
  Node *val = Node_val(sig);
  node->mul.data = val->out.data;
  node->mul.size = val->out.size;

  return Val_unit;
}

CAMLprim value caml_play_node(value node_ptr) {
  Node *node = Node_val(node_ptr);

  Node *container = container_node(node);
  ctx_add_after_tail(container);

  return Val_node(container);
}

CAMLprim value caml_set_list_float(value ml_ptr, value l) {
  Node *node = Node_val(ml_ptr);
  /* int sig_idx = Int_val(sig); */
  /*  */
  /* if (sig_idx >= node->num_ins) { */
  /*   printf("node has no input %d\n", sig_idx); */
  /*   return Val_unit; */
  /* } */
  /* Signal signal = IN(node, sig_idx); */
  /* set_signal(signal, Double_val(d)); */
  return Val_unit;
}
