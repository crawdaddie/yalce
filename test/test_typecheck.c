#include "../lang/parse.h"
#include "../lang/serde.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"

// #define xT(input, type)

#define T(input, type)                                                         \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {.env = NULL};                                                 \
    stat &= (infer(ast, &ctx) != NULL);                                        \
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
    Type tvar = arithmetic_var("`0");
    T("x + 1", &tvar);
  });
  ({
    Type tvar = arithmetic_var("`0");
    T("1 + x", &tvar);
  });
  //
  ({
    Type tvar = arithmetic_var("`0");
    T("(1 + 2) * 8 - x", &tvar);
  });
  //
  ({
    Type tvar = arithmetic_var("`0");
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
      "  | Some y -> y + 1\n"
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
    Type t0 = arithmetic_var("`0");
    Type t1 = arithmetic_var("`1");
    Type t2 = arithmetic_var("`2");

    T("let f = fn x y z -> x + y + z;",
      &MAKE_FN_TYPE_4(
          &t0, &t1, &t2,
          &MAKE_TC_RESOLVE_2("arithmetic",
                             &MAKE_TC_RESOLVE_2("arithmetic", &t0, &t1), &t2)));
  });

  T("let count_10 = fn x ->\n"
    "  match x with\n"
    "  | 10 -> 10\n"
    "  | _ -> count_10 (x + 1)\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_int, &t_int));

  T("let fib = fn x ->\n"
    "  match x with\n"
    "  | 0 -> 0\n"
    "  | 1 -> 1\n"
    "  | _ -> (fib (x - 1)) + (fib (x - 2))\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_int, &t_int));

  T("let f = fn x: (Int) (y, z): (Int * Double) -> x + y + z;;",
    &MAKE_FN_TYPE_3(&t_int, &TTUPLE(2, &t_int, &t_num), &t_num));

  // first-class functions
  ({
    Type t0 = {T_VAR, .data = {.T_VAR = "`5"}};
    Type t1 = {T_VAR, .data = {.T_VAR = "`6"}};
    Type t2 = {T_VAR, .data = {.T_VAR = "`8"}};
    Ast *b = T("let sum = fn a b -> a + b;;\n"
               "let proc = fn f a b -> f a b;;\n"
               "proc sum 1 2;\n",
               &t_int);
    TASSERT(b->data.AST_BODY.stmts[1]->md,
            &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t0, &t1, &t2), &t0, &t1, &t2),
            "proc == (`5 -> `6 -> `8) -> `5 -> `6 -> `8");
  });

  ({
    Type t0 = {T_VAR, .data = {.T_VAR = "`5"}};
    Type t1 = {T_VAR, .data = {.T_VAR = "`6"}};
    Type t2 = {T_VAR, .data = {.T_VAR = "`8"}};
    Ast *b = T("let sum = fn a b -> a + b;;\n"
               "let proc = fn f a b -> f a b;;\n"
               "proc sum 1.0 2.0;\n"
               "proc sum 1 2;\n",
               &t_int);

    TASSERT(b->data.AST_BODY.stmts[2]->md, &t_num, "proc sum 1. 2. == Double");
    TASSERT(b->data.AST_BODY.stmts[1]->md,
            &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t0, &t1, &t2), &t0, &t1, &t2),
            "proc == (`5 -> `6 -> `8) -> `5 -> `6 -> `8");
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
    Type t = arithmetic_var("`0");
    T("let list_sum = fn s l ->\n"
      "  match l with\n"
      "  | [] -> s\n"
      "  | x::rest -> list_sum (s + x) rest\n"
      ";;\n",
      &MAKE_FN_TYPE_3(&t, &TLIST(&s), &t));
  });

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

    Ast *original_func =
        b->data.AST_BODY.stmts[1]->data.AST_APPLICATION.function;
    bool is_partial = application_is_partial(b->data.AST_BODY.stmts[1]);

    const char *msg = "application_is_partial fn test\n";
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

  return status == true ? 0 : 1;
}
