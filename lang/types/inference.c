#include "inference.h"
#include "serde.h"
#include "types/type_declaration.h"
#include <stdlib.h>

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
      char *buf = malloc(sizeof(char) * 500);                                  \
      ast_to_sexpr(ast, buf);                                                  \
      if (msg)                                                                 \
        fprintf(stderr, "%s %s\n", msg, buf);                                  \
      free(buf);                                                               \
      return NULL;                                                             \
    }                                                                          \
    t;                                                                         \
  })

void unify(Type *l, Type *r) {}

static char *op_to_name[] = {
    [TOKEN_PLUS] = "+",      [TOKEN_MINUS] = "-",      [TOKEN_STAR] = "*",
    [TOKEN_SLASH] = "/",     [TOKEN_MODULO] = "%",     [TOKEN_LT] = "<",
    [TOKEN_GT] = ">",        [TOKEN_LTE] = "<=",       [TOKEN_GTE] = ">=",
    [TOKEN_EQUALITY] = "==", [TOKEN_NOT_EQUAL] = "!=",
};

Type *infer_binop(Ast *ast, TypeEnv **env) {

  Type *lt = TRY_INFER(ast->data.AST_BINOP.left, env,
                       "Failure typechecking lhs of binop: ");

  Type *rt = TRY_INFER(ast->data.AST_BINOP.right, env,
                       "Failure typechecking rhs of binop: ");

  token_type op = ast->data.AST_BINOP.op;

  if (types_equal(lt, rt)) {
    return lt;
  }

  // builtin-binops
  TypeClass *tcl = NULL;
  TypeClass *tcr = NULL;
  if (op >= TOKEN_PLUS && op <= TOKEN_MODULO) {
    // arithmetic typeclass
    tcl = get_typeclass_by_name(lt, "arithmetic");
    tcr = get_typeclass_by_name(rt, "arithmetic");
  }

  if (op >= TOKEN_LT && op <= TOKEN_GTE) {
    // ord typeclass
    tcl = get_typeclass_by_name(lt, "ord");
    tcr = get_typeclass_by_name(rt, "ord");
  }

  if (op >= TOKEN_EQUALITY && op <= TOKEN_NOT_EQUAL) {
    // eq typeclass
    tcl = get_typeclass_by_name(lt, "eq");
    tcr = get_typeclass_by_name(rt, "eq");
  }

  if (tcl && tcr) {
    if (tcl->rank >= tcr->rank) {
      Type *method = typeclass_method_signature(tcl, op_to_name[op]);
      return fn_return_type(method);
    } else {
      Type *method = typeclass_method_signature(tcr, op_to_name[op]);
      return fn_return_type(method);
    }
  }

  if (!tcl && tcr && is_generic(lt)) {
    // unify(lt, rt);
    Type *method = typeclass_method_signature(tcr, op_to_name[op]);
    return fn_return_type(method);
  }

  if (!tcr && tcl && is_generic(rt)) {
    // unify(rt, lt);
    Type *method = typeclass_method_signature(tcl, op_to_name[op]);
    return fn_return_type(method);
  }
  if (!tcl && !tcr) {
    // TODO: implement if neither required typeclass exists
  }

  return NULL;
}

static TypeEnv *add_binding_to_env(TypeEnv *env, Ast *binding, Type *type) {

  switch (binding->tag) {
  case AST_IDENTIFIER: {
    return env_extend(env, binding->data.AST_IDENTIFIER.value, type);
  }
  }
  return env;
}

static TypeEnv *set_param_binding(Ast *ast, TypeEnv **env) {
  switch (ast->tag) {
  case AST_IDENTIFIER: {
    ast->md = next_tvar();
    const char *name = ast->data.AST_IDENTIFIER.value;
    *env = add_binding_to_env(*env, ast, ast->md);
    return *env;
  }
  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;
    Type **tuple_mems = talloc(sizeof(Type) * len);
    for (int i = 0; i < len; i++) {
      *env = set_param_binding(ast->data.AST_LIST.items + i, env);
      tuple_mems[i] = ast->data.AST_LIST.items[i].md;
      ast->md = talloc(sizeof(Type));
      *((Type *)ast->md) =
          (Type){T_CONS, {.T_CONS = {"Tuple", tuple_mems, len}}};
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
    type = infer_binop(ast, env);
    break;
  }
  case AST_LET: {
    Ast *expr = ast->data.AST_LET.expr;
    Type *expr_type = TRY_INFER(expr, env, NULL);
    Ast *binding = ast->data.AST_LET.binding;
    *env = add_binding_to_env(*env, binding, expr_type);

    Ast *in_expr = ast->data.AST_LET.in_expr;
    if (in_expr) {
      Type *in_expr_type = TRY_INFER(in_expr, env, NULL);
      type = in_expr_type;
    } else {
      type = expr_type;
    }

    break;
  }
  case AST_IDENTIFIER: {
    if (ast_is_placeholder_id(ast)) {
      type = next_tvar();
      break;
    }
    Type *_type = find_type_in_env(*env, ast->data.AST_IDENTIFIER.value);
    if (!_type) {
      return NULL;
    }
    type = _type;
    break;
  }
  case AST_TYPE_DECL: {
    type = type_declaration(ast, env);
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
