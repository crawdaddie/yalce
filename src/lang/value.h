#ifndef _VALUE_H
#define _VALUE_H
#include "common.h"
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
    struct Object *object;
  } as;
} Value;

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define AS_BOOL(value) ((value).as.boolean)
#define IS_BOOL(value) ((value).type == VAL_BOOL)

#define NIL_VAL ((Value){VAL_NIL, {.integer = 0}})
#define AS_NIL(value) ((value).as.integer)
#define IS_NIL(value) ((value).type == VAL_NIL)

#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define INTEGER_VAL(value) ((Value){VAL_INTEGER, {.integer = value}})
#define AS_NUMBER(value)                                                       \
  ((value).type == VAL_NUMBER                                                  \
       ? (value).as.number                                                     \
       : ((value).type == VAL_INTEGER ? (value).as.integer : VAL_NIL))
#define AS_INTEGER(value) ((value).as.integer)
#define IS_NUMERIC(value)                                                      \
  ((value).type == VAL_INTEGER || (value).type == VAL_NUMBER)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_INTEGER(value) ((value).type == VAL_INTEGER)

#define OBJ_VAL(object) ((Value){VAL_OBJ, {.object = (Object *)object}})
#define AS_OBJ(value) ((Object *)(value).as.object)
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define IS_LIST(value) is_obj_type(value, OBJ_LIST)
bool values_equal(Value a, Value b);

Value make_string_val(char *chars);

#endif
