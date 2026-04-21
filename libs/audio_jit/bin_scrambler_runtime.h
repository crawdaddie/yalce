#ifndef _LIBS_AUDIO_JIT_BIN_SCRAMBLER_RUNTIME_H
#define _LIBS_AUDIO_JIT_BIN_SCRAMBLER_RUNTIME_H

#include <stdint.h>

typedef struct {
  double re;
  double im;
} BinScramblerComplex;

typedef struct {
  int32_t phasor;
  int32_t max_phase;
  int32_t fft_size;
  int32_t *a_idx;
  int32_t *b_idx;
} BinScramblerState;

int32_t bin_scrambler_state_bytes(int32_t fft_size);
void bin_scrambler_state_init(void *state_raw, int32_t fft_size);

void bin_scrambler_reset_identity(int32_t *idx, int32_t n);
void bin_scrambler_apply_scatter(int32_t *idx, int32_t n, double scatter);
void bin_scrambler_apply_scramble(int32_t *idx, int32_t n, double scramble);

void bin_scrambler_update(BinScramblerState *st, int32_t hop_size, double rate,
                          double scramble, double scatter, int32_t sample_rate);

void bin_scrambler_process_cartesian(const BinScramblerComplex *in,
                                     BinScramblerComplex *out,
                                     const BinScramblerState *st);

void bin_scrambler_process_polar(const BinScramblerComplex *in,
                                 BinScramblerComplex *out,
                                 const BinScramblerState *st);

void bin_scrambler_process_runtime(void *state_raw,
                                   const BinScramblerComplex *in,
                                   BinScramblerComplex *out, double rate,
                                   double scramble, double scatter,
                                   int32_t sample_rate, int32_t hop_size);

#endif
