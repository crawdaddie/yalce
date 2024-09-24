#include "../lang/parse.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "serde.h"
#include "types/unification.h"
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
    bool stat = true;                                                          \
    Ast *p = parse_input(i, NULL);                                             \
    Type *infer_res = infer(p, &env);                                          \
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
      fprintf(stderr, "❌ typecheck doesn't fail " i " (got %s)\n",            \
              type_to_string(p->md, buf2));                                    \
      fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                          \
    }                                                                          \
    status &= (succeed == NULL);                                               \
  })

#define TEST_TYPECHECK_ENV_FAIL(i, env)                                        \
  ({                                                                           \
    Ast *p = parse_input(i, NULL);                                             \
    TypeEnv *env = NULL;                                                       \
    reset_type_var_counter();                                                  \
    Type *succeed = infer(p, &env);                                            \
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
  // TEST_SIMPLE_AST_TYPE("let x = 1 in x + 2.", &t_num);

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

  TEST_SIMPLE_AST_TYPE("[1,2,3]", &(TLIST(&t_int)));
  TEST_TYPECHECK_FAIL("[1,2.0,3]");

  TEST_SIMPLE_AST_TYPE("[|1,2,3|]", &(TARRAY(&t_int)));
  TEST_TYPECHECK_FAIL("[|1,2.0,3|]");
  TEST_SIMPLE_AST_TYPE("(1,2,3.9)", &TTUPLE(3, &t_int, &t_int, &t_num, ));

  ({
    Type t_arithmetic = arithmetic_var("t1");
    TEST_SIMPLE_AST_TYPE(
        "let f = fn x -> (1 + 2) * 8 - x;",
        &MAKE_FN_TYPE_2(&t_arithmetic, &TYPECLASS_RESOLVE("arithmetic", &t_int,
                                                          &t_arithmetic)));
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
                                                  &t_arithmetic2),
                               &t_arithmetic1)));
  });

  ({
    RESET;
    Type t_arithmetic = arithmetic_var("t1");
    Type t1 = tvar("t2");
    TEST_SIMPLE_AST_TYPE(
        "let f = fn (x, y) -> (1 + 2) * 8 - x;",
        &MAKE_FN_TYPE_2(
            &TTUPLE(2, &t_arithmetic, &t1),
            &TYPECLASS_RESOLVE("arithmetic", &t_int, &t_arithmetic)));
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
                                                          &t_arithmetic)),
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
    TEST_SIMPLE_AST_TYPE_ENV("s + x",
                             &TYPECLASS_RESOLVE("arithmetic", &t1, &t2), env);
  });

  ({
    TITLE("var types in arithmetic expr");
    RESET;
    TypeEnv *env = NULL;
    Type t_arithmetic2 = arithmetic_var("t2");
    Type t_arithmetic1 = arithmetic_var("t1");

    TEST_SIMPLE_AST_TYPE_ENV(
        "let f = fn x y -> x + y;",
        &MAKE_FN_TYPE_3(
            &t_arithmetic2, &t_arithmetic1,
            &TYPECLASS_RESOLVE("arithmetic", &t_arithmetic2, &t_arithmetic1)),
        env);

    TEST_SIMPLE_AST_TYPE_ENV(
        "f 1",
        &MAKE_FN_TYPE_2(&t_arithmetic1, &TYPECLASS_RESOLVE("arithmetic", &t_int,
                                                           &t_arithmetic1)),
        env);
  });

  ({
    RESET;
    Type t0 = tvar("t0");
    TypeEnv *env = NULL;
    Type *u = unify(&t0, &t_int, &env);
    status &= TEST(types_equal(u, &t_int), "unify t0 with Int");
  });

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
        &MAKE_FN_TYPE_3(
            &t_arithmetic2, &t_arithmetic1,
            &TYPECLASS_RESOLVE("arithmetic", &t_arithmetic2, &t_arithmetic1)),
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

  return status == true ? 0 : 1;
}
