#include "bindings.h"
#include "audio/out.h"
#include "audio/sq.h"
#include "channel.h"
#include "ctx.h"
#include "lang/obj.h"
#include "lang/obj_graph.h"

#include <time.h>

Value clock_native(int arg_count, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value square_generator_native(int arg_count, Value *args) {
  ObjGraph *sq = (ObjGraph *)allocate_object(sizeof(ObjGraph), OBJ_GRAPH);
  sq->graph = sq_create(NULL);
  ctx_graph_head()->_graph = sq->graph;
  return (Value){VAL_OBJ, {.object = (Object *)sq}};
}

Value out_native(int arg_count, Value *args) {
  ObjGraph *out = (ObjGraph *)allocate_object(sizeof(ObjGraph), OBJ_GRAPH);
  out->graph = replace_out(
      NULL, arg_count == 0 ? channel_out(0) : channel_out(AS_INTEGER(args[0])));
  return (Value){VAL_OBJ, {.object = (Object *)out}};
}
