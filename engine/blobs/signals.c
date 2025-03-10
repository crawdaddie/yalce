#include "./signals.h"
#include "./common.h"
#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>

void out_sig(NodeRef node, Signal *sig) {
  sig->buf = (char *)node + node->output_buf_offset;
  sig->layout = node->output_layout;
  sig->size = node->output_size;
}

SignalRef inlet(double default_val) {

  SignalRef sig = engine_alloc(sizeof(Signal) * BUF_SIZE);

  if (_current_blob != NULL) {
    BlobTemplate *tpl = _current_blob;

    int idx = tpl->num_inputs;
    if (idx == MAX_INPUTS) {
      fprintf(stderr, "Too many inputs for node\n");
      return NULL;
    }
    tpl->default_vals[idx] = default_val;
    tpl->input_slot_offsets[idx] = (char *)sig - (char *)tpl->blob_data;
    tpl->num_inputs++;
  }
  sig->layout = 1;
  sig->size = BUF_SIZE;

  return sig;
}

double *get_node_input_buf(NodeRef node, int input) {
  Signal *sig = (Signal *)((char *)node + (node->input_offsets[input]));
  return sig->buf;
}

int get_node_input_size(NodeRef node, int input) {
  return node->input_sizes[input];
}

int get_node_input_layout(NodeRef node, int input) {
  return node->input_layouts[input];
}

double *get_node_out_buf(NodeRef node) {
  return (double *)((char *)node + (node->output_buf_offset));
}

int get_node_out_layout(NodeRef node) { return node->output_layout; }

double *get_val(SignalRef in) {
  if (in->size == 1) {
    return in->buf;
  }
  double *ptr = in->buf;
  in->buf += in->layout;
  return ptr;
}

SignalRef get_sig_default(int layout, double value) {
  Signal *sig = engine_alloc(sizeof(Signal));
  *sig = (Signal){.layout = layout,
                  .size = BUF_SIZE,
                  .buf = malloc(sizeof(double) * BUF_SIZE * layout)};
  for (int i = 0; i < BUF_SIZE * layout; i++) {
    sig->buf[i] = value;
  }
  return sig;
}

SignalRef signal_of_double(double val) {
  Signal *signal = get_sig_default(1, val);

  printf("const signal %p of double %f\n", signal, val);
  return signal;
}
