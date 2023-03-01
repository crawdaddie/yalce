#include "obj_function.h"
#include "obj.h"

ObjFunction *make_function() {
  ObjFunction *function =
      (ObjFunction *)allocate_object(sizeof(ObjFunction), OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  init_chunk(&function->chunk);
  return function;
}
