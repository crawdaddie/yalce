#include "../lang/parse.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "serde.h"
#include <stdbool.h>
#include <stdlib.h>

#define TITLE(msg)                                                             \
  ({                                                                           \
    SEP;                                                                       \
    printf(msg "\n");                                                          \
    SEP;                                                                       \
  });

#define TEST_SIMPLE_AST_TYPE(i, t)                                             \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *p = parse_input(i, NULL);                                             \
    TICtx ctx = {};                                                            \
    stat &= (infer(p, &ctx) != NULL);                                          \
    stat &= (types_equal(p->md, t));                                           \
    char buf[100] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ " i " => %s\n", type_to_string(t, buf));             \
    } else {                                                                   \
      char buf2[100] = {};                                                     \
      fprintf(stderr, "❌ " i " => %s (got %s)\n", type_to_string(t, buf),     \
              type_to_string(p->md, buf2));                                    \
      fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                          \
    }                                                                          \
    status &= stat;                                                            \
    p;                                                                         \
  })

#define TEST(res, msg)                                                         \
  ({                                                                           \
    if (res) {                                                                 \
      fprintf(stderr, "✅ " msg "\n");                                         \
    } else {                                                                   \
      fprintf(stderr, "❌ " msg "\n");                                         \
    }                                                                          \
    res;                                                                       \
  })

#define TEST_SIMPLE_AST_TYPE_ENV(i, t, env)                                    \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *p = parse_input(i, NULL);                                             \
    TICtx ctx = {.env = env};                                                  \
    Type *infer_res = infer(p, &ctx);                                          \
    stat &= ((t != NULL) ? (infer_res != NULL) : infer_res == NULL);           \
    stat &= (types_equal(p->md, t));                                           \
    char buf[100] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ " i " => %s\n", type_to_string(t, buf));             \
    } else {                                                                   \
      char buf2[100] = {};                                                     \
      fprintf(stderr, "❌ " i " => %s (got %s)\n", type_to_string(t, buf),     \
              type_to_string(p->md, buf2));                                    \
      fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                          \
    }                                                                          \
    status &= stat;                                                            \
    p;                                                                         \
  })

#define TEST_TYPECHECK_FAIL(i)                                                 \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    Ast *p = parse_input(i, NULL);                                             \
    TypeEnv *env = NULL;                                                       \
    TICtx ctx = {.env = env};                                                  \
    Type *succeed = infer(p, &ctx);                                            \
    if (succeed == NULL) {                                                     \
      fprintf(stderr, "✅ typecheck should fail: " i "\n");                    \
    } else {                                                                   \
      char buf2[100] = {};                                                     \
      fprintf(stderr, "❌ typecheck doesn't fail " i " (got %s)\n",            \
              type_to_string(p->md, buf2));                                    \
      fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                          \
    }                                                                          \
    status &= (succeed == NULL);                                               \
  })

#define TEST_TYPECHECK_ENV_FAIL(i, env)                                        \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    Ast *p = parse_input(i, NULL);                                             \
    TypeEnv *env = NULL;                                                       \
    TICtx ctx = {.env = env};                                                  \
    Type *succeed = infer(p, &ctx);                                            \
    if (succeed == NULL) {                                                     \
      fprintf(stderr, "✅ typecheck should fail: " i "\n");                    \
    } else {                                                                   \
      char buf2[100] = {};                                                     \
      fprintf(stderr, "❌ typecheck doesn't fail " i " (got %s)\n",            \
              type_to_string(p->md, buf2));                                    \
      fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                          \
    }                                                                          \
    status &= (succeed == NULL);                                               \
  })

#define TLIST(_t)                                                              \
  ((Type){T_CONS, {.T_CONS = {TYPE_NAME_LIST, (Type *[]){_t}, 1}}})
#define TARRAY(_t)                                                             \
  ((Type){T_CONS, {.T_CONS = {TYPE_NAME_ARRAY, (Type *[]){_t}, 1}}})

#define TTUPLE(num, ...)                                                       \
  ((Type){T_CONS, {.T_CONS = {TYPE_NAME_TUPLE, (Type *[]){__VA_ARGS__}, num}}})

#define tcons(name, num, ...)                                                  \
  ((Type){T_CONS, {.T_CONS = {name, (Type *[]){__VA_ARGS__}, num}}})

#define tvar(n)                                                                \
  (Type) { T_VAR, {.T_VAR = n}, }

#define TNONE                                                                  \
  ((Type){T_CONS, {.T_CONS = {"None", .args = NULL, .num_args = 0}}})
#define RESET reset_type_var_counter()

#define SEP printf("------------------------------------------------\n")

int main() {

  initialize_builtin_types();
  bool status = true;

  TEST_SIMPLE_AST_TYPE("1", &t_int);
  // TODO: u64 parse not yet implemented
  // TEST_SIMPLE_AST_TYPE("1u64", t_uint64);
  TEST_SIMPLE_AST_TYPE("1.0", &t_num);
  TEST_SIMPLE_AST_TYPE("1.", &t_num);
  TEST_SIMPLE_AST_TYPE("'c'", &t_char);
  TEST_SIMPLE_AST_TYPE("\"hello\"", &t_string);
  TEST_SIMPLE_AST_TYPE("true", &t_bool);
  TEST_SIMPLE_AST_TYPE("false", &t_bool);
  TEST_SIMPLE_AST_TYPE("()", &t_void);

  TEST_SIMPLE_AST_TYPE("(1 + 2) * 8", &t_int);
  TEST_SIMPLE_AST_TYPE("1 + 2.0 * 8", &t_num);
  TEST_SIMPLE_AST_TYPE("1 + 2.0", &t_num);
  TEST_SIMPLE_AST_TYPE("2.0 - 1", &t_num);
  TEST_SIMPLE_AST_TYPE("1 == 1", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 == 2", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 == 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 != 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 < 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 > 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 >= 2.0", &t_bool);
  TEST_SIMPLE_AST_TYPE("1 <= 2.0", &t_bool);

  TEST_SIMPLE_AST_TYPE("let x = 1", &t_int);

  ({
    Type t_arithmetic = arithmetic_var("t1");

    TEST_SIMPLE_AST_TYPE(
        "1 + x", &TYPECLASS_RESOLVE("arithmetic", &t_int, &t_arithmetic, NULL));
  });

  /*
  ({
    Type t = {T_VAR, {.T_VAR = "t"}};

    // Type opt = MAKE_FN_TYPE_2(
    //     &t, &tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t), &TNONE));
    //
    Type opt = tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t), &TNONE);

    TEST_SIMPLE_AST_TYPE("type Option t =\n"
                         "  | Some of t\n"
                         "  | None\n"
                         "  ;\n",
                         &opt);
  });

*/
  TEST_SIMPLE_AST_TYPE("[1,2,3]", &(TLIST(&t_int)));
  TEST_TYPECHECK_FAIL("[1,2.0,3]");

  TEST_SIMPLE_AST_TYPE("[|1,2,3|]", &(TARRAY(&t_int)));
  TEST_TYPECHECK_FAIL("[|1,2.0,3|]");
  TEST_SIMPLE_AST_TYPE("(1,2,3.9)", &TTUPLE(3, &t_int, &t_int, &t_num, ));

  ({
    Type t_arithmetic = arithmetic_var("t1");

    TEST_SIMPLE_AST_TYPE(
        "let f = fn x -> (1 + 2) * 8 - x;",
        &MAKE_FN_TYPE_2(
            &t_arithmetic,
            &TYPECLASS_RESOLVE("arithmetic", &t_int, &t_arithmetic, NULL)));
  });

  ({
    Type t1 = tvar("t1");
    TEST_SIMPLE_AST_TYPE("let f = fn x -> x;", &MAKE_FN_TYPE_2(&t1, &t1));
  });

  TEST_SIMPLE_AST_TYPE("let f = fn () -> (1 + 2) * 8;",
                       &MAKE_FN_TYPE_2(&t_void, &t_int));

  ({
    Type t_arithmetic0 = arithmetic_var("t0");
    Type t_arithmetic1 = arithmetic_var("t1");
    Type t_arithmetic2 = arithmetic_var("t2");
    Type t_arithmetic3 = arithmetic_var("t3");

    TEST_SIMPLE_AST_TYPE(
        "let f = fn x y z -> x + y + z;",
        &MAKE_FN_TYPE_4(
            &t_arithmetic3, &t_arithmetic2, &t_arithmetic1,
            &TYPECLASS_RESOLVE("arithmetic",
                               &TYPECLASS_RESOLVE("arithmetic", &t_arithmetic3,
                                                  &t_arithmetic2, NULL),
                               &t_arithmetic1, NULL)));
  });

  ({
    RESET;
    Type t_arithmetic = arithmetic_var("t1");
    Type t1 = tvar("t2");
    TEST_SIMPLE_AST_TYPE(
        "let f = fn (x, y) -> (1 + 2) * 8 - x;",
        &MAKE_FN_TYPE_2(
            &TTUPLE(2, &t_arithmetic, &t1),
            &TYPECLASS_RESOLVE("arithmetic", &t_int, &t_arithmetic, NULL)));
  });

  ({
    RESET;
    Type t = {T_VAR, {.T_VAR = "t"}};
    Type opt = tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t), &TNONE);

    Type some_int = tcons("Some", 1, &t_int);
    TEST_SIMPLE_AST_TYPE("type Option t =\n"
                         "  | Some of t\n"
                         "  | None\n"
                         "  ;\n"
                         "let x = Some 1;\n",
                         &some_int);
  });

  ({
    SEP;
    RESET;
    Type some_int = tcons("Some", 1, &t_int);
    TEST_SIMPLE_AST_TYPE("type Option t =\n"
                         "  | Some of t\n"
                         "  | None\n"
                         "  ;\n"
                         "let x = Some 1;\n"
                         "let y = Some 2;\n",
                         &some_int);
  });

  ({
    RESET;
    SEP;
    TypeEnv *env = &(TypeEnv){"x", &tvar("t0")};

    TEST_SIMPLE_AST_TYPE_ENV("match x with\n"
                             "| 1 -> 1\n"
                             "| 2 -> 0\n"
                             "| _ -> 3\n",
                             &t_int, env);
    TEST_SIMPLE_AST_TYPE_ENV("x", &t_int, env);
  });

  // ({
  //   SEP;
  //   TEST_TYPECHECK_FAIL("type Option t =\n"
  //                       "  | Some of t\n"
  //                       "  | None\n"
  //                       "  ;\n"
  //                       "let x = Some 1;\n"
  //                       "match x with\n"
  //                       "  | Some 1 -> 1\n"
  //                       "  | Some 1.0 -> 1\n"
  //                       "  | None -> 0\n"
  //                       "  ;\n");
  // });

  ({
    TITLE("match result different types")
    TEST_TYPECHECK_FAIL("type Option t =\n"
                        "  | Some of t\n"
                        "  | None\n"
                        "  ;\n"
                        "let x = Some 1;\n"
                        "match x with\n"
                        "  | Some 1 -> 1\n"
                        "  | None -> 2.0\n"
                        "  ;\n");
  });

  ({
    TITLE("variant type matching")
    Type opt_int =
        tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);

    TEST_SIMPLE_AST_TYPE("type Option t =\n"
                         "  | Some of t\n"
                         "  | None\n"
                         "  ;\n"
                         "let f = fn x ->\n"
                         "match x with\n"
                         "  | Some 1 -> 1\n"
                         "  | None -> 0\n"
                         "  ;;\n",
                         &MAKE_FN_TYPE_2(&opt_int, &t_int));
  });

  ({
    TITLE("## matching on tuple")
    RESET;
    ;
    Type tuple = tcons(TYPE_NAME_TUPLE, 2, &t_int, &t_int);
    TEST_SIMPLE_AST_TYPE("let f = fn x ->\n"
                         "match x with\n"
                         "  | (1, 2) -> 1\n"
                         "  | (1, 3) -> 0\n"
                         "  ;;\n",
                         &MAKE_FN_TYPE_2(&tuple, &t_int));
  });

  ({
    TITLE("## matching on tupl containing var")
    RESET;
    ;
    Type tuple = tcons(TYPE_NAME_TUPLE, 2, &t_int, &t_int);
    TEST_SIMPLE_AST_TYPE("let f = fn x ->\n"
                         "match x with\n"
                         "  | (1, y) -> y\n"
                         "  | (1, 3) -> 0\n"
                         "  ;;\n",
                         &MAKE_FN_TYPE_2(&tuple, &t_int));
  });

  // ({
  //   TITLE("inexhaustive match expr")
  //   RESET;
  //   ;
  //
  //   TEST_TYPECHECK_FAIL("type Option t =\n"
  //                       "  | Some of t\n"
  //                       "  | None\n"
  //                       "  ;\n"
  //                       "let x = Some 1;\n"
  //                       "match x with\n"
  //                       "  | Some 1 -> 1\n"
  //                       "  ;\n");
  // });

  ({
    SEP;
    RESET;
    ;
    Type fn = MAKE_FN_TYPE_3(&t_int, &t_num, &t_int);
    TEST_SIMPLE_AST_TYPE("let ex_fn = extern fn Int -> Double -> Int;", &fn);
  });
  ({
    SEP;
    RESET;
    TypeEnv *env = NULL;
    Type t_arithmetic = arithmetic_var("t1");
    TEST_SIMPLE_AST_TYPE_ENV(
        "let f = fn x -> (1 + 2) * 8 - x;",
        &MAKE_FN_TYPE_2(&t_arithmetic, &TYPECLASS_RESOLVE("arithmetic", &t_int,
                                                          &t_arithmetic, NULL)),
        env);
    TEST_SIMPLE_AST_TYPE_ENV("f 1", &t_int, env);
    TEST_SIMPLE_AST_TYPE_ENV("f 1.", &t_num, env);
  });
  ({
    SEP;
    RESET;
    Type t1 = tvar("t1");
    Type t2 = tvar("t2");
    TypeEnv *env = NULL;
    env = env_extend(env, "s", &t1);
    env = env_extend(env, "x", &t2);
    TEST_SIMPLE_AST_TYPE_ENV(
        "s + x", &TYPECLASS_RESOLVE("arithmetic", &t1, &t2, NULL), env);
  });

  ({
    TITLE("var types in arithmetic expr");
    RESET;
    TypeEnv *env = NULL;
    Type t_arithmetic2 = arithmetic_var("t2");
    Type t_arithmetic1 = arithmetic_var("t1");

    TEST_SIMPLE_AST_TYPE_ENV(
        "let f = fn x y -> x + y;",
        &MAKE_FN_TYPE_3(&t_arithmetic2, &t_arithmetic1,
                        &TYPECLASS_RESOLVE("arithmetic", &t_arithmetic2,
                                           &t_arithmetic1, NULL)),
        env);

    TEST_SIMPLE_AST_TYPE_ENV(
        "f 1",
        &MAKE_FN_TYPE_2(
            &t_arithmetic1,
            &TYPECLASS_RESOLVE("arithmetic", &t_int, &t_arithmetic1, NULL)),
        env);
  });

  /*
  ({
    RESET;
    Type t0 = tvar("t0");
    TypeEnv *env = NULL;
    Type *u = unify(&t0, &t_int, &env);
    status &= TEST(types_equal(u, &t_int), "unify t0 with Int");
  });
  */

  ({
    RESET;
    SEP;
    TypeEnv *env = NULL;
    Type t_arithmetic1 = arithmetic_var("t1");
    Type t_arithmetic2 = arithmetic_var("t2");
    Type t4 = tvar("t4");
    Type t5 = tvar("t5");
    Type t6 = tvar("t6");
    Type t7 = tvar("t7");

    TEST_SIMPLE_AST_TYPE_ENV(
        "let sum = fn a b -> a + b;;",
        &MAKE_FN_TYPE_3(&t_arithmetic2, &t_arithmetic1,
                        &TYPECLASS_RESOLVE("arithmetic", &t_arithmetic2,
                                           &t_arithmetic1, NULL)),
        env);

    TEST_SIMPLE_AST_TYPE_ENV(
        "let proc = fn f a b -> f a b;;\n",
        &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t5, &t4, &t7), &t5, &t4, &t7), env);

    TEST_SIMPLE_AST_TYPE_ENV("proc sum 1 2", &t_int, env);
  });

  ({
    RESET;
    SEP;
    TypeEnv *env = NULL;
    TEST_SIMPLE_AST_TYPE_ENV("let fib = fn x ->\n"
                             "  match x with\n"
                             "  | 0 -> 0\n"
                             "  | 1 -> 1\n"
                             "  | _ -> (fib (x - 1)) + (fib (x - 2))\n"
                             ";;\n",
                             &MAKE_FN_TYPE_2(&t_int, &t_int), env);

    TEST_SIMPLE_AST_TYPE_ENV("fib 50", &t_int, env);
  });

  ({
    SEP;
    Type tenum = tcons(TYPE_NAME_VARIANT, 3, &tcons("A", 0, NULL),
                       &tcons("B", 0, NULL), &tcons("C", 0, NULL));
    TypeEnv *env = NULL;

    TEST_SIMPLE_AST_TYPE_ENV("type Enum = \n"
                             "  | A       \n"
                             "  | B       \n"
                             "  | C       \n"
                             "  ;         \n"
                             "let f = fn x->\n"
                             "  match x with\n"
                             "    | A -> 1\n"
                             "    | B -> 2\n"
                             "    | C -> 3\n"
                             ";;",
                             &MAKE_FN_TYPE_2(&tenum, &t_int), env);

    TEST_SIMPLE_AST_TYPE_ENV("f C", &t_int, env);
  });

  ({
    TITLE("## tuple destructure")
    TEST_SIMPLE_AST_TYPE("let (x, y, z) = (1, 2, 3); x\n", &t_int);
  });

  ({
    TITLE("## tuple destructure more")
    TypeEnv *env = NULL;
    TEST_SIMPLE_AST_TYPE_ENV("let (x, y, z) = (1, 2., 3);",
                             &TTUPLE(3, &t_int, &t_num, &t_int), env);
    TEST_SIMPLE_AST_TYPE_ENV("x;", &t_int, env);
    TEST_SIMPLE_AST_TYPE_ENV("y;", &t_num, env);
    TEST_SIMPLE_AST_TYPE_ENV("z;", &t_int, env);
  });

  ({
    SEP;
    TEST_SIMPLE_AST_TYPE("let get_stderr = extern fn () -> Ptr;\n"
                         "let stderr = get_stderr ();\n",
                         &t_ptr);
  });

  ({
    TITLE("LIST PROCESSING")
    TEST_SIMPLE_AST_TYPE("let x::_ = [1,2,3,4]; x\n", &t_int);
  });

  ({
    TITLE("LIST PROCESSING FUNCTIONS")
    TEST_SIMPLE_AST_TYPE(
        "let f = fn l->\n"
        "  match l with\n"
        "    | x::_ -> x\n"
        "    | [] -> 0\n"
        ";;",
        &MAKE_FN_TYPE_2(&tcons(TYPE_NAME_LIST, 1, &t_int), &t_int));

    TEST_SIMPLE_AST_TYPE(
        "let f = fn l->\n"
        "  match l with\n"
        "    | x1::x2::_ -> x1\n"
        "    | [] -> 0\n"
        ";;",
        &MAKE_FN_TYPE_2(&tcons(TYPE_NAME_LIST, 1, &t_int), &t_int));

    TEST_SIMPLE_AST_TYPE(
        "let f = fn l->\n"
        "  match l with\n"
        "    | x1::x2::[] -> x1\n"
        "    | [] -> 0\n"
        ";;",
        &MAKE_FN_TYPE_2(&tcons(TYPE_NAME_LIST, 1, &t_int), &t_int));
  });

  ({
    RESET;
    TITLE("LIST SUM FUNCTION")
    Type t4 = arithmetic_var("t4");
    TypeEnv *env = NULL;
    TEST_SIMPLE_AST_TYPE_ENV(
        "let list_sum = fn s l ->\n"
        "  match l with\n"
        "  | [] -> s\n"
        "  | x::rest -> list_sum (s + x) rest\n"
        ";;\n",
        &MAKE_FN_TYPE_3(&t4, &tcons(TYPE_NAME_LIST, 1, &t4), &t4), env);

    TEST_SIMPLE_AST_TYPE_ENV(
        "list_sum 0",
        &MAKE_FN_TYPE_2(&tcons(TYPE_NAME_LIST, 1, &t_int), &t_int), env);
  });

  ({
    RESET;
    TITLE("generic function with conflicting args should fail")
    Type t = tvar("x");
    Type f = MAKE_FN_TYPE_3(&t, &tcons(TYPE_NAME_LIST, 1, &t), &t);
    TypeEnv *env = NULL;
    env = env_extend(env, "sum", &f);
    TEST_SIMPLE_AST_TYPE_ENV("sum 0 [1., 2.]", NULL, env);
  });

  ({
    RESET;
    TITLE("## variables within cons values in match")
    Type opt_int =
        tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);

    TEST_SIMPLE_AST_TYPE("type Option t =\n"
                         "  | Some of t\n"
                         "  | None\n"
                         "  ;\n"
                         "let f = fn x ->\n"
                         "match x with\n"
                         "  | Some y -> y + 1\n"
                         "  | None -> 0\n"
                         "  ;;\n",
                         &MAKE_FN_TYPE_2(&opt_int, &t_int));
  });

  ({
    RESET;
    TITLE("## match on boolean fn")

    Type t = tvar("t1");
    TEST_SIMPLE_AST_TYPE("let test_val = fn b msg -> \n"
                         "match b with\n"
                         "| true  -> 1\n"
                         "| _     -> 0\n"
                         ";;\n",
                         &MAKE_FN_TYPE_3(&t_bool, &t, &t_int));
  });

  ({
    RESET;
    TITLE("## first class functions & specific")
    Ast *ast = TEST_SIMPLE_AST_TYPE("let sum = fn a b -> a + b;;\n"
                                    "let proc = fn f a b -> f a b;;\n"
                                    "proc sum 1 2\n",
                                    &t_int);
    Ast *call = ast->data.AST_BODY.stmts[2];
    Type *call_type = call->data.AST_APPLICATION.function->md;
    bool t_equal = types_equal(
        call_type, &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t_int, &t_int, &t_int),
                                   &t_int, &t_int, &t_int));
    if (t_equal) {
      fprintf(stderr,
              "✅ proc sum 1 2 => (Int -> Int -> Int) -> Int -> Int -> Int\n");
    } else {
      fprintf(
          stderr,
          "❌ proc sum 1 2 => (Int -> Int -> Int) -> Int -> Int -> Int\ngot ");
      print_type_err(call_type);
    }
    status &= t_equal;

    printf("proc: \n");
    print_ast(ast->data.AST_BODY.stmts[1]);
    print_type(ast->data.AST_BODY.stmts[1]->md);

    printf("sum: \n");
    print_ast(ast->data.AST_BODY.stmts[0]);
    print_type(ast->data.AST_BODY.stmts[0]->md);
  });

  ({
    RESET;
    TITLE("## first class functions & specific")
    Ast *ast = TEST_SIMPLE_AST_TYPE("let sum = fn a b -> a + b;;\n"
                                    "let proc = fn f a b -> f a b;;\n"
                                    "proc sum 1.0 2.0;\n"
                                    "proc sum 1 2\n",
                                    &t_int);
    Ast *call = ast->data.AST_BODY.stmts[2];
    Type *call_type = call->data.AST_APPLICATION.function->md;
    bool t_equal = types_equal(
        call_type, &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t_num, &t_num, &t_num),
                                   &t_num, &t_num, &t_num));
    if (t_equal) {
      fprintf(stderr, "✅ proc sum 1.0 2.0 => (Double -> Double -> Double) -> "
                      "Double -> Double -> Double\n");
    } else {
      fprintf(stderr, "❌ proc sum 1.0 2.0 => (Double -> Double -> Double) -> "
                      "Double -> Double -> Double\n");
      print_type_err(call_type);
    }
    status &= t_equal;

    printf("proc: \n");
    print_ast(ast->data.AST_BODY.stmts[1]);
    print_type(ast->data.AST_BODY.stmts[1]->md);

    printf("sum: \n");
    print_ast(ast->data.AST_BODY.stmts[0]);
    print_type(ast->data.AST_BODY.stmts[0]->md);
  });
  ({
    RESET;
    TITLE("## match expr with guard clause")
    TypeEnv *env = NULL;
    env = env_extend(env, "x", &t_int);
    TEST_SIMPLE_AST_TYPE_ENV("match x with\n"
                             "| x if x > 300 -> x\n"
                             "| 2 -> 0\n"
                             "| _ -> 3",
                             &t_int, env);
  });

  ({
    RESET;
    TITLE("## function type decl")
    TypeEnv *env = NULL;
    TEST_SIMPLE_AST_TYPE_ENV(
        "type Cb = Double -> (Int * Int) -> ();",
        &MAKE_FN_TYPE_3(&t_num, &TTUPLE(2, &t_int, &t_int), &t_void), env);

    Type *cb = env_lookup(env, "Cb");
    bool lut = types_equal(
        cb, &MAKE_FN_TYPE_3(&t_num, &TTUPLE(2, &t_int, &t_int), &t_void));
    if (lut) {
      fprintf(stderr, "✅ cb lookup\n");
    } else {
      fprintf(stderr, "❌ cb lookup\n");
    }

    status &= lut;
  });

  ({
    RESET;
    TITLE("## choose from array fn")
    TypeEnv *env = NULL;
    env = env_extend(env, "array_at", &t_array_at_fn_sig);
    env = env_extend(env, "array_size", &t_array_size_fn_sig);
    TEST_SIMPLE_AST_TYPE_ENV("let choose = fn arr ->\n"
                             "  let idx = rand_int (array_size arr);\n"
                             "  array_at arr idx \n"
                             ";;",
                             &MAKE_FN_TYPE_2(&t_array_var, &t_array_var_el),
                             env);
    TEST_SIMPLE_AST_TYPE_ENV("choose [|1, 2, 3|]", &t_int, env);
    TEST_SIMPLE_AST_TYPE_ENV("let x = [|1., 2.|]; choose x", &t_num, env);
    TEST_SIMPLE_AST_TYPE_ENV("let f = fn () -> choose x;; f ()", &t_num, env);
  });

  ({
    RESET;
    TITLE("## return Option from generic fn")
    TypeEnv *env = NULL;

    Type opt_tuple = tcons("Some", 1, &TTUPLE(3, &t_num, &t_num, &t_num));

    TEST_SIMPLE_AST_TYPE_ENV("type Option t =\n"
                             "  | Some of t\n"
                             "  | None\n"
                             "  ;\n"
                             "let f = fn (note, filter_freq) -> \n"
                             "  Some (\n"
                             "    0.25, # duration\n"
                             "    note,\n"
                             "    filter_freq\n"
                             "  )\n"
                             ";;\n"
                             "f (200., 200.);\n",
                             &opt_tuple, env);
  });

  ({
    RESET;
    TITLE("## return Option from generic fn")
    TypeEnv *env = NULL;

    Type opt_tuple = tcons("Some", 1, &TTUPLE(3, &t_num, &t_num, &t_num));

    TEST_SIMPLE_AST_TYPE_ENV("type Option t =\n"
                             "  | Some of t\n"
                             "  | None\n"
                             "  ;\n"
                             "let f = fn (note, filter_freq) -> \n"
                             "  Some (\n"
                             "    0.25, # duration\n"
                             "    note + 1,\n"
                             "    filter_freq + 1\n"
                             "  )\n"
                             ";;\n"
                             "f (200., 200.);\n",
                             &opt_tuple, env);
  });

  ({
    RESET;
    TITLE("## return Option from generic fn")
    TypeEnv *env = NULL;

    Ast *ast = TEST_SIMPLE_AST_TYPE_ENV(
        "type Option t =\n"
        "  | Some of t\n"
        "  | None\n"
        "  ;\n"
        "# stream that returns (duration * value1 * value2 *...)\n"
        "let f = fn (note, filter_freq) -> \n"
        "  Some (\n"
        "    0.25, # duration\n"
        "    note + 1.,\n"
        "    filter_freq + 1.\n"
        "  )\n"
        ";;\n"
        "let sched_wrap = fn def (note, filter_freq) -> \n"
        "  let result = def (note, filter_freq) in\n"
        "  match result with\n"
        "  | Some (duration, note, filter_freq) -> (\n"
        "    # apply note & filter_freq to node and then schedule\n"
        "    # def with the duration\n"
        "    set_input_scalar x 0 note;\n"
        "    set_input_scalar x 1 filter_freq;\n"
        "    schedule_event def (note, filter_freq) duration\n"
        "  )\n"
        "  | None -> ()\n"
        ";;\n"
        "sched_wrap f (200., 200.)\n",
        &t_void, env);

    Ast *call = ast->data.AST_BODY.stmts[3];
    Type *spec_fn_type = call->data.AST_APPLICATION.function->md;
    // print_type(spec_fn_type);
  });

  ({
    RESET;
    TITLE("## tuple ptr deref")
    TypeEnv *env = NULL;
    env = env_extend(env, "p", &tvar("tx"));
    Type t0 = tvar("t0");
    Type t1 = tvar("t1");
    Type t2 = tvar("t2");

    TEST_SIMPLE_AST_TYPE_ENV("let (x, y, z) = deref p;",
                             &TTUPLE(3, &t0, &t1, &t2), env);
    TEST_SIMPLE_AST_TYPE_ENV("x;", &t0, env);
    TEST_SIMPLE_AST_TYPE_ENV("y;", &t1, env);
    TEST_SIMPLE_AST_TYPE_ENV("z;", &t2, env);

    Type *lookup_ptr = env_lookup(env, "ptr_deref_var");

    bool tc = types_equal(lookup_ptr, &TTUPLE(3, &t0, &t1, &t2));

    if (tc) {
      fprintf(stderr, "✅ ptr is Ptr of (t0 * t1 * t2)\n");
    } else {
      fprintf(stderr, "❌ ptr is Ptr of (t0 * t1 * t2)\n");
    }
  });

  ({
    RESET;
    TITLE("## tuple ptr deref fn")
    TypeEnv *env = NULL;
    env = env_extend(env, "p", &tvar("tx"));
    Type t2 = tvar("t2");
    Type t3 = tvar("t3");
    Type t4 = tvar("t4");

    TEST_SIMPLE_AST_TYPE_ENV(
        "let f = fn p -> \n"
        "  let (x, y, z) = deref p;\n"
        "  ()\n"
        ";;\n",
        &MAKE_FN_TYPE_2(&tcons(TYPE_NAME_PTR, 1, &TTUPLE(3, &t2, &t3, &t4)),
                        &t_void),
        env);
  });

  ({
    RESET;
    TITLE("## typed function")

    TEST_SIMPLE_AST_TYPE(
        "let f = fn x: (Int) (y, z): (Int * Double) -> x + y + z;;",
        &MAKE_FN_TYPE_3(&t_int, &TTUPLE(2, &t_int, &t_num), &t_num));
  });
  ({
    RESET;

    TITLE("## deref in fn")

    TEST_SIMPLE_AST_TYPE(
        "let f = fn args: (Ptr of (Double * Double)) frame_offset: (Int) ->\n"
        "  let (duration, note, filter_freq) = *args;\n"
        "  ()\n"
        ";;\n",
        &MAKE_FN_TYPE_3(&tcons(TYPE_NAME_PTR, 1, &TTUPLE(2, &t_num, &t_num)),
                        &t_int, &t_void));
  });

  ({
    RESET;
    TITLE("## synth input ADT")

    Type t_param_variant =
        tcons(TYPE_NAME_VARIANT, 2, &tcons("Scalar", 1, &t_int),
              &tcons("Trig", 1, &t_int));
    TypeEnv *env = NULL;
    env = env_extend(env, "SynthInput", &t_param_variant);

    TEST_SIMPLE_AST_TYPE_ENV(
        "let set_input_scalar_offset = extern fn Synth -> Int -> Int -> Double "
        "-> Synth;\n"
        "let set_input_trig_offset = extern fn Int -> Int -> Synth -> Synth;\n"
        "let synth_meta = [|\n"
        "  Scalar 0,\n"
        "  Scalar 1,\n"
        "|];\n"
        "let set_synth_param = fn syn p offset v ->\n"
        "  match p with\n"
        "  | Scalar i -> set_input_scalar_offset syn i offset v\n"
        "  | Trig i -> set_input_trig_offset i offset syn \n"
        ";;\n",
        &MAKE_FN_TYPE_5(&t_ptr, &t_param_variant, &t_int, &t_num, &t_ptr), env);
  });
  ({
    RESET;
    TITLE("Array2d cons")
    Type array_double = tcons(TYPE_NAME_ARRAY, 1, &t_num);
    TEST_SIMPLE_AST_TYPE("type Array2d t = Int * Int * Array of t;\n"
                         "let x = Array2d 2 3 [|1.,2.|];",
                         &TTUPLE(3, &t_int, &t_int, &array_double));
  });
  ({
    RESET;
    TITLE("## array iter fn");
    TypeEnv *env;
    env = env_extend(env, "array_size", &t_array_size_fn_sig);
    env = env_extend(env, "array_incr", &t_array_incr_fn_sig);
    TEST_SIMPLE_AST_TYPE_ENV("let iter_arr = fn arr ->\n"
                             "  match arr with\n"
                             "  | a if (array_size a) > 0 -> (\n"
                             "    iter_arr (array_incr a) \n"
                             "  )   \n"
                             "  | _ -> ()\n"
                             ";;\n",
                             &MAKE_FN_TYPE_2(&t_array_var, &t_void), env);
  });

  ({
    RESET;
    TITLE("## array iter fn call");
    TypeEnv *env;
    env = env_extend(env, "array_size", &t_array_size_fn_sig);
    env = env_extend(env, "array_incr", &t_array_incr_fn_sig);
    Ast *prog = TEST_SIMPLE_AST_TYPE_ENV("let iter_arr = fn arr ->\n"
                                         "  match arr with\n"
                                         "  | a if (array_size a) > 0 -> (\n"
                                         "    iter_arr (array_incr a) \n"
                                         "  )   \n"
                                         "  | _ -> ()\n"
                                         ";;\n"
                                         "iter_arr [|1,2,3,4|]\n",
                                         &t_void, env);
    Ast *call = prog->data.AST_BODY.stmts[1];

    Type array_int = tcons(TYPE_NAME_ARRAY, 1, &t_int);
    bool tc = types_equal(call->data.AST_APPLICATION.function->md,
                          &MAKE_FN_TYPE_2(&array_int, &t_void));

    if (tc) {
      fprintf(stderr, "✅ call is Array of Int -> ()\n");
    } else {
      fprintf(stderr, "❌ call is Array of Int -> ()\n");
    }
  });

  ({
    RESET;
    TITLE("string concat")
    TEST_SIMPLE_AST_TYPE(
        "let concat_strs = fn res strs ->\n"
        "  match strs with \n"
        "  | [] -> res\n"
        "  | s::rest -> concat_strs `{res}{s}` rest\n"
        ";;\n",
        &MAKE_FN_TYPE_3(&t_string, &TLIST(&t_string), &t_string));
  });

  ({
    RESET;
    TITLE("coroutine function")

    Type opt = tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);
    Type instance = (Type){
        T_COROUTINE_INSTANCE,
        {.T_FN = {.from = &TTUPLE(3, &t_int, &t_int, &t_int), .to = &opt}}};

    // tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);

    TEST_SIMPLE_AST_TYPE("let f = fn a b c ->\n"
                         "  yield a;\n"
                         "  yield b;\n"
                         "  yield c;\n"
                         "  yield 4;\n"
                         "  yield 5\n"
                         ";;\n"
                         "let x = f 1 2 3",
                         &instance);
  });

  ({
    RESET;
    TITLE("coroutine function")

    Type opt = tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);
    Type instance =
        (Type){T_COROUTINE_INSTANCE, {.T_FN = {.from = &t_void, .to = &opt}}};

    // tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);

    TEST_SIMPLE_AST_TYPE("let f = fn () ->\n"
                         "  yield 4;\n"
                         "  yield 5\n"
                         ";;\n"
                         "let x = f ()",
                         &instance);
  });

  ({
    RESET;
    TITLE("coroutine function")

    Type opt = tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);
    Type instance =
        (Type){T_COROUTINE_INSTANCE, {.T_FN = {.from = &t_int, .to = &opt}}};

    // tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);

    TEST_SIMPLE_AST_TYPE("let f = fn a ->\n"
                         "  yield a;\n"
                         "  yield 5\n"
                         ";;\n"
                         "let x = f 1",
                         &instance);
  });

  ({
    RESET;
    TypeEnv *env = NULL;
    Type fn = MAKE_FN_TYPE_2(&MAKE_FN_TYPE_2(&t_num, &t_void), &t_void);
    Ast *prog =
        TEST_SIMPLE_AST_TYPE_ENV("let ex_fn = extern fn (Double -> ()) -> ();\n"
                                 "ex_fn f",
                                 &t_void, env);
    Ast *call = prog->data.AST_BODY.stmts[1];
    Type *cb_type = call->data.AST_APPLICATION.function->md;

    bool lut = types_equal(cb_type, &fn);
    if (lut) {
      fprintf(stderr, "✅ cb type => ");
      print_type_err(&fn);
    } else {
      fprintf(stderr, "❌ cb type !=");
      print_type_err(&fn);
    }

    status &= lut;
  });
  ({
    RESET;
    TITLE("concat struct types")
    Type t1 = TTUPLE(3, &t_int, &t_num, &t_bool);
    t1.data.T_CONS.names = (char *[]){"a", "b", "c"};
    Type t2 = TTUPLE(3, &t_int, &t_num, &t_bool);
    t2.data.T_CONS.names = (char *[]){"c", "d", "e"};

    Type t3 = TTUPLE(6, &t_int, &t_num, &t_bool, &t_int, &t_num, &t_bool);
    t3.data.T_CONS.names = (char *[]){"a", "b", "c", "d", "e", "f"};
    Type *concat = concat_struct_types(&t1, &t2);
    bool te = types_equal(concat, &t3);
    if (te) {
      fprintf(stderr, "✅ (a: Int * b: Double * c: Bool) '+' (c: Int * d: "
                      "Double * e: Bool) == (a: Int * b: Double * c: Bool * c: "
                      "Int * d: Double * e: Bool))\n");
    } else {

      fprintf(stderr, "❌ (a: Int * b: Double * c: Bool) '+' (c: Int * d: "
                      "Double * e: Bool) != (a: Int * b: Double * c: Bool * c: "
                      "Int * d: Double * e: Bool)), got");
      print_type_err(concat);
    }

    status &= te;
  });

  ({
    RESET;
    TITLE("concat struct types")
    Type t1 = TTUPLE(3, &t_int, &t_num, &t_bool);
    Type t2 = TTUPLE(3, &t_int, &t_num, &t_bool);

    Type t3 = TTUPLE(6, &t_int, &t_num, &t_bool, &t_int, &t_num, &t_bool);
    Type *concat = concat_struct_types(&t1, &t2);
    bool te = types_equal(concat, &t3);
    if (te) {
      fprintf(stderr, "✅ (Int * Double * Bool) '+' (Int * "
                      "Double * Bool) == (Int * Double * Bool * "
                      "Int * Double * Bool))\n");
    } else {

      fprintf(stderr, "❌ (Int * Double * Bool) '+' (Int * "
                      "Double * Bool) != (Int * Double * Bool * "
                      "Int * Double * Bool)), got");
      print_type_err(concat);
    }

    status &= te;
  });
  ({
    RESET;
    TITLE("loop coroutine")
    Type opt = tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_int), &TNONE);

    Type instance =
        (Type){T_COROUTINE_INSTANCE, {.T_FN = {.from = &t_void, .to = &opt}}};

    TEST_SIMPLE_AST_TYPE("let cor = fn () ->\n"
                         "  yield 1;\n"
                         "  yield 2;\n"
                         "  yield 3\n"
                         ";;\n"
                         "let inst = loop cor ();\n",
                         &instance);
  });
  ({
    RESET;
    TITLE("struct contains dur")
    Type v = tvar("t2");

    Type input = TTUPLE(1, &v);
    input.data.T_CONS.names = (char *[]){"dur"};

    Type fn = MAKE_FN_TYPE_2(&input, &v);

    TEST_SIMPLE_AST_TYPE("let get_dur = fn x ->\n"
                         "  x.dur\n"
                         ";;\n",
                         &fn);
  });

  ({
    RESET;
    TITLE("## typed named struct function")
    Type el = MAKE_FN_TYPE_2(&t_void, &t_num);
    Type input = TTUPLE(1, &el);
    input.data.T_CONS.names = (char *[]){"dur"};

    TEST_SIMPLE_AST_TYPE("let f = fn x: (dur: (() -> Double)) -> x.dur ();;",
                         &MAKE_FN_TYPE_2(&input, &t_num));
  });

  ({
    RESET;
    TITLE("## typed named struct function opt")
    Type opt = tcons(TYPE_NAME_VARIANT, 2, &tcons("Some", 1, &t_num), &TNONE);
    Type el = MAKE_FN_TYPE_2(&t_void, &opt);
    Type input = TTUPLE(1, &el);
    input.data.T_CONS.names = (char *[]){"dur"};

    TEST_SIMPLE_AST_TYPE(
        "let f = fn x: (dur: (() -> Option of Double)) -> x.dur ();;",
        &MAKE_FN_TYPE_2(&input, &opt));
  });

  return status == true ? 0 : 1;
}
