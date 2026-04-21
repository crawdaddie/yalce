#include "./bin_scrambler_runtime.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

static inline double bs_clip01(double x) {
  if (x < 0.0) {
    return 0.0;
  }
  if (x > 1.0) {
    return 1.0;
  }
  return x;
}

static inline double bs_complex_mag(BinScramblerComplex z) {
  return sqrt(z.re * z.re + z.im * z.im);
}

static inline double bs_complex_phase(BinScramblerComplex z) {
  return atan2(z.im, z.re);
}

static inline BinScramblerComplex bs_from_polar(double mag, double phase) {
  return (BinScramblerComplex){.re = mag * cos(phase), .im = mag * sin(phase)};
}

static inline int32_t bs_pair_count(int32_t fft_size) {
  if (fft_size < 4) {
    return 0;
  }
  return (fft_size / 2) - 1;
}

int32_t bin_scrambler_state_bytes(int32_t fft_size) {
  int32_t bins = bs_pair_count(fft_size);
  int32_t bytes = (int32_t)sizeof(BinScramblerState);
  bytes = (bytes + 7) & ~7;
  bytes += (int32_t)(sizeof(int32_t) * (size_t)bins);
  bytes += (int32_t)(sizeof(int32_t) * (size_t)bins);
  return bytes;
}

void bin_scrambler_state_init(void *state_raw, int32_t fft_size) {
  if (!state_raw || fft_size <= 0) {
    return;
  }

  char *cursor = (char *)state_raw;
  BinScramblerState *st = (BinScramblerState *)cursor;
  cursor += ((int32_t)sizeof(BinScramblerState) + 7) & ~7;

  int32_t bins = bs_pair_count(fft_size);
  int32_t *a_idx = (int32_t *)cursor;
  cursor += (int32_t)(sizeof(int32_t) * (size_t)bins);
  int32_t *b_idx = (int32_t *)cursor;

  st->phasor = 0;
  st->max_phase = 1;
  st->fft_size = fft_size;
  st->a_idx = a_idx;
  st->b_idx = b_idx;

  bin_scrambler_reset_identity(a_idx, bins);
  bin_scrambler_reset_identity(b_idx, bins);
}

void bin_scrambler_reset_identity(int32_t *idx, int32_t n) {
  if (!idx || n <= 0) {
    return;
  }
  for (int32_t i = 0; i < n; ++i) {
    idx[i] = i;
  }
}

void bin_scrambler_apply_scatter(int32_t *idx, int32_t n, double scatter) {
  if (!idx || n <= 0) {
    return;
  }

  int32_t dist_amount = (int32_t)((1.0 - (scatter * scatter)) * 100.0);
  int32_t limit = n / 5;
  if (limit < 1) {
    return;
  }

  for (int32_t i = 0; i < limit; ++i) {
    if ((rand() % 100) >= dist_amount) {
      idx[i] = idx[(rand() % n) / 5];
    }
  }
}

void bin_scrambler_apply_scramble(int32_t *idx, int32_t n, double scramble) {
  if (!idx || n <= 1) {
    return;
  }

  int32_t chunk = ((int32_t)(pow(scramble, 3.0) * (double)n)) / 2;
  if (chunk <= 1) {
    return;
  }

  for (int32_t start = 0; start < n - chunk; start += chunk) {
    for (int32_t i = chunk - 1; i > 0; --i) {
      int32_t j = rand() % (i + 1);
      int32_t a = start + i;
      int32_t b = start + j;
      int32_t tmp = idx[a];
      idx[a] = idx[b];
      idx[b] = tmp;
    }
  }
}

void bin_scrambler_update(BinScramblerState *st, int32_t hop_size, double rate,
                          double scramble, double scatter,
                          int32_t sample_rate) {
  if (!st || !st->a_idx || !st->b_idx || st->fft_size <= 0 || rate <= 0.0 ||
      sample_rate <= 0) {
    return;
  }

  st->max_phase = (int32_t)((double)sample_rate / rate);
  if (st->max_phase < 1) {
    st->max_phase = 1;
  }

  if (st->phasor >= st->max_phase) {
    int32_t *tmp = st->a_idx;
    st->a_idx = st->b_idx;
    st->b_idx = tmp;

    int32_t bins = bs_pair_count(st->fft_size);
    bin_scrambler_reset_identity(st->b_idx, bins);
    bin_scrambler_apply_scatter(st->b_idx, bins, scatter);
    bin_scrambler_apply_scramble(st->b_idx, bins, scramble);

    st->phasor -= st->max_phase;
  }

  st->phasor += hop_size;
}

void bin_scrambler_process_cartesian(const BinScramblerComplex *in,
                                     BinScramblerComplex *out,
                                     const BinScramblerState *st) {
  if (!in || !out || !st || !st->a_idx || !st->b_idx || st->fft_size <= 0 ||
      st->max_phase <= 0) {
    return;
  }

  int32_t bins = bs_pair_count(st->fft_size);
  double t = bs_clip01((double)st->phasor / (double)st->max_phase);

  if (st->fft_size > 0) {
    out[0] = in[0];
  }
  if ((st->fft_size & 1) == 0 && st->fft_size > 1) {
    int32_t nyquist = st->fft_size / 2;
    out[nyquist] = in[nyquist];
  }

  for (int32_t i = 0; i < bins; ++i) {
    int32_t dst = i + 1;
    int32_t src_a = st->a_idx[i] + 1;
    int32_t src_b = st->b_idx[i] + 1;
    int32_t mirror = st->fft_size - dst;
    BinScramblerComplex a = in[src_a];
    BinScramblerComplex b = in[src_b];
    BinScramblerComplex z = {
        .re = a.re + ((b.re - a.re) * t),
        .im = a.im + ((b.im - a.im) * t),
    };
    out[dst] = z;
    out[mirror] = (BinScramblerComplex){.re = z.re, .im = -z.im};
  }
}

void bin_scrambler_process_polar(const BinScramblerComplex *in,
                                 BinScramblerComplex *out,
                                 const BinScramblerState *st) {
  if (!in || !out || !st || !st->a_idx || !st->b_idx || st->fft_size <= 0 ||
      st->max_phase <= 0) {
    return;
  }

  int32_t bins = bs_pair_count(st->fft_size);
  double t = bs_clip01((double)st->phasor / (double)st->max_phase);

  if (st->fft_size > 0) {
    out[0] = in[0];
    out[0].im = 0.0;
  }
  if ((st->fft_size & 1) == 0 && st->fft_size > 1) {
    int32_t nyquist = st->fft_size / 2;
    out[nyquist] = in[nyquist];
    out[nyquist].im = 0.0;
  }

  for (int32_t i = 0; i < bins; ++i) {
    int32_t dst = i + 1;
    int32_t src_a = st->a_idx[i] + 1;
    int32_t src_b = st->b_idx[i] + 1;
    int32_t mirror = st->fft_size - dst;
    BinScramblerComplex za = in[src_a];
    BinScramblerComplex zb = in[src_b];

    double ma = bs_complex_mag(za);
    double mb = bs_complex_mag(zb);
    double pa = bs_complex_phase(za);
    double pb = bs_complex_phase(zb);

    double m = ma + ((mb - ma) * t);
    double p = pa + ((pb - pa) * t);
    BinScramblerComplex z = bs_from_polar(m, p);
    out[dst] = z;
    out[mirror] = (BinScramblerComplex){.re = z.re, .im = -z.im};
  }
}

void bin_scrambler_process_runtime(void *state_raw,
                                   const BinScramblerComplex *in,
                                   BinScramblerComplex *out, double rate,
                                   double scramble, double scatter,
                                   int32_t sample_rate, int32_t hop_size) {
  if (!state_raw || !in || !out) {
    return;
  }

  BinScramblerState *st = (BinScramblerState *)state_raw;
  bin_scrambler_update(st, hop_size, rate, scramble, scatter, sample_rate);
  bin_scrambler_process_polar(in, out, st);
}
