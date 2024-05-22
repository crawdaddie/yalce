#include "extern.h"
#include <dlfcn.h>
#include <stdlib.h>

Value *register_external_symbol(Ast *ast) {
  const char *sym_name = ast->data.AST_EXTERN_FN_DECLARATION.fn_name.chars;
  // typedef int (*printf_t)(const char *, ...);
  dlerror(); // Clear any existing error
  extern_handle_t extern_handle =
      (extern_handle_t)dlsym(RTLD_DEFAULT, sym_name);

  Value *extern_val = malloc(sizeof(Value));
  extern_val->type = VALUE_EXTERN_FN;
  extern_val->value.extern_fn.handle = extern_handle;
  extern_val->value.extern_fn.len = ast->data.AST_LAMBDA.len;
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

Value *call_external_function(Value *extern_fn, Value *vals, int len) {
  extern_handle_t handle = extern_fn->value.extern_fn.handle;

  switch (len) {
  case 0: {
    handle(NULL);
  }

  case 1: {

    handle(to_raw_value(vals));
    break;
  }
  }

  return NULL;
};
