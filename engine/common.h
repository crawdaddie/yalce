#ifndef _ENGINE_COMMON_H
#define _ENGINE_COMMON_H
#include <math.h>
#include <stdbool.h>
#define BUF_SIZE 512
#define PI M_PI
#define EPSILON 2.220446e-16
#define LAYOUT 2
#define MAX_INPUTS 16
#define MAX_SF_CHANNELS 16

inline double pow2table_read(double pos, int tabsize, double *table);

#endif
