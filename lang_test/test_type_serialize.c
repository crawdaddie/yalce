#include "../lang/types/util.h"
#include "test_typecheck_utils.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

TypeSerBuf *write_types(size_t len, Type **types) {
  TypeSerBuf *buf = create_type_ser_buffer(10);
  for (size_t i = 0; i < len; i++) {
    serialize_type(types[i], buf);
  }
  return buf;
}

bool TEST_CONCAT_TYPES(size_t len, Type **types, const char *expected) {
  TypeSerBuf *b = write_types(len, types);
  bool status = strcmp(expected, (char *)b->data) == 0;
  if (status) {
    printf("✅\e[1m%s\e[0m\n", expected);
  } else {
    printf("❌\e[1mexpected %s\e[0m got %s\n", expected, (char *)b->data);
  }
  free(b->data);
  free(b);
  return status;
}

int main() {
  bool status = true;
  status &= TEST_CONCAT_TYPES(3, T(&t_int, &t_int, &t_int), "IntIntInt");
  status &=
      TEST_CONCAT_TYPES(3, T(&t_num, &t_string, &t_int), "DoubleStringInt");

  status &= TEST_CONCAT_TYPES(
      3, T(TUPLE(3, &t_num, &t_string, &t_int), &t_string, &t_int),
      "cons(Tuple, 3, Double, String, Int)StringInt");

  return !status;
}
