#ifndef _LANG_YLC_DATATYPES_H
#define _LANG_YLC_DATATYPES_H
#include <stdint.h>

typedef struct {
  int size;
  const char *chars;
} _String;

#define YLC_STRING_TYPE(n)                                                     \
  typedef struct {                                                             \
    int size;                                                                  \
    const char *chars;                                                         \
  } n;

typedef struct {
  int32_t size;
  double *data;
} _DoubleArray;

#define YLC_ARRAY_TYPE(t)                                                      \
  typedef struct {                                                             \
    int32_t size;                                                              \
    t *data;                                                                   \
  } _YLC_##t##_Array;

#define LIST_T(t) _YLC_##t##_List

#define YLC_LIST_TYPE(t)                                                       \
  typedef struct {                                                             \
    t data;                                                                    \
    struct LIST_T(t) * next;                                                   \
  } LIST_T(t);

#endif
