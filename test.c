#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
  struct Node *next;
  void (*perform)(double *out, int frame_count, double seconds_per_frame);
} Node;


static const double PI = 3.14159265358979323846264338328;
void perform_sq_detune(double *out, int frame_count, double seconds_per_frame) {
  double pitch = 220.0;
  double radians_per_second = pitch * 2.0 * PI;
}
void perform_tanh(double *out, int frame_count, double seconds_per_frame) {
}

Node *get_graph() {
  Node *tanh_node = malloc(sizeof(Node));
  tanh_node->perform = perform_tanh;
  Node *sq_node = malloc(sizeof(Node));
  sq_node->perform = perform_sq_detune;
  sq_node->next = tanh_node;
  return sq_node;
}


void perform_graph(Node *graph, double *out, int frame_count, double seconds_per_frame) {
  Node *node = graph;

  printf("perform graph\n");
  printf("node &: %ul\n", node);
  printf("node->perform &: %ul\n", node->perform);
  printf("node-next &: %ul\n", node->next);
  printf("------\n");
  node->perform(out, frame_count, seconds_per_frame);
  if (node->next) {
    Node *next = node->next;
    perform_graph(next, out, frame_count, seconds_per_frame);
  }
}

int main(int argc, char **argv) {
  int frame_count = 1;
  double seconds_per_frame = 0.01;
  double buffer[frame_count];
  double *out = &buffer[0];
  Node *graph = get_graph();
  perform_graph(graph, out, frame_count, seconds_per_frame);
  return 0;
}

