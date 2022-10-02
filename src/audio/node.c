#include "node.h"
#include <stdlib.h>

Node *alloc_node(NodeData *data, t_perform perform, char *name) {
  Node *node = malloc(sizeof(Node) + sizeof(data));
  node->name = name ? (name) : "";
  node->perform = perform;
  node->next = NULL;
  node->data = data;
  return node;
}

void debug_node(Node *node, char *text) {
  if (text)
    printf("%s\n", text);
  printf("node name: %s\n", node->name);
  printf("node &: %#08x\n", node);
  printf("node perform: %#08x\n", node->perform);
  printf("node next: %#08x\n", node->next);
  printf("-------\n");
}

void perform_lp(Node *node, double *out, int frame_count,
                double seconds_per_frae, double seconds_offset) {

  double input;
  lp_data *data = (lp_data *)node->data;
  double cutoff = data->cutoff >= 22100 ? (22100) : cutoff;
  double resonance = data->resonance;

  double sigout;

  for (int i = 0; i < frame_count; i++) {
    input = out[i];

    double pi = PI;
    double v2 = 40000; // twice the 'thermal voltage of a transistor'
    double sr = 22100;

    double cutoff_hz = cutoff;

    double x; // temp var: input for taylor approximations
    double xabs;
    double exp_out;
    double tanh1_out, tanh2_out;
    double kfc;
    double kf;
    double kfcr;
    double kacr;
    double k2vg;

    kfc = cutoff_hz / sr; // sr is half the actual filter sampling rate
    kf = cutoff_hz / (sr * 2);
    // frequency & amplitude correction
    kfcr = 1.8730 * (kfc * kfc * kfc) + 0.4955 * (kfc * kfc) - 0.6490 * kfc +
           0.9988;
    kacr = -3.9364 * (kfc * kfc) + 1.8409 * kfc + 0.9968;

    x = -2.0 * pi * kfcr * kf;
    exp_out = exp(x);

    k2vg = v2 * (1 - exp_out); // filter tuning

    // cascade of 4 1st order sections
    double x1 = (input - 4 * resonance * data->amf * kacr) / v2;
    double tanh1 = tanh(x1);
    double x2 = data->az1 / v2;
    double tanh2 = tanh(x2);
    data->ay1 = data->az1 + k2vg * (tanh1 - tanh2);

    // data->ay1  = data->az1 + k2vg * ( tanh( (input -
    // 4*resonance*data->amf*kacr) / v2) - tanh(data->az1/v2) );
    data->az1 = data->ay1;

    data->ay2 =
        data->az2 + k2vg * (tanh(data->ay1 / v2) - tanh(data->az2 / v2));
    data->az2 = data->ay2;

    data->ay3 =
        data->az3 + k2vg * (tanh(data->ay2 / v2) - tanh(data->az3 / v2));
    data->az3 = data->ay3;

    data->ay4 =
        data->az4 + k2vg * (tanh(data->ay3 / v2) - tanh(data->az4 / v2));
    data->az4 = data->ay4;

    // 1/2-sample deldata->ay for phase compensation
    data->amf = (data->ay4 + data->az5) * 0.5;
    data->az5 = data->ay4;

    // oversampling (repeat same block)
    data->ay1 = data->az1 +
                k2vg * (tanh((input - 4 * resonance * data->amf * kacr) / v2) -
                        tanh(data->az1 / v2));
    data->az1 = data->ay1;

    data->ay2 =
        data->az2 + k2vg * (tanh(data->ay1 / v2) - tanh(data->az2 / v2));
    data->az2 = data->ay2;

    data->ay3 =
        data->az3 + k2vg * (tanh(data->ay2 / v2) - tanh(data->az3 / v2));
    data->az3 = data->ay3;

    data->ay4 =
        data->az4 + k2vg * (tanh(data->ay3 / v2) - tanh(data->az4 / v2));
    data->az4 = data->ay4;

    // 1/2-sample deldata->ay for phase compensation
    data->amf = (data->ay4 + data->az5) * 0.5;
    data->az5 = data->ay4;

    sigout = data->amf;

    // end of sm code

    out[i] = sigout;
  }
}

Node *get_lp_node(lp_data *data) {
  return alloc_node((NodeData *)data, (t_perform)perform_lp, "lp");
}
