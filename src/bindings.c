#include "bindings.h"
#include "audio/sq.h"
#include "lang/obj.h"
#include "lang/obj_graph.h"
#include "lang/vm.h"

#include <time.h>

Value clock_native(int arg_count, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
Value square_generator_native(int arg_count, Value *args) {
  ObjGraph *sq = (ObjGraph *)allocate_object(sizeof(ObjGraph), OBJ_GRAPH);
  sq->graph = sq_create(NULL);
  return (Value){VAL_OBJ, {.object = (Object *)sq}};
}
Value out_native(int arg_count, Value *args) {
  ObjGraph *out = (ObjGraph *)allocate_object(sizeof(ObjGraph), OBJ_GRAPH);
  return (Value){VAL_OBJ, {.object = (Object *)out}};
}
