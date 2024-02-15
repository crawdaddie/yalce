#ifndef _SOUNDFILE_H
#define _SOUNDFILE_H
#include "signal.h"
#include <sndfile.h>

int read_file(const char *filename, Signal *read_result);
#endif
