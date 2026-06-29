#include "../lang/parse.h"
#include "../lang/serde.h"
#include "../lang/types/builtins.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "../lang/types/type_ser.h"

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

typedef struct {
  int left_id;
  int right_id;
  const char *left_name;
  const char *right_name;
} TypeVarAlphaPair;

#define MAX_ALPHA_TYPE_VARS 512

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
    char full_msg[MAX_FAILURE_MSG_LEN];
    char *ex_ts = type_to_string_dynamic(got);
    char *got_ts = type_to_string_dynamic(expected);
    snprintf(full_msg, MAX_FAILURE_MSG_LEN, "%s\nExpected: %s\nGot: %s",
             message, ex_ts, got_ts);
    strncpy(failures[failure_count].message, full_msg, MAX_FAILURE_MSG_LEN - 1);
    failures[failure_count].message[MAX_FAILURE_MSG_LEN - 1] = '\0';
    strncpy(failures[failure_count].file, file, 255);
    failures[failure_count].file[255] = '\0';
    failures[failure_count].line = line;
    failure_count++;
    free(ex_ts);
    free(got_ts);
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

static bool alpha_type_equal_inner(Type *left, Type *right,
                                   TypeVarAlphaPair *pairs, int *pair_count);

static bool alpha_typelist_equal(TypeList *left, TypeList *right,
                                 TypeVarAlphaPair *pairs, int *pair_count) {
  while (left && right) {
    if (!alpha_type_equal_inner(left->type, right->type, pairs, pair_count)) {
      return false;
    }
    left = left->next;
    right = right->next;
  }
  return left == NULL && right == NULL;
}

static bool alpha_typeclass_equal(TypeClass *left, TypeClass *right,
                                  TypeVarAlphaPair *pairs, int *pair_count) {
  if (!left || !right) {
    return true;
  }
  while (left && right) {
    if (strcmp(left->name, right->name) != 0 || left->rank != right->rank) {
      return false;
    }
    if (!alpha_typelist_equal(left->params, right->params, pairs, pair_count)) {
      return false;
    }
    if (!alpha_type_equal_inner(left->module, right->module, pairs,
                                pair_count)) {
      return false;
    }
    left = left->next;
    right = right->next;
  }
  return left == NULL && right == NULL;
}

static bool alpha_typeenv_equal(TypeEnv *left, TypeEnv *right,
                                TypeVarAlphaPair *pairs, int *pair_count) {
  while (left && right) {
    if (strcmp(left->name, right->name) != 0 ||
        !alpha_type_equal_inner(left->type, right->type, pairs, pair_count)) {
      return false;
    }
    left = left->next;
    right = right->next;
  }
  return left == NULL && right == NULL;
}

static bool alpha_bind_vars(int left_id, const char *left_name, int right_id,
                            const char *right_name, TypeVarAlphaPair *pairs,
                            int *pair_count) {
  int left_idx = -1;
  int right_idx = -1;
  for (int i = 0; i < *pair_count; i++) {
    bool left_matches =
        left_id >= 0
            ? pairs[i].left_id == left_id
            : (pairs[i].left_id < 0 && pairs[i].left_name && left_name &&
               strcmp(pairs[i].left_name, left_name) == 0);
    bool right_matches =
        right_id >= 0
            ? pairs[i].right_id == right_id
            : (pairs[i].right_id < 0 && pairs[i].right_name && right_name &&
               strcmp(pairs[i].right_name, right_name) == 0);
    if (left_matches) {
      left_idx = i;
    }
    if (right_matches) {
      right_idx = i;
    }
  }

  if (left_idx >= 0 || right_idx >= 0) {
    return left_idx >= 0 && right_idx >= 0 && left_idx == right_idx;
  }

  if (*pair_count >= MAX_ALPHA_TYPE_VARS) {
    return false;
  }

  pairs[*pair_count].left_id = left_id;
  pairs[*pair_count].right_id = right_id;
  pairs[*pair_count].left_name = left_name;
  pairs[*pair_count].right_name = right_name;
  (*pair_count)++;
  return true;
}

static bool alpha_type_equal_inner(Type *left, Type *right,
                                   TypeVarAlphaPair *pairs, int *pair_count) {
  if (left == right) {
    return true;
  }

  if (left == NULL || right == NULL) {
    return left == right;
  }

  if (left->kind != right->kind) {
    if (left->kind == T_MODULE && right->kind == T_CONS) {
      return false;
    }
    if (right->kind == T_MODULE && left->kind == T_CONS) {
      return false;
    }
    return false;
  }

  if (left->alias || right->alias) {
    if (!(left->alias && right->alias &&
          strcmp(left->alias, right->alias) == 0)) {
      return false;
    }
  }

  if (left->is_coroutine_instance != right->is_coroutine_instance ||
      left->is_recursive_type_ref != right->is_recursive_type_ref) {
    return false;
  }

  switch (left->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_STRING:
  case T_BOOL:
  case T_CHAR:
  case T_VOID:
  case T_EMPTY_LIST:
    return true;

  case T_VAR:
    if (left->data.T_VAR.id < 0 && right->data.T_VAR.id < 0 &&
        left->data.T_VAR.name && right->data.T_VAR.name &&
        strcmp(left->data.T_VAR.name, right->data.T_VAR.name) == 0) {
      return true;
    }
    return alpha_bind_vars(left->data.T_VAR.id, left->data.T_VAR.name,
                           right->data.T_VAR.id, right->data.T_VAR.name, pairs,
                           pair_count);

  case T_RECURSIVE_REF:
    return strcmp(left->data.T_RECURSIVE_REF.name,
                  right->data.T_RECURSIVE_REF.name) == 0;

  case T_TYPECLASS_RESOLVE:
  case T_CONS:
  case T_SUM:
    if (strcmp(left->data.T_CONS.name, right->data.T_CONS.name) != 0 ||
        left->data.T_CONS.num_args != right->data.T_CONS.num_args) {
      return false;
    }
    for (int i = 0; i < left->data.T_CONS.num_args; i++) {
      if (!alpha_type_equal_inner(left->data.T_CONS.args[i],
                                  right->data.T_CONS.args[i], pairs,
                                  pair_count)) {
        return false;
      }
    }
    return true;

  case T_FN:
    return alpha_type_equal_inner(left->data.T_FN.from, right->data.T_FN.from,
                                  pairs, pair_count) &&
           alpha_type_equal_inner(left->data.T_FN.to, right->data.T_FN.to,
                                  pairs, pair_count) &&
           (!left->closure_meta || !right->closure_meta ||
            alpha_type_equal_inner(left->closure_meta, right->closure_meta,
                                   pairs, pair_count));

  case T_MODULE:
    return alpha_typeenv_equal(left->data.T_MODULE.env,
                               right->data.T_MODULE.env, pairs, pair_count);
  }

  return false;
}

static bool test_types_equal(Type *left, Type *right) {
  if (types_equal(left, right)) {
    return true;
  }

  TypeVarAlphaPair pairs[MAX_ALPHA_TYPE_VARS] = {0};
  int pair_count = 0;
  return alpha_type_equal_inner(left, right, pairs, &pair_count);
}

#define xT(input, type)

static TICtx test_ctx;
#define T(input, _type)                                                        \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    printf("\n--------------------------------------\n%s\n", input);           \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {.env = NULL};                                                 \
    stat &= (infer(ast, &ctx) != NULL);                                        \
    stat &= (test_types_equal(ast->type, _type));                              \
    if (stat) {                                                                \
      char *ts = type_to_string_dynamic(_type);                                \
      fprintf(stderr, "✅ => %s\n", ts);                                       \
      free(ts);                                                                \
    } else {                                                                   \
      char fail_msg[MAX_FAILURE_MSG_LEN];                                      \
      char *ts1 = type_to_string_dynamic(_type);                               \
      char *ts2 = type_to_string_dynamic(ast->type);                           \
      snprintf(fail_msg, MAX_FAILURE_MSG_LEN, "%s\nExpected: %s\nGot: %s",     \
               input, ts1, ts2);                                               \
      add_failure(fail_msg, __FILE__, __LINE__);                               \
      free(ts1);                                                               \
      free(ts2);                                                               \
    }                                                                          \
    test_ctx = ctx;                                                            \
    status &= stat;                                                            \
    ast;                                                                       \
  })

#define _T(input)                                                              \
  ({                                                                           \
    reset_type_var_counter();                                                  \
    bool stat = true;                                                          \
    Ast *ast = parse_input(input, NULL);                                       \
    TICtx ctx = {.env = NULL};                                                 \
    infer(ast, &ctx);                                                          \
    char buf[200] = {};                                                        \
    test_ctx = ctx;                                                            \
    ast;                                                                       \
  })
#define TASSERT_EQ(t1, t2, msg)                                                \
  ({                                                                           \
    if (test_types_equal(t1, t2)) {                                            \
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
    TICtx ctx = {.env = NULL};                                                 \
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

static bool assert_bool(bool ok, const char *msg, const char *file, int line) {
  if (ok) {
    fprintf(stderr, "✅ %s\n", msg);
  } else {
    add_failure(msg, file, line);
  }
  return ok;
}

static bool astlist_contains_closed_val(AstList *closed_vals, const char *name,
                                        Type *type) {
  for (AstList *l = closed_vals; l; l = l->next) {
    Ast *ast = l->ast;
    if (!ast || ast->tag != AST_IDENTIFIER) {
      continue;
    }
    if (strcmp(ast->data.AST_IDENTIFIER.value, name) == 0 &&
        types_equal(ast->type, type)) {
      return true;
    }
  }
  return false;
}

static bool assert_lambda_closed_vals(Ast *lambda, int expected_count,
                                      const char **names, Type **types,
                                      const char *msg, const char *file,
                                      int line) {

  bool ok = lambda && lambda->tag == AST_LAMBDA &&
            lambda->data.AST_LAMBDA.num_closed_vals == expected_count;
  for (int i = 0; ok && i < expected_count; i++) {
    ok &= astlist_contains_closed_val(lambda->data.AST_LAMBDA.closed_vals,
                                      names[i], types[i]);
  }
  if (ok) {
    fprintf(stderr, "✅ %s\n", msg);
  } else {
    add_failure(msg, file, line);
  }
  return ok;
}

static bool assert_lambda_coroutine_state_vals(Ast *lambda, int expected_count,
                                               const char **names, Type **types,
                                               const char *msg,
                                               const char *file, int line) {
  bool ok =
      lambda && lambda->tag == AST_LAMBDA &&
      lambda->data.AST_LAMBDA.num_yield_boundary_crossers == expected_count;

  for (int i = 0; ok && i < expected_count; i++) {
    ok &= astlist_contains_closed_val(
        lambda->data.AST_LAMBDA.yield_boundary_crossers, names[i], types[i]);
  }

  if (ok) {
    fprintf(stderr, "✅ %s\n", msg);
  } else {
    add_failure(msg, file, line);
  }
  return ok;
}

static int typelist_len(TypeList *list) {
  int n = 0;
  for (; list; list = list->next) {
    n++;
  }
  return n;
}

static TypeEnv *lookup_test_env_binding(const char *name) {
  return lookup_type_ref(test_ctx.env, name);
}

static bool env_binding_has_trait_predicate(TypeEnv *entry,
                                            const char *trait_name) {
  if (!entry) {
    return false;
  }
  for (Predicate *p = entry->predicates; p; p = p->next) {
    if (p->trait && strcmp(p->trait->name, trait_name) == 0) {
      return true;
    }
  }
  return false;
}

static bool assert_env_binding_predicates(const char *name,
                                          const char *trait_name,
                                          int min_scheme_vars, const char *msg,
                                          const char *file, int line) {
  TypeEnv *entry = lookup_test_env_binding(name);
  bool ok = entry != NULL && entry->type != NULL &&
            typelist_len(entry->scheme_vars) >= min_scheme_vars &&
            env_binding_has_trait_predicate(entry, trait_name);
  if (ok) {
    fprintf(stderr, "✅ %s\n", msg);
  } else {
    add_failure(msg, file, line);
  }
  return ok;
}

static bool assert_env_binding_polymorphic(const char *name,
                                           int min_scheme_vars, const char *msg,
                                           const char *file, int line) {
  TypeEnv *entry = lookup_test_env_binding(name);
  bool ok = entry != NULL && entry->type != NULL && entry->type->kind == T_FN &&
            typelist_len(entry->scheme_vars) >= min_scheme_vars;
  if (ok) {
    fprintf(stderr, "✅ %s\n", msg);
  } else {
    add_failure(msg, file, line);
  }
  return ok;
}

static bool type_is_named_recursive_ref(Type *type, const char *name) {
  return type && type->kind == T_RECURSIVE_REF &&
         strcmp(type->data.T_RECURSIVE_REF.name, name) == 0;
}

static bool is_option_returning_fn(Type *type) {
  return type && type->kind == T_FN && type->data.T_FN.from &&
         is_string_type(type->data.T_FN.from) && type->data.T_FN.to &&
         is_option_type(type->data.T_FN.to);
}

static bool ctx_has_trait_predicate(const char *trait_name) {
  for (Predicate *p = test_ctx.predicates; p; p = p->next) {
    if (p->trait && strcmp(p->trait->name, trait_name) == 0) {
      return true;
    }
  }
  return false;
}

int test_type_declarations() {
  printf("TEST TYPE DECLARATIONS\n---------------------------------\n");
  bool status = true;

  T("type Cb = Double -> (Int, Int) -> ();",
    &MAKE_FN_TYPE_3(&t_num, &TTUPLE(2, &t_int, &t_int), &t_void));

  ({
    Type tenum = {.kind = T_SUM,
                  .data = {.T_CONS = {.name = "Enum",
                                      .args = (Type *[]){&TCONS("A", 0, NULL),
                                                         &TCONS("B", 0, NULL),
                                                         &TCONS("C", 0, NULL)},
                                      .num_args = 3}}};

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
    Ast *ast = _T("type Value = (data: Double, children: (Array of Value), "
                  "grad: Double);\n"
                  "Value 0. [| |] 0.\n");
    Type *vt = ast ? ast->type : NULL;
    Type *children = vt ? get_struct_member_type("children", vt) : NULL;
    status &= TASSERT("recursive Value constructor application returns Value",
                      vt && vt->kind == T_CONS &&
                          strcmp(vt->data.T_CONS.name, "Value") == 0);
    status &=
        TASSERT("Value.data field is Double",
                vt && types_equal(get_struct_member_type("data", vt), &t_num));
    status &=
        TASSERT("Value.children field is Array of recursive Value",
                children && is_array_type(children) &&
                    (type_is_named_recursive_ref(children->data.T_CONS.args[0],
                                                 "Value") ||
                     (children->data.T_CONS.args[0] &&
                      children->data.T_CONS.args[0]->kind == T_CONS &&
                      strcmp(children->data.T_CONS.args[0]->data.T_CONS.name,
                             "Value") == 0)));
  });

  ({
    Ast *ast = _T("type Value = (data: Double, children: (Array of Value), "
                  "grad: Double);\n"
                  "let const = fn i ->\n"
                  "  Value i [| |] 0.\n"
                  ";;\n");
    TypeEnv *entry = lookup_test_env_binding("const");
    Type *ret = entry && entry->type && entry->type->kind == T_FN
                    ? fn_return_type(entry->type)
                    : NULL;
    status &= TASSERT("recursive Value constructor function infers",
                      ast && entry && entry->type && entry->type->kind == T_FN);
    status &=
        TASSERT("const takes a Double",
                entry && types_equal(entry->type->data.T_FN.from, &t_num));
    status &= TASSERT("const returns Value",
                      ret && ret->kind == T_CONS &&
                          strcmp(ret->data.T_CONS.name, "Value") == 0);
  });

  ({
    Type t1 = TVAR("`2");
    Type t3 = TVAR("`4");
    Type t5 = TVAR("`5");

    T("let tensor_ndims = fn (_, sizes, _) -> \n"
      "  array_size sizes\n"
      ";;",
      &MAKE_FN_TYPE_2(&TTUPLE(3, &t1, &TARRAY(&t5), &t3), &t_int));
  });

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
  T("let f = fn l ->\n"
    "  match l with\n"
    "    | x::_ -> x\n"
    "    | [] -> 0\n"
    ";;",
    &MAKE_FN_TYPE_2(&TLIST(&t_int), &t_int));

  T("let f = fn l ->\n"
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
    Ast *ast = _T("let list_sum = fn s l ->\n"
                  "  match l with\n"
                  "  | x::rest -> list_sum (s + x) rest\n"
                  "  | [] -> s\n"
                  ";;\n");
    bool ok = ast && ast->type && ast->type->kind == T_FN;
    if (ok) {
      fprintf(stderr, "✅ list_sum has function type\n");
    } else {
      add_failure("list_sum has function type", __FILE__, __LINE__);
    }
    status &= ok;
    status &= assert_env_binding_predicates(
        "list_sum", TYPE_NAME_TYPECLASS_ARITHMETIC, 2,
        "list_sum env binding preserves Arithmetic predicates", __FILE__,
        __LINE__);
  });

  ({
    Ast *ast = _T("let print_list = fn l ->\n"
                  "  match l with\n"
                  "  | x::rest -> (print `{x}, `; print_list rest)\n"
                  "  | [] -> ()\n"
                  ";;\n");
    status &= TASSERT("print_list has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "print_list", 1, "print_list env binding is polymorphic", __FILE__,
        __LINE__);
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

  ({
    Ast *ast = _T("let list_pop_left = fn l ->\n"
                  "  match l with\n"
                  "  | x::rest -> Some x \n"
                  "  | [] -> None\n"
                  ";; \n");
    status &= TASSERT("list_pop_left has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "list_pop_left", 1, "list_pop_left env binding is polymorphic",
        __FILE__, __LINE__);
  });

  ({
    Ast *ast = _T("let enqueue = fn (head, tail) item ->\n"
                  "  let last = [item] in\n"
                  "  match head with\n"
                  "  | [] -> (last, last)\n"
                  "  | _ -> (\n"
                  "    let _ = list_concat tail last in\n"
                  "    (head, last)\n"
                  "  )\n"
                  ";;\n");
    status &= TASSERT("enqueue has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "enqueue", 1, "enqueue env binding is polymorphic", __FILE__, __LINE__);
  });

  ({
    Ast *ast = _T("let enqueue = fn (head, tail): (List of Int, List of Int) "
                  "item: (Int) "
                  "->\n"
                  "  let last = [item] in\n"
                  "  match head with\n"
                  "  | [] -> (last, last)\n"
                  "  | _ -> (\n"
                  "    let _ = list_concat tail last in\n"
                  "    (head, last)\n"
                  "  )\n"
                  ";;\n");
    TypeEnv *entry = lookup_test_env_binding("enqueue");
    status &= TASSERT("annotated enqueue binding is a function",
                      ast && entry && entry->type && entry->type->kind == T_FN);
  });
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
    Ast *b = _T("let pop_left = fn (head, tail) ->\n"
                "  match head with\n"
                "  | x::rest -> ((rest, tail), Some x)  \n"
                "  | [] -> ((head, tail), None)\n"
                ";;\n");
    status &= TASSERT("pop_left has function type",
                      b && b->type && b->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "pop_left", 2, "pop_left env binding is polymorphic", __FILE__,
        __LINE__);
  });

  ({
    Ast *ast = _T("(+) 1");
    status &= TASSERT("partial (+) 1 has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= TASSERT("partial (+) 1 preserves Arithmetic predicate",
                      ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC));
  });

  ({
    Ast *b = _T("let list_map = fn f l ->\n"
                "  let aux = fn f l res -> \n"
                "    match l with\n"
                "    | [] -> res\n"
                "    | x :: rest -> aux f rest (f x :: res) \n"
                "  ;;\n"
                "  aux f l []\n"
                ";;\n"
                "(list_map ((+) 1) [0,1,2,3])");
    status &= TASSERT("list_map application returns a list",
                      b && b->type && is_list_type(b->type));
    Ast *aux_app = AST_LIST_NTH(
        AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
            ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_BODY.stmts,
        1);
    // print_ast(aux_app);
    // print_type(aux_app->data.AST_APPLICATION.function->md);
  });

  ({
    Ast *ast = _T("let list_rev = fn l ->\n"
                  "  let aux = fn ll res -> \n"
                  "    match ll with\n"
                  "    | [] -> res\n"
                  "    | x :: rest -> aux rest (x :: res) \n"
                  "  ;;\n"
                  "  aux l []\n"
                  ";;\n");
    status &= TASSERT("list_rev has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "list_rev", 1, "list_rev env binding is polymorphic", __FILE__,
        __LINE__);
  });

  ({
    Ast *ast = _T("let of_list = fn l ->\n"
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
                  ";;\n");
    status &= TASSERT("append binding exists",
                      ast && lookup_test_env_binding("append") != NULL);
    status &= assert_env_binding_polymorphic(
        "append", 1, "append env binding is polymorphic", __FILE__, __LINE__);
  });

  ({
    Ast *b = _T("let pop_left = fn (h, t) ->\n"
                "  match h with\n"
                "  | x::rest -> Some (x, (rest, t))  \n"
                "  | [] -> None\n"
                ";;\n");
    Ast *m =
        b->data.AST_BODY.stmts->ast->data.AST_LET.expr->data.AST_LAMBDA.body;
    status &= TASSERT("match result type is Option",
                      m && m->type && is_option_type(m->type));
    status &= assert_env_binding_polymorphic(
        "pop_left", 2, "pop_left option-returning binding is polymorphic",
        __FILE__, __LINE__);
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

    TypeEnv *q_entry = lookup_test_env_binding("q");
    Type queue_int_type = TTUPLE(2, &TLIST(&t_int), &TLIST(&t_int));

    status &= TASSERT("piped append queue binding exists",
                      b && q_entry && q_entry->type != NULL);
    status &= TASSERT("piped append queue binding is monomorphic",
                      q_entry && typelist_len(q_entry->scheme_vars) == 0);
    status &= TASSERT("piped append queue binding specializes to "
                      "(List<Int>, List<Int>)",
                      q_entry && types_equal(q_entry->type, &queue_int_type));
  });
  ({
    Ast *ast = _T("let list_rev = fn l ->\n"
                  "  let aux = fn ll res ->\n"
                  "    match ll with\n"
                  "    | [] -> res\n"
                  "    | x :: rest -> aux rest (x :: res)\n"
                  "  ;;\n"
                  "  aux l []\n"
                  ";;\n");
    status &= TASSERT("second list_rev has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "list_rev", 1, "second list_rev env binding is polymorphic", __FILE__,
        __LINE__);
  });
  ({
    Ast *ast = _T("let aux = fn f l res -> \n"
                  "  match l with\n"
                  "  | [] -> res\n"
                  "  | x :: rest -> aux f rest (f x :: res) \n"
                  ";;\n");
    status &= TASSERT("aux has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "aux", 2, "aux env binding is polymorphic", __FILE__, __LINE__);
  });

  ({
    Ast *ast = _T("let list_map = fn f l ->\n"
                  "  let aux = fn f l res -> \n"
                  "    match l with\n"
                  "    | [] -> res\n"
                  "    | x :: rest -> aux f rest (f x :: res) \n"
                  "  ;;\n"
                  "  aux f l []\n"
                  ";;\n");
    status &= TASSERT("list_map has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "list_map", 2, "list_map env binding is polymorphic", __FILE__,
        __LINE__);
  });
  ({
    Ast *ast = _T("let list_map = fn f l ->\n"
                  "  let aux = fn f l res -> \n"
                  "    match l with\n"
                  "    | [] -> res\n"
                  "    | x :: rest -> aux f rest (f x :: res) \n"
                  "  ;;\n"
                  "  aux f l []\n"
                  ";;\n"
                  "list_map ((+) 1) [0,1,2,3] == [1,2,3,4]\n");
    status &= TASSERT("list_map equality expression has bool type",
                      ast && ast->type && types_equal(ast->type, &t_bool));
    status &= TASSERT("list_map equality preserves Arithmetic predicate",
                      ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC));
  });
  ({
    Ast *ast = _T("let list_rev = fn l ->\n"
                  "  let aux = fn ll res ->\n"
                  "    match ll with\n"
                  "    | [] -> res\n"
                  "    | x :: rest -> aux rest (x :: res)\n"
                  "  ;;\n"
                  "  aux l []\n"
                  ";;\n"
                  "let list_map = fn f l ->\n"
                  "  let aux = fn f l res -> \n"
                  "    match l with\n"
                  "    | [] -> res\n"
                  "    | x :: rest -> aux f rest (f x :: res) \n"
                  "  ;;\n"
                  "  aux f l [] |> list_rev\n"
                  ";;\n"
                  "list_map (fn x -> Double x) [0,1,2,3] == [0.,1.,2.,3.]\n");
    status &= TASSERT("list_map can map Int to Double",
                      ast && ast->type && types_equal(ast->type, &t_bool));
  });
  ({
    Ast *ast = _T("let list_rev = fn l ->\n"
                  "  let aux = fn ll res ->\n"
                  "    match ll with\n"
                  "    | [] -> res\n"
                  "    | x :: rest -> aux rest (x :: res)\n"
                  "  ;;\n"
                  "  aux l []\n"
                  ";;\n"
                  "let list_map = fn f l ->\n"
                  "  let aux = fn f l res -> \n"
                  "    match l with\n"
                  "    | [] -> res\n"
                  "    | x :: rest -> aux f rest (f x :: res) \n"
                  "  ;;\n"
                  "  aux f l [] |> list_rev\n"
                  ";;\n"
                  "list_map Double [0,1,2,3] == [0.,1.,2.,3.]\n");
    status &= TASSERT("list_map accepts Double constructor directly",
                      ast && ast->type && types_equal(ast->type, &t_bool));
  });

  ({
    Type r = TVAR("'4");
    Type a = TVAR("'5");
    Type l = TLIST(&a);
    T("let fold = fn f res a ->\n"
      "match a with\n"
      "| [] -> res\n"
      "| x::rest -> fold f (f res x) rest\n"
      ";;\n",
      &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&r, &a, &r), &r, &l, &r));
  });
  return status;
}

// int test_aux() {
//
//   bool status = true;
//   Type t0 = TVAR("`5");
//   Type t1 = TVAR("`7");
//   T("let aux = fn f l res -> \n"
//     "  match l with\n"
//     "  | [] -> res\n"
//     "  | x :: rest -> aux f rest (f x :: res) \n"
//     ";;\n",
//     &TSCHEME(&MAKE_FN_TYPE_4(&MAKE_FN_TYPE_2(&t0, &t1), &TLIST(&t0),
//                              &TLIST(&t1), &TLIST(&t1)),
//              &t0, &t1));
//
//   return status;
// }

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
    Type t0 = TVAR("`0");
    Type t1 = TVAR("`1");
    Type t2 = TVAR("`2");
    Ast *t = T("(+)", &MAKE_FN_TYPE_3(&t0, &t1, &t2));
    print_type(t->type);
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
  T("`{x} and {y} and {3} and {3.}`", &t_string);
  ({
    Ast *b = T("`{2} and`", &t_string);
    Ast *strf = AST_LIST_NTH(b->data.AST_BODY.stmts, 0)->data.AST_LIST.items;
    TASSERT("fmt string member has type string",
            types_equal(strf->type, &t_string));

    TASSERT("fmt string member fn has type Int -> string",
            types_equal(strf->data.AST_APPLICATION.function->type,
                        &MAKE_FN_TYPE_2(&t_int, &t_string)));
  });
  T("let id = fn x -> x;;\n"
    "(id 1, id true)",
    &TTUPLE(2, &t_int, &t_bool));
  T("let id = (fn x -> x) in (id 1, id true)", &TTUPLE(2, &t_int, &t_bool));
  return status;
}

int test_funcs() {
  printf("## TEST FUNCS\n---------------------------------------------\n");
  bool status = true;
  ({
    Type t0 = TVAR("`1");
    Type t1 = TVAR("`2");
    T("let f = fn a b -> 2;;", &MAKE_FN_TYPE_3(&t0, &t1, &t_int));
  });

  ({
    Ast *b = _T("let f = fn a: (Int) b: (Int) -> 2;;");
    TypeEnv *entry = lookup_test_env_binding("f");
    bool ok = entry != NULL && entry->type != NULL &&
              entry->type->kind == T_FN && entry->type->data.T_FN.to != NULL &&
              entry->type->data.T_FN.to->kind == T_FN &&
              entry->type->data.T_FN.to->data.T_FN.to == &t_int;
    status &= assert_bool(ok, "annotated f has arity 2 and returns Int",
                          __FILE__, __LINE__);
    (void)b;
  });

  T("let ex_fn = extern fn Int -> Double -> Int;",
    &MAKE_FN_TYPE_3(&t_int, &t_num, &t_int));

  ({
    Ast *body = T("let f = fn x -> 1 + x;;\n"
                  "f 1;\n"
                  "f 1.;\n",
                  &t_num);

    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_ARITHMETIC, 1,
        "f env binding preserves Arithmetic predicate", __FILE__, __LINE__);

    TASSERT_EQ(AST_LIST_NTH(body->data.AST_BODY.stmts, 1)->type, &t_int,
               "f 1 == Int");
    TASSERT_EQ(AST_LIST_NTH(body->data.AST_BODY.stmts, 2)->type, &t_num,
               "f 1. == Num");
  });

  ({
    Ast *b = _T("let f = fn (x, y, z) -> (z, y, x);");
    TypeEnv *entry = lookup_test_env_binding("f");
    Type *ft = entry ? entry->type : NULL;
    bool ok = ft != NULL && ft->kind == T_FN &&
              ft->data.T_FN.from->kind == T_CONS &&
              ft->data.T_FN.to->kind == T_CONS &&
              ft->data.T_FN.from->data.T_CONS.num_args == 3 &&
              ft->data.T_FN.to->data.T_CONS.num_args == 3 &&
              ft->data.T_FN.from->data.T_CONS.args[0] ==
                  ft->data.T_FN.to->data.T_CONS.args[2] &&
              ft->data.T_FN.from->data.T_CONS.args[1] ==
                  ft->data.T_FN.to->data.T_CONS.args[1] &&
              ft->data.T_FN.from->data.T_CONS.args[2] ==
                  ft->data.T_FN.to->data.T_CONS.args[0];
    status &= assert_bool(ok, "tuple-reversing f preserves component order",
                          __FILE__, __LINE__);
    (void)b;
  });

  ({
    Ast *b = _T("let f = fn (x, y, z) frame_offset: (Int) -> (z, y, x);");
    TypeEnv *entry = lookup_test_env_binding("f");
    Type *ft = entry ? entry->type : NULL;
    bool ok = ft != NULL && ft->kind == T_FN && ft->data.T_FN.to != NULL &&
              ft->data.T_FN.to->kind == T_FN &&
              ft->data.T_FN.from->kind == T_CONS &&
              ft->data.T_FN.to->data.T_FN.to->kind == T_CONS &&
              ft->data.T_FN.from->data.T_CONS.num_args == 3 &&
              ft->data.T_FN.to->data.T_FN.to->data.T_CONS.num_args == 3 &&
              ft->data.T_FN.from->data.T_CONS.args[0] ==
                  ft->data.T_FN.to->data.T_FN.to->data.T_CONS.args[2] &&
              ft->data.T_FN.from->data.T_CONS.args[1] ==
                  ft->data.T_FN.to->data.T_FN.to->data.T_CONS.args[1] &&
              ft->data.T_FN.from->data.T_CONS.args[2] ==
                  ft->data.T_FN.to->data.T_FN.to->data.T_CONS.args[0];
    status &= assert_bool(ok, "annotated tuple-reversing f has expected shape",
                          __FILE__, __LINE__);
    (void)b;
  });

  ({
    Ast *b = _T("let f = fn x y z -> x + y + z;");
    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_ARITHMETIC, 3,
        "three-arg arithmetic f preserves Arithmetic predicates", __FILE__,
        __LINE__);
    (void)b;
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

  ({
    Ast *b = T("let f = fn x: (Int) (y, z): (Int, Double) -> x + y + z;;\n"
               "f 1 (2, 3.)",
               &t_num);
    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_ARITHMETIC, 0,
        "annotated mixed arithmetic f preserves Arithmetic predicates",
        __FILE__, __LINE__);
    (void)b;
  });
  ({
    Ast *b = _T("let f = fn x (y, z) -> x + y + z;;");
    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_ARITHMETIC, 3,
        "unannotated mixed arithmetic f preserves Arithmetic predicates",
        __FILE__, __LINE__);
    (void)b;
  });

  ({
    Ast *b = _T("let add1 = fn x -> 1 + x;;");
    status &= assert_env_binding_predicates(
        "add1", TYPE_NAME_TYPECLASS_ARITHMETIC, 1,
        "add1 env binding preserves Arithmetic predicate", __FILE__, __LINE__);
    (void)b;
  });

  T("let add1 = fn x -> 1 + x;; add1 1", &t_int);
  T("let add1 = fn x -> 1 + x;; add1 1; add1 1.", &t_num);

  ({
    Ast *b = _T("(1, 2, fn a b -> a + b;);\n");
    Type *tt = b->type;
    bool ok = tt != NULL && tt->kind == T_CONS &&
              tt->data.T_CONS.num_args == 3 &&
              tt->data.T_CONS.args[2] != NULL &&
              tt->data.T_CONS.args[2]->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &= assert_bool(
        ok, "tuple with anonymous arithmetic fn preserves Arithmetic predicate",
        __FILE__, __LINE__);
  });

  ({
    Ast *b = _T("(a: 1, b: 2, f: (fn a b -> a + b))\n");
    Type *tt = b->type;
    bool ok = tt != NULL && tt->kind == T_CONS &&
              tt->data.T_CONS.num_args == 3 &&
              tt->data.T_CONS.args[2] != NULL &&
              tt->data.T_CONS.args[2]->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &= assert_bool(
        ok,
        "record with anonymous arithmetic fn preserves Arithmetic predicate",
        __FILE__, __LINE__);
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

  // ({
  //   Type s = arithmetic_var("`4");
  //   Type t = arithmetic_var("`0");
  //   Ast *b = T("let arr_sum = fn s a ->\n"
  //              "  let len = array_size a in\n"
  //              "  let aux = fn i su -> \n"
  //              "    match i with\n"
  //              "    | i if i == len -> su\n"
  //              "    | i -> aux (i + 1) (su + array_at a i)\n"
  //              "    ;;\n"
  //              "  aux 0 s\n"
  //              "  ;;\n",
  //              &MAKE_FN_TYPE_3(&t, &TARRAY(&s), &t));
  // });

  ({
    Ast *b = _T("(1, 2, fn a b -> a + b;);\n");
    Type *tt = b->type;
    bool ok = tt != NULL && tt->kind == T_CONS &&
              tt->data.T_CONS.num_args == 3 &&
              tt->data.T_CONS.args[2] != NULL &&
              tt->data.T_CONS.args[2]->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &= assert_bool(ok,
                          "second tuple with anonymous arithmetic fn preserves "
                          "Arithmetic predicate",
                          __FILE__, __LINE__);
  });

  ({
    Ast *b = _T("(a: 1, b: 2, f: (fn a b -> a + b))\n");
    Type *tt = b->type;
    bool ok = tt != NULL && tt->kind == T_CONS &&
              tt->data.T_CONS.num_args == 3 &&
              tt->data.T_CONS.args[2] != NULL &&
              tt->data.T_CONS.args[2]->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &= assert_bool(ok,
                          "second record with anonymous arithmetic fn "
                          "preserves Arithmetic predicate",
                          __FILE__, __LINE__);
  });

  T("let sq = fn x: (Int) -> x * 1.;;\n"
    "sq 1",
    &t_num);

  // 1st-class callback typing
  ({
    Ast *b = _T("let f = fn c a b -> c a b;;");
    TypeEnv *entry = lookup_test_env_binding("f");
    Type *ft = entry ? entry->type : NULL;
    bool ok = ft != NULL && ft->kind == T_FN &&
              ft->data.T_FN.from->kind == T_FN &&
              ft->data.T_FN.to->kind == T_FN &&
              ft->data.T_FN.to->data.T_FN.to->kind == T_FN &&
              ft->data.T_FN.from->data.T_FN.from ==
                  ft->data.T_FN.to->data.T_FN.from &&
              ft->data.T_FN.from->data.T_FN.to->kind == T_FN &&
              ft->data.T_FN.to->data.T_FN.to->data.T_FN.from ==
                  ft->data.T_FN.from->data.T_FN.to->data.T_FN.from &&
              ft->data.T_FN.to->data.T_FN.to->data.T_FN.to ==
                  ft->data.T_FN.from->data.T_FN.to->data.T_FN.to;
    status &= assert_bool(ok, "higher-order f has expected callback shape",
                          __FILE__, __LINE__);
    (void)b;
  });

  ({
    Ast *b = _T("let f = fn cb -> cb 1 2;;");
    TypeEnv *entry = lookup_test_env_binding("f");
    Type *ft = entry ? entry->type : NULL;
    bool ok =
        ft != NULL && ft->kind == T_FN && ft->data.T_FN.from->kind == T_FN &&
        ft->data.T_FN.from->data.T_FN.from == &t_int &&
        ft->data.T_FN.from->data.T_FN.to->kind == T_FN &&
        ft->data.T_FN.from->data.T_FN.to->data.T_FN.from == &t_int &&
        ft->data.T_FN.from->data.T_FN.to->data.T_FN.to == ft->data.T_FN.to;
    status &= assert_bool(ok, "callback-applying f has expected shape",
                          __FILE__, __LINE__);
    (void)b;
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
                   .type,
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
    print_type(fb->type);
    print_type(fb->data.AST_APPLICATION.function->type);

    TASSERT("references in sub-nodes properly typed :: fib (x-1) + fib (x-2) "
            "== Int -> Int -> Int",
            types_equal(fb->data.AST_APPLICATION.function->type,
                        &MAKE_FN_TYPE_3(&t_int, &t_int, &t_int)));
  });

  ({
    Ast *b = T("let sum = fn a b -> a + b;;\n"
               "let proc = fn f a b -> f a b;;\n"
               "proc sum 1 2;\n",
               &t_int);

    Ast *proc_inst =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_APPLICATION.function;
    print_ast(proc_inst);
    print_type(proc_inst->type);
    TASSERT(
        "instance of proc function in app is (Int -> Int -> Int) -> Int -> Int",
        types_equal(proc_inst->type,
                    &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t_int, &t_int, &t_int),
                                    &t_int, &t_int, &t_int)));
    Ast *sum_inst =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_APPLICATION.args;
    print_ast(sum_inst);
    print_type(sum_inst->type);
    TASSERT(
        "instance of sum function in app is Int -> Int -> Int",
        types_equal(sum_inst->type, &MAKE_FN_TYPE_3(&t_int, &t_int, &t_int)));
  });
  ({
    Ast *b = T("let sum = fn a b -> a + b;;\n"
               "let proc = fn f a b -> f a b;;\n"
               "let t1 = proc sum 1 2 == 3;\n"
               "let t2 = proc sum 1.0 2.0 == 3.0;\n"
               "let t3 = proc (+) 1 2 == 3;\n"
               "let t4 = proc (+) 1.0 2.0 == 3.0;\n"
               "t4;\n",
               &t_bool);

    status &=
        TASSERT("first-class sum/proc mixed numeric specializations typecheck",
                b && b->type && types_equal(b->type, &t_bool));
    TypeEnv *t1_entry = lookup_test_env_binding("t1");
    TypeEnv *t2_entry = lookup_test_env_binding("t2");
    TypeEnv *t3_entry = lookup_test_env_binding("t3");
    TypeEnv *t4_entry = lookup_test_env_binding("t4");
    status &= TASSERT("t1 binding exists and is Bool",
                      t1_entry && t1_entry->type &&
                          types_equal(t1_entry->type, &t_bool));
    status &= TASSERT("t2 binding exists and is Bool",
                      t2_entry && t2_entry->type &&
                          types_equal(t2_entry->type, &t_bool));
    status &= TASSERT("t3 binding exists and is Bool",
                      t3_entry && t3_entry->type &&
                          types_equal(t3_entry->type, &t_bool));
    status &= TASSERT("t4 binding exists and is Bool",
                      t4_entry && t4_entry->type &&
                          types_equal(t4_entry->type, &t_bool));
  });

  T("let bind = extern fn Int -> Int -> Int -> Int;\n"
    "let _bind = fn server_fd server_addr ->\n"
    "  match (bind server_fd server_addr 10) with\n"
    "  | 0 -> Some server_fd\n"
    "  | _ -> None \n"
    ";;\n",
    &MAKE_FN_TYPE_3(&t_int, &t_int, &TOPT(&t_int)));

  T("let abs = fn a ->\n"
    "  match a > 0 with\n"
    "  | true -> a\n"
    "  | _ -> a * -1\n"
    ";;\n"
    "abs 1",
    &t_int);
  ({
    Ast *b = _T("let f = fn x ->\n"
                "  let a = [|0,1,2,3|];\n"
                "  array_range (x + 3) (x + 5) a\n"
                ";;\n");
    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_ARITHMETIC, 0,
        "array_range function preserves Arithmetic predicate on x", __FILE__,
        __LINE__);
    (void)b;
  });

  return status;
}

int test_curried_funcs() {
  printf(
      "## TEST CURRIED FUNCS\n---------------------------------------------\n");
  bool status = true;

  ({
    Ast *b = _T("let f = fn a b c -> a + b + c;;\n"
                "f 1. 2.\n");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &=
        assert_bool(ok, "partial numeric sum retains Arithmetic predicate",
                    __FILE__, __LINE__);
  });

  ({
    Ast *b = _T("let f = fn a b c -> (a == b) && (a == c);;\n"
                "f 1 2\n");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN;
    status &= assert_bool(ok, "equality partial application returns a function",
                          __FILE__, __LINE__);
    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_EQ, 3,
        "equality function preserves Eq predicates", __FILE__, __LINE__);
    (void)b;
  });

  ({
    Ast *b = _T("let f = fn a b c d -> a == b && c == d;;\n"
                "f 1. 2. 3.;\n"
                "f 1 2 3\n");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN;
    status &=
        assert_bool(ok, "mixed equality partial application returns a function",
                    __FILE__, __LINE__);
    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_EQ, 4,
        "mixed equality function preserves Eq predicates", __FILE__, __LINE__);
    (void)b;
  });

  ({
    Ast *b = _T("let f = fn a b c -> a + b + c;;\n"
                "f 1 2\n");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &=
        assert_bool(ok, "partial integer sum retains Arithmetic predicate",
                    __FILE__, __LINE__);
  });

  /*
  ({
    Type a = TVAR("`35");
    Type b = TVAR("`35");
    Type c = TVAR("`35");
    // Type f = MAKE_FN_TYPE

    Ast *bd = T("let f = fn a b c d e f -> a + b + c + d + e + f;;\n"
                "let x1 = f 1;\n"
                "let x2 = x1 2;\n"
                "let x3 = x2 3;\n", &t_int);

    print_ast(AST_LIST_NTH(bd->data.AST_BODY.stmts, 1)->data.AST_LET.expr);
    print_ast(AST_LIST_NTH(bd->data.AST_BODY.stmts, 2)->data.AST_LET.expr);
    print_ast(AST_LIST_NTH(bd->data.AST_BODY.stmts, 3)->data.AST_LET.expr);

  });
  */

  ({
    Ast *b = _T("let f = fn a b c d -> a == b && c == d;;\n"
                "f 1. 2. 3.;\n"
                "f 1 2 3\n");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN;
    status &= assert_bool(
        ok, "second mixed equality partial application returns a function",
        __FILE__, __LINE__);
    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_EQ, 4,
        "second mixed equality function preserves Eq predicates", __FILE__,
        __LINE__);
  });

  ({
    Ast *b = _T("let f = fn a b c -> a + b + c;;\n"
                "f 1 2\n");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &= assert_bool(
        ok, "second partial integer sum retains Arithmetic predicate", __FILE__,
        __LINE__);
  });
  ({
    Ast *b = _T("let f = fn a b c -> a + b + c;;\n"
                "f 1. 2.\n");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &= assert_bool(
        ok, "second partial numeric sum retains Arithmetic predicate", __FILE__,
        __LINE__);
  });

  ({
    Ast *b =
        _T("let f = fn a: (Int) b: (Int) c: (Int) -> (a == b) && (a == c);;\n"
           "f 1 2\n");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN;
    status &= assert_bool(
        ok, "annotated equality partial application returns a function",
        __FILE__, __LINE__);
    status &= assert_env_binding_predicates(
        "f", TYPE_NAME_TYPECLASS_EQ, 0,
        "annotated equality function preserves Eq predicates", __FILE__,
        __LINE__);
    (void)b;
  });

  ({
    Ast *b = _T("let f = fn a b ->\n"
                "  a * b\n"
                ";;\n"
                "let K = f 3;\n");
    status &= assert_env_binding_predicates(
        "K", TYPE_NAME_TYPECLASS_ARITHMETIC, 1,
        "curried multiplication closure preserves Arithmetic predicate",
        __FILE__, __LINE__);
    (void)b;
  });

  ({
    Ast *root = _T("let list_map = fn f l ->\n"
                   "  let aux = fn f l res -> \n"
                   "    match l with\n"
                   "    | [] -> res\n"
                   "    | x :: rest -> aux f rest (f x :: res) \n"
                   "  ;;\n"
                   "  aux f l []\n"
                   ";;\n"
                   "list_map ((+) 1) [0,1,2,3]\n");

    Ast *b = AST_LIST_NTH(root->data.AST_BODY.stmts, 1);
    b = b->data.AST_APPLICATION.function;
    Type *f = b->type;
    Type clos = MAKE_FN_TYPE_2(&t_int, &t_int);
    TASSERT("concrete func application type has closure\n",
            types_equal(f->data.T_FN.from, &clos));
    status &= assert_bool(root->type != NULL && root->type->kind == T_CONS,
                          "list_map application returns a list-shaped result",
                          __FILE__, __LINE__);
  });
  ({
    Ast *b = _T("(+) 1");
    Type *t = b->type;
    bool ok = t != NULL && t->kind == T_FN &&
              ctx_has_trait_predicate(TYPE_NAME_TYPECLASS_ARITHMETIC);
    status &=
        assert_bool(ok, "partial builtin addition retains Arithmetic predicate",
                    __FILE__, __LINE__);
  });
  ({
    Ast *b = T("let fmul = fn a b -> a * b;;\n"
               "let K = fmul 3 in K 3",
               &t_int);
    b = AST_LIST_NTH(b->data.AST_BODY.stmts, 1)->data.AST_LET.in_expr;
    b = b->data.AST_APPLICATION.function;
    Type *bt = b->type;

    Type clos = MAKE_FN_TYPE_2(&t_int, &t_int);
    TASSERT("K has type of curried closure\n", types_equal(bt, &clos));
  });

  T("let sum3 = fn a b c -> a + b + c;;\n"
    "let c = sum3 1 2;\n"
    "c 3;\n",
    &t_int);

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
    Type v = t_int;
    Type opt = TOPT(&v);
    T("let f = fn x ->\n"
      "match x with\n"
      "  | Some y -> y + 1\n"
      "  | None -> 0\n"
      "  ;;\n",

      &MAKE_FN_TYPE_2(&opt, &t_int));
  });

  ({
    Type v = t_int;
    Type opt = TOPT(&v);
    T("let f = fn x ->\n"
      "match x with\n"
      "  | Some y -> y * 2\n"
      "  | None -> 0\n"
      "  ;;\n",
      &MAKE_FN_TYPE_2(&opt, &t_int));
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

    TASSERT_EQ(guard->data.AST_APPLICATION.function->type,
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
    TASSERT("match expr input has type Int", types_equal(match->type, &t_int));

    Ast *sum = match->data.AST_MATCH.branches + 5;

    TASSERT("match branch body has type Int", types_equal(sum->type, &t_int));
  });
  // "    | x if (x % 4.) > 2. -> 2\n"
  // "    | x if (x % 4.) > 1. -> 1\n"
  // "    | x if (x % 4.) > 0. -> 0 \n"

  ({
    // Type free_var = arithmetic_var("`0");
    T("let quantize_mod = fn i ->\n"
      "  match i with\n"
      "    | x if (x % 4.) > 3. -> 3 \n"
      "    | _ -> 0\n"
      ";;\n",
      &MAKE_FN_TYPE_2(&t_num, &t_int));
  });

  ({
    // Type free_var = arithmetic_var("`0");
    T("let quantize_mod = fn i ->\n"
      "  match i with\n"
      "    | x if (x % 4.) > 3. -> 3 \n"
      "    | _ -> 0\n"
      ";;\n"
      "quantize_mod 1",
      &t_int);
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
    Type constructor = MAKE_FN_TYPE_2(&t_void, &cor);

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
  //   // print_type(rec_yield->type);
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

  ({
    Type cor = COROUTINE_INST(&t_num);
    Type cor_cons = MAKE_FN_TYPE_2(&t_num, &cor);

    T("let cor = fn a ->\n"
      "  yield 1.;\n"
      "  yield a;\n"
      "  yield 3.\n"
      ";;\n",
      &cor_cons);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    Type cor_cons = MAKE_FN_TYPE_2(&t_void, &cor);
    Ast *l = T("let f = fn () -> \n"
               "  let x = 1;\n"
               "  yield x;\n"
               "  yield x + 2\n"
               "  ;;\n",
               &cor_cons);
    Ast *lambda = AST_LIST_NTH(l->data.AST_BODY.stmts, 0)->data.AST_LET.expr;
    const char *names[] = {"x"};
    Type *types[] = {&t_int};
    status &= assert_lambda_coroutine_state_vals(
        lambda, 1, names, types, "coroutine closes over x across yield",
        __FILE__, __LINE__);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    Type cor_cons = MAKE_FN_TYPE_2(&t_void, &cor);
    Ast *l = T("let f = fn () -> \n"
               "  let x = 1;\n"
               "  yield x;\n"
               "  yield x + 2;\n"
               "  let y = 200;\n"
               "  yield x + y;\n"
               "  yield y\n"
               "  ;;\n",
               &cor_cons);
    Ast *lambda = AST_LIST_NTH(l->data.AST_BODY.stmts, 0)->data.AST_LET.expr;
    const char *names[] = {"x", "y"};
    Type *types[] = {&t_int, &t_int};
    status &= assert_lambda_coroutine_state_vals(
        lambda, 2, names, types, "coroutine closes over x and y across yields",
        __FILE__, __LINE__);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    Type cor_cons = MAKE_FN_TYPE_2(&t_void, &cor);
    Ast *l = T("let f = fn () -> \n"
               "  let x = 1;\n"
               "  yield x;\n"
               "  let y = 200;\n"
               "  yield x + 2 + y;\n"
               "  yield y\n"
               "  ;;\n",
               &cor_cons);
    Ast *lambda = AST_LIST_NTH(l->data.AST_BODY.stmts, 0)->data.AST_LET.expr;
    const char *names[] = {"x", "y"};
    Type *types[] = {&t_int, &t_int};
    status &= assert_lambda_coroutine_state_vals(
        lambda, 2, names, types,
        "coroutine closes over x and y after later yield use", __FILE__,
        __LINE__);
  });

  T("let str_map = fn x -> `{ANSI_BOLD}[str {x}]{ANSI_RESET}`;;",
    &MAKE_FN_TYPE_2(&TVAR("`4"), &t_string));

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
    bool res = test_types_equal(&mapper, cor_map_arg->type);
    const char *msg = "runner arg can be materialised to specific type:";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(&mapper);
      status &= true;
    } else {
      status &= false;
      add_type_failure(msg, &mapper, cor_map_arg->type, __FILE__, __LINE__);
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
      "  yield iter l1;\n"
      "  yield iter l2\n"
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

    // Type cor_type = COROUTINE_INST(&t_num);
    Type cor_type = MAKE_FN_TYPE_2(&t_void, &TOPT(&t_num));
    // cor_type.is_coroutine_instance = true;
    Type runner_fn_arg_type = MAKE_FN_TYPE_3(&cor_type, &t_int, &t_void);

    bool res = types_equal(runner_arg->type, &runner_fn_arg_type);
    const char *msg = "runner arg can be materialised to specific type:";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(&runner_fn_arg_type);
      status &= true;
    } else {
      status &= false;
      add_type_failure(msg, &runner_fn_arg_type, runner_arg->type, __FILE__,
                       __LINE__);
    }
  });

  ({
    Type cor = COROUTINE_INST(&t_num);
    T("let f = iter [1,2,3]\n"
      "  |> cor_map (fn x -> x * 2.;)\n"
      "  |> cor_loop\n",
      &cor);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    T("cor_loop [1,2,3]\n", &cor);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    T("cor_loop [|1,2,3|]\n", &cor);
  });

  ({
    Type cor = COROUTINE_INST(&t_num);
    T("let co_void = fn () ->\n"
      "  yield 1.;\n"
      "  yield 2.;\n"
      "  yield 3.\n"
      ";;\n"
      "co_void () |> iter\n",
      &cor);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    T("[1,2,3] |> iter\n", &cor);
  });

  ({
    Type cor = COROUTINE_INST(&t_int);
    T("[|1,2,3|] |> iter\n", &cor);
  });

  ({
    Type cor = COROUTINE_INST(&t_num);
    Type cor_cons = MAKE_FN_TYPE_2(&t_void, &cor);
    T("let f = fn () ->\n"
      "  yield 1.;\n"
      "  yield f ()\n"
      ";;\n ",
      &cor_cons);
  });
  ({
    Type t = TVAR("`3");
    Type t2 = arithmetic_var("`1");
    Type inst = COROUTINE_INST(&t);
    Type cons = MAKE_FN_TYPE_3(&t, &t, &inst);
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
    // print_type(yeld->data.AST_APPLICATION.function->md);
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
      "  note: iter [|39,    32, 41,   42,  35,    37,   41, 42, "
      "|]\n"
      ");\n",
      &tuple);
  });

  T("let full_notes = [|\n"
    "   60, 64, 67, 72, 76, 60, 62, 69, 74, 77, 59, 62, 67, 74, 77, 60, 64, "
    "67, 72, 76,\n"
    "   60, 64, 69, 76, 81, 60, 62, 66, 69, 74, 59, 62, 67, 74, 79, 59, 60, "
    "64, 67, 72,\n"
    "   57, 60, 64, 67, 72, 50, 57, 62, 66, 72, 55, 59, 62, 67, 71, 55, 58, "
    "64, 67, 73,\n"
    "   53, 57, 62, 69, 74, 53, 56, 62, 65, 71, 52, 55, 60, 67, 72, 52, 53, "
    "57, 60, 65,\n"
    "   50, 53, 57, 60, 65, 43, 50, 55, 59, 65, 48, 52, 55, 60, 64, 48, 55, "
    "58, 60, 64,\n"
    "   41, 53, 57, 60, 64, 42, 48, 57, 60, 63, 44, 53, 59, 60, 62, 43, 53, "
    "55, 59, 62,\n"
    "   43, 52, 55, 60, 64, 43, 50, 55, 60, 65, 43, 50, 55, 59, 65, 43, 51, "
    "57, 60, 66,\n"
    "   43, 52, 55, 60, 67, 43, 50, 55, 60, 65, 43, 50, 55, 59, 65, 36, 48, "
    "55, 58, 64,\n"
    "|];\n"
    "let note_seq = fn idx -> \n"
    "  yield iter @@ array_range (idx * 5) 5 full_notes;\n"
    "  yield iter @@ array_range (idx * 5 + 2) 3 full_notes;\n"
    "  yield iter @@ array_range (idx * 5) 5 full_notes;\n"
    "  yield iter @@ array_range (idx * 5 + 2) 3 full_notes;\n"
    "  yield note_seq ((idx + 1) % 32)\n"
    ";;\n",
    &MAKE_FN_TYPE_2(&t_int, &COROUTINE_INST(&t_int)));

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
  //
  T("let schedule_event = extern fn (T -> Int -> ()) -> Double -> T -> ()",
    &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&TVAR("T"), &t_int, &t_void), &t_num,
                    &TVAR("T"), &t_void));
  ({
    Ast *b =
        T("let schedule_event = extern fn (T -> Int -> ()) -> Double -> T -> "
          "();\n"
          "let runner = fn c off ->\n"
          "  match c () with\n"
          "  | Some dur -> schedule_event runner dur c\n"
          "  | None -> () \n"
          ";;\n"
          "schedule_event runner 0. c\n",
          &t_void);

    Ast *runner_arg =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_APPLICATION.args;
    Type cor_type = MAKE_FN_TYPE_2(&t_void, &TOPT(&t_num));
    // cor_type.is_coroutine_instance = true;
    Type runner_fn_arg_type = MAKE_FN_TYPE_3(&cor_type, &t_int, &t_void);

    bool res = types_equal(runner_arg->type, &runner_fn_arg_type);
    const char *msg = "runner arg can be materialised to specific type:";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(&runner_fn_arg_type);
      status &= true;
    } else {
      status &= false;

      add_type_failure(msg, &runner_fn_arg_type, runner_arg->type, __FILE__,
                       __LINE__);
    }
  });

  return status;
}

int test_closures() {
  bool status = true;

  ({
    Type f = MAKE_FN_TYPE_3(&t_void, &t_void, &t_int);
    f.data.T_FN.to->closure_meta = &TTUPLE(1, {&t_int});
    Ast *b = T("fn () ->\n"
               "let z = 2;\n"
               "(fn () -> z + 2);\n"
               ";\n",
               &f);

    Ast *closure = AST_LIST_NTH(AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                                    ->data.AST_LAMBDA.body->data.AST_BODY.stmts,
                                1);
    Type *closure_type = closure->type;

    bool res = types_equal(closure_type, f.data.T_FN.to);

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

    const char *closed_names[] = {"z"};
    Type *closed_types[] = {&t_int};
    status &= assert_lambda_closed_vals(closure, 1, closed_names, closed_types,
                                        "inner closure closes over z : Int",
                                        __FILE__, __LINE__);
  });

  ({
    Type f = MAKE_FN_TYPE_3(&t_void, &t_void, &t_num);

    Ast *b = T("fn () ->\n"
               "let z = 2;\n"
               "let x = 3.;\n"
               "(fn () -> z + 2 + x);\n"
               ";\n",
               &f);

    Ast *closure = AST_LIST_NTH(AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                                    ->data.AST_LAMBDA.body->data.AST_BODY.stmts,
                                2);
    Type *closure_type = closure->type;

    Type ex = MAKE_FN_TYPE_2(&t_void, &t_num);

    ex.closure_meta = &TTUPLE(2, {&t_num, &t_int});
    bool res = types_equal(closure_type, &ex);

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

    const char *closed_names[] = {"z", "x"};
    Type *closed_types[] = {&t_int, &t_num};
    status &= assert_lambda_closed_vals(
        closure, 2, closed_names, closed_types,
        "inner closure closes over z : Int and x : Double", __FILE__, __LINE__);
  });

  // ({
  // TODO: typecheck thunks
  //   Type f = MAKE_FN_TYPE_3(&t_void, &t_void, &t_num);
  //   f.data.T_FN.to->closure_meta = &TTUPLE(2, &t_num, &t_int);
  //   Ast *b = T("let x = 1; let y = 2.; \\(x + y)", &f);
  // });
  ({
    Ast *b = T("fn () ->\n"
               "  let z = 2.;\n"
               "  let aux = fn () ->\n"
               "    z + 3.\n"
               "  ;;\n"
               "  aux ()\n"
               ";;\n",
               &MAKE_FN_TYPE_2(&t_void, &t_num));
    Ast *aux = AST_LIST_NTH(AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                                ->data.AST_LAMBDA.body->data.AST_BODY.stmts,
                            1)
                   ->data.AST_LET.expr;

    const char *closed_names[] = {"z"};
    Type *closed_types[] = {&t_num};
    status &= assert_lambda_closed_vals(aux, 1, closed_names, closed_types,
                                        "inner closure closes over z : Double",
                                        __FILE__, __LINE__);
    // printf("[AUX TYPE]: ");
    // print_type(aux->type);
  });
  return status;
}

int test_refs() {
  bool status = true;

  T("let Ref = fn item -> [|item|];;",
    &MAKE_FN_TYPE_2(&TVAR("`2"), &TARRAY(&TVAR("`2"))));

  ({
    Ast *b = T("let rx = [| 3. |] in rx[0] := 2.\n", &TARRAY(&t_num));
    Ast *f = AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                 ->data.AST_LET.in_expr->data.AST_APPLICATION.function;
    TASSERT(
        "array set operator := is Array of Double -> Int -> Double -> Array "
        "of Double",
        types_equal(f->type, &MAKE_FN_TYPE_4(&TARRAY(&t_num), &t_int, &t_num,
                                             &TARRAY(&t_num))));
  });
  T("let rx = [| 3. |] in rx[0]\n", &t_num);

  ({
    Type *v = &TVAR("`2");
    T("let incr_ref = fn rx ->\n"
      "  rx[0] := rx[0] + 1\n"
      ";;\n",
      &MAKE_FN_TYPE_2(&TARRAY(v), &TARRAY(v)));
  });

  ({
    Type v = arithmetic_var("`5");
    Ast *b = T("let incr_ref = fn rx ->\n"
               "  let x = rx[0];\n"
               "  rx[0] := x + 1\n"
               ";;\n"
               "incr_ref [| 1 |]\n",
               &TARRAY(&t_int));

    status &= EXTRA_CONDITION(
        types_equal(AST_LIST_NTH(b->data.AST_BODY.stmts, 1)
                        ->data.AST_APPLICATION.function->type,
                    &MAKE_FN_TYPE_2(&TARRAY(&t_int), &TARRAY(&t_int))),
        "incr_ref [|1|] has type Array of Int -> Int");
  });
  return status;
}
int test_modules() {
  bool status = true;

  ({
    Type t2 = TVAR("`2");
    Type size_type = MAKE_FN_TYPE_2(&TARRAY(&t2), &t_int);
    TypeEnv size_env = {
        .name = "size", .type = &size_type, .scheme_vars = TYPELIST(&t2)};
    TypeEnv x_env = {.name = "x", .type = &t_int, .next = &size_env};
    Type mod_type = {.kind = T_MODULE,
                     .data = {.T_MODULE = {.env = &x_env, .size = 2}}};

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
    Type t2 = TVAR("`2");
    Type size_type = MAKE_FN_TYPE_2(&TARRAY(&t2), &t_int);
    TypeEnv size_env = {
        .name = "size", .type = &size_type, .scheme_vars = TYPELIST(&t2)};
    TypeEnv x_env = {.name = "x", .type = &t_int, .next = &size_env};
    Type mod_type = {.kind = T_MODULE,
                     .data = {.T_MODULE = {.env = &x_env, .size = 2}}};

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
  printf("## ARRAY PROCESSING\n----------------------------\n");
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
  // ({
  //   Type t0 = TVAR("`0");
  //   T("let (@) = array_at",
  //     &TSCHEME(&MAKE_FN_TYPE_3(&TARRAY(&t0), &t_int, &t0), &t0));
  // });

  ({
    Type t6 = TVAR("`3");
    T("let rand_int = extern fn Int -> Int;\n"
      "let array_choose = fn arr ->\n"
      "  let idx = rand_int (array_size arr);\n"
      "  array_at arr idx \n"
      ";;\n",
      &MAKE_FN_TYPE_2(&TARRAY(&t6), &t6));
  });

  T("let rand_int = extern fn Int -> Int;\n"
    "let array_choose = fn arr ->\n"
    "  let idx = rand_int (array_size arr);\n"
    "  array_at arr idx \n"
    ";;\n"
    "array_choose [|1,2,3|]",
    &t_int);
  //
  T("let rand_int = extern fn Int -> Int;\n"
    "let array_choose = fn arr ->\n"
    "  let idx = rand_int (array_size arr);\n"
    "  array_at arr idx \n"
    ";;\n"
    "\\array_choose [|1,2,3|]",
    &MAKE_FN_TYPE_2(&t_void, &t_int));

  ({
    Type r = TVAR("`5");
    Type t = TVAR("`4");
    Ast *b =
        T("let fold = fn f: (R -> T -> R) res: (R) a: (Array of T) ->\n"
          "  match array_size a with\n"
          "  | 0 -> res\n"
          "  | _ -> (\n"
          "    fold f (f res (a[0])) (array_succ a)\n"
          "  )\n"
          ";;\n",
          &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&r, &t, &r), &r, &TARRAY(&t), &r));
    Ast *app =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
            ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_MATCH.expr;
    // print_type(app->type);
    // print_type(app->data.AST_APPLICATION.function->md);
    // print_type(app->data.AST_APPLICATION.args->md);
  });

  // T("let map = fn f: (T -> R) a: (Array of T) ->\n"
  T("let map = fn f a ->\n"
    "  let res = array_fill_const (array_size a) (f (a[0]));\n"
    "  for i = 1 .. (array_size a) in (\n"
    "    let v = f (a [i]);\n"
    "    res[i] :=  v\n"
    "  );\n"
    "  res\n"
    ";;\n"
    "map ((+) 3) [| 1,2,3 |]\n",
    &TARRAY(&t_int));

  T("let map = fn f a ->\n"
    "  let res = array_fill_const (array_size a) (f (a[0]));\n"
    "  for i = 1 .. (array_size a) in (\n"
    "    let v = f (a [i]);\n"
    "    res[i] :=  v\n"
    "  );\n"
    "  res\n"
    ";;\n"
    "map ((+) 3) [| 1,2,3 |];\n"
    "map ((+) 3) [| 1.,2.,3. |]\n",
    &TARRAY(&t_num));

  ({
    Type a = TVAR("`9");
    Type array_el = TVAR("`7");
    Ast *bd = T("let map = fn f a ->\n"
                "  let res = array_fill_const (array_size a) (f (a[0]));\n"
                "  for i = 1 .. (array_size a) in (\n"
                "    let v = f (a [i]);\n"
                "    res[i] :=  v\n"
                "  );\n"
                "  res\n"
                ";;\n",
                &MAKE_FN_TYPE_3(&MAKE_FN_TYPE_2(&array_el, &a),
                                &TARRAY(&array_el), &TARRAY(&a)));
    Ast *app =
        AST_LIST_NTH(
            AST_LIST_NTH(bd->data.AST_BODY.stmts, 0)
                ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_BODY.stmts,
            0)
            ->data.AST_LET.expr;

    print_ast((app->data.AST_APPLICATION.args + 1)->data.AST_APPLICATION.args);

    print_type(
        (app->data.AST_APPLICATION.args + 1)->data.AST_APPLICATION.args->type);

    print_ast((app->data.AST_APPLICATION.args + 1)
                  ->data.AST_APPLICATION.args->data.AST_APPLICATION.args);

    print_type(
        (app->data.AST_APPLICATION.args + 1)
            ->data.AST_APPLICATION.args->data.AST_APPLICATION.args->type);

    print_type(
        (app->data.AST_APPLICATION.args + 1)->data.AST_APPLICATION.args->type);

    TASSERT("array arg at has type `13 -- ",
            test_types_equal((app->data.AST_APPLICATION.args + 1)
                                 ->data.AST_APPLICATION.args->type,
                             &array_el));
  });

  ({
    Type r = TVAR("`6");
    Type t = TVAR("`5");
    Ast *b = T("let fold = fn f: (R -> T -> R) res: (R) a: (Array of T) ->\n"
               "  match array_size a with\n"
               "  | 0 -> res\n"
               "  | _ -> (\n"
               "    fold f (f res (a[0])) (array_succ a)\n"
               "  )\n"
               ";;\n"
               "fold (fn acc s -> (acc * 4) + s) 0 [|0, 1, 2|]\n",
               &t_int);
    Ast *app = b->data.AST_BODY.stmts->next->ast;
  });

  ({
    Type r = TVAR("`6");
    Type t = TVAR("`5");
    Ast *b = T("let fold = fn f: (R -> T -> R) res: (R) a: (Array of T) ->\n"
               "  match array_size a with\n"
               "  | 0 -> res\n"
               "  | _ -> (\n"
               "    fold f (f res (a[0])) (array_succ a)\n"
               "  )\n"
               ";;\n"
               "let encode_hist = fn m arr -> fold (fn acc s -> (acc * m) + s) "
               "0 arr ;;\n"
               "encode_hist 4 [|0,1,2|]\n",
               &t_int);
    Ast *app = b->data.AST_BODY.stmts->next->ast;
  });

  return status;
}
int test_networking_funcs() {
  bool status = true;

  ({
    Ast *b = _T("let pop_left = fn (head, tail) ->\n"
                "  match head with\n"
                "  | [] -> ((head, tail), None)\n"
                "  | x::rest -> ((rest, tail), Some x)  \n"
                ";;\n");
    Type *pop_left_type = AST_LIST_NTH(b->data.AST_BODY.stmts, 0)->type;
    Type t2 = TVAR("`3");
    Type t6 = TVAR("`5");
    Type expected =
        MAKE_FN_TYPE_2(&TTUPLE(2, &TLIST(&t6), &t2),
                       &TTUPLE(2, &TTUPLE(2, &TLIST(&t6), &t2), &TOPT(&t6)));
    bool pop_left_ok = test_types_equal(pop_left_type, &expected);
    if (pop_left_ok) {
      fprintf(stderr, "✅ pop_left has expected type\n");
    } else {
      status &= false;
      add_type_failure("pop_left has expected type", &expected, pop_left_type,
                       __FILE__, __LINE__);
    }
    status &= pop_left_ok;

    Ast none = AST_LIST_NTH(b->data.AST_BODY.stmts, 0)
                   ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_MATCH
                   .branches[1]
                   .data.AST_LIST.items[1];

    bool res = test_types_equal(none.type, &TOPT(&t6));
    const char *msg = "None return val";
    if (res) {
      printf("✅ %s\n", msg);
      print_type(none.type);
      status &= true;
    } else {
      status &= false;
      add_type_failure("None return val", &TOPT(&t6), none.type, __FILE__,
                       __LINE__);
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
    Type fc = MAKE_FN_TYPE_2(&t_ptr, &t_ptr);
    fc.closure_meta = &TTUPLE(1, &TLIST(&TTUPLE(2, &t_int, &t_num)));
    Type f = MAKE_FN_TYPE_2(&t_num, &fc);
    T("let instantiate_template = extern fn List of (Int, Double) -> Ptr -> "
      "Ptr;\n"
      "let f = fn freq ->\n"
      "  instantiate_template [(0, freq),]\n"
      ";;\n",
      &f);
  });

  ({
    Type fc = MAKE_FN_TYPE_2(&t_ptr, &t_ptr);
    fc.closure_meta = &TTUPLE(1, &TLIST(&TTUPLE(2, &t_int, &t_num)));
    Type f = MAKE_FN_TYPE_2(&TTUPLE(2, &t_int, &t_num), &fc);
    T("let instantiate_template = extern fn List of (Int, Double) -> Ptr -> "
      "Ptr;\n"
      "let f = fn (idx, freq) ->\n"
      "  instantiate_template [(idx, freq),]\n"
      ";;\n",
      &f);
  });

  ({
    Ast *b = T("type NoteCallback = Int -> Double -> ();\n"
               "let register_note_on_handler = extern fn NoteCallback -> Int "
               "-> ();\n"
               "register_note_on_handler (fn n vel -> vel + 0.0; ()) 0\n",
               &t_void);

    Ast *plus_app =
        AST_LIST_NTH(b->data.AST_BODY.stmts, 2)->data.AST_APPLICATION.args;
    status &= EXTRA_CONDITION(
        types_equal(plus_app->type, &MAKE_FN_TYPE_3(&t_int, &t_num, &t_void)),
        "callback constraint passed down to lambda is Int -> Double -> "
        "()");

    // print_ast(plus_app);
    // print_type(plus_app->type);

    // status &= EXTRA_CONDITION(
    //     types_equal(
    //         plus_app->md,
    //         &MAKE_TC_RESOLVE_2(TYPE_NAME_TYPECLASS_ARITHMETIC, &t_num,
    //         &t_num)),
    //     "callback constraint passed down to lambda -> (arithmetic resolve "
    //     "Double : Double)");
  });
  ({
    Type s = {T_CONS,
              {.T_CONS = {.name = TYPE_NAME_PTR, .num_args = 0}},
              .alias = "Synth"};
    Ast *b =
        T("type Synth = Ptr;\n"
          "let Synth : Constructor = module () ->\n"
          "  let of_int = fn a: (Int) ->\n"
          "    const_sig a\n"
          "  ;;\n"
          "  let of_num = fn a: (Double) ->\n"
          "    const_sig a\n"
          "  ;;\n"
          ";\n"
          "let Synth : Arithmetic = module () ->\n"
          "  let rank = 5.; \n"
          "  let add = fn a: (Synth) b: (Synth) -> sum2_node a b;;\n"
          "  let sub = fn a: (Synth) b: (Synth) -> sub2_node a b;;\n"
          "  let mul = fn a: (Synth) b: (Synth) -> mul2_node a b;;\n"
          "  let div = fn a: (Synth) b: (Synth) -> div2_node a b;;\n"
          "  let mod = fn a: (Synth) b: (Synth) -> mod2_node a b;;\n"
          ";\n"
          "let Math = module () ->\n"
          "  let sin = extern fn Double -> Double;\n"
          ";\n"
          "let sin_node = extern fn Synth -> Synth;\n"
          "let math_node = extern fn (Double -> Double) -> Synth -> Synth;\n"
          "sin_node 100. |> math_node (fn x -> 0.5 * (x + Math.sin (10. * "
          "x)));\n",
          &s);
    Ast *n = body_tail(b);
    Ast *problem = (n->data.AST_APPLICATION.args->data.AST_LAMBDA.body->data
                        .AST_APPLICATION.args +
                    1)
                       ->data.AST_APPLICATION.function;
    TASSERT(
        "internal fn binop type Double -> Double -> Double",
        types_equal(problem->type, &MAKE_FN_TYPE_3(&t_num, &t_num, &t_num)));
  });

  return status;
}

bool test_type_exprs() {
  bool status = true;
  ({
    Type t = TVAR("T");
    T("type F = (T -> Int -> ()) -> Double -> T -> ();",
      &MAKE_FN_TYPE_4(&MAKE_FN_TYPE_3(&t, &t_int, &t_void), &t_num, &t,
                      &t_void));
  });
  ({
    Ast *ast = _T("type Pat =\n"
                  "  | PatInt of Int\n"
                  "  | PatDouble of Double\n"
                  "  | PatList of List of Pat\n"
                  "  ;\n");
    Type *pt = ast ? ast->type : NULL;
    Type *plist = pt && pt->kind == T_SUM ? pt->data.T_CONS.args[2] : NULL;
    Type *plist_payload =
        plist && plist->kind == T_CONS && plist->data.T_CONS.num_args == 1
            ? plist->data.T_CONS.args[0]
            : NULL;
    status &= TASSERT("Pat declaration builds a named sum",
                      pt && pt->kind == T_SUM &&
                          strcmp(pt->data.T_CONS.name, "Pat") == 0);
    status &= TASSERT("Pat has three constructors",
                      pt && pt->data.T_CONS.num_args == 3);
    status &= TASSERT("PatList payload is List of recursive Pat",
                      plist_payload && is_list_type(plist_payload) &&
                          type_is_named_recursive_ref(
                              plist_payload->data.T_CONS.args[0], "Pat"));
  });
  return status;
}

bool test_parser_combinators() {
  printf("### TEST PARSER COMBINATORS\n--------------------------------\n");
  bool status = true;

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
          types_equal(digit->type,
                      &MAKE_FN_TYPE_2(&t_string,
                                      &TOPT(&TTUPLE(2, &t_string, &t_string))));
        }));

    Ast *fn_bd = AST_LIST_NTH(bd->data.AST_BODY.stmts, 5)
                     ->data.AST_LET.expr->data.AST_LAMBDA.body;
    Ast *fn_first = fn_bd->data.AST_APPLICATION.args + 1;

    Type ex_fn_first = MAKE_FN_TYPE_3(
        &t_string, &t_string,
        &TOPT(&TTUPLE(2, &TTUPLE(2, &t_string, &t_string), &t_string)));

    Type nested_fn = MAKE_FN_TYPE_3(
        &t_string, &t_string,
        &TOPT(&TTUPLE(2, &TTUPLE(2, &t_string, &t_string), &t_string)));

    Type clmeta2 = TTUPLE(2, &t_string, &t_string);
    nested_fn.data.T_FN.to->closure_meta = &clmeta2;
    Type clmeta1 = TTUPLE(
        2, &MAKE_FN_TYPE_2(&t_string, &TOPT(&TTUPLE(2, &t_string, &t_string))),
        &nested_fn);

    ex_fn_first.data.T_FN.to->closure_meta = &clmeta1;

    status &=
        TASSERT("(fn first -> ...) arg has type Parser of (String, String)",
                types_equal(fn_first->type, &ex_fn_first));

    Ast *fn_second =
        fn_first->data.AST_LAMBDA.body->data.AST_APPLICATION.args + 1;

    Type exp_fn_second = MAKE_FN_TYPE_3(
        &t_string, &t_string,
        &TOPT(&TTUPLE(2, &TTUPLE(2, &t_string, &t_string), &t_string)));

    exp_fn_second.data.T_FN.to->closure_meta = &TTUPLE(2, &t_string, &t_string);

    status &=
        TASSERT("(fn second -> ...) arg has type Parser of (String, String)",
                types_equal(fn_second->type, &exp_fn_second));

    Ast *clos = fn_second->data.AST_LAMBDA.body;

    Type exp_closure_type = MAKE_FN_TYPE_2(
        &t_string,
        &TOPT(&TTUPLE(2, &TTUPLE(2, &t_string, &t_string), &t_string)));

    exp_closure_type.closure_meta = &TTUPLE(2, &t_string, &t_string);

    // print_type(&exp_closure_type);
    // print_ast(clos);
    // print_type(clos->md);
    // status &=
    status &= TASSERT("(fn int -> Some ((first, second), inp)) is a closure "
                      "object with the correct internal types\n",
                      types_equal(clos->type, &exp_closure_type));
  });

  ({
    Type a = TVAR("`3");
    Type b = TVAR("`7");
    Type c = TVAR("`8");
    Type d = TVAR("`12");
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

  return status;
}

bool test_opts() {
  printf("## TEST OPTION OPS\n---------------------------------------------\n");
  bool status = true;
  ({
    Ast *b = T("Some 1", &TOPT(&t_int));
    TASSERT(
        "Some instance has application form of Option of Int\n",
        types_equal(
            b->data.AST_BODY.stmts->ast->data.AST_APPLICATION.function->type,
            &MAKE_FN_TYPE_2(&t_int, &TOPT(&t_int))));
  });
  ({
    Ast *b = T("Some 1 == None", &t_bool);
    TASSERT(
        "LHS of == operation forces None to be same type as RHS: ",
        types_equal(
            b->data.AST_BODY.stmts->ast->data.AST_APPLICATION.function->type,
            &MAKE_FN_TYPE_3(&TOPT(&t_int), &TOPT(&t_int), &t_bool)));

    // print_ast(b->data.AST_BODY.stmts->ast->data.AST_APPLICATION.args + 1);
    // print_type(
    //     (b->data.AST_BODY.stmts->ast->data.AST_APPLICATION.args + 1)->md);
  });
  return status;
}

bool test_record_types() {
  bool status = true;
  printf("## TEST RECORD "
         "TYPES\n---------------------------------------------\n");
  T("let x = (a: 1, b: 3); x.a + x.b == 4", &t_bool);
  T("let x = (a: 1, b: 3); x.a", &t_int);
  T("let X = module () ->\n"
    "  let a = 1;\n"
    ";\n"
    "X.a",
    &t_int);

  T("let X = module () ->\n"
    "  let a = 1;\n"
    "  let f = fn () -> 22;;\n"
    ";\n"
    "X.f ()",
    &t_int);

  ({
    Ast *ast = _T("let X = module () ->\n"
                  "  let f = fn a b -> a + b;;\n"
                  ";\n"
                  "X.f 1 2;\n"
                  "X");
    Type *mod = ast->type;
    bool ok = mod && mod->kind == T_MODULE && mod->data.T_MODULE.env &&
              strcmp(mod->data.T_MODULE.env->name, "f") == 0 &&
              mod->data.T_MODULE.env->type &&
              mod->data.T_MODULE.env->type->kind == T_FN &&
              mod->data.T_MODULE.env->scheme_vars != NULL &&
              mod->data.T_MODULE.env->predicates != NULL;
    if (ok) {
      fprintf(stderr, "✅ module arithmetic member preserves predicates\n");
    } else {
      status &= false;
      add_failure("module arithmetic member preserves predicates", __FILE__,
                  __LINE__);
    }
    status &= ok;
  });

  return status;
}

bool test_math_funcs() {
  bool status = true;

  Ast *b = T("let rand_int = extern fn Int -> Int;\n"
             "let shuffle = fn n -> \n"
             "  let arr = array_fill n (fn i -> i);\n"
             "\n"
             "  for _i = 0 .. n in (\n"
             "    let i = n - 1 - _i; # reverse\n"
             "    let j = rand_int (i + 1);\n"
             "    let iv = arr[i];\n"
             "    arr[i] := arr[j];\n"
             "    arr[j] := iv\n"
             "  );\n"
             "  arr\n"
             ";;\n",
             &MAKE_FN_TYPE_2(&t_int, &TARRAY(&t_int)));
  Ast *l = AST_LIST_NTH(
      AST_LIST_NTH(b->data.AST_BODY.stmts, 1)
          ->data.AST_LET.expr->data.AST_LAMBDA.body->data.AST_BODY.stmts,
      1);

  TASSERT(
      "Int",
      types_equal(
          AST_LIST_NTH(l->data.AST_LET.in_expr->data.AST_BODY.stmts, 1)->type,
          &t_int));

  return status;
}

bool test_sum_types() {

  bool status = true;
  Ast *sum_type_expr = parse_input("type Seq =\n"
                                   "  | SeqInt of Int\n"
                                   "  | SeqNum of Double\n"
                                   "  | SeqKey of String\n"
                                   "  | SeqList of List of Seq\n"
                                   "  ;\n",
                                   NULL);
  TICtx ctx = {.env = NULL};
  infer(sum_type_expr, &ctx);
  Type *sum_type = sum_type_expr->type;
  Type t19 = TVAR("`14");

  Ast *b = T("type Seq =\n"
             "  | SeqInt of Int\n"
             "  | SeqNum of Double\n"
             "  | SeqKey of String\n"
             "  | SeqList of List of Seq\n"
             "  ;\n"
             "let compile = fn seq ba ->\n"
             "  match seq with\n"
             "  | SeqInt i -> (print `{i},`; ba)\n"
             "  | SeqNum n -> (print `{n},`; ba)\n"
             "  | SeqList (x::rest) -> ( \n"
             "    ba\n"
             "    |> compile x\n"
             "    |> compile (SeqList rest)\n"
             "  )\n"
             "  | _ -> ba\n"
             ";;",
             &MAKE_FN_TYPE_3(sum_type, &t19, &t19));
  return status;
}

bool test_monads() {
  printf("## TEST MONADS\n---------------------------------------------\n");
  bool status = true;

  ({
    Ast *ast = _T("let option_map = fn f mx ->\n"
                  "  match mx with\n"
                  "  | Some x -> Some (f x)\n"
                  "  | None -> None\n"
                  ";;\n");
    status &= TASSERT("option_map has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "option_map", 2, "option_map env binding is polymorphic", __FILE__,
        __LINE__);
  });

  T("let option_map = fn f mx ->\n"
    "  match mx with\n"
    "  | Some x -> Some (f x)\n"
    "  | None -> None\n"
    ";;\n"
    "option_map (fn x -> x + 1) (Some 1)\n",
    &TOPT(&t_int));

  ({
    Ast *ast = _T("let option_bind = fn mx f ->\n"
                  "  match mx with\n"
                  "  | Some x -> f x\n"
                  "  | None -> None\n"
                  ";;\n");
    status &= TASSERT("option_bind has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "option_bind", 2, "option_bind env binding is polymorphic", __FILE__,
        __LINE__);
  });

  T("let option_bind = fn mx f ->\n"
    "  match mx with\n"
    "  | Some x -> f x\n"
    "  | None -> None\n"
    ";;\n"
    "option_bind (Some 1) (fn x -> Some (x + 1))\n",
    &TOPT(&t_int));

  T("let option_bind = fn mx f ->\n"
    "  match mx with\n"
    "  | Some x -> f x\n"
    "  | None -> None\n"
    ";;\n"
    "option_bind (Some 1) Some\n",
    &TOPT(&t_int));

  TFAIL("let option_bind = fn mx f ->\n"
        "  match mx with\n"
        "  | Some x -> f x\n"
        "  | None -> None\n"
        ";;\n"
        "option_bind (Some 1) (fn x -> x + 1)\n");

  ({
    Ast *ast = _T("let option_join = fn mmx ->\n"
                  "  match mmx with\n"
                  "  | Some mx -> mx\n"
                  "  | None -> None\n"
                  ";;\n");
    status &= TASSERT("option_join has function type",
                      ast && ast->type && ast->type->kind == T_FN);
    status &= assert_env_binding_polymorphic(
        "option_join", 1, "option_join env binding is polymorphic", __FILE__,
        __LINE__);
  });

  T("let option_join = fn mmx ->\n"
    "  match mmx with\n"
    "  | Some mx -> mx\n"
    "  | None -> None\n"
    ";;\n"
    "option_join (Some (Some 1))\n",
    &TOPT(&t_int));

  T("let option_bind = fn mx f ->\n"
    "  match mx with\n"
    "  | Some x -> f x\n"
    "  | None -> None\n"
    ";;\n"
    "let safe_div = fn x y ->\n"
    "  match y == 0 with\n"
    "  | true -> None\n"
    "  | _ -> Some (x / y)\n"
    ";;\n"
    "let half_even = fn x ->\n"
    "  match x % 2 == 0 with\n"
    "  | true -> Some (x / 2)\n"
    "  | _ -> None\n"
    ";;\n"
    "option_bind (safe_div 12 3) half_even\n",
    &TOPT(&t_int));

  T("let option_bind = fn mx f ->\n"
    "  match mx with\n"
    "  | Some x -> f x\n"
    "  | None -> None\n"
    ";;\n"
    "let safe_div = fn x y ->\n"
    "  match y == 0 with\n"
    "  | true -> None\n"
    "  | _ -> Some (x / y)\n"
    ";;\n"
    "let half_even = fn x ->\n"
    "  match x % 2 == 0 with\n"
    "  | true -> Some (x / 2)\n"
    "  | _ -> None\n"
    ";;\n"
    "option_bind (safe_div 12 0) half_even\n",
    &TOPT(&t_int));

  return status;
}

bool test_rec_coroutines() {
  bool status = true;

  Ast *b = T("let next = fn (N, pm) idx ->\n"
             "  let row = pm[(idx * N) .. (idx * N + N)];\n"
             "  let ran = 0.2;\n"
             "  let aux = fn acc i ->\n"
             "    match ran <= acc with\n"
             "    | true -> i\n"
             "    | _ -> aux (acc + row[i + 1]) (i + 1) \n"
             "  ;;\n"
             "  aux (row[0]) 0\n"
             ";;\n"
             "let mcor = fn id m ->\n"
             "  yield id;\n"
             "  let next = next m id;\n"
             "  yield mcor next m\n"
             ";;\n",
             &MAKE_FN_TYPE_3(&t_int, &TTUPLE(2, &t_int, &TARRAY(&t_num)),
                             &COROUTINE_INST(&t_int)));

  return status;
}

bool test_variadic_templates();

int main() {
  // initialize_builtin_schemes();
  reset_type_var_counter();
  initialize_builtin_types();

  bool status = true;
  status &= test_basic_ops();
  status &= test_opts();
  status &= test_match_exprs();
  status &= test_first_class_funcs();

  status &= test_modules();
  status &= test_networking_funcs();
  status &= test_type_exprs();
  status &= test_type_declarations();
  status &= test_record_types();
  status &= test_refs();
  status &= test_list_processing();
  status &= test_math_funcs();
  status &= test_funcs();
  status &= test_curried_funcs();
  status &= test_sum_types();
  status &= test_array_processing();
  // status &= test_audio_funcs();
  // status &= test_parser_combinators();
  status &= test_monads();
  status &= test_coroutines();
  status &= test_rec_coroutines();
  //
  status &= test_closures();
  status &= test_variadic_templates();
  print_all_failures();
  return status == true ? 0 : 1;
}

bool test_variadic_templates() {
  printf("\n=== Variadic Templates ===\n");
  bool status = true;

  // @Audio fn a -> a : Double -> Double
  T("let Audio = extern fn (T: (Double ... -> Double)) -> T;\n"
    "let f = @Audio fn a -> a;;\n"
    "f;",
    &MAKE_FN_TYPE_2(&t_num, &t_num));

  // @Audio fn a b -> a + b : Double -> Double -> Double
  T("let Audio = extern fn (T: (Double ... -> Double)) -> T;\n"
    "let f = @Audio fn a b -> a + b;;\n"
    "f 1. 2.;",
    &t_num);

  // @Audio fn a b c -> a + b + c : three Double args → Double
  T("let Audio = extern fn (T: (Double ... -> Double)) -> T;\n"
    "let f = @Audio fn a b c -> a + b + c;;\n"
    "f 1. 2. 3.;",
    &t_num);

  // Int args coerce to Double via From
  T("let Audio = extern fn (T: (Double ... -> Double)) -> T;\n"
    "let f = @Audio fn a b -> a + b;;\n"
    "f 1 2;",
    &t_num);

  // String arg fails — String is not coercible to Double
  TFAIL("let Audio = extern fn (T: (Double ... -> Double)) -> T;\n"
        "let f = @Audio fn a -> a;;\n"
        "f \"hello\";;");

  // Non-function arg fails — Audio expects a function
  TFAIL("let Audio = extern fn (T: (Double ... -> Double)) -> T;\n"
        "let f = @Audio 1.;;");

  ({
    // @Audio fn a b -> a + b : Double -> Double -> Double
    Ast *b = T("let Audio = extern fn (T: (Double ... -> Double)) -> T;\n"
               "let f = @Audio fn a b -> a + b;;\n"
               "f 1. 2.;",
               &t_num);
    print_ast(b);
  });

  return status;
}

// (Array of `5 [Arithmetic, ] -> tc resolve Arithmetic [ `5 [Arithmetic, ] :
// tc resolve Arithmetic [ Int : `5 [Arithmetic, ]]]) (Array of `5 -> tc
// resolve Arithmetic [ `5                : tc resolve Arithmetic [ tc resolve
// Arithmetic [ Int : `5] : `5]])
