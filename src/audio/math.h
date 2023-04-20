#ifndef _AUDIO_MATH_H
#define _AUDIO_MATH_H
double scale_val_2(double env_val, // 0-1
                   double min, double max);
/*
** Read sample at read_ptr from an array of doubles
** if read_ptr is in between two samples, get_sample_interp will interpolate
** linearly between the two samples
**
*/
double get_sample_interp(double read_ptr, double *buf, int max_frames);
#endif
