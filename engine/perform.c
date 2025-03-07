#include "./perform.h"
#include "./node.h"
#include "./signals.h"
#include "common.h"
#include <stdbool.h>
#include <stdio.h>
void write_to_dac(int dac_layout, double *dac_buf, int layout, double *buf,
                  int output_num, int nframes) {
  if (output_num > 0) {

    while (nframes--) {
      for (int i = 0; i < dac_layout; i++) {
        *(dac_buf + i) += *(buf + (i < layout ? i : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  } else {
    while (nframes--) {
      for (int i = 0; i < dac_layout; i++) {
        *(dac_buf + i) = *(buf + (i < layout ? i : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  }
}

static double _BUFS[BUF_SIZE * (1 << 10)] = {0.};
static double *_buf_ptr = _BUFS;

void reset_buf_ptr() { _buf_ptr = _BUFS; }

void set_out_buf(NodeRef node) {
  node->out.buf = _buf_ptr;
  int size = node->out.size;
  int layout = node->out.layout;
  _buf_ptr += (size * layout);
}

bool should_write_to_dac(Node *node) { return node->write_to_dac; }
void offset_node_bufs(Node *node, int frame_offset) {

  if (frame_offset == 0) {
    return;
  }

  if (node->num_ins == 0) {
    return;
  }

  node->out.buf += frame_offset;
  // for (int i = 0; i < node->num_ins; i++) {
  //   get_node_input(node, i)->buf += frame_offset;
  // }
}

void unoffset_node_bufs(Node *node, int frame_offset) {
  if (frame_offset == 0) {
    return;
  }

  node->out.buf -= frame_offset;
  // for (int i = 0; i < node->num_ins; i++) {
  //   get_node_input(node, i)->buf -= frame_offset;
  // }
  node->frame_offset = 0;
}

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num) {
  if (!head) {
    // printf("Error: NULL head\n");
    return;
  }

  int frame_offset = head->frame_offset;

  set_out_buf(head);
  offset_node_bufs(head, frame_offset);

  if (!head->can_free) {
    void *blob = head->node_perform(head, frame_count, spf);
  }

  if (should_write_to_dac(head)) {
    write_to_dac(layout, dac_buf, head->out.layout, head->out.buf, output_num,
                 frame_count);
  }
  unoffset_node_bufs(head, frame_offset);

  if (head->next) {
    perform_graph(head->next, frame_count, spf, dac_buf, layout,
                  output_num + 1);
  }
}
