#include "delay.h"
static node_perform delay_perform(Node *node, int nframes, double spf) {}

Node *simple_delay_node(double delay_time_s, double delay_fb, int bufsize_s,
                        Signal *ins) {
  Node *delay = ALLOC_NODE(delay_data, "Delay");
}
