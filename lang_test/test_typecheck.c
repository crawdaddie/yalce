#include "../lang/parse.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "serde.h"
#include <stdbool.h>

#define MAKE_FN_TYPE_1(ret_type)                                               \
  {                                                                            \
    T_FN, {                                                                    \
      .T_FN = {.from = NULL, .to = (ret_type) }                                \
    }                                                                          \
  }

#define MAKE_FN_TYPE_2(arg_type, ret_type)                                     \
  ((Type){                                                                     \
      T_FN,                                                                    \
      {.T_FN = {.from = (arg_type), .to = &(Type)MAKE_FN_TYPE_1(ret_type)}}})

#define MAKE_FN_TYPE_3(arg1_type, arg2_type, ret_type)                         \
  {                                                                            \
    T_FN, {                                                                    \
      .T_FN = {                                                                \
        .from = (arg1_type),                                                   \
        .to = &(Type)MAKE_FN_TYPE_2(arg2_type, ret_type)                       \
      }                                                                        \
    }                                                                          \
  }
int main() {
  bool status = true;

#define TEST_SIMPLE_AST_TYPE(i, t)                                             \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *p = parse_input(i, NULL);                                             \
    TypeEnv *env = NULL;                                                       \
    stat &= (infer(p, &env) != NULL);                                          \
    stat &= (types_equal(p->md, t));                                           \
    char buf[100];                                                             \
    if (stat) {                                                                \
      fprintf(stderr, "✅ " i " -> %s\n", type_to_string(t, buf));             \
    } else {                                                                   \
      char buf2[100];                                                          \
      fprintf(stderr, "❌ " i " -> %s (got %s)\n", type_to_string(t, buf),     \
              type_to_string(p->md, buf2));                                    \
    }                                                                          \
    status &= stat;                                                            \
  })

#define TEST_TYPECHECK_FAIL(i)                                                 \
  ({                                                                           \
    Ast *p = parse_input(i, NULL);                                             \
    TypeEnv *env = NULL;                                                       \
    reset_type_var_counter();                                                  \
    Type *succeed = infer(p, &env);                                            \
    if (succeed == NULL) {                                                     \
      fprintf(stderr, "✅ typecheck should fail: " i "\n");                    \
    } else {                                                                   \
      char buf2[100];                                                          \
      fprintf(stderr, "❌ typecheck doesn't fail" i " (got %s)\n",             \
              type_to_string(p->md, buf2));                                    \
    }                                                                          \
    status &= (succeed == NULL);                                               \
  })
  TEST_SIMPLE_AST_TYPE("1", &t_int);
  // TODO: u64 parse not yet implemented
  // TEST_SIMPLE_AST_TYPE("1u64", t_uint64);
  TEST_SIMPLE_AST_TYPE("1.0", &t_num);
  TEST_SIMPLE_AST_TYPE("1.", &t_num);
  TEST_SIMPLE_AST_TYPE("'c'", &t_char);
  // TEST_SIMPLE_AST_TYPE("\"hello\"", t_string);
  TEST_SIMPLE_AST_TYPE("true", &t_bool);
  TEST_SIMPLE_AST_TYPE("false", &t_bool);
  TEST_SIMPLE_AST_TYPE("()", &t_void);

  TEST_SIMPLE_AST_TYPE("(1 + 2) * 8", &t_int);
  TEST_SIMPLE_AST_TYPE("1 + 2.0 * 8", &t_num);
  TEST_SIMPLE_AST_TYPE("1 + 2.0", &t_num);
  TEST_SIMPLE_AST_TYPE("2.0 - 1", &t_num);
  TEST_SIMPLE_AST_TYPE("1 == 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 != 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 < 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 > 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 >= 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 <= 2.0", &t_bool);

  TEST_SIMPLE_AST_TYPE("let x = 1", &t_int);
  TEST_SIMPLE_AST_TYPE("let x = 1 in x + 2.", &t_num);

  Type opt = {T_VARIANT, {}};
  TEST_SIMPLE_AST_TYPE("type Option t =\n"
                       "  | Some of t\n"
                       "  | None\n"
                       "  ;\n",
                       &opt);

#define TLIST(t) ((Type){T_CONS, {.T_CONS = {"List", (Type *[]){t}, 1}}})

  TEST_SIMPLE_AST_TYPE("[1,2,3]", &(TLIST(&t_int)));

  TEST_TYPECHECK_FAIL("[1,2.0,3]");

#define TTUPLE(num, ...)                                                       \
  ((Type){T_CONS, {.T_CONS = {"Tuple", (Type *[]){__VA_ARGS__}, num}}})

  TEST_SIMPLE_AST_TYPE("(1,2,3.9)", &TTUPLE(3, &t_int, &t_int, &t_num, ));

  TEST_SIMPLE_AST_TYPE("let f = fn x -> (1 + 2) * 8 - x;",
                       &MAKE_FN_TYPE_2(&t_int, &t_int));

  TEST_SIMPLE_AST_TYPE("let f = fn (x, y) -> (1 + 2) * 8 - x;",
                       &MAKE_FN_TYPE_2(&t_int, &t_int));

  return !status;
}
