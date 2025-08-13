#ifndef _ENGINE_COMMON_H
#define _ENGINE_COMMON_H
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#define BUF_SIZE 512
#define PI M_PI
#define EPSILON 2.220446e-16f
#define LAYOUT 2
#define MAX_INPUTS 16
#define MAX_SF_CHANNELS 16

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

typedef float sample_t;
// typedef double sample_t;

inline sample_t pow2table_read(sample_t pos, int tabsize, sample_t *table);

#endif
