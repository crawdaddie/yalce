#include "osc.h"
#include "math.h"
#include <math.h>
#include <stdlib.h>

#define SIN_TABSIZE (1 << 10)

// ----------------------------- SINE WAVE OSCILLATORS
static double sin_table[SIN_TABSIZE];

static void maketable_sin(void) {
  double phase, phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {

    sin_table[i] = sin(phase);
    phase += phsinc;
  }
}

static node_perform sin_perform(Node *node, int nframes, double spf) {
  sin_data *data = NODE_DATA(sin_data, node);
  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double freq = unwrap(*data->freq, 0);

  for (int f = 0; f < nframes; f++) {
    d_index = data->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);

    data->phase = fmod(freq * spf + data->phase, 1.0);
    data->out->data[f] = sample;
  }
}

Node *sin_node(double freq) {
  Node *sin = ALLOC_NODE(sin_data, "sin");
  sin->perform = sin_perform;
  sin_data *data = sin->data;
  data->freq = new_signal_heap_default(1, 1, freq);
  data->phase = 0.0;
  data->out = new_signal_heap(BUF_SIZE, 1);
  return sin;
}

double sine_sample(double phase /* 0-1 */) {
  double d_index;
  int index;

  double frac, a, b, sample;

  d_index = phase * SIN_TABSIZE;
  index = (int)d_index;
  frac = d_index - index;
  a = sin_table[index % SIN_TABSIZE];

  b = sin_table[(index + 1) % SIN_TABSIZE];

  double s = (1.0 - frac) * a + (frac * b);
  return s;
}

static node_perform pmsin_perform(Node *node, int nframes, double spf) {
  pmsin_data *data = NODE_DATA(pmsin_data, node);

  double sample;
  double freq = data->freq;
  double pm_index = data->pm_index;
  double pm_freq = data->pm_freq;
  for (int f = 0; f < nframes; f++) {
    sample = sine_sample(data->phase);
    data->phase = fmod(freq * spf + data->phase, 1.0);
    data->out->data[f] = sample;
  }
}

Node *pmsin_node(double freq, double pm_index, double pm_freq) {

  Node *osc = ALLOC_NODE(pmsin_data, "sin");
  osc->perform = pmsin_perform;
  pmsin_data *data = osc->data;
  data->freq = freq;
  data->phase = 0.0;
  data->pm_index = pm_index;
  data->pm_freq = pm_freq;
  data->out = new_signal_heap(BUF_SIZE, 1);
  return osc;
}

static double sq_sample(double phase, double freq) {
  return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
}

static node_perform sq_perform(Node *node, int nframes, double spf) {

  sq_data *data = NODE_DATA(sq_data, node);
  Signal *freq = data->freq;

  for (int f = 0; f < nframes; f++) {
    double sample = sq_sample(data->ramp, unwrap(*freq, f));

    data->ramp += spf;
    data->out->data[f] = sample;
  }
}
// ----------------------------- SQUARE WAVE OSCILLATORS

#define SQ_TABSIZE (1 << 10)
static double sq_table[SQ_TABSIZE];
static void maketable_blsq() {
  // fill sq_table with values for a band-limited square wave
}

Node *sq_node(double freq) {
  Node *sq = ALLOC_NODE(sq_data, "square");
  sq->perform = sq_perform;
  sq_data *data = sq->data;
  data->freq = new_signal_heap_default(1, 1, freq);
  data->out = new_signal_heap(BUF_SIZE, 1);
  return sq;
}

static node_perform sq_detune_perform(Node *node, int nframes, double spf) {
  sq_data *data = NODE_DATA(sq_data, node);
  Signal *freq = data->freq;

  for (int f = 0; f < nframes; f++) {
    double sample = sq_sample(data->ramp, unwrap(*freq, f)) +
                    sq_sample(data->ramp, unwrap(*freq, f) * 1.01);

    data->ramp += spf;
    data->out->data[f] = 0.5 * sample;
  }
}

Node *sq_detune_node(double freq) {
  Node *sq = ALLOC_NODE(sq_data, "square");
  sq->perform = sq_detune_perform;
  sq_data *data = sq->data;
  data->freq = new_signal_heap_default(1, 1, freq);
  data->out = new_signal_heap(BUF_SIZE, 1);
  return sq;
}

// ----------------------------- IMPULSE OSCILLATORS
static node_perform impulse_perform(Node *node, int nframes, double spf) {
  impulse_data *data = NODE_DATA(impulse_data, node);
  double threshold = 1 / (spf * unwrap(*data->freq, 0));
  for (int f = 0; f < nframes; f++) {
    if (data->counter >= threshold) {
      data->out->data[f] = 1.0;
      data->counter = 0.0;
    } else {
      data->out->data[f] = 0.0;
    }
    data->counter++;
  }
}
Node *impulse_node(double freq) {
  Node *imp = ALLOC_NODE(impulse_data, "impulse");
  imp->perform = impulse_perform;
  impulse_data *data = imp->data;
  data->freq = new_signal_heap_default(1, 1, freq);
  data->out = new_signal_heap(BUF_SIZE, 1);
  return imp;
}
// ------------------------------ POLY BLEP SAW
//

/* static double poly_blep(double *t, double dt) { */
/*   // 0 <= t < 1 */
/*   if (*t < dt) { */
/*     *t /= dt; */
/*     // 2 * (t - t^2/2 - 0.5) */
/*     return *t + *t - *t * *t - 1.; */
/*   } */
/*  */
/*   // -1 < t < 0 */
/*   else if (*t > 1. - dt) { */
/*     *t = (*t - 1.) / dt; */
/*     // 2 * (t^2/2 + t + 0.5) */
/*     return *t * *t + *t + *t + 1.; */
/*   } */
/*  */
/*   // 0 otherwise */
/*   else { */
/*     return 0.; */
/*   } */
/* } */
/*  */
/* static double poly_saw(double *t, double dt) { */
/*   // Correct phase, so it would be in line with sin(2.*M_PI * t) */
/*   *t += 0.5; */
/*   if (*t >= 1.) */
/*     *t -= 1.; */
/*  */
/*   double naive_saw = 2. * *t - 1.; */
/* return naive_saw - poly_blep(t, dt); */
/*   return naive_saw; */
/* } */
double poly_blep(double t, double dt) {
  // 0 <= t < 1
  if (t < dt) {
    t /= dt;
    // 2 * (t - t^2/2 - 0.5)
    return t + t - t * t - 1.;
  }

  // -1 < t < 0
  else if (t > 1. - dt) {
    t = (t - 1.) / dt;
    // 2 * (t^2/2 + t + 0.5)
    return t * t + t + t + 1.;
  }

  // 0 otherwise
  else {
    return 0.;
  }
}
static node_perform poly_saw_perform(Node *node, int nframes, double spf) {
  poly_saw_data *data = NODE_DATA(poly_saw_data, node);

  double naive_saw;
  double freq = unwrap(*data->freq, 0);

  double dt = freq / (1 / spf);
  for (int f = 0; f < nframes; f++) {

    naive_saw = data->phase;

    data->out->data[f] = (2. * naive_saw) - 1 - poly_blep(data->phase, dt);
    data->phase = fmod(freq * spf + data->phase, 1.0);
  }
}

Node *poly_saw_node(double freq) {
  Node *osc = ALLOC_NODE(poly_saw_data, "poly_saw");
  osc->perform = poly_saw_perform;
  poly_saw_data *data = NODE_DATA(poly_saw_data, osc);
  data->freq = new_signal_heap_default(1, 1, freq);
  data->out = new_signal_heap(BUF_SIZE, 1);
  return osc;
}

static node_perform hoover_perform(Node *node, int nframes, double spf) {
  hoover_data *data = NODE_DATA(hoover_data, node);
  double dt_amt = data->dt_amt;

  double naive_saw;
  double freq = unwrap(*data->freq, 0);
  double samp;

  double dt = freq / (1 / spf);
  double phase;

  for (int f = 0; f < nframes; f++) {
    double dt_freq = freq;
    double dt_dt = dt;
    for (int o = 0; o < data->num_oscs; o++) {
      dt_freq *= dt_amt;
      dt_dt *= dt_amt;

      samp += (2. * data->phase[o]) - poly_blep(data->phase[o], dt_dt);
      data->phase[o] = fmod(dt_freq * spf + data->phase[o], 1.0);
    }
    data->out->data[f] = samp / data->num_oscs;
  }
}

Node *hoover_node(double freq, int num_oscs, double detune_spread) {
  Node *osc = ALLOC_NODE(hoover_data, "hoover");
  osc->perform = hoover_perform;
  hoover_data *data = NODE_DATA(hoover_data, osc);
  data->freq = new_signal_heap_default(1, 1, freq);
  data->out = new_signal_heap(BUF_SIZE, 1);

  data->num_oscs = num_oscs;
  data->phase = calloc(sizeof(double), num_oscs);
  data->dt_amt = detune_spread;

  return osc;
}

static node_perform pulse_perform(Node *node, int nframes, double spf) {
  pulse_data *data = NODE_DATA(pulse_data, node);

  double naive_saw;
  double freq = unwrap(*data->freq, 0);

  double pw = unwrap(*data->pw, 0);

  double dt = freq / (1 / spf);
  double samp;
  double phase2;
  double saw2;
  for (int f = 0; f < nframes; f++) {

    naive_saw = data->phase;
    samp = (2. * naive_saw) - 1 - poly_blep(data->phase, dt);

    data->out->data[f] = samp;

    phase2 = data->phase + pw;
    if (phase2 >= 1.0) {
      phase2 -= 1.0;
    }

    saw2 = (2. * phase2) - 1 - poly_blep(phase2, dt);
    samp += saw2 * -1.0 + 2 * pw - 1.0;

    data->out->data[f] = samp;
    data->phase = fmod(freq * spf + data->phase, 1.0);
  }
}

Node *pulse_node(double freq, double pw) {
  Node *osc = ALLOC_NODE(pulse_data, "pulse");
  osc->perform = pulse_perform;
  pulse_data *data = NODE_DATA(pulse_data, osc);
  data->freq = new_signal_heap_default(1, 1, freq);
  data->pw = new_signal_heap_default(1, 1, pw);
  data->out = new_signal_heap(BUF_SIZE, 1);
  return osc;
}

// ----------------------------- OSCILLATOR SETUP
void osc_setup() {
  maketable_sin();
  maketable_blsq();
}
