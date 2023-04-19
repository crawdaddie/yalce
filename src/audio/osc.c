#include "osc.h"
#include "math.h"
#include <math.h>
#include <stdlib.h>

#define SIN_TABSIZE (1 << 11)

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

  Signal out = OUTS(node);
  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double freq;

  for (int f = 0; f < nframes; f++) {
    freq = unwrap(IN(node, SIN_SIG_FREQ), f);
    d_index = data->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);

    data->phase = fmod(freq * spf + data->phase, 1.0);
    node_write_out(node, f, sample);
  }
}

Node *sin_node(double freq, Signal *ins) {
  Node *osc = ALLOC_NODE(sin_data, "Sin");
  osc->perform = sin_perform;
  sin_data *data = osc->data;

  INS(osc) = ins == NULL ? ALLOC_SIGS(SIN_SIG_OUT) : ins;
  NUM_INS(osc) = SIN_SIG_OUT;
  init_signal(INS(osc), 1, freq);

  init_out_signal(&OUTS(osc), BUF_SIZE, 1);

  data->phase = 0.0;
  return osc;
}

// ----------------------------- IMPULSE OSCILLATORS
static node_perform impulse_perform(Node *node, int nframes, double spf) {

  impulse_data *data = NODE_DATA(impulse_data, node);
  Signal freq_sig = IN(node, IMPULSE_SIG_FREQ);
  double freq = unwrap(freq_sig, 0);

  double threshold = 1 / (spf * freq);

  for (int f = 0; f < nframes; f++) {
    if (data->counter >= threshold) {

      node_write_out(node, f, 1.0);
      data->counter = 0.0;
    } else {

      node_write_out(node, f, 0.0);
    }
    data->counter++;
  }
}
Node *impulse_node(double freq, Signal *ins) {
  Node *osc = ALLOC_NODE(impulse_data, "Impulse");
  osc->perform = impulse_perform;

  impulse_data *data = osc->data;

  INS(osc) = ins == NULL ? ALLOC_SIGS(IMPULSE_SIG_OUT) : ins;
  NUM_INS(osc) = IMPULSE_SIG_OUT;

  init_signal(INS(osc), 1, freq);

  init_out_signal(&OUTS(osc), BUF_SIZE, 1);

  return osc;
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
  Signal freq_sig = IN(node, POLY_SAW_SIG_FREQ);
  double freq = unwrap(freq_sig, 0);

  double dt = freq / (1 / spf);
  for (int f = 0; f < nframes; f++) {

    naive_saw = data->phase;

    node_write_out(node, f, (2. * naive_saw) - 1 - poly_blep(data->phase, dt));
    data->phase = fmod(freq * spf + data->phase, 1.0);
  }
}

Node *poly_saw_node(double freq, Signal *ins) {

  Node *osc = ALLOC_NODE(poly_saw_data, "poly_saw");
  osc->perform = poly_saw_perform;
  pulse_data *data = NODE_DATA(pulse_data, osc);

  INS(osc) = ins == NULL ? ALLOC_SIGS(POLY_SAW_SIG_OUT) : ins;
  NUM_INS(osc) = POLY_SAW_SIG_OUT;
  init_signal(INS(osc), 1, freq);

  init_out_signal(&OUTS(osc), BUF_SIZE, 1);
  return osc;
}

// ----------------------------- SQUARE WAVE OSCILLATORS
static double sq_sample(double phase, double freq) {
  return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
}

static node_perform sq_perform(Node *node, int nframes, double spf) {

  sq_data *data = NODE_DATA(sq_data, node);
  Signal freq = IN(node, SQ_SIG_FREQ);

  for (int f = 0; f < nframes; f++) {
    double sample = sq_sample(data->phase, unwrap(freq, f));
    data->phase += spf;
    node_write_out(node, f, sample);
  }
}

#define SQ_TABSIZE (1 << 10)
static double sq_table[SQ_TABSIZE];
static void maketable_blsq() {
  // fill sq_table with values for a band-limited square wave
}

Node *sq_node(double freq, Signal *ins) {

  Node *osc = ALLOC_NODE(sq_data, "sq");
  osc->perform = sq_perform;
  sq_data *data = NODE_DATA(sq_data, osc);

  INS(osc) = ins == NULL ? ALLOC_SIGS(SQ_SIG_OUT) : ins;
  NUM_INS(osc) = 1;
  init_signal(INS(osc), 1, freq);

  init_out_signal(&OUTS(osc), BUF_SIZE, 1);

  return osc;
}

static node_perform sq_detune_perform(Node *node, int nframes, double spf) {
  sq_data *data = NODE_DATA(sq_data, node);
  Signal freq = IN(node, SQ_SIG_FREQ);

  for (int f = 0; f < nframes; f++) {
    double sample = sq_sample(data->phase, unwrap(freq, f)) +
                    sq_sample(data->phase, unwrap(freq, f) * 1.01);

    data->phase += spf;

    node_write_out(node, f, 0.5 * sample);
  }
}

Node *sq_detune_node(double freq, Signal *ins) {

  Node *osc = ALLOC_NODE(sq_data, "sq_detune");
  osc->perform = sq_detune_perform;
  sq_data *data = NODE_DATA(sq_data, osc);

  INS(osc) = ins == NULL ? ALLOC_SIGS(SQ_SIG_OUT) : ins;
  NUM_INS(osc) = SQ_SIG_OUT;
  init_signal(INS(osc), 1, freq);

  init_out_signal(&OUTS(osc), BUF_SIZE, 1);

  return osc;
}
static node_perform pulse_perform(Node *node, int nframes, double spf) {
  pulse_data *data = NODE_DATA(pulse_data, node);

  double naive_saw;
  double freq;
  double pw;

  double dt = freq / (1 / spf);
  double sample;
  double phase2;
  double saw2;
  for (int f = 0; f < nframes; f++) {
    freq = unwrap(IN(node, PULSE_SIG_FREQ), f);
    pw = unwrap(IN(node, PULSE_SIG_PW), f);

    naive_saw = data->phase;
    sample = (2. * naive_saw) - 1 - poly_blep(data->phase, dt);

    phase2 = data->phase + pw;
    if (phase2 >= 1.0) {
      phase2 -= 1.0;
    }

    saw2 = (2. * phase2) - 1 - poly_blep(phase2, dt);
    sample += saw2 * -1.0 + 2 * pw - 1.0;

    node_write_out(node, f, sample);
    data->phase = fmod(freq * spf + data->phase, 1.0);
  }
}

Node *pulse_node(double freq, double pw, Signal *ins) {
  Node *osc = ALLOC_NODE(pulse_data, "Pulse");
  osc->perform = pulse_perform;
  pulse_data *data = NODE_DATA(pulse_data, osc);
  double init_values[PULSE_SIG_OUT] = {freq, pw};
  node_build_ins(osc, PULSE_SIG_OUT, init_values);
  return osc;
}

// ----------------------------- OSCILLATOR SETUP
void osc_setup() {
  maketable_sin();
  maketable_blsq();
}
