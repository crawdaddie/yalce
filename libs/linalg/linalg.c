#include <stdint.h>
#include <stdlib.h>
struct _DoubleArray {
  int32_t size;
  double *data;
};

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
