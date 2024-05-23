#include "native_functions.h"
#include "types.h"
#include "value.h"

Value _strlen(int argc, Value *argv) {
  Value string = *argv;
  return INT(string.value.vstr.length);
}

Value _strconcat(int argc, Value *argv) {
  Value string = *argv;
  return INT(string.value.vstr.length);
}

Value _print(int argc, Value *argv) {
  printf("%s", argv->value.vstr.chars);
  return (Value){VALUE_VOID};
}

static native_symbol_map builtin_native_fns[2] = {
    {"strlen", NATIVE_FN(_strlen, 1)}, {"print", NATIVE_FN(_print, 1)}};

void add_native_functions(ht *stack) {

  for (int i = 0; i < 2; i++) {
    native_symbol_map t = builtin_native_fns[i];
    ht_set(stack, t.id, t.type);
  }
}
