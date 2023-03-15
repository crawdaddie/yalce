#include "bindings.h"
#include "channel.h"
#include "ctx.h"
#include "lang/dbg.h"
#include "lang/obj.h"
#include "lang/obj_node.h"
#include "lang/vm.h"

#include "audio/sq.h"
#include <time.h>

static Value clock_native(int arg_count, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value sq_native(int arg_count, Value *args) {
  ObjNode *obj = (ObjNode *)allocate_object(sizeof(ObjNode), OBJ_NODE);
  obj->node = make_sq_node();

  return (Value){VAL_OBJ, {.object = (Object *)obj}};
}

static Value print_native(int arg_count, Value *args) {
  for (int i = 0; i < arg_count; i++) {
    print_value(args[i]);
    printf("\n");
  }
  return VOID_VAL;
};

static Value add_after_native(int arg_count, Value *args) {
  printf("add after - is nil? %d\n", IS_NIL(args[0]));
  if (IS_NIL(args[0])) {
    return VOID_VAL;
  }

  struct Node *after = AS_NODE(args[0]);

  // for (int i = 1; i < arg_count; i++) {
  //   Value node_arg = args[i];
  //   ObjNode *obj_node = (ObjNode *)AS_OBJ(node_arg);
  //   Node *node = obj_node->node;
  // }
  return VOID_VAL;
}

void bindings_setup() {
  define_native("print", print_native);
  define_native("clock", clock_native);
  define_native("sq", sq_native);
  define_native("play_after", add_after_native);
}
