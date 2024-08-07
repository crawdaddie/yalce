#include "../lang/parse.h"
#include "../lang/serde.h"
#include "../lang/types/inference.h"
#include "../lang/types/util.h"
#include "format_utils.h"
#include "test_typecheck_utils.h"
#include <stdlib.h>
#include <string.h>

bool is_builtin_type(Type *t) {
  return (t == &t_int) || (t == &t_num) || (t == &t_string) || (t == &t_bool) ||
         (t == &t_void) || (t == &t_ptr);
}

bool typecheck_prog(Ast *prog) {
  infer_ast(NULL, prog);
  return true;
}

bool test(char *input, Ast *prog, Type **exp_types) {
  int pass = true;
  for (size_t i = 0; i < prog->data.AST_BODY.len; ++i) {

    Ast *stmt = prog->data.AST_BODY.stmts[i];

    if (stmt->tag == AST_BODY) {
      pass &= test(input, stmt, exp_types + i);
    } else {
      bool t = types_equal(stmt->md, exp_types[i]);
      if (t) {
        printf("✅ ");
        print_ast(stmt);
        printf(" => \e[1mtype: \e[0m");
        print_type(stmt->md);
      } else {
        printf("❌ ");
        print_ast(stmt);
        printf(" => ");
        printf(STYLE_BOLD "type: " STYLE_RESET_ALL);
        print_type(stmt->md);
        printf(STYLE_BOLD " expected: " STYLE_RESET_ALL);
        print_type(exp_types[i]);
      }
      printf("\n");

      pass &= t;
    }
    // free_type(exp_types[i]);
  }
  return pass;
}

bool tcheck(char input[], Type **exp_types) {
  Ast *prog;

  prog = parse_input(input);

  char *sexpr = malloc(sizeof(char) * 200);
  if (prog == NULL) {
    return false;
  }

  reset_type_var_counter();

  TypeEnv *env = NULL;

  Type *tcheck_val = infer_ast(&env, prog);

  if (!tcheck_val) {
    printf("❌ input: %s Type inference failed\n", input);
  } else {
    bool pass = test(input, prog, exp_types);
  }

  printf("---------------------------\n");
  free(sexpr);
  free(prog);
  ast_root = NULL;
  return true;
}

bool tcheck_w_env(TypeEnv *env, char input[], Type **exp_types) {
  Ast *prog;

  prog = parse_input(input);

  char *sexpr = malloc(sizeof(char) * 200);
  if (prog == NULL) {
    return false;
  }

  reset_type_var_counter();

  Type *tcheck_val = infer_ast(&env, prog);
  if (!tcheck_val) {
    printf("❌ input: %s Type inference failed\n", input);
  } else {
    bool pass = test(input, prog, exp_types);
  }

  printf("---------------------------\n");

  free(sexpr);
  free(prog);
  yyrestart(NULL);
  ast_root = NULL;
  return true;
}

#define TYPES_EQUAL(desc, actual_type, exp_type, status)                       \
  if (types_equal(actual_type, exp_type)) {                                    \
    printf("✅ \e[1m" desc "\e[0m");                                           \
    print_type(actual_type);                                                   \
    printf("\n");                                                              \
    status &= true;                                                            \
  } else {                                                                     \
    printf("❌ \e[1m" desc "\e[0m got ");                                      \
    print_type(actual_type);                                                   \
    printf(" expected ");                                                      \
    print_type(exp_type);                                                      \
    printf("\n");                                                              \
    status &= false;                                                           \
  };

bool test_unify() {
  bool status = true;
  {
    Type *result = &TVAR("t0");
    Type *t0 = create_type_fn(TUPLE(2, &t_int, &t_int), result);
    Type *t1 = create_type_fn(TUPLE(2, &TVAR("t0"), &TVAR("t1")), &TVAR("t0"));

    unify(t0, t1);

    TYPES_EQUAL("Unify: specific vs generic type:", t0,
                create_type_fn(tcons("Tuple", T(&t_int, &t_int), 2), &t_int),
                status)
  }

  return status;
}

int typecheck_ast() {
  bool status = true;

  status &= tcheck("1", T(&t_int));
  status &= tcheck("(1 + 2) * 8", T(&t_int));
  status &= tcheck("(1 + 2) * 8. < 2", T(&t_bool));
  status &= tcheck("(1 + 2) * 8. <= 2", T(&t_bool));
  status &= tcheck("let a = (1 + 2) * 8 in a + 212.", T(&t_num));
  status &= tcheck("let a = (1 + 2) * 8 in a + 212", T(&t_int));

  //
  status &= tcheck("let f = fn x -> (1 + 2 ) * 8 - x;",
                   T(&(Type){T_FN, {.T_FN = {&t_int, &t_int}}}));

  status &= tcheck("let f = fn x y -> x + y;",
                   T(create_type_multi_param_fn(2, T(&TVAR("t2"), &TVAR("t2")),
                                                &TVAR("t2"))));

  status &= tcheck("let f = fn x -> (1 + 2 ) * 8 - x > 2; f 1;",
                   T(&(Type){T_FN, {.T_FN = {&t_int, &TVAR("t2")}}}));

  status &= tcheck("let f = fn a b -> a + b;",
                   T(create_type_multi_param_fn(2, T(&TVAR("t2"), &TVAR("t2")),
                                                &TVAR("t2"))));

  status &= tcheck(
      "let f = fn (a, b) -> a + b;",
      T(&(Type){T_FN,
                {.T_FN = {TUPLE(2, &TVAR("t2"), &TVAR("t2")), &TVAR("t2")}}}));

  status &= tcheck("let f = fn () -> 1 + 2;",

                   T(&(Type){T_FN, {.T_FN = {&t_void, &t_int}}}));

  status &= tcheck(
      "let f = fn a b -> a + b;;\n"
      "f 1 2",
      T(create_type_multi_param_fn(2, T(&TVAR("t2"), &TVAR("t2")), &TVAR("t2")),
        &t_int));

  {
    Type tvx = TVAR("'x");
    status &= tcheck_w_env(&(TypeEnv){"x", &tvx},
                           "match x with\n"
                           "| 1 -> 1\n"
                           "| 2 -> 0\n"
                           "| _ -> 3",
                           T(&t_int));
    TYPES_EQUAL("match with expr (x) type:", &tvx, &t_int, status)
  }
  //
  status &= tcheck("let m = fn x ->\n"
                   "(match x with\n"
                   "| 1 -> 1\n"
                   "| 2 -> 0\n"
                   "| _ -> m (x - 1))\n"
                   ";",
                   T(create_type_multi_param_fn(1, T(&t_int), &t_int)));

  // currying
  status &=
      tcheck("let m = fn x y z -> x + y + z;;\n"
             "m 1 2",
             T(create_type_multi_param_fn(
                   3, T(&TVAR("t3"), &TVAR("t2"), &TVAR("t3")), &TVAR("t3")),

               create_type_multi_param_fn(1, T(&t_int), &t_int)));

  status &= tcheck("(1, 2, 3)", T(TUPLE(3, &t_int, &t_int, &t_int)));

  status &= tcheck("(1, 2., \"hello\", false)",
                   T(TUPLE(4, &t_int, &t_num, &t_string, &t_bool)));

  status &= tcheck("let (x, y) = (1, 2) in x", T(&t_int));
  status &= tcheck("let (x, y) = (1, 2) in y", T(&t_int));
  status &= tcheck("let (x, _) = (1, 2) in x", T(&t_int));

  status &= tcheck(
      "let complex_match = fn x -> \n"
      "match x with\n"
      "| (1, _) -> 0\n"
      "| (2, _) -> 100\n"
      "| _      -> 1000\n"
      ";",
      T(&(Type){T_FN, {.T_FN = {TUPLE(2, &t_int, &TVAR("t4")), &t_int}}}));

  status &= tcheck(
      "(1, 2., \"hello\", false, ())",
      T(tcons("Tuple", T(&t_int, &t_num, &t_string, &t_bool, &t_void), 5)));

  status &= tcheck("[1, 2, 3]", T(TLIST(&t_int)));
  status &= tcheck("[1., 2., 3.]", T(TLIST(&t_num)));

  status &= tcheck("1::[1,2]", T(TLIST(&t_int)));

  status &= tcheck("let x::rest = [1,2,3,4]", T(TLIST(&t_int)));

  status &= tcheck("let x::_ = [1,2,3,4] in x + 1", T(&t_int));
  {
    Type tvx = TVAR("'x");
    status &= tcheck_w_env(&(TypeEnv){"x", &tvx}, "x::[1,2]", T(TLIST(&t_int)));
    TYPES_EQUAL("list prepend x with means x has type:", &tvx, &t_int, status)
  }

  status &= tcheck("let x::rest = [1,2,3] in x", T(&t_int));

  status &=
      tcheck("let first = fn (a, _) -> \n"
             "  a\n"
             ";;\n"
             "first (1, 2)",
             T(create_type_fn(TUPLE(2, &TVAR("t1"), &TVAR("t2")), &TVAR("t1")),
               &t_int));

  status &= tcheck("let complex_match = fn x -> \n"
                   "match x with\n"
                   "| (1, _) -> 0\n"
                   "| (2, z) -> 100 + z\n"
                   "| _      -> 1000\n"
                   ";",
                   T(create_type_fn(TUPLE(2, &t_int, &t_int), &t_int)));
  {
    Type *t = TUPLE(2, &TVAR("t3"), &TVAR("t4"));
    Type *res = TUPLE(2, &t_int, &t_int);
    status &= tcheck("let add_tuples = fn (a1, a2) (b1, b2) -> \n"
                     "  (a1 + b1, a2 + b2)\n"
                     ";;\n"
                     "add_tuples (1, 2) (3, 4)",
                     T(create_type_multi_param_fn(2, T(t, t), t), res));
  }

  status &= tcheck("let iter = fn l ->\n"
                   "  match l with\n"
                   "  | [] -> 0\n"
                   "  | x::rest -> iter rest\n"
                   "  ;",
                   T(create_type_fn(TLIST(&TVAR("t6")), &t_int)));
  {

    Type *t = &TVAR("t7");
    status &= tcheck("let sum = fn s l ->\n"
                     "  match l with\n"
                     "  | [] -> s\n"
                     "  | x::rest -> sum (s + x) rest\n"
                     "  ;",
                     T(create_type_multi_param_fn(2, T(t, TLIST(t)), t)));
  }

  {

    Type *t = &TVAR("t7");
    status &=
        tcheck("let list_sum = fn s l ->\n"
               "  match l with\n"
               "  | [] -> s\n"
               "  | x::rest -> list_sum (s + x) rest\n"
               "  \n"
               ";;\n"
               "list_sum 0 [1,2,3,4]",
               T(create_type_multi_param_fn(2, T(t, TLIST(t)), t), &t_int));
  }
  status &= tcheck(
      "let first = fn (a, _) b -> \n"
      "  a\n"
      ";;\n"
      "first (1, 2) (1, 2)",
      T(create_type_multi_param_fn(
            2, T(TUPLE(2, &TVAR("t1"), &TVAR("t2")), &TVAR("t3")), &TVAR("t1")),
        &t_int));

  {
    Type tvx = TVAR("'x");
    status &= tcheck_w_env(&(TypeEnv){"x", &tvx}, "1 + x", T(&t_int));
    bool implements_num = implements_typeclass(&tvx, &TCNum);
    printf("%s",
           implements_num
               ? "✅ \e[1mx implements typeclass 'Num'\e[0m\n"
               : "❌ \e[1mx does not implement typeclass 'Num'\e[0m got \n");
    status &= implements_num;
  }

  {
    Type tvx = TVAR("'x");
    Type tvy = TVAR("'y");
    status &= tcheck_w_env(&(TypeEnv){"x", &tvx, &(TypeEnv){"y", &tvy}},
                           "x + y", T(&tvy));

    bool implements_num = implements_typeclass(&tvy, &TCNum);
    implements_num &= implements_typeclass(&tvy, &TCNum);
    printf("%s", implements_num
                     ? "✅ \e[1m'x & 'y implement typeclass 'Num'\e[0m\n"
                     : "❌ \e[1m'x or 'y does not implement typeclass "
                       "'Num'\e[0m got \n");
    status &= implements_num;
  }

  return status;
}

bool test_first_class_fns() {
  bool result = true;

  Ast *prog = parse_input("let sum = fn a b -> a + b;;\n"
                          "let proc = fn f a b -> f a b;;\n"
                          "\n"
                          "proc sum 1 2\n");
  TypeEnv *env;
  infer_ast(&env, prog);
  // sum typecheck
  Ast *sum_ast = prog->data.AST_BODY.stmts[0];
  TYPES_EQUAL(
      "sum func ", sum_ast->md,
      create_type_multi_param_fn(2, T(&TVAR("t2"), &TVAR("t2")), &TVAR("t2")),
      result)

  // proc typecheck
  Ast *proc_ast = prog->data.AST_BODY.stmts[1];
  TYPES_EQUAL("proc func ", proc_ast->md,
              create_type_multi_param_fn(
                  3,
                  T(create_type_multi_param_fn(2, T(&TVAR("t6"), &TVAR("t7")),
                                               &TVAR("t9")),
                    &TVAR("t6"), &TVAR("t7")),
                  &TVAR("t9")),
              result);

  // application typecheck
  Ast *proc_application_ast = prog->data.AST_BODY.stmts[2];

  TYPES_EQUAL("application arg 1 ",
              proc_application_ast->data.AST_APPLICATION.args[0].md,
              create_type_multi_param_fn(2, T(&t_int, &t_int), &t_int), result)

  TYPES_EQUAL("application arg 2 ",
              proc_application_ast->data.AST_APPLICATION.args[1].md, &t_int,
              result)

  TYPES_EQUAL("application arg 3 ",
              proc_application_ast->data.AST_APPLICATION.args[2].md, &t_int,
              result)

  TYPES_EQUAL("application result ", proc_application_ast->md, &t_int, result)
  return result;
}

int main() {
  bool status = true;
  status &= test_first_class_fns();
  status &= typecheck_ast();
  status &= test_unify();
  return !status;
}

#undef T
