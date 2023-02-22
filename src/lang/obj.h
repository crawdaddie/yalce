#ifndef _OBJ_H
#define _OBJ_H
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CHAR_PTR(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_LIST(value) ((ObjList *)AS_OBJ(value))

#endif
