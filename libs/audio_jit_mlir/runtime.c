#include "../../engine/node.h"
#include <stdint.h>

#ifndef SIN_TABSIZE
#define SIN_TABSIZE (1 << 11)
#endif

const double ylc_sin_table[SIN_TABSIZE] = {
#include "../../engine/assets/sin_table.csv"
};

#ifndef SQ_TABSIZE
#define SQ_TABSIZE (1 << 11)
#endif

const double ylc_sq_table[SQ_TABSIZE] = {
#include "../../engine/assets/sq_table.csv"
};

#ifndef SAW_TABSIZE
#define SAW_TABSIZE (1 << 11)
#endif

const double ylc_saw_table[SAW_TABSIZE] = {
#include "../../engine/assets/saw_table.csv"
};

void dsp_write_output(void *node_raw, int64_t frame, double val) {
  ((Node *)node_raw)->output.buf[frame] = val;
}

double ylc_read_inlet_node(void *node_raw, int64_t frame) {
  return ((Node *)node_raw)->output.buf[frame];
}

double wrap_upper(double limit, double v) {
  if (v >= limit) {
    return v - limit;
  }
  return v;
}
