#include "inference.h"
#include "serde.h"
#include "types/type_declaration.h"
#include <stdlib.h>
#include <string.h>

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

void unify(Type *l, Type *r) {

  if (types_equal(l, r)) {
    return;
  }

  if (l->kind == T_VAR && r->kind != T_VAR) {
    *l = *r;
  }

  if (l->kind == T_VAR && r->num_implements > 0) {
    // l->implements = r->implements;
    // l->num_implements = r->num_implements;
    *l = *r;
  }

  if (l->kind == T_VAR && r->kind == T_VAR) {
    // l->implements = r->implements;
    // l->num_implements = r->num_implements;
    *l = *r;
  }

  if (l->kind == T_CONS && r->kind == T_CONS) {
    if (strcmp(l->data.T_CONS.name, r->data.T_CONS.name) != 0 ||
        l->data.T_CONS.num_args != r->data.T_CONS.num_args) {
      fprintf(stderr, "Error: Type mismatch between %s and %s\n",
              l->data.T_CONS.name, r->data.T_CONS.name);
    }

    for (int i = 0; i < l->data.T_CONS.num_args; i++) {
      unify(l->data.T_CONS.args[i], r->data.T_CONS.args[i]);
    }

    return;
  }
}

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

  Type *result_type = NULL;

  if ((!is_generic(lt)) && (!is_generic(rt))) {
    return resolve_binop_typeclass(lt, rt, op);
  }

  if (is_generic(lt) && (!is_generic(rt))) {
    result_type = lt;
  } else if (is_generic(rt) && (!is_generic(lt))) {
    result_type = rt;
  }

  if (op >= TOKEN_PLUS && op <= TOKEN_MODULO) {
    add_typeclass(result_type, derive_arithmetic_for_type(result_type));
  } else if (op >= TOKEN_LT && op <= TOKEN_GTE) {
    add_typeclass(result_type, derive_ord_for_type(result_type));
  } else if (op >= TOKEN_EQUALITY && op <= TOKEN_NOT_EQUAL) {
    add_typeclass(result_type, derive_eq_for_type(result_type));
  }

  // TODO: find typeclass rank for non-generic operand and apply to result_type
  // as 'min rank'
  // for example x + 1.0 -> 'x [arithmetic]
  // the result type is at least the same rank as float
  // if x is an int, then the result should be a float
  // if x is something 'higher' than a float use that instead

  return result_type;
}

static TypeEnv *add_binding_to_env(TypeEnv *env, Ast *binding, Type *type) {

  switch (binding->tag) {
  case AST_IDENTIFIER: {
    return env_extend(env, binding->data.AST_IDENTIFIER.value, type);
  }
  case AST_TUPLE: {
    for (int i = 0; i < binding->data.AST_LIST.len; i++) {
      env = add_binding_to_env(env, binding->data.AST_LIST.items + i,
                               type->data.T_CONS.args[i]);
    }
    return env;
  }
  }
  return env;
}

/*

static TypeEnv *set_param_binding(Ast *ast, TypeEnv **env) {
    if (is_variant_member(test_type, *env)) {
    }
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
*/

static Type *binding_type(Ast *ast) {
  switch (ast->tag) {
  case AST_IDENTIFIER: {
    return next_tvar();
  }
  case AST_TUPLE: {
    int len = ast->data.AST_LIST.len;
    Type **tuple_mems = malloc(sizeof(Type *) * len);
    for (int i = 0; i < len; i++) {
      tuple_mems[i] = binding_type(ast->data.AST_LIST.items + i);
    }
    Type *tup = empty_type();
    *tup = (Type){
        T_CONS,
        {.T_CONS = {.name = "Tuple", .args = tuple_mems, .num_args = len}}};
    return tup;
  }

  default: {
    fprintf(stderr, "Typecheck err: lambda arg type %d unsupported\n",
            ast->tag);
    return NULL;
  }
  }
}

static Type *infer_lambda(Ast *ast, TypeEnv **env) {
  int len = ast->data.AST_LAMBDA.len;
  TypeEnv *fn_scope_env = *env;

  Type *fn = NULL;
  if (len == 1 && ast->data.AST_LAMBDA.params->tag == AST_VOID) {
    fn = &t_void;
  } else {
    for (int i = 0; i < ast->data.AST_LAMBDA.len; i++) {
      Ast *param_ast = ast->data.AST_LAMBDA.params + i;
      Type *ptype = binding_type(param_ast);
      fn_scope_env = add_binding_to_env(fn_scope_env, param_ast, ptype);
      fn = fn == NULL ? ptype : type_fn(fn, ptype);
    }
  }

  if (fn == NULL) {
    return NULL;
  }

  Type *return_type = next_tvar();
  fn = type_fn(fn, return_type);
  if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
    fn_scope_env =
        env_extend(fn_scope_env, ast->data.AST_LAMBDA.fn_name.chars, fn);
  }

  Type *body_type = infer(ast->data.AST_LAMBDA.body, &fn_scope_env);

  // if (!types_equal(fn->data.T_FN.from, body_type)) {
  // }
  unify(return_type, body_type);
  return fn;
}
static Type *copy_type(Type *t) {
  Type *copy = empty_type();

  *copy = *t;

  if (copy->kind == T_CONS) {
    Type **args = t->data.T_CONS.args;
    copy->data.T_CONS.args = talloc(sizeof(Type *) * t->data.T_CONS.num_args);
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      copy->data.T_CONS.args[i] = copy_type(t->data.T_CONS.args[i]);
    }
  }

  if (t->implements != NULL && t->num_implements > 0) {
    copy->implements = talloc(sizeof(TypeClass *) * t->num_implements);
    copy->num_implements = t->num_implements;
    for (int i = 0; i < t->num_implements; i++) {
      copy->implements[i] = t->implements[i];
    }
  }

  return copy;
}

Type *generic_cons(Type *generic_cons, int len, Ast *args, TypeEnv **env) {

  if (len > generic_cons->data.T_CONS.num_args) {
    fprintf(stderr, "Error, too many arguments to cons type");
    return NULL;
  }

  // if (len < generic_cons->data.T_CONS.num_args) {
  //   // TODO: return function t1 -> t2 -> cons of arg1 arg2 t1 t2
  //   return NULL;
  // }

  Type *type = copy_type(generic_cons);

  for (int i = 0; i < len; i++) {
    type->data.T_CONS.args[i] = infer(args + i, env);
  }
  return type;
}

static Type *resolve_generic_variant(Type *t, Type *member) { return t; }
static bool type_is_variant_member(Type *member, Type *variant) { return true; }

Type *infer_match(Ast *ast, TypeEnv **env) {
  Ast *with = ast->data.AST_MATCH.expr;
  Type *wtype = infer(with, env);
  Type *result_type = NULL;
  int len = ast->data.AST_MATCH.len;
  Ast *branches = ast->data.AST_MATCH.branches;

  Type *res_type = NULL;

  for (int i = 0; i < len; i++) {
    Ast *test_expr = branches + (2 * i);
    Ast *result_expr = branches + (2 * i + 1);
    Type *test_type = infer(test_expr, env);

    if (test_type == NULL) {
      fprintf(stderr, "could not resolve match exprs\n");
      return NULL;
    }

    TypeEnv *_env = add_binding_to_env(*env, test_expr, test_type);
    Type *_res_type = infer(result_expr, &_env);
    unify(wtype, test_type);
    if (res_type != NULL) {
      unify(res_type, _res_type);
    }
    res_type = _res_type;
  }
  return res_type;
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
    type = find_type_in_env(*env, ast->data.AST_IDENTIFIER.value);
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
    type = infer_lambda(ast, env);
    break;
  }
  case AST_APPLICATION: {
    Type *t = infer(ast->data.AST_APPLICATION.function, env);

    if (t == NULL) {
      fprintf(stderr, "%s\n", "Failure could not infer type of callee ");
      return NULL;
    }

    if (is_generic(t) && t->kind == T_CONS) {
      type = generic_cons(t, ast->data.AST_APPLICATION.len,
                          ast->data.AST_APPLICATION.args, env);
      break;
    }

    break;
  }

  case AST_MATCH: {
    type = infer_match(ast, env);
    break;
  }
  }

  ast->md = type;
  return type;
}
