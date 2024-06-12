#include "node.h"
#include <stdio.h>
#include <stdlib.h>

void write_to_output(double *src, double *dest, int nframes, int output_num) {
  for (int f = 0; f < nframes; f++) {
    for (int ch = 0; ch < 2; ch++) {
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
  // printf("frame offset %d\n", frame_offset);
  // int frame_offset = 0;
  // printf("head %p perf: %p\n", head, head->perform);

  if (head->perform) {
    // printf("head perform?\n");
    offset_node_bufs(head, frame_offset);

    head->perform(head->state, head->output_buf + frame_offset, head->num_ins,
                  head->ins, nframes - frame_offset, spf);

    unoffset_node_bufs(head, frame_offset);
  }

  if (head->type == OUTPUT) {
    write_to_output(head->output_buf, dac_buf + frame_offset,
                    nframes - frame_offset, output_num);
    output_num++;
  }

  Node *next = head->next;
  if (next) {
    // printf("next??");
    // keep going until you return tail
    return perform_graph(next, nframes, spf, dac_buf, output_num);
  };
  return head;
}
void group_perform(void *state, double *output, int num_ins, double **inputs,
                   int nframes, double spf) {

  group_state *group = state;

  Node *head = group->head;

  perform_graph(head, nframes, spf, output, 0);
}
