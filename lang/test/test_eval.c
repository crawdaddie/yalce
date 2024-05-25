#include "builtins.h"
#include "eval.h"
#include "ht.h"
#include "parse.h"
#include "serde.h"
#include "synth_functions.h"
#include "types.h"
#include "value.h"
#include <stdbool.h>
#include <string.h>

Value test_eval(Ast *ast, native_symbol_map *ctx, int ctx_size, ht *stack) {
  for (int i = 0; i < STACK_MAX; i++) {
    ht_init(stack + i);
  }
  add_type_lookups(stack);
  add_native_functions(stack);
  add_synth_functions(stack);

  for (int i = 0; i < ctx_size; i++) {
    native_symbol_map t = ctx[i];
    ht_set(stack, t.id, t.type);
  }

  return eval(ast, stack, 0);
}

#define ast_binop(_op, l, r)                                                   \
  &(Ast) {                                                                     \
    AST_BINOP, {                                                               \
      .AST_BINOP = {                                                           \
        .op = _op,                                                             \
        .left = l,                                                             \
        .right = r,                                                            \
      }                                                                        \
    }                                                                          \
  }
#define ast_int(i)                                                             \
  &(Ast) {                                                                     \
    AST_INT, {                                                                 \
      .AST_INT = {.value = i }                                                 \
    }                                                                          \
  }

#define ast_num(i)                                                             \
  &(Ast) {                                                                     \
    AST_NUMBER, {                                                              \
      .AST_NUMBER = {.value = i }                                              \
    }                                                                          \
  }

#define ast_app(fn, _len, ...)                                                 \
  &(Ast) {                                                                     \
    AST_APPLICATION, {                                                         \
      .AST_APPLICATION = {                                                     \
        .function = fn,                                                        \
        .len = _len,                                                           \
        .args = &(Ast *[]){__VA_ARGS__}                                        \
      }                                                                        \
    }                                                                          \
  }
#define ast_id(id)                                                             \
  &(Ast) {                                                                     \
    AST_IDENTIFIER, {                                                          \
      .AST_IDENTIFIER = { id, strlen(id) }                                     \
    }                                                                          \
  }
#define ast_body(len, ...)                                                     \
  &(Ast) {                                                                     \
    AST_BODY, {                                                                \
      .AST_BODY = { len, &(Ast *[]){__VA_ARGS__} }                             \
    }                                                                          \
  }

int main() {
  bool status = true;

  ht stack[STACK_MAX];
  Value res;
  int test_result_bool;
  char *title;
#define TEST(_title, _eval, _test)                                             \
  title = _title;                                                              \
  res = _eval;                                                                 \
  ht_reinit(stack);                                                            \
  _test;                                                                       \
  if (test_result_bool) {                                                      \
    printf("✅ %s\n", title);                                                  \
  } else {                                                                     \
    printf("❌ %s\n", title);                                                  \
  }                                                                            \
  status &= test_result_bool;

  TEST(
      "simple arithmetic 98. * 2",
      test_eval(ast_binop(TOKEN_STAR, ast_num(98), ast_int(2)), NULL, 0, stack),
      test_result_bool = (res.type == VALUE_NUMBER && res.value.vnum == 196.))

  TEST("simple arithmetic 98. / 2",
       test_eval(ast_binop(TOKEN_SLASH, ast_num(98), ast_int(2)), NULL, 0,
                 stack),
       (res.type == VALUE_NUMBER && res.value.vnum == 49.))

  TEST("simple arithmetic 97 / 2 == 48",
       test_eval(ast_binop(TOKEN_SLASH, ast_int(97), ast_int(2)), NULL, 0,
                 stack),
       test_result_bool = (res.type == VALUE_INT && res.value.vint == 48))

#define TEST_CTX                                                               \
  (native_symbol_map[]) {                                                      \
    {                                                                          \
      "f", &(Value) {                                                          \
        VALUE_FN, {                                                            \
          .function = {                                                        \
            .len = 2,                                                          \
            .params =                                                          \
                (ObjString[]){                                                 \
                    {.chars = "a"},                                            \
                    {.chars = "b"},                                            \
                },                                                             \
            .fn_name = "f",                                                    \
            .scope_ptr = 0,                                                    \
            .body = ast_body(1, ast_int(100))                                  \
          }                                                                    \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }
  TEST("simple function application",
       test_eval(ast_app(ast_id("f"), 2, ast_int(1), ast_int(2)), TEST_CTX, 1,
                 stack),
       test_result_bool = (res.type == VALUE_INT && res.value.vint == 100))

  TEST("partial function application (currying)",
       test_eval(ast_app(ast_id("f"), 1, ast_int(1)), TEST_CTX, 1, stack),
       test_result_bool =
           (res.type == VALUE_FN && res.value.function.num_partial_args == 1 &&
            res.value.function.partial_args[0].type == VALUE_INT &&
            res.value.function.partial_args[0].value.vint == 1))

#undef TEST_CTX

  return status ? 0 : 1;
}
#undef ast_binop
#undef ast_int
#undef ast_num
#undef ast_app
#undef ast_id
#undef ast_body
