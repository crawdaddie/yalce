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

typedef Signal *SignalRef;
typedef Node *Synth;
typedef Node *NodeRef;

NodeRef perform_graph(NodeRef head, int frame_count, double spf, double *dest,
                      int dest_layout, int output_num);

typedef struct {
  NodeRef head;
  NodeRef tail;
} group_state;

extern NodeRef _chain_head;
extern NodeRef _chain_tail;
extern NodeRef _chain;

node_perform group_perform(NodeRef group, int nframes, double spf);

NodeRef group_add_tail(NodeRef group, NodeRef node);

NodeRef node_new(void *data, node_perform *perform, int num_ins, SignalRef ins);

NodeRef node_new_w_alloc(void *data, node_perform *perform, int num_ins,
                         SignalRef ins, void **mem);
SignalRef get_sig(int layout);
SignalRef get_sig_default(int layout, double value);

NodeRef group_new(int chans);

node_perform sum_perform(NodeRef node, int nframes, double spf);
node_perform mul_perform(NodeRef node, int nframes, double spf);

NodeRef sum2_node(NodeRef a, NodeRef b);
NodeRef sub2_node(NodeRef a, NodeRef b);
NodeRef mul2_node(NodeRef a, NodeRef b);
NodeRef div2_node(NodeRef a, NodeRef b);
NodeRef mod2_node(NodeRef a, NodeRef b);

SignalRef sum2_sigs(SignalRef a, SignalRef b);
SignalRef sub2_sigs(SignalRef a, SignalRef b);
SignalRef mul2_sigs(SignalRef a, SignalRef b);
SignalRef div2_sigs(SignalRef a, SignalRef b);
SignalRef mod2_sigs(SignalRef a, SignalRef b);

NodeRef node_of_double(double val);

SignalRef out_sig(NodeRef n);

SignalRef input_sig(int i, NodeRef n);

int num_inputs(NodeRef n);

SignalRef signal_of_double(double val);
SignalRef sig_of_array(int num, double *val);
SignalRef signal_of_int(int val);

SignalRef inlet(double default_val);

NodeRef node_of_sig(SignalRef val);

Signal *group_add_input(Node *group);

#define MAX_INPUTS 16
// Definition for a blob template
typedef struct {
  int total_size;  // Total size of all nodes in the blob
  int num_inputs;  // Number of inputs the blob expects
  void *blob_data; // The actual compiled blob data
  int first_node_offset;
  int last_node_offset;
  int input_slot_offsets[MAX_INPUTS]; // Offsets for where inputs should connect
  double default_vals[MAX_INPUTS];    // Offsets for where inputs should connect
} BlobTemplate;

void *create_new_blob_template();
#endif
