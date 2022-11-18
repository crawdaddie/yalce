#ifndef _WINDOWS
#define _WINDOWS
#include "../config.h"
#include <math.h>

//
//SINE
//windowFunction[i] = sin( PI * (double)i / samplesPerGrain  );

//HANN

//HAMMING
//windowFunction[i] = 0.54 - 0.46 * cos(2*PI*i/samplesPerGrain);

//TUKEY
//float truncationHeight = 0.5;
//float f = 1/(2*truncationHeight) * (1-cos(2*PI*i/samplesPerGrain));
//windowFunction[i] = f < 1 ? f : 1;

//GAUSSIAN
// float sigma = 0.3;
// windowFunction[i] = pow(e, -0.5* pow(((i-samplesPerGrain/2) / (sigma*samplesPerGrain/2)), 2) ); 

//TRAPEZOIDAL
//float slope = 10;
//float x = (float) i / framesPerGrain;
//float f1 = slope * x;
//float f2 = -1 * slope * (x-(slope-1) / slope) + 1;
// windowFunction[i] = x < 0.5 ? (f1 < 1 ? f1 : 1) : (f2 < 1 ? f2 : 1);

t_sample hann(int i, int length_in_samps) {
  return 0.5 * (1 - cos(2*PI*i/length_in_samps));
}
#endif
