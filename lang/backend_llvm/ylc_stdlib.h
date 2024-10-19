#ifndef _LANG_YLC_STDLIB_H
#define _LANG_YLC_STDLIB_H
#include <stdint.h>
#include <stdio.h>

typedef struct String {
  int length;
  char *chars;
} String;

void str_copy(char *dest, char *src, int len);

void print(String str);
void printc(char c);

int rand_int(int range);

// uniformly distributed double between 0 and 1.0
double rand_double();

// uniformly distributed double between min and max
double rand_double_range(double min, double max);

double amp_db(double amplitude);
double db_amp(double db);

double semitone_to_rate(double i);
// bipolar input is in the range [-1, 1]
double bipolar_scale(double min, double max, double bipolar_input);
// unipolar input is in the range [0, 1]
double unipolar_scale(double min, double max, double unipolar_input);

FILE *get_stderr();
FILE *get_stdout();

const char *_string_concat(const char **strings, int num_strings);

String string_concat(String *strings, int num_strings);

String string_add(String a, String b);
char *cstr(String);

int char_to_hex_int(char c);

String transpose_string(int, int, int, int, String);

double *double_array_init(int32_t size, double val);
struct _DoubleArray {
  int32_t size;
  double *data;
};

struct _DoubleArray double_array(int32_t size, double val);
#endif
