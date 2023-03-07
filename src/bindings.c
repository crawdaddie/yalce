#include "bindings.h"
#include "audio/out.h"
#include "audio/sq.h"
#include "channel.h"
#include "ctx.h"
#include "lang/dbg.h"
#include "lang/obj.h"
#include "lang/obj_graph.h"

#include <time.h>

Value clock_native(int arg_count, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value square_generator_native(int arg_count, Value *args) {
  ObjGraph *sq = (ObjGraph *)allocate_object(sizeof(ObjGraph), OBJ_GRAPH);
  Signal freq = new_signal(arg_count == 0 ? 220 : AS_NUMBER(args[0]), 1);
  sq->graph = sq_create(NULL, &freq);
  ctx_graph_head()->_graph = sq->graph;
  return (Value){VAL_OBJ, {.object = (Object *)sq}};
}

Value out_native(int arg_count, Value *args) {
  ObjGraph *out = (ObjGraph *)allocate_object(sizeof(ObjGraph), OBJ_GRAPH);
  out->graph = replace_out(
      NULL, arg_count == 0 ? channel_out(0) : channel_out(AS_INTEGER(args[0])));
  return (Value){VAL_OBJ, {.object = (Object *)out}};
}

Value print_native(int arg_count, Value *args) {
  for (int i = 0; i < arg_count; i++) {
    print_value(args[i]);
    printf("\n");
  }
  return VOID_VAL;
};
