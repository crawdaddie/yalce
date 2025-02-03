#include "ylc_stdlib.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void str_copy(char *dest, char *src, int len) {
  // printf("calling str copy %s %s %d\n", dest, src, len);
  memcpy(dest, src, len);
  dest[len + 1] = '\0';
}

void print(String str) { printf("%s", str.chars); }
void printc(char c) { printf("%c", c); }

void fprint(FILE *f, String str) { fprintf(f, "%s", str.chars); }
struct char_matrix {
  int32_t rows;
  int32_t cols;
  String data;
};

void print_char_matrix(int32_t m, int32_t n, char *A) {

  for (int row = 0; row < m; row++) {
    for (int col = 0; col < n; col++) {
      printf("%c", A[row * m + col]);
    }
    printf("\n");
  }
}

// YALCE STDLIB
//

// uniformly distributed integer between 0 and range-1
int rand_int(int range) {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * range;
  return (int)rand_double;
}

// uniformly distributed double between 0 and 1.0
double rand_double() {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * 2 - 1;
  return rand_double;
}

// uniformly distributed double between min and max
double rand_double_range(double min, double max) {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * (max - min) + min;
  return rand_double;
}

double amp_db(double amplitude) { return 20.0f * log10(amplitude); }
double db_amp(double db) { return pow(10.0f, db / 20.0f); }

double semitone_to_rate(double i) { return pow(2.0, i / 12.0f); }

// bipolar input is in the range [-1, 1]
double bipolar_scale(double min, double max, double bipolar_input) {
  return min + (max - min) * (0.5 + (bipolar_input * 0.5));
}
// unipolar input is in the range [0, 1]
double unipolar_scale(double min, double max, double unipolar_input) {
  return min + (max - min) * (unipolar_input);
}

FILE *get_stderr() { return stderr; }
FILE *get_stdout() { return stdout; }

String string_concat(String *strings, int num_strings) {
  int total_len = 0;
  int lengths[num_strings];

  for (int i = 0; i < num_strings; i++) {
    lengths[i] = strings[i].length;
    total_len += lengths[i];
  }

  char *concatted = malloc(sizeof(char) * (total_len + 1));
  int offset = 0;
  for (int i = 0; i < num_strings; i++) {
    strncpy(concatted + offset, strings[i].chars, lengths[i]);
    offset += lengths[i];
  }

  return (String){total_len, concatted};
}

String string_add(String a, String b) {
  return string_concat((String[]){a, b}, 2);
}

char *cstr(String s) { return s.chars; }

int char_to_hex_int(char c) {
  // Convert the character to lowercase for easier processing
  c = tolower(c);

  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else {
    // Return -1 or another error indicator for invalid input
    return -1;
  }
}

String transpose_string(int input_rows, int input_cols, int output_rows,
                        int output_cols, String input) {
  char *transposed =
      malloc(sizeof(char) * ((output_rows * (output_cols + 1)) + 1));

  memset(transposed, '\n', output_rows * (output_cols + 1));

  int output_idx;
  for (int i = 0; i < input_rows; i++) {
    for (int j = 0; j < input_cols; j++) {
      int input_idx = (input_cols + 1) * i + j;
      char c = input.chars[input_idx];
      output_idx = (output_cols + 1) * j + i;
      transposed[output_idx] = c;
    }
  }
  transposed[output_idx + 1] = '\0';
  String result = {output_idx, transposed};
  return result;
}

double *double_array_init(int32_t size, double val) {
  double *data = malloc(sizeof(double) * size);
  for (int32_t i = 0; i < size; i++) {
    data[i] = val;
  }
  return data;
}

struct _DoubleArray double_array(int32_t size, double val) {
  double *data = double_array_init(size, val);
  return (struct _DoubleArray){size, data};
}
