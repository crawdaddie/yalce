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
    ast;                                                                       \
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

  ({
    Type tvar = {T_VAR, .data = {.T_VAR = "`0"}};
    T("x + 1", &tvar);
  });

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

  T("let z = [1, 2] in let x::_ = z in x", &t_int);
  TFAIL("let z = 1 in let x::_ = z in x");

  T("let f = fn a b -> 2;;", &t_int);

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

  return status == true ? 0 : 1;
}
