#include "types/inference.h"
#include "print_ast.h"
#include "types/type_declaration.h"
#include <string.h>

// Global variables
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }
static const char *fresh_tvar_name() {
  char *new_name = malloc(5 * sizeof(char));

  if (new_name == NULL) {
    return NULL;
  }
  sprintf(new_name, "t%d", type_var_counter);
  type_var_counter++;
  return new_name;
}

Type next_tvar() { return (Type){T_VAR, {.T_VAR = fresh_tvar_name()}}; }

// forward decl
Type *get_builtin_type(const char *id_chars);

static TypeEnv *add_binding_to_env(TypeEnv *env, Ast *binding, Type *type) {

  switch (binding->tag) {
  case AST_IDENTIFIER: {
    return env_extend(env, binding->data.AST_IDENTIFIER.value, type);
  }
  }
  return env;
}

#define INFER(ast, msg)                                                        \
  ({                                                                           \
    if (infer(ast, env)) {                                                     \
      char buf[500];                                                           \
      ast_to_sexpr(ast, buf);                                                  \
      if (msg)                                                                 \
        fprintf(stderr, "%s %s\n", msg, buf);                                  \
      return 1;                                                                \
    }                                                                          \
  })

#define INFER_ENV(ast, _env, msg)                                              \
  ({                                                                           \
    if (infer(ast, _env)) {                                                    \
      char buf[500];                                                           \
      ast_to_sexpr(ast, buf);                                                  \
      if (msg)                                                                 \
        fprintf(stderr, "%s %s\n", msg, buf);                                  \
      return 1;                                                                \
    }                                                                          \
  })

#define TTUPLE(num, ...)                                                       \
  ((Type){T_CONS, {.T_CONS = {"Tuple", (Type[]){__VA_ARGS__}, num}}})

#define DEBUG_UNIFY

static void unify(Type *l, Type *r, TypeEnv *env) {
#ifdef DEBUG_UNIFY
  printf("unify: \t");
  print_type(*l);
  printf("with \t");
  print_type(*r);
#endif

  if (r->kind == T_VAR) {
    return unify(r, l, env);
  }
  if (l->kind == T_VAR && r->num_implements > 0) {
    // merge_typeclasses(l, r);
    printf("l typeclasses: %d\n", l->num_implements);
  }
}

static TypeEnv *set_param_binding_type(Ast *ast, TypeEnv **env) {
  switch (ast->tag) {
  case AST_IDENTIFIER: {
    ast->md = next_tvar();
    const char *name = ast->data.AST_IDENTIFIER.value;
    *env = add_binding_to_env(*env, ast, &ast->md);
    return *env;
  }
  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;
    Type tuple_mems[len];
    for (int i = 0; i < len; i++) {
      *env = set_param_binding_type(ast->data.AST_LIST.items + i, env);
      tuple_mems[i] = ast->data.AST_LIST.items[i].md;
      ast->md = (Type){T_CONS, {.T_CONS = {"Tuple", tuple_mems, len}}};
    }
    return *env;
  }
  default: {
    fprintf(stderr, "Typecheck err: lambda arg type %d unsupported\n",
            ast->tag);
    return NULL;
  }
  }
}

int infer(Ast *ast, TypeEnv **env) {
  Type type;
  switch (ast->tag) {
  case AST_BODY: {
    Ast *stmt;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {

      stmt = ast->data.AST_BODY.stmts[i];
      INFER(stmt, "Failure typechecking body statement: ");
    }
    type = stmt->md;
    break;
  }
  case AST_INT: {
    type = t_int;
    break;
  }

  case AST_DOUBLE: {
    type = t_num;
    break;
  }

  case AST_BOOL: {
    type = t_bool;
    break;
  }

  case AST_VOID: {
    type = t_void;
    break;
  }
  case AST_CHAR: {
    type = t_char;
    break;
  }
  case AST_BINOP: {
    INFER(ast->data.AST_BINOP.left, "Failure typechecking lhs of binop: ");
    INFER(ast->data.AST_BINOP.right, "Failure typechecking rhs of binop: ");

    Type *lt = &ast->data.AST_BINOP.left->md;
    Type *rt = &ast->data.AST_BINOP.right->md;

    token_type op = ast->data.AST_BINOP.op;

    if (types_equal(*lt, *rt)) {
      type = *lt;
      break;
    }

    if (is_generic(lt) || is_generic(rt)) {
      unify(lt, rt, *env);
      print_ast(ast);
      print_full_type(*rt);
    }

    TypeClass *l_op_tc = find_op_impl(*lt, op);
    TypeClass *r_op_tc = find_op_impl(*rt, op);

    if (l_op_tc && r_op_tc) {
      if (l_op_tc->rank > r_op_tc->rank) {
        type = *fn_ret_type(l_op_tc->method_signature);
        break;
      } else {
        type = *fn_ret_type(r_op_tc->method_signature);
        break;
      }
    }

    break;
  }
  case AST_LET: {
    Ast *expr = ast->data.AST_LET.expr;
    INFER(expr, NULL);
    Ast *binding = ast->data.AST_LET.binding;
    *env = add_binding_to_env(*env, binding, &ast->data.AST_LET.expr->md);
    INFER(binding, NULL);

    Ast *in_expr = ast->data.AST_LET.in_expr;
    if (in_expr) {
      INFER(in_expr, NULL);
      type = in_expr->md;
      break;
    } else {
      type = expr->md;
      break;
    }

    break;
  }
  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(ast)) {
      type = next_tvar();
      break;
    }
    Type *_type = get_type(*env, ast->data.AST_IDENTIFIER.value);
    if (!_type) {
      return 1;
    }
    type = *_type;
    break;
  }
  case AST_TYPE_DECL: {
    if (type_declaration(ast, env)) {
      return 0;
    };

    Ast *type_expr_ast = ast->data.AST_LET.expr;
    type = type_expr_ast->md;
    break;
  }

  case AST_LIST: {
    INFER(ast->data.AST_LIST.items,
          "Could not infer type of list literal elements (first element)");

    int len = ast->data.AST_LIST.len;

    Type *cons_args = malloc(sizeof(Type));
    if (len == 0) {
      *cons_args = next_tvar();
      set_list_type(&type, cons_args);
      break;
    }

    Type el_type = ast->data.AST_LIST.items[0].md;

    for (int i = 1; i < ast->data.AST_LIST.len; i++) {
      Ast *element = ast->data.AST_LIST.items + i;
      INFER(element, "Error: typechecking failed for list literal element");
      if (!types_equal(el_type, element->md)) {
        fprintf(stderr,
                "Error: all list literal elements must be the same type\n");
        return 1;
      }
    }

    *cons_args = el_type;
    set_list_type(&type, cons_args);

    break;
  }

  case AST_TUPLE: {
    int arity = ast->data.AST_LIST.len;

    Type *cons_args = malloc(sizeof(Type) * arity);
    for (int i = 0; i < arity; i++) {

      Ast *member = ast->data.AST_LIST.items + i;
      INFER(member, "Error typechecking tuple item");
      cons_args[i] = member->md;
    }

    set_tuple_type(&type, cons_args, arity);
    break;
  }
  case AST_LAMBDA: {
    set_fn_type(&type, ast->data.AST_LAMBDA.len);
    printf("typecheck lambda len %zu\n", ast->data.AST_LAMBDA.len);
    print_ast(ast);

    Type fn_type = next_tvar();

    TypeEnv *fn_scope = *env;
    for (size_t i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      fn_scope =
          set_param_binding_type(ast->data.AST_LAMBDA.params + i, &fn_scope);

      Type t = ast->data.AST_LAMBDA.params[i].md;
    }

    if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
      fn_scope =
          env_extend(fn_scope, ast->data.AST_LAMBDA.fn_name.chars, &fn_type);
    }

    INFER_ENV(ast->data.AST_LAMBDA.body, &fn_scope,
              "Typechecking function body failed");
    print_type_env(fn_scope);

    type = fn_type;
    print_type(type);
    break;
  }
  }
  ast->md = type;
  return 0;
}
