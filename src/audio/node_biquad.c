#include "node.h"
#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif

/* this holds the data required to update samples thru a filter */
typedef struct {
  double a0, a1, a2, a3, a4;
  double x1, x2, y1, y2;
} biquad_data;

typedef enum {
  BIQUAD_LPF,   /* low pass filter */
  BIQUAD_HPF,   /* High pass filter */
  BIQUAD_BPF,   /* band pass filter */
  BIQUAD_NOTCH, /* Notch Filter */
  BIQUAD_PEQ,   /* Peaking band EQ filter */
  BIQUAD_LSH,   /* Low shelf filter */
  BIQUAD_HSH    /* High shelf filter */
} biquad_type;

void perform_biquad_static(Node *node, double *out, int frame_count,
                           double seconds_per_frame, double seconds_offset) {
  biquad_data *b = (biquad_data *)node->data;
  for (int i = 0; i < frame_count; i++) {
    double sample = out[i];

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
    out[i] = result;
  };
}

Node *get_biquad_static(biquad_type type, double freq, double bandwidth,
                        double gain, int sample_rate, double *in) {

  biquad_data *b = malloc(sizeof(biquad_data));

  double A, omega, sn, cs, alpha, beta;
  double a0, a1, a2, b0, b1, b2;

  /* setup variables */
  A = pow(10, gain / 40);
  omega = 2 * M_PI * freq / sample_rate;
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
    free(b);
    return NULL;
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
  return alloc_node((NodeData *)b, in, (t_perform)perform_biquad_static,
                    "biquad", NULL);
}

typedef struct {
  double a0, a1, a2, a3, a4;
  double x1, x2, y1, y2;
} biquad_lp_data;

void set_biquad_params(biquad_data *b, biquad_type type, double freq,
                       double bandwidth, double gain, int sample_rate) {

  double A, omega, sn, cs, alpha, beta;
  double a0, a1, a2, b0, b1, b2;

  /* setup variables */
  A = pow(10, gain / 40);
  omega = 2 * M_PI * freq / sample_rate;
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
  }

  /* precompute the coefficients */
  b->a0 = b0 / a0;
  b->a1 = b1 / a0;
  b->a2 = b2 / a0;
  b->a3 = a1 / a0;
  b->a4 = a2 / a0;
}

void perform_biquad_lp(Node *node, int frame_count, double seconds_per_frame,
                       double seconds_offset) {
  double *out = node->out;
  double *in = node->in;
  biquad_lp_data *b = (biquad_lp_data *)node->data;
  for (int i = 0; i < frame_count; i++) {
    double sample = in[i];

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
    out[i] = result;
  };
}

void set_filter_params(Node *node, double freq, double bandwidth, double gain,
                       int sample_rate) {
  biquad_lp_data *data = (biquad_lp_data *)node->data;
  set_biquad_params(data, BIQUAD_LPF, freq, bandwidth, gain, sample_rate);
}

Node *get_biquad_lpf(double freq, double bandwidth, double gain,
                     int sample_rate, double *in) {

  biquad_lp_data *b = malloc(sizeof(biquad_lp_data));
  set_biquad_params(b, BIQUAD_LPF, freq, bandwidth, gain, sample_rate);

  /* zero initial samples */
  b->x1 = b->x2 = 0;
  b->y1 = b->y2 = 0;

  return alloc_node((NodeData *)b, in, (t_perform)perform_biquad_lp,
                    "biquad_lp", NULL);
}
