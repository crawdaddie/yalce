#ifndef _OBJ_H
#define _OBJ_H
typedef enum { OBJ_STRING } ObjectType;
typedef struct {
  ObjectType type;
  void *value;
} Object;
#endif
