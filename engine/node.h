#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H
typedef struct Node Node;

typedef struct {
  double *buf;
  int size;
  int layout;
} Signal;

typedef struct Node (*node_perform)(struct Node *node, int nframes, double spf);

typedef struct Node {
  enum { INTERMEDIATE = 0, OUTPUT } type;
  void *state;
  node_perform perform;
  Signal out;

  int num_ins;
  Signal *ins;

  struct Node *next;
  int frame_offset;
} Node;

Node *perform_graph(Node *head, int frame_count, double spf, double *dest,
                    int dest_layout, int output_num);

typedef struct {
  Node *head;
  Node *tail;
} group_state;

extern Node *_chain_head;
extern Node *_chain_tail;
extern Node *_chain;
void reset_chain();

node_perform group_perform(Node *group, int nframes, double spf);

Node *group_add_tail(Node *group, Node *node);

Node *node_new(void *data, node_perform *perform, int num_ins, Signal *ins);

Node *node_new_w_alloc(void *data, node_perform *perform, int num_ins,
                       Signal *ins, void **mem);
Signal *get_sig(int layout);
Signal *get_sig_default(int layout, double value);

Node *group_new(int chans);

node_perform sum_perform(Node *node, int nframes, double spf);
node_perform mul_perform(Node *node, int nframes, double spf);

Node *sum2_node(Node *a, Node *b);
Node *sub2_node(Node *a, Node *b);
Node *mul2_node(Node *a, Node *b);
Node *div2_node(Node *a, Node *b);
Node *mod2_node(Node *a, Node *b);

Signal *sum2_sigs(Signal *a, Signal *b);
Signal *sub2_sigs(Signal *a, Signal *b);
Signal *mul2_sigs(Signal *a, Signal *b);
Signal *div2_sigs(Signal *a, Signal *b);
Signal *mod2_sigs(Signal *a, Signal *b);

Node *node_of_double(double val);

Signal *out_sig(Node *n);

Signal *input_sig(int i, Node *n);

int num_inputs(Node *n);

Signal *signal_of_double(double val);
Signal *sig_of_array(int num, double *val);
Signal *signal_of_int(int val);

Signal *inlet(double default_val);

Node *node_of_sig(Signal *val);
#endif
