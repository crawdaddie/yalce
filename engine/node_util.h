#ifndef _ENGINE_NODE_UTIL_H
#define _ENGINE_NODE_UTIL_H
#include "./node.h"

#define INVAL(_sig)                                                            \
  ({                                                                           \
    sample_t *val;                                                             \
    if (_sig.size == 1 && _sig.layout == 1) {                                  \
      val = _sig.buf;                                                          \
    } else {                                                                   \
      val = _sig.buf;                                                          \
      _sig.buf += _sig.layout;                                                 \
    }                                                                          \
    val;                                                                       \
  })

#define READ(_sig)                                                             \
  ({                                                                           \
    sample_t *val;                                                             \
    if (_sig.size == 1 && _sig.layout == 1) {                                  \
      val = _sig.buf;                                                          \
    } else {                                                                   \
      val = _sig.buf;                                                          \
      _sig.buf += _sig.layout;                                                 \
    }                                                                          \
    val;                                                                       \
  })

#define WRITE(_sig, v)                                                         \
  ({                                                                           \
    if (_sig.size == 1 && _sig.layout == 1) {                                  \
      *(_sig.buf) = *v;                                                        \
    } else {                                                                   \
      for (int i = 0; i < _sig.layout; i++) {                                  \
        *(_sig.buf + i) = *(v + i);                                            \
      }                                                                        \
      _sig.buf += _sig.layout;                                                 \
    }                                                                          \
  })

#define WRITEV(_sig, v)                                                        \
  ({                                                                           \
    if (_sig.size == 1 && _sig.layout == 1) {                                  \
      *(_sig.buf) = v;                                                         \
    } else {                                                                   \
      *(_sig.buf) = v;                                                         \
      _sig.buf = _sig.buf + 1;                                                 \
    }                                                                          \
  })

NodeRef sum2_node(NodeRef input1, NodeRef input2);
NodeRef mul2_node(NodeRef input1, NodeRef input2);
NodeRef sub2_node(NodeRef input1, NodeRef input2);
NodeRef mod2_node(NodeRef input1, NodeRef input2);
NodeRef div2_node(NodeRef input1, NodeRef input2);

NodeRef const_sig(double val);

NodeRef const_buf(double val, int layout, int size);

NodeRef stereo_node(NodeRef input);

NodeRef pan_node(NodeRef pan, NodeRef input);
NodeRef empty_synth();

NodeRef sah_node(NodeRef trig, NodeRef input);
// NodeRef set_math(void *math_fn, NodeRef n);
#endif
