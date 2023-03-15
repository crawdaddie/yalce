#ifndef _LANG_OBJ_NODE_H
#define _LANG_OBJ_NODE_H

#include "../node.h"
#include "obj.h"
typedef struct {
  Object object;
  Node *node;
} ObjNode;
#endif
