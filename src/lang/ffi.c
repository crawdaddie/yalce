#include "ffi.h"
#include "../audio/filter.h"
#include "../audio/osc.h"
#include "../ctx.h"
#include "../log.h"
#include "../scheduling.h"

Node *play_node(Node *node) {
  printf("%p play node\n", node);
  double timestamp = get_time();
  int block_offset = (int)(timestamp - ctx.block_time) * 48000;
  node->_block_offset = block_offset;

  Node *container = container_node(node);
  ctx_add_after_tail(container);
  return container;
}

Node *sq_detune(double freq) {
  Node *s = sq_detune_node(freq);
  return s;
}

Node *impulse(double freq) { return impulse_node(freq); }

Node *lpf(double freq, double bw) { return biquad_lpf_node(freq, bw, NULL); }

void node_set(Node *n, int index, double value) {
  printf("node set %p %d %f\n", n, index, value);

  Signal signal = n->ins[index];
  set_signal(signal, value);
}

void print_node(Node *n) {
  printf("node: %p\nnum_ins: %d", n, (int)n->num_ins);
}

Node *chain(int sig_idx, Node *dest, Node *src) {
  printf("%p->%p\n", src, dest);
  printf("??\n");
  node_add_after(src, dest);
  dest->ins[sig_idx].data = src->out.data;
  dest->ins[sig_idx].size = src->out.size;
  dest->ins[sig_idx].layout = src->out.layout;
  return src;
}

// CAMLprim value caml_load_soundfile(value ml_filename) {
//   const char *filename = String_val(ml_filename);
//   Signal *res = malloc(sizeof(Signal));
//   int read = read_file(filename, res);
//   return Val_signal(res);
// }
//
// CAMLprim value dump_soundfile_data(value ml_read_result_ptr) {
//   Signal *res = Signal_val(ml_read_result_ptr);
//   for (int i = 0; i < res->size; i++) {
//     printf("%f\n", *(res->data + i));
//   }
//   return Val_unit;
// }
