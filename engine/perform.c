#include "./perform.h"
#include "./node.h"
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

bool should_write_to_dac(Node *node) { return node->write_to_dac; }

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num) {
  if (!head) {
    printf("Error: NULL head\n");
    return;
  }

  void *blob = head->node_perform(head, frame_count, spf);

  if (should_write_to_dac(head)) {
    write_to_dac(layout, dac_buf, head->out.layout, head->out.buf, output_num,
                 frame_count);
  }
  if (head->next) {
    perform_graph(head->next, frame_count, spf, dac_buf, layout,
                  output_num + 1);
  }
}
