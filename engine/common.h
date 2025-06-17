#ifndef _ENGINE_COMMON_H
#define _ENGINE_COMMON_H
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#define BUF_SIZE 512
#define PI M_PI
#define EPSILON 2.220446e-16
#define LAYOUT 2
#define MAX_INPUTS 16
#define MAX_SF_CHANNELS 16

inline float pow2table_read(float pos, int tabsize, float *table);
typedef struct {
  int32_t size;
  const char *chars
} _YLC_String;

#endif
