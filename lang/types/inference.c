#include "inference.h"
#include "serde.h"

// Global variables
static int type_var_counter = 0;
void reset_type_var_counter() { type_var_counter = 0; }

static const char *fresh_tvar_name() {
  char *new_name = talloc(5 * sizeof(char));
  if (new_name == NULL) {
    return NULL;
  }
  sprintf(new_name, "t%d", type_var_counter);
  type_var_counter++;
  return new_name;
}

Type *next_tvar() {
  Type *tvar = talloc(sizeof(Type));
  *tvar = (Type){T_VAR, {.T_VAR = fresh_tvar_name()}};
  return tvar;
}

#define TRY_INFER(ast, env, msg)                                               \
  ({                                                                           \
    Type *t = infer(ast, env);                                                 \
    if (!t) {                                                                  \
      char buf[500];                                                           \
      ast_to_sexpr(ast, buf);                                                  \
      if (msg)                                                                 \
        fprintf(stderr, "%s %s\n", msg, buf);                                  \
      return NULL;                                                             \
    }                                                                          \
    t;                                                                         \
  })

static char *op_to_name[] = {
    [TOKEN_PLUS] = "+",      [TOKEN_MINUS] = "-",      [TOKEN_STAR] = "*",
    [TOKEN_SLASH] = "/",     [TOKEN_MODULO] = "%",     [TOKEN_LT] = "<",
    [TOKEN_GT] = ">",        [TOKEN_LTE] = "<=",       [TOKEN_GTE] = ">=",
    [TOKEN_EQUALITY] = "==", [TOKEN_NOT_EQUAL] = "!=",
};

Type *infer(Ast *ast, TypeEnv **env) {
  Type *type = NULL;
  switch (ast->tag) {
  case AST_BODY: {
    Ast *stmt;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {

      stmt = ast->data.AST_BODY.stmts[i];
      TRY_INFER(stmt, env, "Failure typechecking body statement: ");
    }
    type = stmt->md;
    break;
  }
  case AST_INT: {
    type = &t_int;
    break;
  }

  case AST_DOUBLE: {
    type = &t_num;
    break;
  }

  case AST_BOOL: {
    type = &t_bool;
    break;
  }

  case AST_VOID: {
    type = &t_void;
    break;
  }
  case AST_CHAR: {
    type = &t_char;
    break;
  }
  case AST_BINOP: {
    Type *lt = TRY_INFER(ast->data.AST_BINOP.left, env,
                         "Failure typechecking lhs of binop: ");

    Type *rt = TRY_INFER(ast->data.AST_BINOP.right, env,
                         "Failure typechecking rhs of binop: ");

    token_type op = ast->data.AST_BINOP.op;

    if (types_equal(lt, rt)) {
      type = lt;
      break;
    }

    TypeClass l_op_tc;
    Type l_op_method_sig;
    if ((find_typeclass_for_method(lt, op_to_name[op], &l_op_tc, &l_op_method_sig)) {
    }
    break;
  }
  case AST_LET: {
    break;
  }
  case AST_IDENTIFIER: {
    break;
  }
  case AST_TYPE_DECL: {
    break;
  }

  case AST_LIST: {
    Type *element_type = TRY_INFER(
        ast->data.AST_LIST.items, env,
        "Could not infer type of list literal elements (first element)");

    int len = ast->data.AST_LIST.len;
    if (len == 0) {
      Type *el_type = next_tvar();
      type = talloc(sizeof(Type));
      *type = (Type){T_CONS, {.T_CONS = {"List", &el_type, 1}}};
    }

    Type *el_type;
    for (int i = 1; i < len; i++) {
      Ast *el = ast->data.AST_LIST.items + i;
      el_type = TRY_INFER(el, env, "Failure typechecking list literal element");

      if (!types_equal(element_type, el_type)) {
        fprintf(stderr, "Error typechecking list literal - all elements must "
                        "be of the same type\n");
        return NULL;
      }
    }
    type = talloc(sizeof(Type));
    *type = (Type){T_CONS, {.T_CONS = {"List", &el_type, 1}}};

    break;
  }

  case AST_TUPLE: {
    int arity = ast->data.AST_LIST.len;

    Type **cons_args = talloc(sizeof(Type *) * arity);
    for (int i = 0; i < arity; i++) {

      Ast *member = ast->data.AST_LIST.items + i;
      Type *mtype = TRY_INFER(member, env, "Error typechecking tuple item");
      cons_args[i] = mtype;
    }
    type = talloc(sizeof(Type));
    *type = (Type){T_CONS, {.T_CONS = {"Tuple", cons_args, arity}}};

    break;
  }
  case AST_LAMBDA: {
    break;
  }
  }
  ast->md = type;
  return type;
}
