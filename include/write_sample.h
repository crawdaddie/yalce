#ifndef _WRITE_SAMPLE_H
#define _WRITE_SAMPLE_H

void write_sample_s16ne(char *ptr, double sample);

void write_sample_s32ne(char *ptr, double sample);

void write_sample_float32ne(char *ptr, double sample);
void add_sample_float32ne_w_offset(char *ptr, int offset, double sample);

void write_sample_float64ne(char *ptr, double sample);
#endif
