#ifndef _LANG_OBJ_H
#define _LANG_OBJ_H
#include "common.h"

typedef enum { OBJ_STRING, OBJ_LIST, OBJ_FUNCTION } ObjectType;

typedef struct {
  ObjectType type;
} Object;

typedef struct {
  Object object;
  int length;
  char *chars;
  uint32_t hash;
} ObjString;

struct Chunk {};
typedef struct {
  Object object;
  int arity;
  struct Chunk chunk;
  ObjString *name;
} ObjFunction;

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CHAR_PTR(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_LIST(value) ((ObjList *)AS_OBJ(value))
ObjString *make_string(char *string);

ObjFunction *make_function();
#endif
