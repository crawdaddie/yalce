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

typedef struct _opt_int_t {
  int8_t tag;
  int32_t val;
} _opt_int_t;

void print_opt_int(_opt_int_t o);

struct _OptFile {
  char status;
  FILE *fd;
};

typedef struct ByteArray {
  size_t size;
  char *bytes;
} ByteArray;

struct ByteArray read_bytes(FILE *f);

typedef struct StrList {
  String data;
  struct StrList *next;

} StrList;
typedef struct ReadLinesResult {
  StrList *list;
  int length;
} ReadLinesResult;

ReadLinesResult read_lines(FILE *fd);

struct _OptFile open_file(String path, String mode);

struct _size_stride_pair {
  int size;
  int stride;
};

typedef struct Tensor {
  size_t dtype_size;
  void *storage;
  int ndim;
  int sizes_strides[] // ndim * 2 [size, stride, size, stride ...]
} Tensor;

void _scanf(const char *fmt_string, const char *input_string, int size,
            void **pointers);

typedef struct DArr {
  int size;
  double *data;
} DArr;

typedef struct DM {
  int rows;
  int cols;
  DArr data;
} DM;

DArr matrix_vec_mul(DM *matrix, DArr vector);

void _matrix_vec_mul(int rows, int cols, double *matrix_data, double *vec_data);

void _vec_add(int size, double *vec1, double *vec2);

// In-place vector addition
DArr vec_add(DArr vec1, DArr vec2);

void _arr_copy(int size, double *from, double *to);

double *mmap_double_array(int32_t data_size, double *data,
                          const char *filename);

struct _DoubleArray double_array_from_raw(int32_t size, double *data);

void _linalg_pool_init(int32_t size);
double *_double_arr_alloc(int32_t size);

void _linalg_pool_reset();

typedef struct DoublePair {
  double z0;
  double z1;
} DoublePair;
#endif
