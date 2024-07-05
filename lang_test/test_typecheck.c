#include "../lang/parse.h"
#include "../lang/serde.h"
#include "../lang/types/inference.h"
#include "../lang/types/util.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

bool is_builtin_type(Type *t) {
  return (t == &t_int) || (t == &t_num) || (t == &t_string) || (t == &t_bool) ||
         (t == &t_void);
}
static void free_type(Type *t) {
  if (is_builtin_type(t)) {
    printf("don't free %p\n", t);
    return;
  }
  switch (t->kind) {
  case T_VAR: {
    // free(t->data.T_VAR);
  }

  case T_FN: {
    free_type(t->data.T_FN.from);
    free_type(t->data.T_FN.to);
  }
  };
  free(t);
}

bool typecheck_prog(Ast *prog) {
  infer_ast(NULL, prog);
  return true;
}

bool test(Ast *prog, Type **exp_types) {
  int pass = true;
  for (size_t i = 0; i < prog->data.AST_BODY.len; ++i) {

    Ast *stmt = prog->data.AST_BODY.stmts[i];

    if (stmt->tag == AST_BODY) {
      pass &= test(stmt, exp_types + i);
    } else {
      bool t = types_equal(stmt->md, exp_types[i]);
      printf("\e[1mstmt:\e[0m\n");
      print_ast(stmt);

      printf("%s\e[1mtype: \e[0m", t ? "✅" : "❌");
      print_type(stmt->md);
      if (!t) {
        printf(" expected ");
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
  printf("\e[1minput:\e[0m\n%s\n", input);

  Type *tcheck_val = infer_ast(&env, prog);

  if (!tcheck_val) {
    printf("❌ Type inference failed\n");
  } else {
    bool pass = test(prog, exp_types);
  }

  printf("-----\n");

  free(sexpr);
  free(prog);
  yyrestart(NULL);
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

  printf("\e[1minput:\e[0m\n%s\n", input);
  Type *tcheck_val = infer_ast(&env, prog);
  if (!tcheck_val) {
    printf("❌ Type inference failed\n");
  } else {
    bool pass = test(prog, exp_types);
  }

  printf("-----\n");

  free(sexpr);
  free(prog);
  yyrestart(NULL);
  ast_root = NULL;
  return true;
}

#define T(...)                                                                 \
  (Type *[]) { __VA_ARGS__ }

#define TVAR(name)                                                             \
  (Type) {                                                                     \
    T_VAR, { .T_VAR = name }                                                   \
  }

// Helper function to compare type schemes
bool schemes_equal(TypeScheme *s1, TypeScheme *s2) {
  if (s1->num_variables != s2->num_variables)
    return false;
  for (int i = 0; i < s1->num_variables; i++) {
    if (!types_equal(&s1->variables[i], &s2->variables[i]))
      return false;
  }
  return types_equal(s1->type, s2->type);
}

#define TYPES_EQUAL(id, tvx, exp_type, status)                                 \
  if (types_equal(tvx, exp_type)) {                                            \
    printf("✅\e[1m" id "\e[0m");                                              \
    print_type(tvx);                                                           \
    printf("\n");                                                              \
    status &= true;                                                            \
  } else {                                                                     \
    printf("❌\e[1m" id "\e[0m");                                              \
    print_type(tvx);                                                           \
    printf("expected ");                                                       \
    print_type(&t_int);                                                        \
    printf("\n");                                                              \
    status &= false;                                                           \
  };

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
                   T(create_type_multi_param_fn(2, T(&TVAR("t1"), &TVAR("t1")),
                                                &TVAR("t1"))));

  status &= tcheck("let f = fn x -> (1 + 2 ) * 8 - x > 2; f 1;",
                   T(&(Type){T_FN, {.T_FN = {&t_int, &TVAR("t3")}}}));

  status &= tcheck("let f = fn a b -> a + b;",
                   T(create_type_multi_param_fn(2, T(&TVAR("t1"), &TVAR("t1")),
                                                &TVAR("t1"))));

  status &= tcheck(
      "let f = fn a b -> a + b;;\n"
      "f 1 2",
      T(create_type_multi_param_fn(2, T(&TVAR("t1"), &TVAR("t1")), &TVAR("t1")),
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
                   3, T(&TVAR("t2"), &TVAR("t1"), &TVAR("t2")), &TVAR("t2")),

               create_type_multi_param_fn(1, T(&t_int), &t_int)));

  status &=
      tcheck("(1, 2, 3)", T(tcons("Tuple", T(&t_int, &t_int, &t_int), 3)));

  status &= tcheck("(1, 2., \"hello\", false)",
                   T(tcons("Tuple", T(&t_int, &t_num, &t_string, &t_bool), 4)));

  status &= tcheck(
      "(1, 2., \"hello\", false, ())",
      T(tcons("Tuple", T(&t_int, &t_num, &t_string, &t_bool, &t_void), 5)));

  status &= tcheck("[1, 2, 3]", T(tcons("List", T(&t_int), 1)));
  status &= tcheck("[1., 2., 3.]", T(tcons("List", T(&t_num), 1)));

  return status;
}

int main() {
  bool status = true;
  status &= typecheck_ast();
  // test_generalize();
  // test_instantiate();
  return !status;
}

#undef T
