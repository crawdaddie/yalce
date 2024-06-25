#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_to_output_(double *src, int src_layout, double *dest,
                      int dest_layout, int nframes, int output_num) {
  if (dest_layout == 1 && src_layout == dest_layout) {
    for (int f = 0; f < nframes; f++) {
      if (output_num > 0) {
        *dest += *(src + f);
      } else {
        *dest = *(src + f);
      }
      dest++;
    }
  } else if (dest_layout == 2 && src_layout == dest_layout) {
    for (int f = 0; f < nframes; f++) {
      if (output_num > 0) {
        dest[2 * f] += *(src + 2 * f);
      } else {
        dest[2 * f] = *(src + 2 * f);
      }

      if (output_num > 0) {
        dest[2 * f + 1] += *(src + (2 * f) + 1);
      } else {
        dest[2 * f + 1] = *(src + (2 * f) + 1);
      }
    }
  } else if (dest_layout == 2 && src_layout == 1) {
    for (int f = 0; f < nframes; f++) {
      double *samp = (src + f);
      if (output_num > 0) {
        dest[2 * f] += *samp;
      } else {
        dest[2 * f] = *samp;
      }

      if (output_num > 0) {
        dest[2 * f + 1] += *samp;
      } else {
        dest[2 * f + 1] = *samp;
      }
    }
  }
}
void write_to_output(double *src, int src_layout, double *dest, int dest_layout,
                     int nframes, int output_num) {
  int i;
  double *src_end = src + nframes * src_layout;

  if (output_num > 0) {
    if (src_layout == dest_layout) {
      if (dest_layout == 1) {
        // both mono
        for (; src < src_end; src++, dest++) {
          *dest += *src;
        }
      } else if (dest_layout == 2) {
        // both stereo
        for (; src < src_end; src += 2, dest += 2) {
          dest[0] += src[0];
          dest[1] += src[1];
        }
      }
    } else if (dest_layout == 2 && src_layout == 1) {
      for (; src < src_end; src++, dest += 2) {
        dest[0] += *src;
        dest[1] += *src;
      }
    }
  } else {
    if (src_layout == dest_layout) {
      memcpy(dest, src, nframes * dest_layout * sizeof(double));
    } else if (dest_layout == 2 && src_layout == 1) {
      for (i = 0; i < nframes; i++) {
        dest[2 * i] = dest[2 * i + 1] = src[i];
      }
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

  node->out.buf += frame_offset;
  for (int i = 0; i < node->num_ins; i++) {
    node->ins[i].buf += frame_offset;
  }
}

static void unoffset_node_bufs(Node *node, int frame_offset) {
  if (frame_offset == 0) {
    return;
  }

  node->out.buf -= frame_offset;
  for (int i = 0; i < node->num_ins; i++) {
    node->ins[i].buf -= frame_offset;
  }
  node->frame_offset = 0;
}

Node *perform_graph(Node *head, int nframes, double spf, double *dest_buf,
                    int dest_layout, int output_num) {

  if (!head) {
    return NULL;
  }

  int frame_offset = head->frame_offset;
  // int frame_offset = 0;

  offset_node_bufs(head, frame_offset);
  head->perform(head, nframes, spf);

  if (head->type == OUTPUT) {
    write_to_output(head->out.buf, head->out.layout, dest_buf + frame_offset,
                    dest_layout, nframes - frame_offset, output_num);
    output_num++;
  }
  unoffset_node_bufs(head, frame_offset);

  if (head->next != NULL) {
    return perform_graph(head->next, nframes, spf, dest_buf, dest_layout,
                         output_num);
  };
  return head;
}

node_perform group_perform(Node *group_node, int nframes, double spf) {
  group_state *state = group_node->state;
  perform_graph(state->head, nframes, spf, group_node->out.buf,
                group_node->out.layout, 0);
}
