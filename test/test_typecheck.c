#include "../lang/parse.h"
#include "../lang/types/builtins.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "serde.h"
#include <stdlib.h>

// #define xT(input, type)

#define T(input, type)                                                         \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {.env = NULL, .err_stream = stderr};                           \
    stat &= (infer(ast, &ctx) != NULL);                                        \
    stat &= (solve_program_constraints(ast, &ctx) != NULL);                    \
    stat &= (types_equal(ast->md, type));                                      \
    char buf[200] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ " input " => %s\n", type_to_string(type, buf));      \
    } else {                                                                   \
      char buf2[200] = {};                                                     \
      fprintf(stderr, "❌ " input " => %s (got %s)\n",                         \
              type_to_string(type, buf), type_to_string(ast->md, buf2));       \
      fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                          \
    }                                                                          \
    status &= stat;                                                            \
    env = ctx.env;                                                             \
    ast;                                                                       \
  })

#define _T(input)                                                              \
  \ 
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {.env = NULL};                                                 \
    infer(ast, &ctx);                                                          \
    char buf[200] = {};                                                        \
    env = ctx.env;                                                             \
    ast;                                                                       \
  })

#define TASSERT(t1, t2, msg)                                                   \
  ({                                                                           \
    if (types_equal(t1, t2)) {                                                 \
      status &= true;                                                          \
      fprintf(stderr, "✅ %s\n", msg);                                         \
    } else {                                                                   \
      status &= false;                                                         \
      char buf[100] = {};                                                      \
      fprintf(stderr, "❌ %s got %s\n", msg, type_to_string(t1, buf));         \
    }                                                                          \
  })

#define TFAIL(input)                                                           \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {.env = NULL};                                                 \
    stat &= (infer(ast, &ctx) == NULL);                                        \
    stat |= (solve_program_constraints(ast, &ctx) == NULL);                    \
    char buf[100] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ " input " fails typecheck\n");                       \
    } else {                                                                   \
      char buf2[100] = {};                                                     \
      fprintf(stderr, "❌ " input " does not fail typecheck\n");               \
      fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                          \
    }                                                                          \
    status &= stat;                                                            \
    ast;                                                                       \
  })

int main() {

  initialize_builtin_types();

  bool status = true;
  TypeEnv *env = NULL;

  T("1", &t_int);
  T("1.", &t_num);
  T("'c'", &t_char);
  T("\"hello\"", &t_string);
  T("true", &t_bool);
  T("false", &t_bool);
  T("()", &t_void);
  T("[]", &t_empty_list);
  T("1 + 2", &t_int);
  T("1 + 2.0", &t_num);
  T("(1 + 2) * 8", &t_int);
  T("1 + 2.0 * 8", &t_num);
  TFAIL("1 + \"hello\"");
  //
  ({
    Type tvar = arithmetic_var("`2");
    T("x + 1", &tvar);
  });
  ({
    Type tvar = arithmetic_var("`2");
    T("1 + x", &tvar);
  });
  //
  ({
    Type tvar = arithmetic_var("`6");
    T("(1 + 2) * 8 - x", &tvar);
  });
  //
  ({
    Type tvar = arithmetic_var("`2");
    T("x - 8", &tvar);
  });
  //
  T("2.0 - 1", &t_num);
  T("1 == 1", &t_bool);
  T("1 == 2", &t_bool);
  T("1 == 2.0", &t_bool);
  T("1 != 2.0", &t_bool);
  T("1 < 2.0", &t_bool);
  T("1 > 2.0", &t_bool);
  T("1 >= 2.0", &t_bool);
  T("1 <= 2.0", &t_bool);

  T("[1,2,3]", &(TLIST(&t_int)));
  TFAIL("[1,2.0,3]");

  T("[|1,2,3|]", &TARRAY(&t_int));
  TFAIL("[|1,2.0,3|]");

  T("[|1|]", &TARRAY(&t_int));
  T("(1,2,3.9)", &TTUPLE(3, &t_int, &t_int, &t_num, ));

  T("let x = 1", &t_int);
  T("let x = 1 + 2.0", &t_num);
  T("let x = 1 in x + 1.0", &t_num);
  T("let x = 1 in let y = x + 1.0", &t_num);
  T("let x, y = (1, 2) in x", &t_int);
  T("let x::_ = [1,2,3] in x", &t_int);
  //
  T("let z = [1, 2] in let x::_ = z in x", &t_int);
  TFAIL("let z = 1 in let x::_ = z in x");

  T("let f = fn a b -> 2;;", &MAKE_FN_TYPE_3(&TVAR("`0"), &TVAR("`1"), &t_int));
  T("let f = fn a: (Int) b: (Int) -> 2;;",
    &MAKE_FN_TYPE_3(&t_int, &t_int, &t_int));

  T("match x with\n"
    "| 1 -> 1\n"
    "| 2 -> 0\n"
    "| _ -> 3\n",
    &t_int);

  ({
    Type opt_int = TOPT(&t_int);
    T("let f = fn x ->\n"
      "match x with\n"
      "  | Some 1 -> 1\n"
      "  | Some 0 -> 1\n"
      "  | None -> 0\n"
      "  ;;\n",
      &MAKE_FN_TYPE_2(&opt_int, &t_int));
  });

  ({
    Type opt_int = TOPT(&t_int);
    T("let f = fn x ->\n"
      "match x with\n"
      "  | Some y -> y\n"
      "  | None -> 0\n"
      "  ;;\n",
      &MAKE_FN_TYPE_2(&opt_int, &t_int));
  });

  ({
    Type opt_int = TOPT(&t_int);
    T("let f = fn x ->\n"
      "match x with\n"
      "  | Some y -> y + 1\n"
      "  | None -> 0\n"
      "  ;;\n",
      &MAKE_FN_TYPE_2(&opt_int, &t_int));
  });

  ({
    Type opt_int = TOPT(&t_int);
    T("let f = fn x ->\n"
      "match x with\n"
      "  | Some y -> y * 2\n"
      "  | None -> 0\n"
      "  ;;\n",
      &MAKE_FN_TYPE_2(&opt_int, &t_int));
  });

  T("let f = fn x ->\n"
    "match x with\n"
    "  | (1, 2) -> 1\n"
    "  | (1, 3) -> 0\n"
    "  ;;\n",
    &MAKE_FN_TYPE_2(&TTUPLE(2, &t_int, &t_int), &t_int));

  T("let f = fn x ->\n"
    "match x with\n"
    "  | (1, y) -> y\n"
    "  | (1, 3) -> 0\n"
    "  ;;\n",
    &MAKE_FN_TYPE_2(&TTUPLE(2, &t_int, &t_int), &t_int));

  T("let ex_fn = extern fn Int -> Double -> Int;",
    &MAKE_FN_TYPE_3(&t_int, &t_num, &t_int));

  ({
    Type tvar = {T_VAR, .data = {.T_VAR = "`0"}};
    Ast *body = T("let f = fn x -> 1 + x;;\n"
                  "f 1;\n"
                  "f 1.;\n",
                  &t_num);

    TASSERT(body->data.AST_BODY.stmts[0]->md, &MAKE_FN_TYPE_2(&tvar, &tvar),
            "f == `0 [arithmetic] -> `0 [arithmetic]");
    TASSERT(body->data.AST_BODY.stmts[1]->md, &t_int, "f 1 == Int");
    TASSERT(body->data.AST_BODY.stmts[2]->md, &t_num, "f 1. == Num");
  });

  ({
    Type t0 = TVAR("`0");
    Type t1 = TVAR("`1");
    Type t2 = TVAR("`2");

    T("let f = fn (x, y, z) -> (z, y, x);",
      &MAKE_FN_TYPE_2(&TTUPLE(3, &t0, &t1, &t2), &TTUPLE(3, &t2, &t1, &t0)));
  });

  ({
    Type t0 = TVAR("`0");
    Type t1 = TVAR("`1");
    Type t2 = TVAR("`2");

    T("let f = fn (x, y, z) frame_offset: (Int) -> (z, y, x);",
      &MAKE_FN_TYPE_3(&TTUPLE(3, &t0, &t1, &t2), &t_int,
                      &TTUPLE(3, &t2, &t1, &t0)));
  });

  ({
    Type t0 = arithmetic_var("`0");
    Type t1 = arithmetic_var("`1");
    Type t2 = arithmetic_var("`2");

    T("let f = fn x y z -> x + y + z;",
      &MAKE_FN_TYPE_4(
          &t0, &t1, &t2,
          &MAKE_TC_RESOLVE_2("arithmetic",
                             &MAKE_TC_RESOLVE_2("arithmetic", &t0, &t1), &t2)));
  });

  ({
    T("let count_10 = fn x ->\n"
      "  match x with\n"
      "  | 10 -> 10\n"
      "  | _ -> count_10 (x + 1)\n"
      ";;\n",
      &MAKE_FN_TYPE_2(&t_int, &t_int));
  });

  T("let fib = fn x ->\n"
    "  match x with\n"
    "  | 0 -> 0\n"
    "  | 1 -> 1\n"
    "  | _ -> (fib (x - 1)) + (fib (x - 2))\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_int, &t_int));

  T("let fib = fn x ->\n"
    "  match x with\n"
    "  | 0 -> 0\n"
    "  | 1 -> 1\n"
    "  | _x -> (fib (_x - 1)) + (fib (_x - 2))\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_int, &t_int));

  T("let f = fn x: (Int) (y, z): (Int * Double) -> x + y + z;;",
    &MAKE_FN_TYPE_3(&t_int, &TTUPLE(2, &t_int, &t_num), &t_num));

  // first-class functions
  ({
    Type t0 = TVAR("`7");
    Type t1 = TVAR("`8");
    Type t2 = TVAR("`10");
    Ast *b = T("let sum = fn a b -> a + b;;\n"
               "let proc = fn f a b -> f a b;;\n"
               "proc sum 1 2;\n",
               &t_int);
    TASSERT(b->data.AST_BODY.stmts[1]->md,
            &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t0, &t1, &t2), &t0, &t1, &t2),
            "proc == (`7 -> `8 -> `10) -> `7 -> `8 -> `10");
  });

  ({
    Type t0 = TVAR("`7");
    Type t1 = TVAR("`8");
    Type t2 = TVAR("`10");
    Ast *b = T("let sum = fn a b -> a + b;;\n"
               "let proc = fn f a b -> f a b;;\n"
               "proc sum 1.0 2.0;\n"
               "proc sum 1 2;\n",
               &t_int);
    TASSERT(b->data.AST_BODY.stmts[2]->md, &t_num, "proc sum 1. 2. == Double");
    TASSERT(b->data.AST_BODY.stmts[1]->md,
            &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t0, &t1, &t2), &t0, &t1, &t2),
            "proc == (`7 -> `8 -> `10) -> `7 -> `8 -> `10");
  });

  T("type Cb = Double -> (Int * Int) -> ();",
    &MAKE_FN_TYPE_3(&t_num, &TTUPLE(2, &t_int, &t_int), &t_void));

  ({
    Type tenum = TCONS(TYPE_NAME_VARIANT, 3, &TCONS("A", 0, NULL),
                       &TCONS("B", 0, NULL), &TCONS("C", 0, NULL));

    T("type Enum =\n"
      "  | A\n"
      "  | B \n"
      "  | C\n"
      "  ;\n"
      "\n"
      "let f = fn x ->\n"
      "  match x with\n"
      "    | A -> 1\n"
      "    | B -> 2 \n"
      "    | C -> 3\n"
      ";;\n",
      &MAKE_FN_TYPE_2(&tenum, &t_int));
  });

  // // LIST PROCESSING FUNCTIONS
  T("let f = fn l->\n"
    "  match l with\n"
    "    | x::_ -> x\n"
    "    | [] -> 0\n"
    ";;",
    &MAKE_FN_TYPE_2(&TLIST(&t_int), &t_int));

  T("let f = fn l->\n"
    "  match l with\n"
    "    | x1::x2::_ -> x1\n"
    "    | [] -> 0\n"
    ";;",
    &MAKE_FN_TYPE_2(&TLIST(&t_int), &t_int));

  T("let f = fn l->\n"
    "  match l with\n"
    "    | x1::x2::[] -> x1\n"
    "    | [] -> 0\n"
    ";;",
    &MAKE_FN_TYPE_2(&TLIST(&t_int), &t_int));

  ({
    Type s = arithmetic_var("`4");
    Type t = arithmetic_var("`8");
    T("let list_sum = fn s l ->\n"
      "  match l with\n"
      "  | [] -> s\n"
      "  | x::rest -> list_sum (s + x) rest\n"
      ";;\n",
      &MAKE_FN_TYPE_3(&t, &TLIST(&s), &t));
  });
  exit(status);

  ({
    Type opt_int = TOPT(&t_int);
    T("Some 1", &opt_int);
  });

  ({
    Ast *b = T("let x = 1;\n"
               "match x with\n"
               "| xx if xx > 300 -> xx\n"
               "| 2 -> 0\n"
               "| _ -> 3",
               &t_int);
    Ast *branch = b->data.AST_BODY.stmts[1]->data.AST_MATCH.branches;

    Ast *guard = branch->data.AST_MATCH_GUARD_CLAUSE.guard_expr;

    TASSERT(guard->data.AST_APPLICATION.function->md,
            &MAKE_FN_TYPE_3(&t_int, &t_int, &t_bool),
            "guard clause has type Int -> Int -> Bool\n");
  });

  ({
    bool res =
        (fn_types_match(&MAKE_FN_TYPE_3(&t_int, &t_num, &t_void),
                        &MAKE_FN_TYPE_3(&t_int, &t_num, &t_num)) == true);

    const char *msg =
        "fn types match function - comparing two fn types ignoring "
        "return type\n";
    if (res) {
      printf("✅ %s", msg);
    } else {

      printf("❌ %s", msg);
    }
    status &= res;
  });

  ({
    bool res =
        (fn_types_match(&MAKE_FN_TYPE_3(&t_int, &t_int, &t_void),
                        &MAKE_FN_TYPE_3(&t_int, &t_num, &t_num)) == false);
    const char *msg =
        "fn types match function - comparing two fn types ignoring "
        "return type\n";
    if (res) {
      printf("✅ %s", msg);
    } else {

      printf("❌ %s", msg);
    }
    status &= res;
  });
  ({
    Ast *b = T("let fib = fn x ->\n"
               "  match x with\n"
               "  | 0 -> 0\n"
               "  | 1 -> 1\n"
               "  | _ -> (fib (x - 1)) + (fib (x - 2))\n"
               ";;",

               &MAKE_FN_TYPE_2(&t_int, &t_int));

    Ast final_branch = b->data.AST_BODY.stmts[0]
                           ->data.AST_LET.expr->data.AST_LAMBDA.body->data
                           .AST_MATCH.branches[5];

    TASSERT(final_branch.data.AST_APPLICATION.args[0]
                .data.AST_APPLICATION.args[0]
                .md,
            &t_int, "references in sub-nodes properly typed :: (x - 1) == Int");
  });
  /*
    ({
      Type s = arithmetic_var("`4");
      Type t = arithmetic_var("`0");
      Ast *b = T("let arr_sum = fn s a ->\n"
                 "  let len = array_size a in\n"
                 "  let aux = fn i su -> \n"
                 "    match i with\n"
                 "    | i if i == len -> su\n"
                 "    | i -> aux (i + 1) (su + array_at a i)\n"
                 "    ; in\n"
                 "  aux 0 s\n"
                 "  ;;\n",
                 &MAKE_FN_TYPE_3(&t, &TARRAY(&s), &t));
    });
    */

  ({
    Type t = TVAR("`3");
    T("let f = fn a b c d -> a == b && c == d;;\n"
      "f 1 2 3\n",
      &MAKE_FN_TYPE_2(&t, &t_bool));
  });

  ({
    Type t = arithmetic_var("`2");
    Ast *b = T("let f = fn a b c -> a + b + c;;\n"
               "f 1 2\n",
               &MAKE_FN_TYPE_2(&t, &t));

    bool is_partial = application_is_partial(b->data.AST_BODY.stmts[1]);

    const char *msg = "let f = fn a b c -> a + b + c;;\n"
                      "f 1 2\n"
                      "application_is_partial fn test\n";
    if (is_partial) {
      printf("✅ %s", msg);
    } else {

      printf("❌ %s", msg);
    }
    status &= is_partial;
  });

  ({
    Ast *b = _T("let f = fn a b c d e f -> a + b + c + d + e + f;;\n"
                "let x1 = f 1;\n"
                "let x2 = x1 2;\n"
                "let x3 = x2 3;\n");
    bool is_partial = true;
    is_partial &=
        application_is_partial(b->data.AST_BODY.stmts[1]->data.AST_LET.expr);
    is_partial &=
        application_is_partial(b->data.AST_BODY.stmts[2]->data.AST_LET.expr);
    is_partial &=
        application_is_partial(b->data.AST_BODY.stmts[3]->data.AST_LET.expr);

    const char *msg = "let f = fn a b c d e f -> a + b + c + d + e + f;;\n"
                      "let x1 = f 1;\n"
                      "let x2 = x1 2;\n"
                      "let x3 = x2 3;\n"
                      "several application_is_partial fn tests\n";
    if (is_partial) {
      printf("✅ %s", msg);
    } else {

      printf("❌ %s", msg);
    }
    status &= is_partial;
  });

  ({
    Type t = arithmetic_var("`2");
    T("let f = fn a b c -> a + b + c;;\n"
      "f 1. 2.\n",
      &MAKE_FN_TYPE_2(&t, &t));
  });

  T("let f = fn a: (Int) b: (Int) c: (Int) -> (a == b) && (a == c);;\n"
    "f 1 2\n",
    &MAKE_FN_TYPE_2(&t_int, &t_bool));

  ({
    Type t0 = arithmetic_var("`0");
    Type t1 = arithmetic_var("`1");
    T("(1, 2, fn a b -> a + b;);\n",
      &TTUPLE(3, &t_int, &t_int,
              &MAKE_FN_TYPE_3(&t0, &t1,
                              &MAKE_TC_RESOLVE_2("arithmetic", &t0, &t1))));
  });

  ({
    Type t0 = arithmetic_var("`2");
    Type t1 = arithmetic_var("`3");
    Type tuple = TTUPLE(
        3, &t_int, &t_int,
        &MAKE_FN_TYPE_3(&t0, &t1, &MAKE_TC_RESOLVE_2("arithmetic", &t0, &t1)));

    tuple.data.T_CONS.names = (char *[]){"a", "b", "f"};
    T("(a: 1, b: 2, f: (fn a b -> a + b))\n", &tuple);
  });

  ({
    Type cor = MAKE_FN_TYPE_2(&t_void, &TOPT(&t_num));
    cor.is_coroutine_instance = true;
    Type constructor = MAKE_FN_TYPE_2(&t_void, &cor);
    constructor.is_coroutine_constructor = true;

    T("let co_void = fn () ->\n"
      "  yield 1.;\n"
      "  yield 2.;\n"
      "  yield 3.\n"
      ";;\n",
      &constructor);
  });

  TFAIL("let co_void = fn () ->\n"
        "  yield 1.;\n"
        "  yield 2;\n"
        "  yield 3.\n"
        ";;\n");

  T("let co_void = fn () ->\n"
    "  yield 1.;\n"
    "  yield 2.;\n"
    "  yield 3.\n"
    ";;\n"
    "let x = co_void () in\n"
    "x ()\n",
    &TOPT(&t_num));

  ({
    Ast *b = T("let co_void_rec = fn () ->\n"
               "  yield 1.;\n"
               "  yield 2.;\n"
               "  yield co_void_rec ()\n"
               ";;\n",
               coroutine_constructor_type_from_fn_type(
                   &MAKE_FN_TYPE_2(&t_void, &t_num)));

    // Ast *rec_yield =
    //     b->data.AST_BODY.stmts[0]
    //         ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_BODY.stmts[2];
    // print_ast(rec_yield);
    // print_type(rec_yield->md);

    // printf("## rec yield:\n");
    // print_type(rec_yield->data.AST_YIELD.expr->md);
  });

  ({
    Type cor =
        TCONS("coroutine", 2, &t_void, &MAKE_FN_TYPE_2(&t_void, &TOPT(&t_num)));
    T("let ne = fn () ->\n"
      "  yield 300.;\n"
      "  yield 400.\n"
      ";;\n"
      "let co_void = fn () ->\n"
      "  yield 1.;\n"
      "  yield 2.;\n"
      "  yield ne ();\n"
      "  yield 3.\n"
      ";;\n",
      coroutine_constructor_type_from_fn_type(
          &MAKE_FN_TYPE_2(&t_void, &t_num)));
  });

  T("let sq = fn x: (Int) -> x * 1.;;", &MAKE_FN_TYPE_2(&t_int, &t_num));

  // ({
  //   Type cor =
  //       TCONS("coroutine", 2, &t_num, &MAKE_FN_TYPE_2(&t_void,
  //       &TOPT(&t_num)));
  //
  //   T("let cor = fn a ->\n"
  //     "  yield 1.;\n"
  //     "  yield a;\n"
  //     "  yield 3.\n"
  //     ";;\n",
  //
  //     &MAKE_FN_TYPE_2(&t_num, &cor));
  // });
  //
  //
  ({
    Ast *b = T(
        "type SchedulerCallback = Ptr -> Int -> ();\n"
        "let schedule_event = extern fn SchedulerCallback -> Double -> Ptr -> "
        "();\n"
        "let runner = fn c off ->\n"
        "  match c () with\n"
        "  | Some dur -> schedule_event runner dur c\n"
        "  | None -> () \n"
        ";;\n"
        "schedule_event runner 0. c\n",
        &t_void);
    Ast *runner_arg = b->data.AST_BODY.stmts[3]->data.AST_APPLICATION.args;
    Type cor_type = MAKE_FN_TYPE_2(&t_void, &TOPT(&t_num));
    // cor_type.is_coroutine_instance = true;
    Type runner_fn_arg_type = MAKE_FN_TYPE_3(&cor_type, &t_int, &t_void);

    bool res = types_equal(runner_arg->md, &runner_fn_arg_type);
    const char *msg = "runner arg can be materialised to specific type:";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(&runner_fn_arg_type);
      status &= true;
    } else {
      status &= false;
      printf("❌ %s\nexpected:\n", msg);
      print_type(&runner_fn_arg_type);
      printf("got:\n");
      print_type(runner_arg->md);
    }
  });

  ({
    Ast *b = T("let schedule_event = extern fn (tUserData -> Int -> ()) -> "
               "Double -> tUserdData -> "
               "();\n"
               "let co_void = fn () ->\n"
               "  yield 0.125;\n"
               "  yield co_void ()\n"
               ";;\n"
               "let c = co_void ();\n"
               "let runner = fn c off ->\n"
               "  match c () with\n"
               "  | Some dur -> schedule_event runner dur c\n"
               "  | None -> () \n"
               ";;\n"
               "schedule_event runner 0. c\n",
               &t_void);
    Ast *runner_arg = b->data.AST_BODY.stmts[4]->data.AST_APPLICATION.args;
    Ast *cor_arg = b->data.AST_BODY.stmts[4]->data.AST_APPLICATION.args + 2;

    print_ast(cor_arg);
    print_type(cor_arg->md);

    Type cor_type = MAKE_FN_TYPE_2(&t_void, &TOPT(&t_num));
    cor_type.is_coroutine_instance = true;
    Type runner_fn_arg_type = MAKE_FN_TYPE_3(&cor_type, &t_int, &t_void);

    bool res = types_equal(runner_arg->md, &runner_fn_arg_type);
    const char *msg = "runner arg can be materialised to specific type:";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(&runner_fn_arg_type);
      status &= true;
    } else {
      status &= false;
      printf("❌ %s\nexpected:\n", msg);
      print_type(&runner_fn_arg_type);
      printf("got:\n");
      print_type(runner_arg->md);
    }
  });

  ({
    Type cor_type = MAKE_FN_TYPE_2(&t_void, &TOPT(&t_int));
    cor_type.is_coroutine_instance = true;

    T("let l1 = [1, 2, 3];\n"
      "let l2 = [6, 5, 4];\n"
      "let co_void = fn () -> \n"
      "  yield iter_of_list l1;\n"
      "  yield iter_of_list l2\n"
      ";;\n"
      "let c = co_void ();\n",
      &cor_type);
  });

  // ({
  //   Type v = TVAR("t");
  //   Type r = TVAR("r");
  //   T("let array_fold = fn f s arr ->\n"
  //     "  let len = array_size arr in\n"
  //     "  let aux = (fn i su -> \n"
  //     "    match i with\n"
  //     "    | i if i == len -> su\n"
  //     "    | i -> aux (i + 1) (f su (array_at arr i))\n"
  //     "    ;) in\n"
  //     "  aux 0 s\n"
  //     ";;\n",
  //     &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&r, &v, &r), &r, &TARRAY(&v), &r));
  // });

  T("let set_ref = array_set 0;",
    &MAKE_FN_TYPE_3(&t_array_var, &t_array_var_el, &t_array_var));

  T("let x = [|1|]; let set_ref = array_set 0; set_ref x 3", &TARRAY(&t_int));
  T("let (@) = array_at", &t_array_at_fn_sig);

  T("let rand_int = extern fn Int -> Int;\n"
    "let array_choose = fn arr ->\n"
    "  let idx = rand_int (array_size arr);\n"
    "  array_at arr idx \n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_array_var, &t_array_var_el));

  T("let rand_int = extern fn Int -> Int;\n"
    "let array_choose = fn arr ->\n"
    "  let idx = rand_int (array_size arr);\n"
    "  array_at arr idx \n"
    ";;\n"
    "array_choose [|1,2,3|]",
    &t_int);

  T("let rand_int = extern fn Int -> Int;\n"
    "let array_choose = fn arr ->\n"
    "  let idx = rand_int (array_size arr);\n"
    "  array_at arr idx \n"
    ";;\n"
    "\\array_choose [|1,2,3|]",
    &MAKE_FN_TYPE_2(&t_void, &t_int));

  T("let bind = extern fn Int -> Int -> Int -> Int;\n"
    "let _bind = fn server_fd server_addr ->\n"
    "  match (bind server_fd server_addr 10) with\n"
    "  | 0 -> Some server_fd\n"
    "  | _ -> None \n"
    ";;\n",
    &MAKE_FN_TYPE_3(&t_int, &t_int, &TOPT(&t_int)));

  ({
    Type ltype = TLIST(&TVAR("`3"));
    T("let print_list = fn l ->\n"
      "  match l with\n"
      "  | x::rest -> (print `{x}, `; print_list rest)\n"
      "  | [] -> ()\n"
      ";;\n",
      &MAKE_FN_TYPE_2(&ltype, &t_void));
  });

  ({
    T("let print_list = fn l ->\n"
      "  match l with\n"
      "  | x::rest -> (print `{x}, `; print_list rest)\n"
      "  | [] -> ()\n"
      ";;\n"
      "print_list [1,2,3]",
      &t_void);
  });

  T("let list_pop_left = fn l ->\n"
    "  match l with\n"
    "  | [] -> None\n"
    "  | x::rest -> Some x \n"
    ";; \n",
    &MAKE_FN_TYPE_2(&TLIST(&TVAR("`3")), &TOPT(&TVAR("`3"))));

  T("let l = Int[]", &TLIST(&t_int));

  T("let enqueue = fn (head, tail) item ->\n"
    "  let last = [item] in\n"
    "  match head with\n"
    "  | [] -> (last, last)\n"
    "  | _ -> (\n"
    "    let _ = list_concat tail last in\n"
    "    (head, last)\n"
    "  )\n"
    ";;\n",
    &MAKE_FN_TYPE_3(&TTUPLE(2, &TLIST(&TVAR("`2")), &TLIST(&TVAR("`2"))),
                    &TVAR("`2"),
                    &TTUPLE(2, &TLIST(&TVAR("`2")), &TLIST(&TVAR("`2")))));

  T("let enqueue = fn (head, tail): (List of Int * List of Int) item: (Int) "
    "->\n"
    "  let last = [item] in\n"
    "  match head with\n"
    "  | [] -> (last, last)\n"
    "  | _ -> (\n"
    "    let _ = list_concat tail last in\n"
    "    (head, last)\n"
    "  )\n"
    ";;\n",
    // &MAKE_FN_TYPE_3(&TTUPLE(2, &TLIST(&TVAR("`0")), &TLIST(&TVAR("`0"))),
    //                 &TVAR("`0"),
    //                 &TTUPLE(2, &TLIST(&TVAR("`0")), &TLIST(&TVAR("`0"))))
    //
    &MAKE_FN_TYPE_3(&TTUPLE(2, &TLIST(&t_int), &TLIST(&t_int)), &t_int,
                    &TTUPLE(2, &TLIST(&t_int), &TLIST(&t_int))));
  ({
    Ast *b = T(
        "let pop_left = fn (head, tail) ->\n"
        "  match head with\n"
        "  | [] -> ((head, tail), None)\n"
        "  | x::rest -> ((rest, tail), Some x)  \n"
        ";;\n",
        &MAKE_FN_TYPE_2(&TTUPLE(2, &TLIST(&TVAR("`5")), &TVAR("`1")),
                        &TTUPLE(2, &TTUPLE(2, &TLIST(&TVAR("`5")), &TVAR("`1")),
                                &TOPT(&TVAR("`5")))));
    Ast none = b->data.AST_BODY.stmts[0]
                   ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_MATCH
                   .branches[1]
                   .data.AST_LIST.items[1];

    print_type(none.md);
    bool res = types_equal(none.md, &TOPT(&TVAR("`5")));
    const char *msg = "None return val";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(none.md);
      status &= true;
    } else {
      status &= false;
      printf("❌ %s\nexpected:\n", msg);
      print_type(&TOPT(&TVAR("`5")));
      printf("got:\n");
      print_type(none.md);
    }
  });
  T("let loop = fn () ->\n"
    "  loop ();\n"
    "  ()\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_void, &t_void));

  T("let accept = extern fn Int -> Ptr -> Ptr -> Int;\n"
    "let proc_tasks = extern fn (Queue of l) -> Int -> ();\n"
    "let proc_tasks = fn tasks server_fd ->\n"
    "  let ts = match (queue_pop_left tasks) with\n"
    "  | Some r -> (\n"
    "    match (r ()) with\n"
    "    | Some _ -> queue_append_right tasks r\n"
    "    | None -> tasks\n"
    "  )\n"
    "  | None -> queue_of_list [ (accept_connections server_fd) ]\n"
    "  in\n"
    "  proc_tasks ts server_fd\n"
    ";;\n",
    &MAKE_FN_TYPE_3(&TCONS(TYPE_NAME_QUEUE, 1, &TVAR("l")), &t_int, &t_void));

  return status == true ? 0 : 1;
}
