#ifndef _SOUNDFILE_H
#define _SOUNDFILE_H
#include "node.h"
#include <sndfile.h>

int read_file(const char *filename, Signal *signal, int *sf_sample_rate);

int read_file_float_deinterleaved(const char *filename,
                                  SignalFloatDeinterleaved *signal,
                                  int *sf_sample_rate);
#endif
