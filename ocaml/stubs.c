#include "../src/audio/bufplayer.h"
#include "../src/audio/delay.h"
#include "../src/audio/filter.h"
#include "../src/audio/osc.h"
#include "../src/audio/out.h"
#include "../src/ctx.h"
#include "../src/dbg.h"
#include "../src/log.h"
#include "../src/midi.h"
#include "../src/node.h"
#include "../src/oscilloscope.h"
#include "../src/scheduling.h"
#include "../src/soundfile.h"
#include "../src/start_audio.h"
#include "caml/misc.h"
#include <caml/alloc.h>
#include <caml/bigarray.h>
#include <caml/callback.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <stdio.h>

#include <pthread.h>

static value Val_node(Node *p) { return caml_copy_nativeint((intnat)p); }
static Node *Node_val(value v) { return (Node *)Nativeint_val(v); }
static value Val_signal(Signal *p) { return caml_copy_nativeint((intnat)p); }
static Signal *Signal_val(value v) { return (Signal *)Nativeint_val(v); }

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
  Node *container = ALLOC_NODE(container_node_data, "Container", 0);
  container->_sub = head;
  container->_sub_tail = tail; // point to the last node in the chain
                               // that's not an add_out or replace_out -
                               // could be the same node as head
  return container;
}

CAMLprim value caml_sin(value freq) {
  Node *osc = sin_node(Double_val(freq));
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */
  return Val_node(osc);
}

CAMLprim value caml_sq(value freq) {
  Node *osc = sq_node(Double_val(freq));
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */
  return Val_node(osc);
}

CAMLprim value caml_sq_detune(value freq) {
  Node *osc = sq_detune_node(Double_val(freq));
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */
  return Val_node(osc);
}

CAMLprim value caml_impulse(value freq) {

  Node *osc = impulse_node(Double_val(freq));
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */
  return Val_node(osc);
}

CAMLprim value caml_poly_saw(value freq) {

  Node *osc = poly_saw_node(Double_val(freq));

  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */

  return Val_node(osc);
}

CAMLprim value caml_pulse(value freq, value pw) {

  Node *osc = pulse_node(Double_val(freq), Double_val(pw));
  /* Node *container = container_node(osc); */
  /* ctx_add_after_tail(container); */

  return Val_node(osc);
}

CAMLprim value caml_simple_delay(value time, value fb, value ml_ptr) {

  Node *node = Node_val(ml_ptr);
  Node *delay =
      simple_delay_node(Double_val(time), Double_val(fb), 1.0, &node->out);
  node->next = delay;
  delay->prev = node;
  return Val_node(delay);
}

CAMLprim value caml_biquad_lpf(value freq, value bw, value ml_ptr) {
  Node *node = Node_val(ml_ptr);
  Node *biquad = biquad_lpf_node(Double_val(freq), Double_val(bw), &node->out);
  node->next = biquad;
  biquad->prev = node;
  return Val_node(biquad);
}
CAMLprim value caml_bufplayer(value ml_rate, value ml_signal_ptr) {
  Signal *buf = Signal_val(ml_signal_ptr);
  Node *osc = bufplayer_node(buf, 48000, Double_val(ml_rate), 0.0, 1);
  return Val_node(osc);
}

CAMLprim value caml_bufplayer_timestretch(value ml_rate,
                                          value ml_pitchshift_rate,
                                          value ml_trig_freq,
                                          value ml_signal_ptr) {
  Signal *buf = Signal_val(ml_signal_ptr);
  Node *osc = bufplayer_timestretch_node(buf, 48000, Double_val(ml_rate),
                                         Double_val(ml_pitchshift_rate),
                                         Double_val(ml_trig_freq), 0.0, 1);
  return Val_node(osc);
}

CAMLprim value caml_oscilloscope() {
  /* pthread_t oscil_thread; */
  /* pthread_create(&oscil_thread, oscilloscope); */
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

CAMLprim value caml_dump_nodes() {
  dump_nodes(ctx.head, 0);
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

  return Val_unit;
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

CAMLprim value caml_set_list_float(value ml_ptr, value ml_num, value ml_values,
                                   value ml_indices) {
  int num_params = Int_val(ml_num);
  Node *node = Node_val(ml_ptr);

  double *values = (double *)Caml_ba_data_val(ml_values);

  printf("got vals\n");
  int *indices = (int *)Caml_ba_data_val(ml_indices);

  printf("got indices\n");
  for (int i = 0; i < num_params; i++) {
    printf("idx: %d val: %f\n", indices[i], values[i]);
  }

  /* int sig_idx = Int_val(sig); */
  /*  */
  /* if (sig_idx >= node->num_ins) { */
  /*   printf("node has no input %d\n", sig_idx); */
  /*   return Val_unit; */
  /* } */
  /* Signal signal = IN(node, sig_idx); */
  /* set_signal(signal, Double_val(d)); */

  return Val_node(node);
}

CAMLprim value caml_load_soundfile(value ml_filename) {
  const char *filename = String_val(ml_filename);
  Signal *res = malloc(sizeof(Signal));
  int read = read_file(filename, res);
  return Val_signal(res);
}

CAMLprim value dump_soundfile_data(value ml_read_result_ptr) {
  Signal *res = Signal_val(ml_read_result_ptr);
  for (int i = 0; i < res->size; i++) {
    printf("%f\n", *(res->data + i));
  }
  return Val_unit;
}

CAMLprim value caml_apply(value vf, value vx) {
  caml_callback(vf, vx);
  return Val_unit;
}

void caml_wrapper(uint8_t chan, uint8_t cc, uint8_t val,
                  const char *registered_name) {
  write_log("caml wrapper %s %d %d %d\n", registered_name, chan, cc, val);
  /* value args[3] = {0, 0, 0}; */
  value closure = *caml_named_value("midi_func");
  write_log("found closure %d\n", closure);

  caml_callback(closure, Val_int(val));

  /* caml_callbackN((value)fn_ptr, 3, args); */
}

CAMLprim value caml_register_midi_handler(value ml_chan, value ml_ccnum,
                                          value ml_name) {
  int chan = Int_val(ml_chan);
  int ccnum = Int_val(ml_ccnum);
  int cchandler_offset = ccnum == -1 ? 0 : 1 + ccnum;
  const char *name = caml_stat_strdup(String_val(ml_name));
  printf("%s - registered name\n", name);

  /* HandlerWrapper wrapper = Handler; */

  /* chan * 129 + cchandler_offset]; */
  Handler[0].wrapper = &caml_wrapper;
  Handler[0].registered_name = name;

  /* printf("registered handler %p\n", wrapper.fn_ptr); */

  /* register_midi_handler(chan, ccnum, handler); */

  return Val_unit;
}

CAMLprim value caml_write_log(value ml_string) {
  const char *str = caml_stat_strdup(String_val(ml_string));
  write_log(str);
  return Val_unit;
}

msg_handler mock_msg_handler(void *ctx, Msg msg, int block_offset) {}

CAMLprim value caml_push_to_q(value ml_string) {
  const char *str = caml_stat_strdup(String_val(ml_string));
  double current = get_time();

  push_message(&ctx.queue, current, (msg_handler)mock_msg_handler, NULL, 0);
  return Val_unit;
}
