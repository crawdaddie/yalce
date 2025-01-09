#include "../lang/parse.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "serde.h"

// #define xT(input, type)

#define T(input, type)                                                         \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {.env = NULL};                                                 \
    stat &= (infer(ast, &ctx) != NULL);                                        \
    stat &= (types_equal(ast->md, type));                                      \
    char buf[100] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ " input " => %s\n", type_to_string(type, buf));      \
    } else {                                                                   \
      char buf2[100] = {};                                                     \
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

  T("let fib = fn x ->\n"
    "  match x with\n"
    "  | 0 -> 0\n"
    "  | 1 -> 1\n"
    "  | _ -> (fib (x - 1)) + (fib (x - 2))\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_int, &t_int));

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

  return status == true ? 0 : 1;
}
