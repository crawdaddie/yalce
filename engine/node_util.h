#ifndef _ENGINE_NODE_UTIL_H
#define _ENGINE_NODE_UTIL_H
#include "./node.h"

#define INVAL(_sig)                                                            \
  ({                                                                           \
    double *val;                                                               \
    if (_sig.size == 1 && _sig.layout == 1) {                                  \
      val = _sig.buf;                                                          \
    } else {                                                                   \
      val = _sig.buf;                                                          \
      _sig.buf += _sig.layout;                                                 \
    }                                                                          \
    val;                                                                       \
  })
NodeRef sum2_node(NodeRef input1, NodeRef input2);
NodeRef mul2_node(NodeRef input1, NodeRef input2);
NodeRef sub2_node(NodeRef input1, NodeRef input2);
NodeRef mod2_node(NodeRef input1, NodeRef input2);

NodeRef const_sig(double val);

NodeRef const_buf(double val, int layout, int size);
#endif
