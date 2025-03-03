#include "./signals.h"
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
