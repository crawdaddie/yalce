#include "extern.h"
#include "serde.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

static value_type type_lookup(ObjString key, ht *stack, int stack_ptr) {

  Value *res;
  int ptr = stack_ptr;

  while (ptr >= 0 &&
         !((res = (Value *)ht_get_hash(stack + ptr, key.chars, key.hash)))) {
    ptr--;
  }
  if (res && res->type == VALUE_TYPE) {
    return res->value.type;
  }
  return VALUE_VOID;
}

Value *register_external_symbol(Ast *ast, ht *stack, int stack_ptr) {
  const char *sym_name = ast->data.AST_EXTERN_FN_DECLARATION.fn_name.chars;
  dlerror(); // Clear any existing error
  void *extern_handle = dlsym(RTLD_DEFAULT, sym_name);

  Value *extern_val = malloc(sizeof(Value));
  extern_val->type = VALUE_EXTERN_FN;
  extern_val->value.extern_fn.handle = extern_handle;
  int len = ast->data.AST_EXTERN_FN_DECLARATION.len;
  extern_val->value.extern_fn.len = len;
  extern_val->value.extern_fn.input_types = malloc(sizeof(value_type) * len);

  for (int i = 0; i < ast->data.AST_EXTERN_FN_DECLARATION.len; i++) {
    ObjString type_id = ast->data.AST_EXTERN_FN_DECLARATION.params[i];
    extern_val->value.extern_fn.input_types[i] =
        type_lookup(type_id, stack, stack_ptr);
  }

  ObjString ret_type_id = ast->data.AST_EXTERN_FN_DECLARATION.return_type;
  extern_val->value.extern_fn.return_type =
      type_lookup(ret_type_id, stack, stack_ptr);

  return extern_val;
}

void *to_raw_value(Value *val) {
  if (!val) {
    return NULL;
  }
  switch (val->type) {
  case VALUE_INT:
    return (void *)val->value.vint;

  case VALUE_NUMBER:
    return NULL;

  case VALUE_STRING:
    return val->value.vstr.chars;

  case VALUE_BOOL:
    return (void *)val->value.vbool;

  case VALUE_VOID:
    return NULL;

  default:
    return NULL;
  }
}
#define TO_C_VAL(value)                                                        \
  ({                                                                           \
    __typeof__(value.value.vint) result;                                       \
    switch (value.type) {                                                      \
    case VALUE_INT:                                                            \
      result = value.value.vint;                                               \
      break;                                                                   \
    case VALUE_NUMBER:                                                         \
      result = value.value.vnum;                                               \
      break;                                                                   \
    case VALUE_STRING:                                                         \
      result = value.value.vstr.chars;                                         \
      break;                                                                   \
    case VALUE_BOOL:                                                           \
      result = value.value.vbool;                                              \
      break;                                                                   \
    case VALUE_OBJ:                                                            \
      result = value.value.vobj;                                               \
      break;                                                                   \
    default:                                                                   \
      fprintf(stderr, "Unsupported type: %d\n", value.type);                   \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
    result;                                                                    \
  })

#define PASS_ARGS_TO_HANDLE(handle, input_vals, num_vals)                      \
  switch (num_vals) {                                                          \
  case 0:                                                                      \
    return handle();                                                           \
  case 1:                                                                      \
    return handle(TO_C_VAL(input_vals[0]));                                    \
  case 2:                                                                      \
    return handle(TO_C_VAL(input_vals[0]), TO_C_VAL(input_vals[1]));           \
  case 3:                                                                      \
    return handle(TO_C_VAL(input_vals[0]), TO_C_VAL(input_vals[1]),            \
                  TO_C_VAL(input_vals[2]));                                    \
  case 4:                                                                      \
    return handle(TO_C_VAL(input_vals[0]), TO_C_VAL(input_vals[1]),            \
                  TO_C_VAL(input_vals[2]), TO_C_VAL(input_vals[3]));           \
  case 5:                                                                      \
    return handle(TO_C_VAL(input_vals[0]), TO_C_VAL(input_vals[1]),            \
                  TO_C_VAL(input_vals[2]), TO_C_VAL(input_vals[3]),            \
                  TO_C_VAL(input_vals[4]));                                    \
  default:                                                                     \
    fprintf(stderr, "Unsupported number of arguments: %d\n", num_vals);        \
    exit(EXIT_FAILURE);                                                        \
  }

Value *call_external_function(Value *extern_fn, Value *input_vals, int len,
                              Value *return_val) {

  value_type return_type = extern_fn->value.extern_fn.return_type;
  switch (return_type) {
  case VALUE_INT: {
    extern_int handle = extern_fn->value.extern_fn.handle;
    return_val->type = return_type;
    return_val->value.vint = handle(input_vals);
    return return_val;
  }

  case VALUE_NUMBER: {
    extern_double handle = extern_fn->value.extern_fn.handle;
    return_val->type = return_type;
    return_val->value.vnum = handle(input_vals);
    return return_val;
  }

  case VALUE_OBJ: {
    extern_void_ptr handle = extern_fn->value.extern_fn.handle;
    return_val->type = return_type;
    return_val->value.vobj = handle(input_vals);
    return return_val;
  }

  case VALUE_STRING: {
    extern_str handle = extern_fn->value.extern_fn.handle;
    return_val->type = return_type;
    char *chars = handle(input_vals);
    return_val->value.vstr.chars = chars;
    int len = strlen(chars);
    return_val->value.vstr.length = len;
    return_val->value.vstr.hash = hash_string(chars, len);
    return return_val;
  }

  case VALUE_VOID:
  default: {
    extern_void handle = extern_fn->value.extern_fn.handle;
    handle(input_vals);
    return_val->type = VALUE_VOID;
    return return_val;
  }
  }
};
