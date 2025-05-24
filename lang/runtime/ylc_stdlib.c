#include "ylc_stdlib.h"
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

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
  // rand_double = rand_double * 2 - 1;
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

ByteArray read_bytes(FILE *f) {
  ByteArray result = {0, NULL};

  long original_pos = ftell(f);
  if (original_pos == -1) {
    perror("Error getting current file position");
    return result;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    perror("Error seeking to end of file");
    return result;
  }

  long file_size = ftell(f);
  if (file_size == -1) {
    perror("Error getting file size");
    return result;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    perror("Error seeking to beginning of file");
    return result;
  }

  result.bytes = (char *)malloc(file_size + 1); // +1 for null terminator
  if (result.bytes == NULL) {
    perror("Memory allocation failed");
    return result;
  }

  size_t bytes_read = fread(result.bytes, 1, file_size, f);
  if (bytes_read < file_size && !feof(f)) {
    perror("Error reading file");
    free(result.bytes);
    result.bytes = NULL;
    return result;
  }

  result.bytes[bytes_read] = '\0';
  result.size = bytes_read;

  if (fseek(f, original_pos, SEEK_SET) != 0) {
    perror("Error restoring file position");
  }

  return result;
}

/*
 * Free the memory used by a StrList
 */
void free_str_list(StrList *list) {
  while (list != NULL) {
    StrList *next = list->next;
    // Note: We don't free data.chars because it points into the original buffer
    free(list);
    list = next;
  }
}

/*
 * Read all lines from a FILE pointer and return them as a linked list
 *
 * Note: This implementation does not copy line data but instead points
 * into the original buffer. The returned lines will be invalid if the
 * original buffer is freed.
 */
ReadLinesResult read_lines(FILE *f) {
  if (f == NULL) {
    fprintf(stderr, "Error: NULL file pointer\n");
    return (ReadLinesResult){NULL, 0};
  }

  // Read the entire file into memory
  ByteArray file_bytes = read_bytes(f);
  if (file_bytes.bytes == NULL || file_bytes.size == 0) {
    // Return empty list for empty file
    return (ReadLinesResult){NULL, 0};
  }

  // Create the first node as a sentinel (will be head of our list)
  StrList *head = (StrList *)malloc(sizeof(StrList));
  if (head == NULL) {
    free(file_bytes.bytes);

    return (ReadLinesResult){NULL, 0};
  }

  // Initialize head node
  head->data.chars = NULL;
  head->data.length = 0;
  head->next = NULL;

  StrList *current = head;
  char *buffer = file_bytes.bytes;
  char *line_start = buffer;
  size_t line_length = 0;

  int num_lines = 0;
  for (size_t i = 0; i < file_bytes.size; i++) {
    if (buffer[i] == '\n' || i == file_bytes.size - 1) {
      if (i == file_bytes.size - 1 && buffer[i] != '\n') {
        line_length++;
      }

      StrList *new_node = (StrList *)malloc(sizeof(StrList));
      if (new_node == NULL) {
        // Memory allocation failed
        free(file_bytes.bytes);
        free_str_list(head);
        return (ReadLinesResult){NULL, 0};
      }

      // Store the line data
      new_node->data.chars = line_start;
      new_node->data.length = line_length;
      new_node->next = NULL;

      // Add to the list
      current->next = new_node;
      current = new_node;

      // Prepare for the next line
      line_start = buffer + i + 1;
      line_length = 0;
      num_lines++;
    } else {
      line_length++;
    }
  }

  head->data.chars = file_bytes.bytes;

  StrList *result = head->next;
  free(head);

  return (ReadLinesResult){result, num_lines};
}

struct _OptFile open_file(String path, String mode) {
  FILE *f = fopen(path.chars, mode.chars);
  if (f) {
    // printf("read file\n");
    return (struct _OptFile){0, f};
  }
  return (struct _OptFile){1, NULL};
}

struct sockaddr *create_server_addr(int af_inet, int inaddr_any, int port) {

  struct sockaddr_in *_server_addr = malloc(sizeof(struct sockaddr_in));

  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));

  printf("%d %d %d\n", af_inet, inaddr_any, port);

  // assign IP, PORT
  servaddr.sin_family = af_inet;
  servaddr.sin_addr.s_addr = htonl(inaddr_any);
  servaddr.sin_port = htons(port);
  *_server_addr = servaddr;
  // printf("sizeof sockaddr_in %lu\n", sizeof(struct sockaddr_in));
  return (struct sockaddr *)_server_addr;
}

double relu_d(double i) { return i < 0. ? 0. : i; }

void _scanf(const char *fmt_string, const char *input_string, int size,
            void **pointers) {
  // The correct order of parameters for sscanf is:
  // sscanf(input_string, fmt_string, arg1, arg2, ...)

  // We have pointers[0], pointers[1], etc. but sscanf needs them as separate
  // arguments Unfortunately, there's no standard way in C to convert an array
  // of pointers to varargs

  // We'll use a switch statement based on 'size' to handle a fixed number of
  // cases
  switch (size) {
  case 0:
    // No arguments to parse
    break;
  case 1:
    sscanf(input_string, fmt_string, pointers[0]);
    break;
  case 2:
    sscanf(input_string, fmt_string, pointers[0], pointers[1]);
    break;
  case 3:
    sscanf(input_string, fmt_string, pointers[0], pointers[1], pointers[2]);
    break;
  case 4:
    sscanf(input_string, fmt_string, pointers[0], pointers[1], pointers[2],
           pointers[3]);
    break;
  case 5:
    sscanf(input_string, fmt_string, pointers[0], pointers[1], pointers[2],
           pointers[3], pointers[4]);
    break;
  case 6:
    sscanf(input_string, fmt_string, pointers[0], pointers[1], pointers[2],
           pointers[3], pointers[4], pointers[5]);
    break;
  case 7:
    sscanf(input_string, fmt_string, pointers[0], pointers[1], pointers[2],
           pointers[3], pointers[4], pointers[5], pointers[6]);
    break;
  case 8:
    sscanf(input_string, fmt_string, pointers[0], pointers[1], pointers[2],
           pointers[3], pointers[4], pointers[5], pointers[6], pointers[7]);
    break;
  default:
    fprintf(stderr, "Error: _scanf supports max 8 arguments\n");
    break;
  }
}
// Correctly defined _matrix_vec_mul implementation
void _matrix_vec_mul(int rows, int cols, double *matrix_data,
                     double *vector_data) {
  // Create a temporary array to store results
  double temp[rows];
  for (int i = 0; i < rows; i++) {
    temp[i] = 0.0;
    for (int j = 0; j < cols; j++) {
      temp[i] += matrix_data[i * cols + j] * vector_data[j];
    }
  }

  // Copy results back to the vector
  for (int i = 0; i < rows; i++) {
    vector_data[i] = temp[i];
  }
}

// Define vec_add to properly add vectors
void _vec_add(int size, double *vec1, double *vec2) {
  for (int i = 0; i < size; i++) {
    vec2[i] += vec1[i];
  }
}

void _arr_copy(int size, double *from, double *to) {
  printf("copy arr %d\n", size);
  int idx = 0;
  while (size--) {
    // printf("%d to %d\n", idx, idx);
    *to = *from;
    to++;
    from++;
    idx++;
  }
}

double *mmap_double_array(int32_t data_size, double *data,
                          const char *filename) {

  struct stat file_stat;
  int file_exists = (stat(filename, &file_stat) == 0);

  if (!file_exists) {
    int fd = open(filename, O_RDWR | O_CREAT, (mode_t)0600);
    if (fd == -1) {
      printf("%s: \n", filename);
      perror("Error opening file for writing");
      return NULL;
    }

    if (write(fd, data, data_size * sizeof(double)) !=
        data_size * sizeof(double)) {

      printf("%s: \n", filename);
      perror("Error writing data to file");
      close(fd);
      return NULL;
    }

    close(fd);
  }

  int fd = open(filename, O_RDWR, (mode_t)0600);
  if (fd == -1) {

    printf("%s: \n", filename);
    perror("Error opening file for mapping");
    return NULL;
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    perror("Error getting file size");
    close(fd);
    return NULL;
  }

  double *mapped_data = (double *)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                                       MAP_SHARED, fd, 0);
  if (mapped_data == MAP_FAILED) {
    perror("Error mmapping the file");
    close(fd);
    return NULL;
  }

  close(fd);
  free(data);

  return mapped_data;
}

struct _DoubleArray double_array_from_raw(int32_t size, double *data) {
  return (struct _DoubleArray){size, data};
}

void mmap_sync_array(int32_t size, double *data) {
  msync(data, size, MS_ASYNC);
}

static double *_linalg_pool;
static double *_linalg_pool_head;
static double *_linalg_pool_tail;

void _linalg_pool_init(int32_t size) {
  _linalg_pool = malloc(sizeof(double) * size);
  _linalg_pool_head = _linalg_pool;
  _linalg_pool_tail = _linalg_pool;
}

double *_double_arr_alloc(int32_t size) {
  double *mem = _linalg_pool_tail;
  // memset(mem, 0, sizeof(double) * size);
  _linalg_pool_tail += size;
  return mem;
}

void _linalg_pool_reset() {
  _linalg_pool_head = _linalg_pool;
  _linalg_pool_tail = _linalg_pool;
}

int char_to_int(char c) { return (int)c; }

int _multinomial(int size, double *weights) {
  double randd = rand_double();
  double cum = 0.;
  for (int i = 0; i < size; i++) {
    cum += weights[i];
    if (randd <= cum) {
      return i;
    }
  }
  return size - 1;
}

static const double two_pi = 2.0 * M_PI;

double nonzero_randu_double() {
  double u1;
  do {
    u1 = rand_double();
  } while (u1 == 0.);
  return u1;
}

DoublePair _randn_pair(double mu, double sigma) {
  // create two random numbers, make sure u1 is greater than zero
  double u1, u2;
  u1 = nonzero_randu_double();
  u2 = rand_double();

  // compute z0 and z1
  double mag = sigma * sqrt(-2.0 * log(u1));
  double z0 = mag * cos(two_pi * u2) + mu;
  double z1 = mag * sin(two_pi * u2) + mu;

  return (DoublePair){z0, z1};
}

#define BLOCK_SIZE 64
struct _DoubleArray _mmul_tmp(int a_rows, int a_cols, double *a_data,
                              int b_rows, int b_cols, double *b_data) {
  int c_rows = a_rows;
  int c_cols = b_cols;
  double *new = _double_arr_alloc(a_rows * b_cols);

  // Blocked multiplication
  for (int ii = 0; ii < a_rows; ii += BLOCK_SIZE) {
    for (int kk = 0; kk < a_cols; kk += BLOCK_SIZE) {
      for (int jj = 0; jj < b_cols; jj += BLOCK_SIZE) {

        // Compute block boundaries
        int i_end = (ii + BLOCK_SIZE < a_rows) ? ii + BLOCK_SIZE : a_rows;
        int k_end = (kk + BLOCK_SIZE < a_cols) ? kk + BLOCK_SIZE : a_cols;
        int j_end = (jj + BLOCK_SIZE < b_cols) ? jj + BLOCK_SIZE : b_cols;

        // Compute the block
        for (int i = ii; i < i_end; i++) {
          for (int k = kk; k < k_end; k++) {
            double a_ik = a_data[i * a_cols + k];
            for (int j = jj; j < j_end; j++) {
              // Initialize to zero only on the very first k value
              if (k == 0) {
                new[i * c_cols + j] = 0.0;
              }
              new[i * c_cols + j] += a_ik *b_data[k * b_cols + j];
            }
          }
        }
      }
    }
  }
  return (struct _DoubleArray){c_rows * c_cols, new};
}

// 1. BASIC OPTIMIZATION: Better cache locality with loop reordering
struct _DoubleArray _mmul_basic_opt(int a_rows, int a_cols, double *a_data,
                                    int b_rows, int b_cols, double *b_data) {
  int c_rows = a_rows;
  int c_cols = b_cols;
  double *new = _double_arr_alloc(a_rows * b_cols);

  // Initialize to zero
  for (int i = 0; i < a_rows * b_cols; i++) {
    new[i] = 0.0;
  }

  // ikj order for better cache locality
  for (int i = 0; i < a_rows; i++) {
    for (int k = 0; k < a_cols; k++) {
      double a_ik = a_data[i * a_cols + k];
      for (int j = 0; j < b_cols; j++) {
        new[i * c_cols + j] += a_ik *b_data[k * b_cols + j];
      }
    }
  }
  return (struct _DoubleArray){c_rows * c_cols, new};
}

// 4. PARALLEL: Using OpenMP
#ifdef _OPENMP
#include <omp.h>

struct _DoubleArray _mmul_parallel(int a_rows, int a_cols, double *a_data,
                                   int b_rows, int b_cols, double *b_data) {
  int c_rows = a_rows;
  int c_cols = b_cols;
  double *new = _double_arr_alloc(a_rows * b_cols);

  memset(new, 0, a_rows * b_cols * sizeof(double));

#pragma omp parallel for
  for (int i = 0; i < a_rows; i++) {
    for (int k = 0; k < a_cols; k++) {
      double a_ik = a_data[i * a_cols + k];
      for (int j = 0; j < b_cols; j++) {
        new[i * c_cols + j] += a_ik *b_data[k * b_cols + j];
      }
    }
  }
  return (struct _DoubleArray){c_rows * c_cols, new};
}
#endif

// 6. FOR REALLY LARGE MATRICES: Use BLAS
// Link with -lopenblas or -lmkl
#ifdef _USE_BLAS
#include <cblas.h>

struct _DoubleArray _mmul_blas(int a_rows, int a_cols, double *a_data,
                               int b_rows, int b_cols, double *b_data,
                               double *out_data) {
  int c_rows = a_rows;
  int c_cols = b_cols;
  double *new = out_data;

  // C = alpha * A * B + beta * C
  // alpha = 1.0, beta = 0.0
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, a_rows, b_cols, a_cols,
              1.0, a_data, a_cols, b_data, b_cols, 0.0, out_data, c_cols);

  return (struct _DoubleArray){c_rows * c_cols, out_data};
}
#endif

// 1. SIMPLE: Basic transpose
struct _DoubleArray _transpose_simple(int rows, int cols, double *data) {
  double *new = _double_arr_alloc(rows * cols);

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      new[j * rows + i] = data[i * cols + j];
    }
  }
  return (struct _DoubleArray){rows * cols, new};
}

// 2. OPTIMIZED: Blocked transpose for better cache performance
#define TRANSPOSE_BLOCK_SIZE 64

struct _DoubleArray _transpose_blocked(int rows, int cols, double *data) {
  double *new = _double_arr_alloc(rows * cols);

  for (int ii = 0; ii < rows; ii += TRANSPOSE_BLOCK_SIZE) {
    for (int jj = 0; jj < cols; jj += TRANSPOSE_BLOCK_SIZE) {

      // Compute block boundaries
      int i_end =
          (ii + TRANSPOSE_BLOCK_SIZE < rows) ? ii + TRANSPOSE_BLOCK_SIZE : rows;
      int j_end =
          (jj + TRANSPOSE_BLOCK_SIZE < cols) ? jj + TRANSPOSE_BLOCK_SIZE : cols;

      // Transpose the block
      for (int i = ii; i < i_end; i++) {
        for (int j = jj; j < j_end; j++) {
          new[j * rows + i] = data[i * cols + j];
        }
      }
    }
  }
  return (struct _DoubleArray){rows * cols, new};
}

// 3. IN-PLACE: For square matrices only (saves memory)
void _transpose_inplace_square(int size, double *data) {
  for (int i = 0; i < size; i++) {
    for (int j = i + 1; j < size; j++) {
      // Swap elements across diagonal
      double temp = data[i * size + j];
      data[i * size + j] = data[j * size + i];
      data[j * size + i] = temp;
    }
  }
}

// 4. IN-PLACE BLOCKED: For square matrices with better cache performance
void _transpose_inplace_blocked(int size, double *data) {
  for (int ii = 0; ii < size; ii += TRANSPOSE_BLOCK_SIZE) {
    for (int jj = ii; jj < size; jj += TRANSPOSE_BLOCK_SIZE) {

      int i_end =
          (ii + TRANSPOSE_BLOCK_SIZE < size) ? ii + TRANSPOSE_BLOCK_SIZE : size;
      int j_end =
          (jj + TRANSPOSE_BLOCK_SIZE < size) ? jj + TRANSPOSE_BLOCK_SIZE : size;

      if (ii == jj) {
        // Diagonal block - transpose in place
        for (int i = ii; i < i_end; i++) {
          for (int j = i + 1; j < j_end; j++) {
            double temp = data[i * size + j];
            data[i * size + j] = data[j * size + i];
            data[j * size + i] = temp;
          }
        }
      } else {
        // Off-diagonal block - swap entire blocks
        for (int i = ii; i < i_end; i++) {
          for (int j = jj; j < j_end; j++) {
            double temp = data[i * size + j];
            data[i * size + j] = data[j * size + i];
            data[j * size + i] = temp;
          }
        }
      }
    }
  }
}

// 5. PARALLEL: Using OpenMP for large matrices
#ifdef _USE_OPENMP
#include <omp.h>

struct _DoubleArray _transpose_parallel(int rows, int cols, double *data) {
  double *new = _double_arr_alloc(rows * cols);

#pragma omp parallel for
  for (int ii = 0; ii < rows; ii += TRANSPOSE_BLOCK_SIZE) {
    for (int jj = 0; jj < cols; jj += TRANSPOSE_BLOCK_SIZE) {

      int i_end =
          (ii + TRANSPOSE_BLOCK_SIZE < rows) ? ii + TRANSPOSE_BLOCK_SIZE : rows;
      int j_end =
          (jj + TRANSPOSE_BLOCK_SIZE < cols) ? jj + TRANSPOSE_BLOCK_SIZE : cols;

      for (int i = ii; i < i_end; i++) {
        for (int j = jj; j < j_end; j++) {
          new[j * rows + i] = data[i * cols + j];
        }
      }
    }
  }
  return (struct _DoubleArray){rows * cols, new};
}
#endif

// 7. CONVENIENCE: Auto-select best transpose method
struct _DoubleArray _transpose(int rows, int cols, double *data) {
  // For your specific use case (228146 x 27 -> 27 x 228146
  if (rows * cols > 10000) {
    return _transpose_blocked(rows, cols, data);
  } else {
    return _transpose_simple(rows, cols, data);
  }
}

#ifdef _USE_OPENMP
void matmulp(float *xout, float *x, float *w, int n, int d) {
  // W (d,n) @ x (n,) -> xout (d,)
  // by far the most amount of time is spent inside this little function
  int i;
#pragma omp parallel for private(i)
  for (i = 0; i < d; i++) {
    float val = 0.0f;
    for (int j = 0; j < n; j++) {
      val += w[i * n + j] * x[j];
    }
    xout[i] = val;
  }
}
#endif
