#include "math.h"
double scale_val_2(double env_val, // 0-1
                   double min, double max) {
  return min + env_val * (max - min);
}
