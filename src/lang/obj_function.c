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

ObjNative *make_native(NativeFn function) {
  ObjNative *native =
      (ObjNative *)allocate_object(sizeof(ObjNative), OBJ_NATIVE);
  native->function = function;
  return native;
}

ObjClosure *make_closure(ObjFunction *function) {
  ObjClosure *closure =
      (ObjClosure *)allocate_object(sizeof(ObjClosure), OBJ_CLOSURE);
  closure->function = function;
  return closure;
}
