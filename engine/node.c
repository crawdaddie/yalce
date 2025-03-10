#include "./node.h"

void offset_node_bufs(Node *node, int frame_offset) {

  if (frame_offset == 0) {
    return;
  }

  node->output.data += (frame_offset * node->output.layout);
}

void unoffset_node_bufs(Node *node, int frame_offset) {
  if (frame_offset == 0) {
    return;
  }

  node->output.data -= (frame_offset * node->output.layout);
  node->frame_offset = 0;
}
