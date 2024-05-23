#include "native_functions.h"
#include "types.h"
#include "value.h"
#define INT(i)                                                                 \
  (Value) {                                                                    \
    VALUE_INT, { .vint = i }                                                   \
  }

#define NATIVE_FN(_handle, _len)                                               \
  &(Value) {                                                                   \
    .type = VALUE_NATIVE_FN, .value = {                                        \
      .native_fn = {.handle = &_handle, .len = _len}                           \
    }                                                                          \
  }

Value _strlen(int argc, Value *argv) {
  Value string = *argv;
  return INT(string.value.vstr.length);
}

Value _strconcat(int argc, Value *argv) {
  Value string = *argv;
  return INT(string.value.vstr.length);
}

static type_map builtin_native_fns[1] = {{"strlen", NATIVE_FN(_strlen, 1)}};

void add_native_functions(ht *stack) {

  for (int i = 0; i < 1; i++) {
    type_map t = builtin_native_fns[i];
    ht_set(stack, t.id, t.type);
  }
}
