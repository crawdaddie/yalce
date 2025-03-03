#include "./signals.h"
#include "./common.h"
#include <stdio.h>
#include <stdlib.h>
SignalRef signal_new() {
  if (!_current_blob) {
    SignalRef sig = malloc(sizeof(Signal));

    // if (_chain == NULL) {
    //   _chain = group_new(0);
    // } else {
    //   group_add_tail(_chain, node);
    // }

    return sig;
  }

  SignalRef sig = (SignalRef)(_current_blob->_mem_ptr);
  _current_blob->_mem_ptr = (char *)_current_blob->_mem_ptr + sizeof(Signal);

  return sig;
}

SignalRef inlet(double default_val) {
  Signal *sig = signal_new();
  if (_current_blob != NULL) {
    BlobTemplate *tpl = _current_blob;

    int idx = tpl->num_inputs;
    tpl->default_vals[idx] = default_val;
    tpl->input_slot_offsets[idx] = (char *)sig - (char *)tpl->blob_data;
    tpl->num_inputs++;
  }

  sig->layout = 1;
  sig->size = 1;
  return sig;
}

Signal *out_sig(Node *n) { return &n->out; }

double *get_val(SignalRef in) {
  if (in->size == 1) {
    return in->buf;
  }
  double *ptr = in->buf;
  in->buf += in->layout;
  return ptr;
}

SignalRef get_sig_default(int layout, double value) {
  Signal *sig = signal_new();
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
