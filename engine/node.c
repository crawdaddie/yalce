#include "node.h"
#include <stdio.h>
#include <stdlib.h>

void write_to_output(double *src, double *dest, int nframes, int output_num) {
  for (int f = 0; f < nframes; f++) {
    for (int ch = 0; ch < 2; ch++) {
      // printf("data %f\n", *(src + f));
      if (output_num > 0) {
        *dest += *(src + f);
      } else {
        *dest = *(src + f);
      }
      dest++;
    }
  }
}
static void offset_node_bufs(Node *node, int frame_offset) {

  if (frame_offset == 0) {
    return;
  }

  if (node->ins == NULL) {
    return;
  }

  for (int i = 0; i < node->num_ins; i++) {
    node->ins[i] += frame_offset;
  }
}

static void unoffset_node_bufs(Node *node, int frame_offset) {
  if (frame_offset == 0) {
    return;
  }
  for (int i = 0; i < node->num_ins; i++) {
    node->ins[i] -= frame_offset;
  }
  node->frame_offset = 0;
}

Node *perform_graph(Node *head, int nframes, double spf, double *dac_buf,
                    int output_num) {

  if (!head) {
    return NULL;
  }

  int frame_offset = head->frame_offset;
  // int frame_offset = 0;

  offset_node_bufs(head, frame_offset);
  head->perform(head, nframes, spf);

  if (head->type == OUTPUT) {
    write_to_output(head->output_buf + frame_offset, dac_buf + frame_offset,
                    nframes - frame_offset, output_num);
    output_num++;
  }
  unoffset_node_bufs(head, frame_offset);

  if (head->next != NULL) {
    return perform_graph(head->next, nframes, spf, dac_buf, output_num);
  };
  return head;
}

node_perform group_perform(Node *group_node, int nframes, double spf) {

  group_state *state = group_node->state;
  printf("group %p graph %p graph head  p\n", group_node, state);
  perform_graph(state->head, nframes, spf, group_node->output_buf, 0);
}
