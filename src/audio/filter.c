#include "filter.h"
#include "common.h"
#include "signal.h"
#include <math.h>
static int SR = 48000;
static void
set_filter_coefficients(biquad_data *b, const biquad_filter_type type,
                        double dbGain,         /* gain of filter */
                        const double freq,     /* center frequency */
                        const double bandwidth /* bandwidth in octaves */
) {
  double A, omega, sn, cs, alpha, beta;
  double a0, a1, a2, b0, b1, b2;
  A = pow(10, dbGain / 40);
  omega = 2 * M_PI * freq / SR;
  sn = sin(omega);
  cs = cos(omega);
  alpha = sn * sinh(M_LN2 / 2 * bandwidth * omega / sn);
  beta = sqrt(A + A);

  switch (type) {
  case BIQUAD_LPF:
    b0 = (1 - cs) / 2;
    b1 = 1 - cs;
    b2 = (1 - cs) / 2;
    a0 = 1 + alpha;
    a1 = -2 * cs;
    a2 = 1 - alpha;
    break;
  case BIQUAD_HPF:
    b0 = (1 + cs) / 2;
    b1 = -(1 + cs);
    b2 = (1 + cs) / 2;
    a0 = 1 + alpha;
    a1 = -2 * cs;
    a2 = 1 - alpha;
    break;
  case BIQUAD_BPF:
    b0 = alpha;
    b1 = 0;
    b2 = -alpha;
    a0 = 1 + alpha;
    a1 = -2 * cs;
    a2 = 1 - alpha;
    break;
  case BIQUAD_NOTCH:
    b0 = 1;
    b1 = -2 * cs;
    b2 = 1;
    a0 = 1 + alpha;
    a1 = -2 * cs;
    a2 = 1 - alpha;
    break;
  case BIQUAD_PEQ:
    b0 = 1 + (alpha * A);
    b1 = -2 * cs;
    b2 = 1 - (alpha * A);
    a0 = 1 + (alpha / A);
    a1 = -2 * cs;
    a2 = 1 - (alpha / A);
    break;
  case BIQUAD_LSH:
    b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
    b1 = 2 * A * ((A - 1) - (A + 1) * cs);
    b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
    a0 = (A + 1) + (A - 1) * cs + beta * sn;
    a1 = -2 * ((A - 1) + (A + 1) * cs);
    a2 = (A + 1) + (A - 1) * cs - beta * sn;
    break;
  case BIQUAD_HSH:
    b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
    b1 = -2 * A * ((A - 1) + (A + 1) * cs);
    b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
    a0 = (A + 1) - (A - 1) * cs + beta * sn;
    a1 = 2 * ((A - 1) - (A + 1) * cs);
    a2 = (A + 1) - (A - 1) * cs - beta * sn;
    break;
  default:
    return;
  }

  /* precompute the coefficients */
  b->a0 = b0 / a0;
  b->a1 = b1 / a0;
  b->a2 = b2 / a0;
  b->a3 = a1 / a0;
  b->a4 = a2 / a0;

  /* zero initial samples */
  b->x1 = b->x2 = 0;
  b->y1 = b->y2 = 0;
}

static double biquad_sample(const double sample, biquad_data *const b) {
  double result;

  /* compute result */
  result = b->a0 * sample + b->a1 * b->x1 + b->a2 * b->x2 - b->a3 * b->y1 -
           b->a4 * b->y2;

  /* shift x1 to x2, sample to x1 */
  b->x2 = b->x1;
  b->x1 = sample;

  /* shift y1 to y2, result to y1 */
  b->y2 = b->y1;
  b->y1 = result;

  return result;
}

static node_perform biquad_perform(Node *node, int nframes, double spf) {
  biquad_data *data = NODE_DATA(biquad_data, node);
  double sample;
  for (int f = get_block_offset(node); f < nframes; f++) {
    sample = unwrap(IN(node, 0), f);
    node_write_out(node, f, biquad_sample(sample, data));
  }
}

Node *biquad_node(const biquad_filter_type type,
                  double dbGain,          /* gain of filter */
                  const double freq,      /* center frequency */
                  const double bandwidth, /* bandwidth in octaves */
                  Signal *ins) {

  Node *node = ALLOC_NODE(biquad_data, "Biquad", 1);
  biquad_data *data = node->data;
  set_filter_coefficients(data, type, dbGain, freq, bandwidth);
  node->perform = (node_perform)biquad_perform;

  INS(node) = ins == NULL ? ALLOC_SIGS(BIQUAD_SIG_OUT) : ins;

  init_signal(INS(node), BUF_SIZE, 0.0);
  NUM_INS(node) = 1;
  init_out_signal(&node->out, BUF_SIZE, 1);

  return node;
}

Node *biquad_lpf_node(                        /* gain of filter */
                      const double freq,      /* center frequency */
                      const double bandwidth, /* bandwidth in octaves */
                      Signal *ins) {

  Node *node = ALLOC_NODE(biquad_data, "BiquadLPF", 1);
  biquad_data *data = node->data;
  set_filter_coefficients(data, BIQUAD_LPF, 0.0, freq, bandwidth);
  node->perform = (node_perform)biquad_perform;

  INS(node) = ins == NULL ? ALLOC_SIGS(BIQUAD_SIG_OUT) : ins;

  init_signal(INS(node), BUF_SIZE, 0.0);
  NUM_INS(node) = 1;
  init_out_signal(&node->out, BUF_SIZE, 1);

  return node;
}
