#ifndef _NODE
#define _NODE
struct Node {
  double (*get_sample)(double in, int frame, double seconds_per_frame,
                       double seconds_offset, double *args);
  double *args;
  struct Node *next;
};

double get_sin_sample(double in, int frame, double seconds_per_frame,
                      double seconds_offset, double *args);

double get_sin_sample_detune(double in, int frame, double seconds_per_frame,
                             double seconds_offset, double *args);

struct Node get_sin_node(double freq);
struct Node get_sin_node_detune(double *args);

double process_tanh(double in, int frame, double seconds_per_frame,
                    double seconds_offset, double *args);

struct Node get_tanh_node(double gain);
void write_node_frame(struct Node *node, double *out, int frame_count,
                      double seconds_per_frame, double seconds_offset);
#endif
