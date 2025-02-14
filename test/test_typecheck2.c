#include "../lang/parse.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "serde.h"

// #define xT(input, type)

#define T(input, proc, type)                                                   \
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

  ({
    // &MAKE_FN_TYPE_3(
    //     &TARRAY(&TTUPLE(2, &TLIST(&TVAR("`9")), &TLIST(&TVAR("`9")))),
    //     &TVAR("`9"),
    //     &TARRAY(&TTUPLE(2, &TLIST(&TVAR("`9")), &TLIST(&TVAR("`9")))));
    char *input = "let set_ref = fn arr v -> array_set 0 arr v;;\n"
                  "let (<-) = set_ref;\n"
                  "let get_ref = fn arr -> array_at arr 0;;\n"
                  "let enqueue = fn q item ->\n"
                  "  let _, t = get_ref q in\n"
                  "  let last = [item] in\n"
                  "  let h = list_concat t last in\n"
                  "  q <- (h, last) \n"
                  ";;  \n";
  });

  // print_ast(b->data.AST_BODY.stmts[0]);
  // print_type(b->data.AST_BODY.stmts[0]->md);

  return status == true ? 0 : 1;
}
