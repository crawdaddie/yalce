#include "../lang/parse.h"
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
    Type runner_fn_arg_type = MAKE_FN_TYPE_3(
        &MAKE_FN_TYPE_2(&t_void, &TOPT(&t_num)), &t_int, &t_void);

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

  return status == true ? 0 : 1;
}
