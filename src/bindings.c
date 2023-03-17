#include "bindings.h"
#include "channel.h"
#include "ctx.h"
#include "lang/dbg.h"
#include "lang/obj.h"
#include "lang/obj_array.h"
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
static Value size_obj_native(int arg_count, Value *args) {
  return INTEGER_VAL(sizeof(args[0]));
}

static Value length_array_native(int arg_count, Value *args) {
  return INTEGER_VAL(((ObjArray *)AS_OBJ(args[0]))->size);
}
static Value buf_int_native(int arg_count, Value *args) {
  int size = AS_INTEGER(args[0]);
  return (Value){VAL_OBJ, {.object = (Object *)make_buffer(size, sizeof(int))}};
}

static Value buf_double_native(int arg_count, Value *args) {
  int size = AS_INTEGER(args[0]);
  return (Value){VAL_OBJ,
                 {.object = (Object *)make_buffer(size, sizeof(double))}};
}

static Value dump_stack(int arg_count, Value *args) {
  print_stack();
  return VOID_VAL;
}

static Value array_set_native(int arg_count, Value *args) {
  ObjArray *array = AS_OBJ(args[0]);
  int index = AS_INTEGER(args[1]);
  Value val = args[2];
  if (((Object *)array)->type == OBJ_BUFFER) {
    return VOID_VAL;
  }
  array->values[index] = val;
  return VOID_VAL;
}
void bindings_setup() {
  define_native("print", print_native);
  define_native("clock", clock_native);
  define_native("sq", sq_native);
  define_native("play_after", add_after_native);
  define_native("size", size_obj_native);
  define_native("length", length_array_native);
  define_native("int_b", buf_int_native);
  define_native("dbl_b", buf_double_native);
  define_native("dump_stack", dump_stack);

  define_native("set", array_set_native);
}
