#ifndef _VALUE_H
#define _VALUE_H
#include <stdbool.h>

typedef enum { VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_INTEGER, VAL_OBJ } ValueType;
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    int integer;
  } as;
} Value;

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.integer = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define INTEGER_VAL(value) ((Value){VAL_NUMBER, {.integer = value}})

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_INTEGER(value) ((value).as.integer)
#define AS_NIL(value) ((value).as.integer)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_INTEGER(value) ((value).type == VAL_INTEGER)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_OBJ(value) ((value).type == VAL_OBJ)
#endif
