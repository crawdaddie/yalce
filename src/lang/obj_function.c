#include "obj_function.h"
#include "lang_memory.h"
#include "obj.h"

ObjFunction *make_function() {
  ObjFunction *function =
      (ObjFunction *)allocate_object(sizeof(ObjFunction), OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
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
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalue_count);
  for (int i = 0; i < function->upvalue_count; i++) {
    upvalues[i] = NULL;
  }
  ObjClosure *closure =
      (ObjClosure *)allocate_object(sizeof(ObjClosure), OBJ_CLOSURE);

  closure->function = function;
  closure->upvalue_count = function->upvalue_count;
  closure->upvalues = upvalues;
  return closure;
}

ObjUpvalue *make_upvalue(Value *slot) {
  ObjUpvalue *upvalue =
      (ObjUpvalue *)allocate_object(sizeof(ObjUpvalue), OBJ_UPVALUE);
  upvalue->location = slot;
  upvalue->next = NULL;
  upvalue->closed = NIL_VAL;
  return upvalue;
};
