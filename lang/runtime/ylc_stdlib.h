#ifndef _LANG_YLC_STDLIB_H
#define _LANG_YLC_STDLIB_H
#include <stdbool.h>
#include <stdint.h>

#include "../ylc_datatypes.h"
#include <stdio.h>

void str_copy(char *dest, char *src, int len);

void print(_String str);
void printc(char c);

typedef struct YlcRcHeader {
  uint32_t rc;
  uint32_t tag_or_size_class;
} YlcRcHeader;

void __ylc_dup(void *ptr);
void __ylc_drop(void *ptr);

int rand_int(int range);

// uniformly distributed double between 0 and 1.0
double rand_double();

// uniformly distributed double between min and max
double rand_double_range(double min, double max);

void *ylc_alloc_zeroed_state(int32_t n_bytes);

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

_String string_concat(_String *strings, int num_strings);

_String string_add(_String a, _String b);
char *cstr(_String);
_String from_cstr(int len, char *s);

int char_to_hex_int(char c);

_String transpose_string(int, int, int, int, _String);

double *double_array_init(int32_t size, double val);

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

typedef _String ByteArray;

ByteArray read_bytes(FILE *f);

// typedef struct _YLC__String_List {
//   _String data;
//   struct _YLC__String_List *next;
// } StrList;

YLC_LIST_TYPE(_String)
typedef LIST_T(_String) STRLIST;

typedef struct ReadLinesResult {
  STRLIST *list;
  int length;
} ReadLinesResult;

ReadLinesResult read_lines(FILE *fd);

struct _OptFile open_file(_String path, _String mode);
int read_line(_String buf, FILE *fd);

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

_DoubleArray matrix_vec_mul_double(int rows, int cols, _DoubleArray matrix,
                                   _DoubleArray vec, _DoubleArray out);
double vec_dot_double(_DoubleArray a, _DoubleArray b);

void _matrix_vec_mul(int rows, int cols, double *matrix_data, double *vec_data);

void _vec_add(int size, double *vec1, double *vec2);

// In-place vector addition
DArr vec_add(DArr vec1, DArr vec2);

void _arr_copy(int size, double *from, double *to);

double *mmap_double_array(int32_t data_size, double *data,
                          const char *filename);

_DoubleArray double_array_from_raw(int32_t size, double *data);

void _linalg_pool_init(int32_t size);
double *_double_arr_alloc(int32_t size);

void _linalg_pool_reset();

typedef struct DoublePair {
  double z0;
  double z1;
} DoublePair;

// Regex match result - Option type containing (start, end) positions
typedef struct __attribute__((packed)) {
  int8_t tag;    // 0 = Some, 1 = None
  int32_t rm_so; // start offset
  int32_t rm_eo; // end offset
} RegexMatchOption;

// String parsing functions
int32_t int32_parse(_String str);
double double_parse(_String str);
uint64_t int64_from_int(int32_t x);
uint64_t int64_from_uint64(uint64_t x);
int32_t int64_to_int(uint64_t x);
_String int64_str(uint64_t x);
uint64_t int64_add(uint64_t x, uint64_t y);
uint64_t int64_mul(uint64_t x, uint64_t y);
bool int64_eq(uint64_t x, uint64_t y);
bool int64_ne(uint64_t x, uint64_t y);
bool int64_lt(uint64_t x, uint64_t y);

// Find first regex match in a string
int regex_find_one(char *str, char *pattern, int32_t *res);

int MAX_INT();
#endif
