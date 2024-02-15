#include "audio_math.h"

double get_sample_interp(double read_ptr, double *buf, int max_frames) {
  double r = read_ptr;
  if (r >= max_frames) {
    return 0.0;
  }
  if (r <= 0) {
    r = max_frames + r;
  }
  int frame = ((int)r);
  double fraction = r - frame;
  double result = buf[frame] * fraction;
  result += buf[frame + 1] * (1.0 - fraction);
  return result;
}

double scale_val_2(double env_val, // 0-1
                   double min, double max) {
  return min + env_val * (max - min);
}
