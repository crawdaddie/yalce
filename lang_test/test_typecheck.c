#include "../lang/parse.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "serde.h"
#include <stdbool.h>
#include <stdlib.h>

#define MAKE_FN_TYPE_2(arg_type, ret_type)                                     \
  ((Type){T_FN, {.T_FN = {.from = arg_type, .to = ret_type}}})

#define MAKE_FN_TYPE_3(arg1_type, arg2_type, ret_type)                         \
  {                                                                            \
    T_FN, {                                                                    \
      .T_FN = {                                                                \
        .from = &(Type)MAKE_FN_TYPE_2(arg1_type, arg2_type),                   \
        .to = ret_type                                                         \
      }                                                                        \
    }                                                                          \
  }

#define TEST_SIMPLE_AST_TYPE(i, t)                                             \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *p = parse_input(i, NULL);                                             \
    TypeEnv *env = NULL;                                                       \
    stat &= (infer(p, &env) != NULL);                                          \
    stat &= (types_equal(p->md, t));                                           \
    char buf[100] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ " i " => %s\n", type_to_string(t, buf));             \
    } else {                                                                   \
      char buf2[100] = {};                                                     \
      fprintf(stderr, "❌ " i " => %s (got %s)\n", type_to_string(t, buf),     \
              type_to_string(p->md, buf2));                                    \
    }                                                                          \
    status &= stat;                                                            \
  })

#define TEST_SIMPLE_AST_TYPE_ENV(i, t, env)                                    \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *p = parse_input(i, NULL);                                             \
    stat &= (infer(p, &env) != NULL);                                          \
    stat &= (types_equal(p->md, t));                                           \
    char buf[100] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ " i " => %s\n", type_to_string(t, buf));             \
    } else {                                                                   \
      char buf2[100] = {};                                                     \
      fprintf(stderr, "❌ " i " => %s (got %s)\n", type_to_string(t, buf),     \
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
      char buf2[100] = {};                                                     \
      fprintf(stderr, "❌ typecheck doesn't fail" i " (got %s)\n",             \
              type_to_string(p->md, buf2));                                    \
    }                                                                          \
    status &= (succeed == NULL);                                               \
  })
#define TLIST(_t) ((Type){T_CONS, {.T_CONS = {"List", (Type *[]){_t}, 1}}})

#define TTUPLE(num, ...)                                                       \
  ((Type){T_CONS, {.T_CONS = {"Tuple", (Type *[]){__VA_ARGS__}, num}}})

#define arithmetic_var(n)                                                      \
  (Type) {                                                                     \
    T_VAR, {.T_VAR = n},                                                       \
        .implements = (TypeClass *[]){&(TypeClass){.name = "arithmetic"}},     \
        .num_implements = 1                                                    \
  }
#define tcons(name, num, ...)                                                  \
  ((Type){T_CONS, {.T_CONS = {name, (Type *[]){__VA_ARGS__}, num}}})

#define tvar(n)                                                                \
  (Type) { T_VAR, {.T_VAR = n}, }

#define SEP printf("------------------------------------------------\n")

int main() {
  bool status = true;

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
  // TEST_SIMPLE_AST_TYPE("let x = 1 in x + 2.", &t_num);

  ({
    Type t = {T_VAR, {.T_VAR = "t"}};

    Type opt = tcons("Variant", 2, &tcons("Some", 1, &t), &tvar("None"));
    Type opt_func = MAKE_FN_TYPE_2(&t, &opt);

    TEST_SIMPLE_AST_TYPE("type Option t =\n"
                         "  | Some of t\n"
                         "  | None\n"
                         "  ;\n",
                         &opt);
  });

  TEST_SIMPLE_AST_TYPE("[1,2,3]", &(TLIST(&t_int)));
  TEST_TYPECHECK_FAIL("[1,2.0,3]");
  TEST_SIMPLE_AST_TYPE("(1,2,3.9)", &TTUPLE(3, &t_int, &t_int, &t_num, ));

  ({
    Type t_arithmetic = arithmetic_var("t0");
    TEST_SIMPLE_AST_TYPE("let f = fn x -> (1 + 2) * 8 - x;",
                         &MAKE_FN_TYPE_2(&t_arithmetic, &t_arithmetic));
  });

  ({
    Type t0 = tvar("t0");
    TEST_SIMPLE_AST_TYPE("let f = fn x -> x;", &MAKE_FN_TYPE_2(&t0, &t0));
  });

  TEST_SIMPLE_AST_TYPE("let f = fn () -> (1 + 2) * 8;",
                       &MAKE_FN_TYPE_2(&t_void, &t_int));

  ({
    Type t_arithmetic = arithmetic_var("t0");
    Type t1 = tvar("t1");
    TEST_SIMPLE_AST_TYPE(
        "let f = fn (x, y) -> (1 + 2) * 8 - x;",
        &MAKE_FN_TYPE_2(&TTUPLE(2, &t_arithmetic, &t1), &t_arithmetic));
  });

  ({
    Type t = {T_VAR, {.T_VAR = "t"}};
    Type opt = tcons("Variant", 2, &tcons("Some", 1, &t), &tvar("None"));

    Type some_int = tcons("Some", 1, &t_int);
    TEST_SIMPLE_AST_TYPE("type Option t =\n"
                         "  | Some of t\n"
                         "  | None\n"
                         "  ;\n"
                         "let x = Some 1",
                         &some_int);
  });
  ({
    SEP;
    TypeEnv *env = &(TypeEnv){"x", &tvar("x")};

    TEST_SIMPLE_AST_TYPE_ENV("match x with\n"
                             "| 1 -> 1\n"
                             "| 2 -> 0\n"
                             "| _ -> 3\n",
                             &t_int, env);
    TEST_SIMPLE_AST_TYPE_ENV("x", &t_int, env);
  });

  ({
    SEP;
    Type t = {T_VAR, {.T_VAR = "t"}};
    Type opt = tcons("Variant", 2, &tcons("Some", 1, &t), &tvar("None"));
    TypeEnv *env = &(TypeEnv){"Option", &opt};
    Type some_int = tcons("Some", 1, &t_int);
    TEST_SIMPLE_AST_TYPE_ENV("let x = Some 1;\n", &some_int, env);
    TEST_SIMPLE_AST_TYPE_ENV("match x with\n"
                             "  | Some y -> y\n"
                             "  | None -> 0\n"
                             "  ;\n",
                             &t_int, env);
    print_type_env(env);
  });

  ({
    SEP;

    Type t = {T_VAR, {.T_VAR = "t"}};
    Type opt = tcons("Variant", 2, &tcons("Some", 1, &t), &tvar("None"));
    Type some_int = tcons("Some", 1, &t_int);
    TEST_SIMPLE_AST_TYPE("type Option t =\n"
                         "  | Some of t\n"
                         "  | None\n"
                         "  ;\n"
                         "let x = Some 1;\n"
                         "let y = Some 2;\n",
                         &some_int);
  });

  return !status;
}
