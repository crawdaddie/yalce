#ifndef _VALUE_H
#define _VALUE_H
#include "obj.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum { VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_INTEGER, VAL_OBJ } ValueType;
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    int integer;
    Object *object;
  } as;
} Value;

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.integer = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define INTEGER_VAL(value) ((Value){VAL_INTEGER, {.integer = value}})

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value)                                                       \
  ((value).type == VAL_NUMBER                                                  \
       ? (value).as.number                                                     \
       : ((value).type == VAL_INTEGER ? (value).as.integer : VAL_NIL))
#define AS_INTEGER(value) ((value).as.integer)
#define AS_NIL(value) ((value).as.integer)

#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_OBJ(value) ((value).as.object)

#define OBJ_VAL(object) ((Value){VAL_OBJ, {.object = (Object *)object}})
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_INTEGER(value) ((value).type == VAL_INTEGER)

#define IS_NUMERIC(value)                                                      \
  ((value).type == VAL_INTEGER || (value).type == VAL_NUMBER)

#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

Value make_string(char *string);

#endif
