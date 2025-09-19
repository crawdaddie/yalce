#include "../lang/parse.h"
#include "../lang/serde.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "../lang/types/unification.h"
#include <string.h>

#define MAX_FAILURES 1000
#define MAX_FAILURE_MSG_LEN 2048

typedef struct {
  char message[MAX_FAILURE_MSG_LEN];
  char file[256];
  int line;
} TestFailure;

static TestFailure failures[MAX_FAILURES];
static int failure_count = 0;

static void add_failure(const char *message, const char *file, int line) {
  if (failure_count < MAX_FAILURES) {
    strncpy(failures[failure_count].message, message, MAX_FAILURE_MSG_LEN - 1);
    failures[failure_count].message[MAX_FAILURE_MSG_LEN - 1] = '\0';
    strncpy(failures[failure_count].file, file, 255);
    failures[failure_count].file[255] = '\0';
    failures[failure_count].line = line;
    failure_count++;
  }
}

static void add_type_failure(const char *message, Type *expected, Type *got,
                             const char *file, int line) {
  if (failure_count < MAX_FAILURES) {
    char buf1[200] = {};
    char buf2[200] = {};
    char full_msg[MAX_FAILURE_MSG_LEN];
    snprintf(full_msg, MAX_FAILURE_MSG_LEN, "%s\nExpected: %s\nGot: %s",
             message, type_to_string(expected, buf1),
             type_to_string(got, buf2));
    strncpy(failures[failure_count].message, full_msg, MAX_FAILURE_MSG_LEN - 1);
    failures[failure_count].message[MAX_FAILURE_MSG_LEN - 1] = '\0';
    strncpy(failures[failure_count].file, file, 255);
    failures[failure_count].file[255] = '\0';
    failures[failure_count].line = line;
    failure_count++;
  }
}

static void print_all_failures() {
  if (failure_count > 0) {
    fprintf(stderr, "\n\n=== FAILING TESTS (%d) ===\n", failure_count);
    for (int i = 0; i < failure_count; i++) {
      fprintf(stderr, "❌ %s\n%s:%d\n\n", failures[i].message, failures[i].file,
              failures[i].line);
    }
  }
}

#define xT(input, type)

#define T(input, type)                                                         \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    printf("\n--------------------------------------\n%s\n", input);           \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {};                                                            \
    stat &= (infer(ast, &ctx) != NULL);                                        \
    stat &= (subst_top_level(ast, &ctx) != NULL);                              \
    stat &= (types_equal(ast->md, type));                                      \
    char buf[200] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ => %s\n", type_to_string(type, buf));                \
    } else {                                                                   \
      char buf2[200] = {};                                                     \
      char fail_msg[MAX_FAILURE_MSG_LEN];                                      \
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s\nExpected: %s\nGot: %s",     \
               input, type_to_string(type, buf),                               \
               type_to_string(ast->md, buf2));                                 \
      add_failure(fail_msg, __FILE__, __LINE__);                               \
    }                                                                          \
    status &= stat;                                                            \
    ast;                                                                       \
  })

#define _T(input)                                                              \
  \ 
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {.subst = NULL};                                               \
    infer(ast, &ctx);                                                          \
    char buf[200] = {};                                                        \
    ast;                                                                       \
  })

#define TASSERT_EQ(t1, t2, msg)                                                \
  ({                                                                           \
    if (types_equal(t1, t2)) {                                                 \
      status &= true;                                                          \
      fprintf(stderr, "✅ %s\n", msg);                                         \
    } else {                                                                   \
      status &= false;                                                         \
      char buf[100] = {};                                                      \
      char buf2[100] = {};                                                     \
      char fail_msg[MAX_FAILURE_MSG_LEN];                                      \
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s\nExpected: %s\nGot: %s",     \
               msg, type_to_string(t2, buf), type_to_string(t1, buf2));        \
      add_failure(fail_msg, __FILE__, __LINE__);                               \
    }                                                                          \
  })

#define TFAIL(input)                                                           \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    printf("\n--------------------------------------\n%s\n", input);           \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {};                                                            \
    stat &= (infer(ast, &ctx) == NULL);                                        \
    char buf[100] = {};                                                        \
    if (stat) {                                                                \
      fprintf(stderr, "✅ fails typecheck\n");                                 \
    } else {                                                                   \
      char fail_msg[MAX_FAILURE_MSG_LEN];                                      \
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN,                                  \
               "Expected to fail typecheck but succeeded:\n%s", input);        \
      add_failure(fail_msg, __FILE__, __LINE__);                               \
    }                                                                          \
    status &= stat;                                                            \
    ast;                                                                       \
  })

#define EXTRA_CONDITION(_cond, _msg)                                           \
  ({                                                                           \
    bool res = (_cond);                                                        \
    if (res) {                                                                 \
      fprintf(stderr, "✅ " _msg "\n");                                        \
    } else {                                                                   \
      char fail_msg[MAX_FAILURE_MSG_LEN];                                      \
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "Condition failed: " _msg);      \
      add_failure(fail_msg, __FILE__, __LINE__);                               \
    }                                                                          \
    res;                                                                       \
  })

#define AST_LIST_NTH(astlist, n)                                               \
  ({                                                                           \
    AstList *ll = astlist;                                                     \
    for (int i = 0; i < n; i++) {                                              \
      ll = ll->next;                                                           \
    }                                                                          \
    ll->ast;                                                                   \
  })

#define TASSERT(_msg, _expr)                                                   \
  ({                                                                           \
    bool res = (_expr);                                                        \
    if (res) {                                                                 \
      fprintf(stderr, "✅ " _msg "\n");                                        \
    } else {                                                                   \
      char fail_msg[MAX_FAILURE_MSG_LEN];                                      \
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "Condition failed: " _msg);      \
      add_failure(fail_msg, __FILE__, __LINE__);                               \
    }                                                                          \
    res;                                                                       \
  })

int test_type_declarations() {
  printf("TEST TYPE DECLARATIONS\n---------------------------------\n");
  bool status = true;

  T("type Cb = Double -> (Int, Int) -> ();",
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

  ({
    Type imatrix = TCONS("Matrix", 3, &t_int, &t_int, &TARRAY(&t_int));

    imatrix.data.T_CONS.names = (char *[]){"rows", "cols", "data"};

    T("type Matrix = (\n"
      "  rows: Int,\n"
      "  cols: Int,\n"
      "  data: Array of T\n"
      ");\n"
      "Matrix 2 2 [|1, 2, 3, 4|]\n",
      &imatrix);
  });

  ({
    Type vtype = TCONS("Value", 3, &t_num, &TARRAY(tvar("Value")), &t_num);

    vtype.data.T_CONS.names = (const char *[3]){"data", "children", "grad"};

    T("type Value = (data: Double, children: (Array of Value), grad: "
      "Double);\n"
      "Value 0. [| |] 0.\n",
      &vtype);
  });

  ({
    Type vtype = TCONS("Value", 3, &t_num, &TARRAY(tvar("Value")), &t_num);

    vtype.data.T_CONS.names = (const char *[3]){"data", "children", "grad"};

    T("type Value = (data: Double, children: (Array of Value), grad: "
      "Double);\n"
      "let const = fn i ->\n"
      "  Value i [| |] 0.\n"
      ";;\n",
      &MAKE_FN_TYPE_2(&t_num, &vtype));
  });

  T("let tensor_ndims = fn (_, sizes, _) -> \n"
    "  array_size sizes \n"
    ";;",
    &MAKE_FN_TYPE_2(&TTUPLE(3, &TVAR("`1"), &TARRAY(&TVAR("`5")), &TVAR("`3")),
                    &t_int));

  // T("type Tensor = (Array of t, Array of Int, Array of Int);\n"
  //   "let tensor_ndims = fn (_, sizes, _): (Tensor of T) -> \n"
  //   "  array_size sizes \n"
  //   ";; \n"
  //   "let x = Tensor [|1,2,3,4|] [|2,2|] [|2,1|];\n"
  //   "tensor_ndims x;\n",
  //   &t_int);
  //
  T("type Tensor = (Array of t, Array of Int, Array of Int);\n"
    "let tensor_ndims = fn (_, sizes, _) -> \n"
    "  array_size sizes \n"
    ";; \n"
    "let x = Tensor [|1,2,3,4|] [|2,2|] [|2,1|];\n"
    "tensor_ndims x;\n",
    &t_int);
  return status;
}

int test_list_processing() {
  bool status = true;
  printf("## LIST PROCESSING FUNCTIONS\n-------------------------------\n");
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
    &MAKE_FN_TYPE_2(&TLIST(&TVAR("`4")), &TOPT(&TVAR("`4"))));

  ({
    Type t = TVAR("`1");
    T("let enqueue = fn (head, tail) item ->\n"
      "  let last = [item] in\n"
      "  match head with\n"
      "  | [] -> (last, last)\n"
      "  | _ -> (\n"
      "    let _ = list_concat tail last in\n"
      "    (head, last)\n"
      "  )\n"
      ";;\n",
      &MAKE_FN_TYPE_3(&TTUPLE(2, &TLIST(&t), &TLIST(&t)), &t,
                      &TTUPLE(2, &TLIST(&t), &TLIST(&t))));
  });

  // T("let enqueue = fn (head, tail): (List of Int, List of Int) item: "
  //   "(Int) "
  //   "->\n"
  //   "  let last = [item] in\n"
  //   "  match head with\n"
  //   "  | [] -> (last, last)\n"
  //   "  | _ -> (\n"
  //   "    let _ = list_concat tail last in\n"
  //   "    (head, last)\n"
  //   "  )\n"
  //   ";;\n",
  //   // &MAKE_FN_TYPE_3(&TTUPLE(2, &TLIST(&TVAR("`0")), &TLIST(&TVAR("`0"))),
  //   //                 &TVAR("`0"),
  //   //                 &TTUPLE(2, &TLIST(&TVAR("`0")), &TLIST(&TVAR("`0"))))
  //   //
  //   &MAKE_FN_TYPE_3(&TTUPLE(2, &TLIST(&t_int), &TLIST(&t_int)), &t_int,
  //                   &TTUPLE(2, &TLIST(&t_int), &TLIST(&t_int))));
  //
  ({
    Type s = arithmetic_var("`4");
    Type t = arithmetic_var("`8");
    T("let list_sum = fn s l ->\n"
      "  match l with\n"
      "  | [] -> s\n"
      "  | x::rest -> list_sum (s + x) rest\n"
      ";;\n"
      "list_sum 0. [1., 2., 3.];\n"
      "list_sum 0 [1, 2, 3];\n",
      &t_int);
  });

  ({
    Type free_var = TVAR("`5");
    Ast *b =
        T("let pop_left = fn (head, tail) ->\n"
          "  match head with\n"
          "  | x::rest -> ((rest, tail), Some x)  \n"
          "  | [] -> ((head, tail), None)\n"
          ";;\n",
          &MAKE_FN_TYPE_2(&TTUPLE(2, &TLIST(&free_var), &TVAR("`2")),
                          &TTUPLE(2, &TTUPLE(2, &TLIST(&free_var), &TVAR("`2")),
                                  &TOPT(&free_var))));
    Ast *match_subj =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
            ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_MATCH.expr;
  });

  T("let list_map = fn f l ->\n"
    "  let aux = fn f l res -> \n"
    "    match l with\n"
    "    | [] -> res\n"
    "    | x :: rest -> aux f rest (f x :: res) \n"
    "  ;;\n"
    "  aux f l []\n"
    ";;\n",
    // (`8 -> `15) -> `8[] -> `15
    &MAKE_FN_TYPE_3(&MAKE_FN_TYPE_2(&TVAR("`8"), &TVAR("`15")),
                    &TLIST(&TVAR("`8")), &TLIST(&TVAR("`15"))));

  T("(+) 1",
    &MAKE_FN_TYPE_2(&arithmetic_var("`1"),
                    &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC,
                                       &arithmetic_var("`1"), &t_int)));

  ({
    Ast *b = T("let list_map = fn f l ->\n"
               "  let aux = fn f l res -> \n"
               "    match l with\n"
               "    | [] -> res\n"
               "    | x :: rest -> aux f rest (f x :: res) \n"
               "  ;;\n"
               "  aux f l []\n"
               ";;\n"
               "(list_map ((+) 1) [0,1,2,3])",
               &TLIST(&t_int));
    Ast *aux_app = AST_LIST_NTH(
        AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
            ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_BODY.stmts,
        1);
    // print_ast(aux_app);
    // print_type(aux_app->data.AST_APPLICATION.function->md);
  });

  T("let list_rev = fn l ->\n"
    "  let aux = fn ll res -> \n"
    "    match ll with\n"
    "    | [] -> res\n"
    "    | x :: rest -> aux rest (x :: res) \n"
    "  ;;\n"
    "  aux l []\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&TLIST(&TVAR("`11")), &TLIST(&TVAR("`11"))));

  T("let of_list = fn l ->\n"
    "  let t = l in \n"
    "  let h = t in\n"
    "  (h, t)\n"
    ";;\n"
    "let append = fn n (h, t) ->\n"
    "  match list_empty h with\n"
    "  | true -> of_list [n,]\n"
    "  | _ -> (\n"
    "    let nt = [n,] in\n"
    "    let tt = list_concat t nt in\n"
    "    (h, tt)\n"
    "  )\n"
    ";;\n",
    &MAKE_FN_TYPE_3(&TVAR("`2"),
                    &TTUPLE(2, &TLIST(&TVAR("`2")), &TLIST(&TVAR("`2"))),
                    &TTUPLE(2, &TLIST(&TVAR("`2")), &TLIST(&TVAR("`2")))));

  ({
    Ast *b =
        T("let pop_left = fn (h, t) ->\n"
          "  match h with\n"
          "  | x::rest -> Some (x, (rest, t))  \n"
          "  | [] -> None\n"
          ";;\n",
          &MAKE_FN_TYPE_2(
              &TTUPLE(2, &TLIST(&TVAR("`5")), &TVAR("`2")),
              &TOPT(&TTUPLE(2, &TVAR("`5"),
                            &TTUPLE(2, &TLIST(&TVAR("`5")), &TVAR("`2"))))));
    Ast *m =
        b->data.AST_BODY.stmts->ast->data.AST_LET.expr->data.AST_LAMBDA.body;
    TASSERT("match result type = Option of `5",
            types_equal(m->md, &TOPT(&TTUPLE(2, &TVAR("`5"),
                                             &TTUPLE(2, &TLIST(&TVAR("`5")),
                                                     &TVAR("`2"))))));
  });

  ({
    Ast *b = T("let of_list = fn l ->\n"
               "  let t = l in \n"
               "  let h = t in\n"
               "  (h, t)\n"
               ";;\n"
               "let append = fn n (h, t) ->\n"
               "  match h with\n"
               "  | [] -> of_list [n,]\n"
               "  | _ -> (\n"
               "    let nt = [n,] in\n"
               "    let tt = list_concat t nt in\n"
               "    (h, tt)\n"
               "  )\n"
               ";;\n"
               "let prepend = fn n (h, t) ->\n"
               "  (n::h, t)\n"
               ";;\n"
               "let pop_left = fn (h, t) ->\n"
               "  match h with\n"
               "  | x::rest -> Some (x, (rest, t))  \n"
               "  | [] -> None\n"
               ";;\n"
               "let q = of_list [1,]\n"
               "  |> append 2 \n"
               "  |> append 3 \n"
               "  |> append 4;  \n"
               "let res = match pop_left q with\n"
               "  | Some (h, _) if h == 1 -> true  \n"
               "  | _ -> false\n"
               ";\n",
               &t_bool);

    // Ast *pop_left = AST_LIST_NTH(b->data.AST_BODY.stmts, 3);
    // print_type(pop_left->md);
    Ast *res_bind = AST_LIST_NTH(b->data.AST_BODY.stmts, 5)
                        ->data.AST_LET.expr->data.AST_MATCH.branches[0]
                        .data.AST_MATCH_GUARD_CLAUSE.test_expr;
    TASSERT("match branch of pop_left q has type Option of (Int * (Int[] * "
            "Int[] ) )",
            types_equal(res_bind->md, &TOPT(&TTUPLE(2, &t_int,
                                                    &TTUPLE(2, &TLIST(&t_int),
                                                            &TLIST(&t_int))))));
  });
  T("let list_rev = fn l ->\n"
    "  let aux = fn ll res ->\n"
    "    match ll with\n"
    "    | [] -> res\n"
    "    | x :: rest -> aux rest (x :: res)\n"
    "  ;;\n"
    "  aux l []\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&TLIST(&TVAR("`11")), &TLIST(&TVAR("`11"))));
  return status;
}

int test_basic_ops() {
  printf("## TEST BASIC OPS\n---------------------------------------------\n");
  bool status = true;
  T("1", &t_int);
  T("1.", &t_num);
  T("'c'", &t_char);
  T("\"hello\"", &t_string);
  T("true", &t_bool);
  T("false", &t_bool);
  T("()", &t_void);
  // T("[]", &TLIST(&TVAR("`0")));
  ({
    Type t0 = arithmetic_var("`0");
    Type t1 = arithmetic_var("`1");
    T("(+)", &MAKE_FN_TYPE_3(
                 &t0, &t1,
                 &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t1, &t0)));
  });
  T("id 1", &t_int);
  T("id 1.", &t_num);
  T("id \'c\'", &t_char);
  T("id \"c\"", &t_string);
  T("id true", &t_bool);
  T("id (1,22)", &TTUPLE(2, &t_int, &t_int));

  T("1 + 2", &t_int);
  T("1. + 2.", &t_num);
  T("1 + 2.0", &t_num);
  T("(1 + 2) * 8", &t_int);
  T("1 + 2.0 * 8", &t_num);
  TFAIL("1 + \"hello\"");
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
  T("let x::y::_ = [1, 2] in x", &t_int);
  T("let x::y::_ = [1, 2] in x + y", &t_int);
  T("let z = [1, 2] in let x::_ = z in x", &t_int);
  TFAIL("let z = 1 in let x::_ = z in x");
  return status;
}

int test_funcs() {
  printf("## TEST FUNCS\n---------------------------------------------\n");
  bool status = true;
  T("let f = fn a b -> 2;;", &MAKE_FN_TYPE_3(&TVAR("`0"), &TVAR("`1"), &t_int));
  T("let f = fn a: (Int) b: (Int) -> 2;;",
    &MAKE_FN_TYPE_3(&t_int, &t_int, &t_int));

  T("let ex_fn = extern fn Int -> Double -> Int;",
    &MAKE_FN_TYPE_3(&t_int, &t_num, &t_int));

  ({
    Type tvar = arithmetic_var("`0");
    Ast *body = T("let f = fn x -> 1 + x;;\n"
                  "f 1;\n"
                  "f 1.;\n",
                  &t_num);

    TASSERT_EQ(AST_LIST_NTH(body->data.AST_BODY.stmts, 0)->md,
               &MAKE_FN_TYPE_2(
                   &tvar, &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC,
                                             &tvar, &t_int)),
               "f == `0 [Arithmetic] -> resolve Arithmetic Int : `0");

    TASSERT_EQ(AST_LIST_NTH(body->data.AST_BODY.stmts, 1)->md, &t_int,
               "f 1 == Int");
    TASSERT_EQ(AST_LIST_NTH(body->data.AST_BODY.stmts, 2)->md, &t_num,
               "f 1. == Num");
  });

  ({
    Type t0 = TVAR("`1");
    Type t1 = TVAR("`2");
    Type t2 = TVAR("`3");

    T("let f = fn (x, y, z) -> (z, y, x);",
      &MAKE_FN_TYPE_2(&TTUPLE(3, &t0, &t1, &t2), &TTUPLE(3, &t2, &t1, &t0)));
  });

  ({
    Type t0 = TVAR("`1");
    Type t1 = TVAR("`2");
    Type t2 = TVAR("`3");

    T("let f = fn (x, y, z) frame_offset: (Int) -> (z, y, x);",
      &MAKE_FN_TYPE_3(&TTUPLE(3, &t0, &t1, &t2), &t_int,
                      &TTUPLE(3, &t2, &t1, &t0)));
  });

  ({
    Type t0 = TVAR("`0");
    Type t1 = TVAR("`1");
    Type t2 = TVAR("`2");
    T("let f = fn x y z -> x + y + z;",
      &MAKE_FN_TYPE_4(
          &t0, &t1, &t2,
          &MAKE_TC_RESOLVE_2(
              TYPE_NAME_TYPECLASS_ARITHMETIC, &t1,
              &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t2, &t0))));
  });

  ({
    T("let count10 = fn x ->\n"
      "  match x with\n"
      "  | 10 -> 10\n"
      "  | _ -> count10 (x + 1)\n"
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
  T("let f = fn x: (Int) (y, z): (Int, Double) -> x + y + z;;",
    &MAKE_FN_TYPE_3(&t_int, &TTUPLE(2, &t_int, &t_num), &t_num));
  ({
    Type t0 = arithmetic_var("`0");
    Type t2 = arithmetic_var("`2");
    Type t3 = arithmetic_var("`3");
    T("let f = fn x (y, z) -> x + y + z;;",
      &MAKE_FN_TYPE_3(
          &t0, &TTUPLE(2, &t2, &t3),
          &MAKE_TC_RESOLVE_2(
              TYPE_NAME_TYPECLASS_ARITHMETIC, &t2,
              &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t3, &t0))));
  });

  Type t0 = arithmetic_var("`0");
  T("let add1 = fn x -> 1 + x;;",
    &MAKE_FN_TYPE_2(
        &t0, &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t0, &t_int)));

  T("let add1 = fn x -> 1 + x;; add1 1", &t_int);
  T("let add1 = fn x -> 1 + x;; add1 1; add1 1.", &t_num);

  ({
    T("let f = fn a b c d -> a == b && c == d;;\n"
      "f 1. 2. 3.;\n"
      "f 1 2 3\n",
      &MAKE_FN_TYPE_2(&t_int, &t_bool));
  });

  ({
    Type t = arithmetic_var("`12");
    Ast *b =
        T("let f = fn a b c -> a + b + c;;\n"
          "f 1 2\n",
          &MAKE_FN_TYPE_2(&t, &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC,
                                                 &t, &t_int)));

    bool is_partial =
        application_is_partial(AST_LIST_NTH(b->data.AST_BODY.stmts, 1));

    const char *msg = "let f = fn a b c -> a + b + c;;\n"
                      "f 1 2\n"
                      "application_is_partial fn test\n";
    if (is_partial) {
      printf("✅ %s", msg);
    } else {
      char fail_msg[MAX_FAILURE_MSG_LEN];
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s", msg);
      add_failure(fail_msg, __FILE__, __LINE__);
    }
    status &= is_partial;
  });

  ({
    Ast *b = _T("let f = fn a b c d e f -> a + b + c + d + e + f;;\n"
                "let x1 = f 1;\n"
                "let x2 = x1 2;\n"
                "let x3 = x2 3;\n");
    bool is_partial = true;
    is_partial &= application_is_partial(
        AST_LIST_NTH(b->data.AST_BODY.stmts, 1)->data.AST_LET.expr);
    is_partial &= application_is_partial(
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_LET.expr);
    is_partial &= application_is_partial(
        AST_LIST_NTH(b->data.AST_BODY.stmts, 3)->data.AST_LET.expr);

    const char *msg = "let f = fn a b c d e f -> a + b + c + d + e + f;;\n"
                      "let x1 = f 1;\n"
                      "let x2 = x1 2;\n"
                      "let x3 = x2 3;\n"
                      "several application_is_partial fn tests\n";
    if (is_partial) {
      printf("✅ %s", msg);
    } else {

      char fail_msg[MAX_FAILURE_MSG_LEN];
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s", msg);
      add_failure(fail_msg, __FILE__, __LINE__);
    }

    ({
      Type t = arithmetic_var("`12");
      T("let f = fn a b c -> a + b + c;;\n"
        "f 1. 2.\n",
        &MAKE_FN_TYPE_2(&t, &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC,
                                               &t, &t_num)));
    });

    T("let f = fn a: (Int) b: (Int) c: (Int) -> (a == b) && (a == c);;\n"
      "f 1 2\n",
      &MAKE_FN_TYPE_2(&t_int, &t_bool));

    ({
      Type t0 = arithmetic_var("`0");
      Type t1 = arithmetic_var("`1");
      T("(1, 2, fn a b -> a + b;);\n",
        &TTUPLE(
            3, &t_int, &t_int,
            &MAKE_FN_TYPE_3(
                &t0, &t1,
                &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t0, &t1))));
    });

    ({
      Type t0 = arithmetic_var("`0");
      Type t1 = arithmetic_var("`1");
      Type tuple = TTUPLE(
          3, &t_int, &t_int,
          &MAKE_FN_TYPE_3(
              &t0, &t1,
              &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t0, &t1)));

      tuple.data.T_CONS.names = (char *[]){"a", "b", "f"};
      T("(a: 1, b: 2, f: (fn a b -> a + b))\n", &tuple);
    });

    status &= is_partial;
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

      char fail_msg[MAX_FAILURE_MSG_LEN];
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s", msg);
      add_failure(fail_msg, __FILE__, __LINE__);
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

      char fail_msg[MAX_FAILURE_MSG_LEN];
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s", msg);
      add_failure(fail_msg, __FILE__, __LINE__);
    }
    status &= res;
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
    Type v = TVAR("`20");
    T("let f = fn a b c d -> a == b && c == d;;\n"
      "f 1. 2. 3.;\n"
      "f 1 2 3\n",
      &MAKE_FN_TYPE_2(&t_int, &t_bool));
  });

  ({
    Type t = arithmetic_var("`12");
    Ast *b =
        T("let f = fn a b c -> a + b + c;;\n"
          "f 1 2\n",
          &MAKE_FN_TYPE_2(&t, &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC,
                                                 &t, &t_int)));

    bool is_partial =
        application_is_partial(AST_LIST_NTH(b->data.AST_BODY.stmts, 1));

    const char *msg = "let f = fn a b c -> a + b + c;;\n"
                      "f 1 2\n"
                      "application_is_partial fn test\n";
    if (is_partial) {
      printf("✅ %s", msg);
    } else {
      char fail_msg[MAX_FAILURE_MSG_LEN];
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s", msg);
      add_failure(fail_msg, __FILE__, __LINE__);
    }
    status &= is_partial;
  });
  ({
    Type t = arithmetic_var("`12");
    T("let f = fn a b c -> a + b + c;;\n"
      "f 1. 2.\n",

      &MAKE_FN_TYPE_2(
          &t, &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t, &t_num)));
  });

  ({
    Ast *b = _T("let f = fn a b c d e f -> a + b + c + d + e + f;;\n"
                "let x1 = f 1;\n"
                "let x2 = x1 2;\n"
                "let x3 = x2 3;\n");
    bool is_partial = true;
    is_partial &= application_is_partial(
        AST_LIST_NTH(b->data.AST_BODY.stmts, 1)->data.AST_LET.expr);
    is_partial &= application_is_partial(
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_LET.expr);
    is_partial &= application_is_partial(
        AST_LIST_NTH(b->data.AST_BODY.stmts, 3)->data.AST_LET.expr);

    const char *msg = "let f = fn a b c d e f -> a + b + c + d + e + f;;\n"
                      "let x1 = f 1;\n"
                      "let x2 = x1 2;\n"
                      "let x3 = x2 3;\n"
                      "several application_is_partial fn tests\n";
    if (is_partial) {
      printf("✅ %s", msg);
    } else {

      char fail_msg[MAX_FAILURE_MSG_LEN];
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s", msg);
      add_failure(fail_msg, __FILE__, __LINE__);
    }
    status &= is_partial;
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
                              &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC,
                                                 &t0, &t1))));
  });

  ({
    Type t0 = arithmetic_var("`0");
    Type t1 = arithmetic_var("`1");
    Type tuple =
        TTUPLE(3, &t_int, &t_int,
               &MAKE_FN_TYPE_3(&t0, &t1,
                               &MAKE_TC_RESOLVE_2(
                                   TYPE_NAME_TYPECLASS_ARITHMETIC, &t0, &t1)));

    tuple.data.T_CONS.names = (char *[]){"a", "b", "f"};
    T("(a: 1, b: 2, f: (fn a b -> a + b))\n", &tuple);
  });

  T("let sq = fn x: (Int) -> x * 1.;;", &MAKE_FN_TYPE_2(&t_int, &t_num));

  T("let bind = extern fn Int -> Int -> Int -> Int;\n"
    "let _bind = fn server_fd server_addr ->\n"
    "  match (bind server_fd server_addr 10) with\n"
    "  | 0 -> Some server_fd\n"
    "  | _ -> None \n"
    ";;\n",
    &MAKE_FN_TYPE_3(&t_int, &t_int, &TOPT(&t_int)));
  // 1st-class callback typing
  ({
    Type t1 = TVAR("`1");
    Type t2 = TVAR("`2");
    Type t4 = TVAR("`4");
    T("let f = fn c a b -> c a b;;",
      &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t1, &t2, &t4), &t1, &t2, &t4));
  });
  ({
    Type t = TVAR("`2");
    T("let f = fn cb -> cb 1 2;;",
      &MAKE_FN_TYPE_2(&MAKE_FN_TYPE_3(&t_int, &t_int, &t), &t));
  });

  ({
    Ast *b = T("let fib = fn x ->\n"
               "  match x with\n"
               "  | 0 -> 0\n"
               "  | 1 -> 1\n"
               "  | _ -> (fib (x - 1)) + (fib (x - 2))\n"
               ";;",

               &MAKE_FN_TYPE_2(&t_int, &t_int));

    Ast final_branch = AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                           ->data.AST_LET.expr->data.AST_LAMBDA.body->data
                           .AST_MATCH.branches[5];

    TASSERT_EQ(final_branch.data.AST_APPLICATION.args[0]
                   .data.AST_APPLICATION.args[0]
                   .md,
               &t_int,
               "references in sub-nodes properly typed :: (x - 1) == Int");

    // TASSERT(
    //            "references in sub-nodes properly typed :: fib == Int -> Int",
    // types_equal(
    // final_branch.data.AST_APPLICATION.function
    //                ->md,
    //            &MAKE_FN_TYPE_2(&t_int,&t_int))
    //         );
  });

  ({
    Ast *b = T("let fib = fn x ->\n"
               "  match x with\n"
               "  | 0 -> 0\n"
               "  | 1 -> 1\n"
               "  | _ -> (fib (x - 1)) + (fib (x - 2))\n"
               ";;\n",
               &MAKE_FN_TYPE_2(&t_int, &t_int));
    Ast *fb = &AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                   ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_MATCH
                   .branches[5];
    print_ast(fb);
    print_type(fb->md);
    print_type(fb->data.AST_APPLICATION.function->md);
    TASSERT("references in sub-nodes properly typed :: fib (x-1) + fib (x-2) "
            "== Int -> Int -> Int",
            types_equal(fb->data.AST_APPLICATION.function->md,
                        &MAKE_FN_TYPE_3(&t_int, &t_int, &t_int)));
  });

  ({
    // Type t0 = TVAR("`7");
    // Type t1 = TVAR("`8");
    // Type t2 = TVAR("`10");
    Ast *b = T("let sum = fn a b -> a + b;;\n"
               "let proc = fn f a b -> f a b;;\n"
               "proc sum 1 2;\n",
               &t_int);

    Ast *proc_inst =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_APPLICATION.function;
    print_ast(proc_inst);
    print_type(proc_inst->md);
    TASSERT(
        "instance of proc function in app is (Int -> Int -> Int) -> Int -> Int",
        types_equal(proc_inst->md,
                    &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t_int, &t_int, &t_int),
                                    &t_int, &t_int, &t_int)));
    Ast *sum_inst =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_APPLICATION.args;
    print_ast(sum_inst);
    print_type(sum_inst->md);
    TASSERT("instance of sum function in app is Int -> Int -> Int",
            types_equal(sum_inst->md, &MAKE_FN_TYPE_3(&t_int, &t_int, &t_int)));
  });

  return status;
}
int test_match_exprs() {
  printf(
      "## TEST MATCH EXPRS\n---------------------------------------------\n");
  bool status = true;

  ({
    Type opt_int = TOPT(&t_int);
    T("Some 1", &opt_int);
  });
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
    Type v = TVAR("`3");
    Type opt = TOPT(&v);
    T("let f = fn x ->\n"
      "match x with\n"
      "  | Some y -> y + 1\n"
      "  | None -> 0\n"
      "  ;;\n",

      &MAKE_FN_TYPE_2(&opt, &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC,
                                               &v, &t_int)));
  });

  ({
    Type v = TVAR("`3");
    Type opt = TOPT(&v);
    T("let f = fn x ->\n"
      "match x with\n"
      "  | Some y -> y * 2\n"
      "  | None -> 0\n"
      "  ;;\n",
      &MAKE_FN_TYPE_2(&opt, &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC,
                                               &v, &t_int)));
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

  ({
    Ast *b = T("let x = 1;\n"
               "match x with\n"
               "| xx if xx > 300 -> xx\n"
               "| 2 -> 0\n"
               "| _ -> 3",
               &t_int);
    Ast *branch =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 1)->data.AST_MATCH.branches;

    Ast *guard = branch->data.AST_MATCH_GUARD_CLAUSE.guard_expr;

    TASSERT_EQ(guard->data.AST_APPLICATION.function->md,
               &MAKE_FN_TYPE_3(&t_int, &t_int, &t_bool),
               "guard clause has type Int -> Int -> Bool\n");
  });

  T("if true then ()\n", &t_void);
  T("if true then 1 else 2\n", &t_int);

  ({
    Ast *b = T("let f = fn x ->\n"
               "  match x with\n"
               "    | Some 1 -> 1\n"
               "    | None -> 0\n"
               ";;\n"
               "f None",
               &t_int);

    // print_type(AST_LIST_NTH(b->data.AST_BODY.stmts, 1)
    //                ->data.AST_APPLICATION.function->md);
    // print_type(
    //     AST_LIST_NTH(b->data.AST_BODY.stmts,
    //     1)->data.AST_APPLICATION.args->md);

    // ->data.AST_LAMBDA.body->data.AST_MATCH
    //              .branches[1]
    //              .data.AST_LIST.items[1];

    //
    // .data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_APPLICATION
    // .args[0]);
  });

  ({
    Ast *b = T("let fib = fn x ->\n"
               "  match x with\n"
               "  | 0 -> 0\n"
               "  | 1 -> 1\n"
               "  | _ -> (fib (x - 1)) + (fib (x - 2))\n"
               ";;\n",
               &MAKE_FN_TYPE_2(&t_int, &t_int));

    Ast *match = AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                     ->data.AST_LET.expr->data.AST_LAMBDA.body;
    TASSERT("match expr input has type Int", types_equal(match->md, &t_int));

    Ast *sum = match->data.AST_MATCH.branches + 5;

    TASSERT("match branch body has type Int", types_equal(sum->md, &t_int));
  });
  return status;
}

int test_coroutines() {
  printf("### TEST COROUTINES\n--------------------------------\n");
  bool status = true;
#define COROUTINE_CONS(f) TCONS(TYPE_NAME_COROUTINE_CONSTRUCTOR, 1, f)
#define COROUTINE_INST(f) TCONS(TYPE_NAME_COROUTINE_INSTANCE, 1, f)

  ({
    Type cor = COROUTINE_INST(&t_num);
    Type constructor = COROUTINE_CONS(&MAKE_FN_TYPE_2(&t_void, &cor));

    T("let co_void = fn () ->\n"
      "  yield 1.;\n"
      "  yield 2.;\n"
      "  yield 3.\n"
      ";;\n",
      &constructor);
  });

  // TFAIL("let co_void = fn () ->\n"
  //       "  yield 1.;\n"
  //       "  yield 2;\n"
  //       "  yield 3.\n"
  //       ";;\n");

  T("let co_void = fn () ->\n"
    "  yield 1.;\n"
    "  yield 2.;\n"
    "  yield 3.\n"
    ";;\n"
    "let x = co_void () in\n"
    "x ()\n",
    &TOPT(&t_num));

  // ({
  //   Ast *b = T("let co_void_rec = fn () ->\n"
  //              "  yield 1.;\n"
  //              "  yield 2.;\n"
  //              "  yield co_void_rec ()\n"
  //              ";;\n",
  //              coroutine_constructor_type_from_fn_type(
  //                  &MAKE_FN_TYPE_2(&t_void, &t_num)));
  //
  //   // Ast *rec_yield =
  //   //     b->data.AST_BODY.stmts[0]
  //   // ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_BODY.stmts[2];
  //   // print_ast(rec_yield);
  //   // print_type(rec_yield->md);
  //
  //   // printf("## rec yield:\n");
  //   // print_type(rec_yield->data.AST_YIELD.expr->md);
  // });

  // ({
  //   Type cor =
  //       TCONS("coroutine", 2, &t_void, &MAKE_FN_TYPE_2(&t_void,
  //       &TOPT(&t_num)));
  //   T("let ne = fn () ->\n"
  //     "  yield 300.;\n"
  //     "  yield 400.\n"
  //     ";;\n"
  //     "let co_void = fn () ->\n"
  //     "  yield 1.;\n"
  //     "  yield 2.;\n"
  //     "  yield ne ();\n"
  //     "  yield 3.\n"
  //     ";;\n",
  //     coroutine_constructor_type_from_fn_type(
  //         &MAKE_FN_TYPE_2(&t_void, &t_num)));
  // });

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
  //
  ({
    Type cor = COROUTINE_INST(&t_int);
    Type cor_cons = COROUTINE_CONS(&MAKE_FN_TYPE_2(&t_void, &cor));
    Ast *l = T("let f = fn () -> \n"
               "  let x = 1;\n"
               "  yield x;\n"
               "  yield x + 2\n"
               "  ;;\n",
               &cor_cons);

    // AstList *boundary_crossers =
    //     l->data.AST_BODY.stmts[0]
    //         ->data.AST_LET.expr->data.AST_LAMBDA.yield_boundary_crossers;
    //
    // Ast *b = boundary_crossers->ast;
    //
    // printf("boundary crosser (implicit state param): \n");
    // print_ast(b);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    Type cor_cons = COROUTINE_CONS(&MAKE_FN_TYPE_2(&t_void, &cor));
    Ast *l = T("let f = fn () -> \n"
               "  let x = 1;\n"
               "  yield x;\n"
               "  yield x + 2;\n"
               "  let y = 200;\n"
               "  yield x + y;\n"
               "  yield y\n"
               "  ;;\n",
               &cor_cons);

    AstList *bx =
        AST_LIST_NTH(l->data.AST_BODY.stmts, 0)
            ->data.AST_LET.expr->data.AST_LAMBDA.yield_boundary_crossers;
    int num_xs = 0;
    Ast *b1 = bx->ast;
    Ast *b2 = bx->next->ast;

    while (bx) {
      num_xs++;
      bx = bx->next;
    }

    status &= EXTRA_CONDITION(num_xs == 2, "2 implicit state params");
    // printf("boundary crossers (implicit state params): \n", num_xs);
    // print_ast(b1);
    // print_ast(b2);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    Type cor_cons = COROUTINE_CONS(&MAKE_FN_TYPE_2(&t_void, &cor));
    Ast *l = T("let f = fn () -> \n"
               "  let x = 1;\n"
               "  yield x;\n"
               "  let y = 200;\n"
               "  yield x + 2 + y;\n"
               "  yield y\n"
               "  ;;\n",
               &cor_cons);

    AstList *bx =
        AST_LIST_NTH(l->data.AST_BODY.stmts, 0)
            ->data.AST_LET.expr->data.AST_LAMBDA.yield_boundary_crossers;

    int num_xs = 0;
    while (bx) {
      num_xs++;
      bx = bx->next;
    }

    status &= EXTRA_CONDITION(num_xs == 2, "2 implicit state params");
  });

  T("let str_map = fn x -> `{ANSI_BOLD}[str {x}]{ANSI_RESET}`;;",
    &MAKE_FN_TYPE_2(&TVAR("`0"), &t_string));

  ({
    Type inst = COROUTINE_INST(&t_string);
    Ast *b = T("let str_map = fn x -> `[str {x}]`;;\n"
               "let co_void = fn () ->\n"
               "  yield 1;\n"
               "  yield 2;\n"
               "  yield 3;\n"
               "  yield 4;\n"
               "  yield 5;\n"
               "  yield 6;\n"
               "  yield 7 \n"
               ";;\n"
               "co_void () |> cor_map str_map;\n",
               &inst);
    // print_ast(b->data.AST_BODY.stmts[2]);
    Ast *cor_map_arg =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_APPLICATION.args;

    Type mapper = MAKE_FN_TYPE_2(&t_int, &t_string);
    // Type mapper = t_ptr;
    bool res = types_equal(cor_map_arg->md, &mapper);
    const char *msg = "runner arg can be materialised to specific type:";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(&mapper);
      status &= true;
    } else {
      status &= false;
      add_type_failure(msg, &mapper, cor_map_arg->md, __FILE__, __LINE__);
    }

    // print_ast(AST_LIST_NTH(b->data.AST_BODY.stmts, 0));
    // print_type(AST_LIST_NTH(b->data.AST_BODY.stmts, 0)->md);
    //
    // print_ast(
    //     AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
    //         ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_LIST.items
    //         +
    //     1);
    //
    // print_type(
    //     (AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
    //          ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_LIST.items
    //          +
    //      1)
    //         ->md);
  });

  ({
    Type cor_type = COROUTINE_INST(&t_int);
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

  ({
    Ast *b = T("let schedule_event = extern fn (T -> Int -> ()) -> Double -> T "
               "-> ();\n"
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
    Ast *runner_arg =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 4)->data.AST_APPLICATION.args;
    Ast *cor_arg =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 4)->data.AST_APPLICATION.args + 2;

    Type cor_type = COROUTINE_INST(&t_num);
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
      add_type_failure(msg, &runner_fn_arg_type, runner_arg->md, __FILE__,
                       __LINE__);
    }
  });

  ({
    Type cor = COROUTINE_INST(&t_num);
    T("let f = iter_of_list [1,2,3]\n"
      "  |> cor_map (fn x -> x * 2.;)\n"
      "  |> cor_loop\n",
      &cor);
  });

  ({
    Type cor = COROUTINE_INST(&t_num);
    Type cor_cons = COROUTINE_CONS(&MAKE_FN_TYPE_2(&t_void, &cor));
    T("let f = fn () ->\n"
      "  yield 1.;\n"
      "  yield f ()\n"
      ";;\n ",
      &cor_cons);
  });
  ({
    Type t = arithmetic_var("`0");
    Type t2 = arithmetic_var("`1");
    Type inst = COROUTINE_INST(&t);
    Type cons = COROUTINE_CONS(&MAKE_FN_TYPE_3(&t, &t2, &inst));
    Ast *b = T("let fib = fn a b ->\n"
               "  yield a;\n"
               "  yield fib b (a + b)\n"
               ";;\n",
               &cons);

    // Ast *yield =
    //
    //     b->data.AST_BODY.stmts[0]
    //         ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_BODY.stmts[1]
    //         ->data.AST_YIELD.expr;
    // print_ast(yield);
    // print_type(yield->md);
    // print_type(yield->data.AST_APPLICATION.function->md);
    // print_type(yield->data.AST_APPLICATION.args->md);
    // print_type((yield->data.AST_APPLICATION.args + 1)->md);
    //
  });

  ({
    Type tuple = TTUPLE(2, &MAKE_FN_TYPE_2(&t_void, &TOPT(&t_num)),
                        &COROUTINE_INST(&t_int));

    tuple.data.T_CONS.names = (char *[]){"dur", "note"};
    T("let seq = fn (durs, notes) ->\n"
      "  let d = use_or_finish @@ durs ();\n"
      "  let n = use_or_finish @@ notes ();\n"
      "  yield d;\n"
      "  yield (seq (durs, notes))\n"
      ";;\n"

      "let vals = (\n"
      "  dur: (fn () -> Some 0.2),\n"
      "  note: iter_of_array [|39,    32, 41,   42,  35,    37,   41, 42, "
      "|]\n"
      ");\n",
      &tuple);
  });

  return status;
}
int test_first_class_funcs() {
  bool status = true;

  // Ast *b = T("let schedule_event = extern fn (T -> Int -> ()) -> "
  //            "Double -> T -> ();\n"
  //            "let co_void = fn () ->\n"
  //            "  yield 0.125;\n"
  //            "  yield co_void ()\n"
  //            ";;\n"
  //
  //            "let c = co_void ();\n"
  //
  //            "let runner = fn c off ->\n"
  //            "  match c () with\n"
  //            "  | Some dur -> schedule_event runner dur c\n"
  //            "  | None -> () \n"
  //            ";;\n"
  //
  //            "schedule_event runner 0. c\n",
  //            &t_void);
  ({
    Ast *b =
        T("type SchedulerCallback = (() -> Option of Double) -> Int -> ();\n"
          "let schedule_event = extern fn (T -> Int -> ()) -> Double -> T -> "
          "();\n"
          "let runner = fn c off ->\n"
          "  match c () with\n"
          "  | Some dur -> schedule_event runner dur c\n"
          "  | None -> () \n"
          ";;\n"
          "schedule_event runner 0. c\n",
          &t_void);

    Ast *runner_arg =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 3)->data.AST_APPLICATION.args;
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

      add_type_failure(msg, &runner_fn_arg_type, runner_arg->md, __FILE__,
                       __LINE__);
    }
  });

  return status;
}

int test_closures() {
  bool status = true;

  ({
    Ast *b = T("fn () ->\n"
               "let z = 2;\n"
               "(fn () -> z + 2);\n"
               ";\n",
               &MAKE_FN_TYPE_3(&t_void, &t_void, &t_int));

    Type *closure_type =
        AST_LIST_NTH(AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                         ->data.AST_LAMBDA.body->data.AST_BODY.stmts,
                     1)
            ->md;

    bool res = types_equal(closure_type, &MAKE_FN_TYPE_2(&t_void, &t_int));
    res &= (closure_type->closure_meta != NULL);
    res &= (types_equal(closure_type->closure_meta, &TTUPLE(1, &t_int)));

    const char *msg = "closure has type () -> Int and contains a reference to "
                      "the types of closed-over vals (Int)\n";
    if (res) {
      printf("✅ %s", msg);
    } else {

      char fail_msg[MAX_FAILURE_MSG_LEN];
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s", msg);
      add_failure(fail_msg, __FILE__, __LINE__);
    }
    status &= res;
  });

  ({
    Ast *b = T("fn () ->\n"
               "let z = 2;\n"
               "let x = 3.;\n"
               "(fn () -> z + 2 + x);\n"
               ";\n",
               &MAKE_FN_TYPE_3(&t_void, &t_void, &t_num));

    Type *closure_type =
        AST_LIST_NTH(AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                         ->data.AST_LAMBDA.body->data.AST_BODY.stmts,
                     2)
            ->md;

    // printf("closure type\n");
    // print_type(closure_type);

    bool res = types_equal(closure_type, &MAKE_FN_TYPE_2(&t_void, &t_num));
    res &= (closure_type->closure_meta != NULL);
    res &=
        (types_equal(closure_type->closure_meta, &TTUPLE(2, &t_num, &t_int)));

    const char *msg =
        "closure has type () -> Double and contains a reference to "
        "the types of closed-over vals (Double * Int)\n";
    if (res) {
      printf("✅ %s", msg);
    } else {

      char fail_msg[MAX_FAILURE_MSG_LEN];
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s", msg);
      add_failure(fail_msg, __FILE__, __LINE__);
    }
    status &= res;
  });
  return status;
}

int test_refs() {
  bool status = true;

  ({
    Type v = arithmetic_var("`5");
    T("let Ref = fn item -> [|item|];;\n"
      "let incr_ref = fn rx ->\n"
      "  let x = rx[0];\n"
      "  let inc = x + 1;\n"
      "  rx[0] := inc;\n"
      "  inc\n"
      ";;\n",
      &MAKE_FN_TYPE_2(
          &TARRAY(&v),
          &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &v, &t_int)));
  });

  ({
    Type v = arithmetic_var("`5");
    Ast *b = T("let Ref = fn item -> [|item|];;\n"
               "let incr_ref = fn rx ->\n"
               "  let x = rx[0];\n"
               "  let inc = x + 1;\n"
               "  rx[0] := inc;\n"
               "  inc\n"
               ";;\n"
               "incr_ref [| 1 |]\n",
               &t_int);

    status &= EXTRA_CONDITION(
        types_equal(
            AST_LIST_NTH(b->data.AST_BODY.stmts, 2)
                ->data.AST_APPLICATION.function->md,
            &MAKE_FN_TYPE_2(&TCONS(TYPE_NAME_ARRAY, 1, &t_int), &t_int)),
        "incr_ref [|1|] has type Array of Int -> Int");
  });
  return status;
}
int test_modules() {
  bool status = true;

  ({
    Type mod_type = TCONS(TYPE_NAME_MODULE, 2, &t_int,
                          &MAKE_FN_TYPE_2(&TARRAY(&TVAR("`2")), &t_int));
    const char *names[2] = {"x", "size"};
    mod_type.data.T_CONS.names = names;
    T("let Mod = module () ->\n"
      "  let x = 1;\n"
      "  let size = fn arr ->\n"
      "    array_size arr\n"
      "  ;;\n"
      ";\n",
      &mod_type);
  });

  ({
    // parametrized module
    Type mod_type = TCONS(TYPE_NAME_MODULE, 2, &t_int,
                          &MAKE_FN_TYPE_2(&TARRAY(&TVAR("`2")), &t_int));
    // const char *names[2] = {"x", "size"};
    // mod_type.data.T_CONS.names = names;

    T("let Mod = module T U ->\n"
      "  let x = 1;\n"
      "  let size = fn arr ->\n"
      "    array_size arr\n"
      "  ;;\n"
      ";\n",
      &mod_type);

    T("let Mod = module T: (Arithmetic, Eq) U ->\n"
      "  let x = 1;\n"
      "  let size = fn arr ->\n"
      "    array_size arr\n"
      "  ;;\n"
      ";\n",
      &mod_type);
  });
  return status;
}

int test_array_processing() {
  bool status = true;
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

  // T("let set_ref = array_set 0;",
  //   &MAKE_FN_TYPE_3(&TARRAY(&TVAR("`0")), &TVAR("`0"),
  //   &TARRAY(&TVAR("`0"))));

  // T("let x = [|1|]; let set_ref = array_set 0; set_ref x 3",
  // &TARRAY(&t_int));
  T("let (@) = array_at",
    &MAKE_FN_TYPE_3(&TARRAY(&TVAR("`0")), &t_int, &TVAR("`0")));

  T("let rand_int = extern fn Int -> Int;\n"
    "let array_choose = fn arr ->\n"
    "  let idx = rand_int (array_size arr);\n"
    "  array_at arr idx \n"
    ";;\n",
    &MAKE_FN_TYPE_2(&TARRAY(&TVAR("`6")), &TVAR("`6")));

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
  return status;
}
int test_networking_funcs() {
  bool status = true;

  ({
    Ast *b = T(
        "let pop_left = fn (head, tail) ->\n"
        "  match head with\n"
        "  | [] -> ((head, tail), None)\n"
        "  | x::rest -> ((rest, tail), Some x)  \n"
        ";;\n",
        &MAKE_FN_TYPE_2(&TTUPLE(2, &TLIST(&TVAR("`6")), &TVAR("`2")),
                        &TTUPLE(2, &TTUPLE(2, &TLIST(&TVAR("`6")), &TVAR("`2")),
                                &TOPT(&TVAR("`6")))));
    Ast none = AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                   ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_MATCH
                   .branches[1]
                   .data.AST_LIST.items[1];

    bool res = types_equal(none.md, &TOPT(&TVAR("`6")));
    const char *msg = "None return val";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(none.md);
      status &= true;
    } else {
      status &= false;
      add_type_failure(msg, &TOPT(&TVAR("`4")), none.md, __FILE__, __LINE__);
    }
  });
  T("let loop = fn () ->\n"
    "  loop ();\n"
    "  ()\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_void, &t_void));

  xT("let accept = extern fn Int -> Ptr -> Ptr -> Int;\n"
     "let proc_tasks = extern fn (Queue of l) -> Int -> ();\n"
     "let proc_loop = fn tasks server_fd ->\n"
     "  let ts = match (queue_pop_left tasks) with\n"
     "  | Some r -> (\n"
     "    match (r ()) with\n"
     "    | Some _ -> queue_append_right tasks r\n"
     "    | None -> tasks\n"
     "  )\n"
     "  | None -> queue_of_list [ (accept_connections server_fd) ]\n"
     "  in\n"
     "  proc_loop ts server_fd\n"
     ";;\n",
     &MAKE_FN_TYPE_3(&TCONS(TYPE_NAME_QUEUE, 1, &TVAR("l")), &t_int, &t_void));
  return status;
}

bool test_audio_funcs() {
  bool status = true;

  ({
    T("let instantiate_template = extern fn List of (Int, Double) -> Ptr -> "
      "Ptr;\n"
      "let f = fn freq ->\n"
      "  instantiate_template [(0, freq),]\n"
      ";;\n",
      &MAKE_FN_TYPE_3(&t_num, &t_ptr, &t_ptr));
  });

  ({
    T("let instantiate_template = extern fn List of (Int, Double) -> Ptr -> "
      "Ptr;\n"
      "let f = fn (idx, freq) ->\n"
      "  instantiate_template [(idx, freq),]\n"
      ";;\n",
      &MAKE_FN_TYPE_3(&TTUPLE(2, &t_int, &t_num), &t_ptr, &t_ptr));
  });

  ({
    Ast *b = T("type NoteCallback = Int -> Double -> ();\n"
               "let register_note_on_handler = extern fn NoteCallback -> Int "
               "-> ();\n"
               "register_note_on_handler (fn n vel -> vel + 0.0) 0\n",
               &t_void);

    Ast *plus_app =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_APPLICATION.args;

    status &= EXTRA_CONDITION(
        types_equal(plus_app->md, &MAKE_FN_TYPE_3(&t_int, &t_num, &t_num)),
        "callback constraint passed down to lambda is Int -> Double -> "
        "Double");

    // print_ast(plus_app);
    // print_type(plus_app->md);

    // status &= EXTRA_CONDITION(
    //     types_equal(
    //         plus_app->md,
    //         &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t_num,
    //         &t_num)),
    //     "callback constraint passed down to lambda -> (arithmetic resolve "
    //     "Double : Double)");
  });
  return status;
}

bool test_type_exprs() {
  bool status = true;
  status &= TASSERT("fn type expression", {
    Ast *ast =
        parse_input("type f = (T -> Int -> ()) -> Double -> T -> ();", NULL);
    TICtx ctx = {};
    infer(ast, &ctx);
    Scheme sch = ctx.env->scheme;
    bool res = true;
    res &= sch.vars != NULL && strcmp(sch.vars->var, "T") == 0;
    Type t = TVAR("T");
    res &= types_equal(sch.type,
                       &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t, &t_int, &t_void),
                                       &t_num, &t, &t_void));
    res;
  });

  return status;
}

bool test_parser_combinators() {
  printf("### TEST PARSER COMBINATORS\n--------------------------------\n");
  bool status = true;

  ({
    Type a = TVAR("`2");
    Type b = TVAR("`7");
    Type c = TVAR("`8");
    Type d = TVAR("`11");
    Ast *bd =
        T("let bind = fn p f input ->\n"
          "  match p input with\n"
          "  | Some (x, rest) -> f x rest  \n"
          "  | None -> None\n"
          ";;\n",
          &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_2(&a, &TOPT(&TTUPLE(2, &b, &c))),
                          &MAKE_FN_TYPE_3(&b, &c, &TOPT(&d)), &a, &TOPT(&d))

        );
  });
  ({
    Ast *bd =
        T("type Parser = String -> Option of (T, String);\n"
          "let bind = fn p f input ->\n"
          "  match p input with\n"
          "  | None -> None\n"
          "  | Some (x, rest) -> f x rest  \n"
          ";;\n"

          "let ( >>= ) = bind;\n"

          "let isdigit = extern fn Char -> Bool;\n"
          "let digit = fn input ->\n"
          "  let s = array_size input;\n"
          "  match (s, input[0]) with\n"
          "  | (0, _) -> None\n"
          "  | (_, x) if isdigit x -> Some (array_range 0 1 input, array_succ "
          "input)\n"
          "  | _ -> None\n"
          ";;\n"

          "let two_digits = fn input ->\n"
          "  digit >>= (fn first ->\n"
          "  digit >>= (fn second ->\n"
          "    (fn inp -> Some ((first, second), inp))\n"
          "  )) input\n"
          ";;\n",
          &MAKE_FN_TYPE_2(
              &t_string,
              &TOPT(&TTUPLE(2, &TTUPLE(2, &t_string, &t_string), &t_string))));

    Ast *digit = AST_LIST_NTH(bd->data.AST_BODY.stmts, 4);

    status &= TASSERT(
        "digit fn has type Parser of String", ({
          types_equal(digit->md,
                      &MAKE_FN_TYPE_2(&t_string,
                                      &TOPT(&TTUPLE(2, &t_string, &t_string))));
        }));

    Ast *fn_bd = AST_LIST_NTH(bd->data.AST_BODY.stmts, 5)
                     ->data.AST_LET.expr->data.AST_LAMBDA.body;
    Ast *fn_first = fn_bd->data.AST_APPLICATION.args + 1;

    status &= TASSERT(
        "(fn first -> ...) arg has type Parser of (String, String)",
        types_equal(
            fn_first->md,
            &MAKE_FN_TYPE_3(&t_string, &t_string,
                            &TOPT(&TTUPLE(2, &TTUPLE(2, &t_string, &t_string),
                                          &t_string)))));

    Ast *fn_second =
        fn_first->data.AST_LAMBDA.body->data.AST_APPLICATION.args + 1;

    status &= TASSERT(
        "(fn second -> ...) arg has type Parser of (String, String)",
        types_equal(
            fn_second->md,
            &MAKE_FN_TYPE_3(&t_string, &t_string,
                            &TOPT(&TTUPLE(2, &TTUPLE(2, &t_string, &t_string),
                                          &t_string)))));
  });
  return status;
}
bool test_opts() {

  printf("## TEST OPTION OPS\n---------------------------------------------\n");
  bool status = true;
  ({
    Ast *b = T("Some 1 == None", &t_bool);
    TASSERT("LHS of == operation forces None to be same type as LHS: ",
            types_equal(
                b->data.AST_BODY.stmts->ast->data.AST_APPLICATION.function->md,
                &MAKE_FN_TYPE_3(&TOPT(&t_int), &TOPT(&t_int), &t_bool)));

    // print_ast(b->data.AST_BODY.stmts->ast->data.AST_APPLICATION.args + 1);
    // print_type(
    //     (b->data.AST_BODY.stmts->ast->data.AST_APPLICATION.args + 1)->md);
  });
  ({
    Ast *b = T("Some 1", &TOPT(&t_int));
    TASSERT("Some instance has application form of Option of Int\n",
            types_equal(
                b->data.AST_BODY.stmts->ast->data.AST_APPLICATION.function->md,
                &TOPT(&t_int)));
  });
  return status;
}

int main() {
  initialize_builtin_schemes();

  bool status = true;
  status &= test_basic_ops();
  status &= test_match_exprs();
  status &= test_coroutines();
  status &= test_first_class_funcs();
  status &= test_closures();
  status &= test_refs();
  status &= test_modules();
  status &= test_array_processing();
  status &= test_networking_funcs();
  status &= test_type_exprs();
  status &= test_parser_combinators();
  status &= test_type_declarations();
  status &= test_audio_funcs();
  status &= test_list_processing();
  status &= test_funcs();
  status &= test_opts();

  print_all_failures();
  return status == true ? 0 : 1;
}

// (Array of `5 [Arithmetic, ] -> tc resolve Arithmetic [ `5 [Arithmetic, ] :
// tc resolve Arithmetic [ Int : `5 [Arithmetic, ]]]) (Array of `5 -> tc
// resolve Arithmetic [ `5                : tc resolve Arithmetic [ tc resolve
// Arithmetic [ Int : `5] : `5]])
