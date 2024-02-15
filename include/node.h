#ifndef _NODE_H
#define _NODE_H
#include <stdbool.h>

typedef struct Node (*node_perform)(struct Node *node, int nframes, double spf);
typedef struct Signal {
  // size of the data array will be size * layout
  // data is interleaved, so sample for frame x, channel 0 will be at index
  // layout * x + 0
  // sample for frame x, channel 1 will be at index layout * x + 1
  double *data;
  int size;   // number of frames
  int layout; // how they are laid out
} Signal;

typedef struct Node {
  enum {
    INTERMEDIATE = 0,
    OUTPUT,
  } type;

  node_perform perform;
  Signal ins;
  Signal out;
  struct Node *next;
  bool killed;
  void *data;
} Node;

static inline double *get_sig_ptr(Signal sig, int frame, int chan) {
  // return sig.data + frame + (chan * sig.size); // non-interleaved
  return sig.data + (frame * sig.layout) + chan; // interleaved samples
}

double random_double();

double random_double_range(double min, double max);

node_perform noise_perform(Node *node, int nframes, double spf);


#define SIN_TABSIZE (1 << 11)
void maketable_sin(void);

typedef struct {
  double phase;
} sin_data;

node_perform sine_perform(Node *node, int nframes, double spf);
static inline double scale_val_2(double env_val, // 0-1
                                 double min, double max) {
  return min + env_val * (max - min);
}
double sq_sample(double phase, double freq);

typedef struct {
  double phase;
} sq_data;

node_perform sq_perform(Node *node, int nframes, double spf);


typedef struct {
  double phase;
  double target;
  double min;
  double max;
} lf_noise_data;

node_perform lf_noise_perform(Node *node, int nframes, double spf);

typedef struct {
  double phase;
  double target;
  double current;
} lf_noise_interp_data;
node_perform lf_noise_interp_perform(Node *node, int nframes,
                                            double spf);

void init_sig_ptrs();

Signal get_sig(int layout);
Node *node_new(void *data, node_perform *perform, Signal ins,
                      Signal out);
#endif
