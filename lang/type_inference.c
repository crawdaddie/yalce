#include "type_inference.h"
#include "serde.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type t_int = {T_INT};
Type t_num = {T_NUM};
Type t_string = {T_STRING};
Type t_bool = {T_BOOL};
Type t_void = {T_VOID};

// Function prototypes
Type *create_type_variable(char *name);
Type *create_type_constructor(char *name, Type **args, int num_args);
Type *create_function_type(Type *from, Type *to);
TypeScheme *create_type_scheme(char **variables, int num_variables, Type *type);
void free_type(Type *type);
void free_type_scheme(TypeScheme *scheme);
TypeEnv extend_env(TypeEnv env, const char *name, TypeScheme *scheme);
TypeScheme *lookup_env(TypeEnv env, const char *name);
Substitution *create_substitution(const char *var_name, Type *type);
void apply_substitution(Substitution *subst, Type *type);
Substitution *unify(Type *t1, Type *t2);
Type *instantiate(TypeScheme *scheme);
TypeScheme *generalize(TypeEnv env, Type *type);
Type *infer(TypeEnv env, Ast *ast);
void print_type(Type *type);
void print_type_scheme(TypeScheme *scheme);
void print_substitution(Substitution *sub);

bool occurs_check(Type *t1, Type *t2);
// Function to print the entire TypeEnv
void print_env(TypeEnv env) {
  printf("Type Environment:\n");
  if (env == NULL) {
    printf("  (empty)\n");
    return;
  }

  TypeEnvNode *current = env;
  int count = 0;
  while (current != NULL) {
    printf("  %d. %s : ", ++count, current->name);
    print_type_scheme(current->scheme);
    printf("\n");
    current = current->next;
  }
}

bool types_equal(Type *t0, Type *t2);
Substitution *compose_substitutions(Substitution *s1, Substitution *s2);

// Helper function to determine if one type is more specific than another
bool is_more_specific(Type *t1, Type *t2) {
  if ((t1->kind >= T_INT && t1->kind <= T_CONS) && t2->kind == T_VAR) {
    return true;
  }
  // Add more rules as needed for your type system
  return false;
}
// Global variables
static int type_var_counter = 0;
void reset_type_var_counter() {}

// Helper function to generate fresh type variable names
char *fresh_name() {
  char prefix = 't';
  char *new_name = malloc(5 * sizeof(char));
  if (new_name == NULL) {
    return NULL;
  }
  sprintf(new_name, "t%d", type_var_counter);
  type_var_counter++;
  return new_name;
}

// Implementation of create_type_variable
Type *create_type_variable(char *name) {
  Type *type = malloc(sizeof(Type));
  type->kind = T_VAR;
  type->data.T_VAR = name;
  return type;
}

// Implementation of create_type_constructor
Type *create_type_constructor(char *name, Type **args, int num_args) {
  Type *type = malloc(sizeof(Type));
  type->kind = T_CONS;
  type->data.T_CONS.name = strdup(name);
  type->data.T_CONS.args = malloc(num_args * sizeof(Type *));
  for (int i = 0; i < num_args; i++) {
    type->data.T_CONS.args[i] = args[i];
  }
  type->data.T_CONS.num_args = num_args;
  return type;
}

// Implementation of create_function_type
Type *create_function_type(Type *from, Type *to) {
  Type *type = malloc(sizeof(Type));
  type->kind = T_FN;
  type->data.T_FN.from = from;
  type->data.T_FN.to = to;
  return type;
}

// Implementation of create_type_scheme
TypeScheme *create_type_scheme(char **variables, int num_variables,
                               Type *type) {
  TypeScheme *scheme = malloc(sizeof(TypeScheme));
  scheme->variables = malloc(num_variables * sizeof(char *));
  for (int i = 0; i < num_variables; i++) {
    scheme->variables[i] = strdup(variables[i]);
  }
  scheme->num_variables = num_variables;
  scheme->type = type;
  return scheme;
}

// Implementation of free_type
void free_type(Type *type) {
  // printf("freeing: ");
  // print_type(type);
  // printf("\n");
  if (type == NULL) {
    return;
  }
  // printf("free type %p %d\n", type, type->kind);
  if (type == &t_int || type == &t_num || type == &t_bool ||
      type == &t_string || type == &t_void) {
    return;
  }
  switch (type->kind) {
  case T_VAR:
    free(type->data.T_VAR);
    break;
  case T_CONS:
    free(type->data.T_CONS.name);
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      free_type(type->data.T_CONS.args[i]);
    }
    free(type->data.T_CONS.args);
    break;
  case T_FN:
    free_type(type->data.T_FN.from);
    free_type(type->data.T_FN.to);
    break;
  default:
    break;
  }
  free(type);
}

// Implementation of free_type_scheme
void free_type_scheme(TypeScheme *scheme) {
  for (int i = 0; i < scheme->num_variables; i++) {
    // free(scheme->variables[i]);
  }
  // free(scheme->variables);
  // free_type(scheme->type);
  // free(scheme);
}

// Implementation of extend_env
TypeEnv extend_env(TypeEnv env, const char *name, TypeScheme *scheme) {
  TypeEnvNode *new_node = malloc(sizeof(TypeEnvNode));
  new_node->name = strdup(name);
  new_node->scheme = scheme;
  new_node->next = env;
  return new_node;
}

// Implementation of lookup_env
TypeScheme *lookup_env(TypeEnv env, const char *name) {
  while (env != NULL) {
    if (strcmp(env->name, name) == 0) {
      return env->scheme;
    }
    env = env->next;
  }
  return NULL;
}

// Implementation of create_substitution
Substitution *create_substitution(const char *var_name, Type *type) {
  Substitution *subst = malloc(sizeof(Substitution));
  subst->var_name = strdup(var_name);
  subst->type = type;
  subst->next = NULL;
  return subst;
}

// Implementation of apply_substitution
void apply_substitution(Substitution *subst, Type *type) {
  if (type == NULL)
    return;

  switch (type->kind) {

  case T_VAR:
    while (subst != NULL) {
      printf("apply subst: ");
      print_type(type);
      printf("\n");
      if (strcmp(type->data.T_VAR, subst->var_name) == 0) {
        *type = *subst->type;
        return;
      }
      subst = subst->next;
    }
    break;

  case T_CONS:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      apply_substitution(subst, type->data.T_CONS.args[i]);
    }
    break;
  case T_FN:
    apply_substitution(subst, type->data.T_FN.from);
    apply_substitution(subst, type->data.T_FN.to);
    break;
  }
}
/*

// Implementation of unify
Substitution *unify(Type *t1, Type *t2) {
  // Check if the types are identical (same pointer or same literal type)
  if (t1 == t2) {
    return NULL; // No substitution needed
  }

  if (t1->kind == T_VAR && t2->kind == T_VAR &&
      strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0) {
    return NULL;
  }
  if (t1->kind == T_VAR) {
    return create_substitution(t1->data.T_VAR, t2);
  }
  if (t2->kind == T_VAR) {
    return create_substitution(t2->data.T_VAR, t1);
  }
  if (t1->kind == T_CONS && t2->kind == T_CONS) {
    if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0 ||
        t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
      return NULL;
    }
    Substitution *result = NULL;
    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      Substitution *s = unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i]);
      if (s == NULL)
        return NULL;
      if (result == NULL) {
        result = s;
      } else {
        Substitution *last = result;
        while (last->next != NULL)
          last = last->next;
        last->next = s;
      }
    }
    return result;
  }

  if (t1->kind == T_FN && t2->kind == T_FN) {
    // Unify the parameter types
    Substitution *s1 = unify(t1->data.T_FN.from, t2->data.T_FN.from);
    if (s1 == NULL) {
      // If s1 is NULL, it could mean either unification failed or no
      // substitution was needed We need to distinguish between these cases
      if (!types_equal(t1->data.T_FN.from, t2->data.T_FN.from)) {
        return NULL; // Unification failed
      }
      // If types are equal, continue with unifying return types
    }

    // Apply the substitution from unifying parameters to the return types
    if (s1 != NULL) {
      apply_substitution(s1, t1->data.T_FN.to);
      apply_substitution(s1, t2->data.T_FN.to);
    }

    // Unify the return types
    Substitution *s2 = unify(t1->data.T_FN.to, t2->data.T_FN.to);
    if (s2 == NULL) {
      // Free s1 if it exists and return NULL
      // free_substitution(s1);
      return NULL;
    }

    // Combine the substitutions
    return compose_substitutions(s1, s2);
  }

  return NULL;
}
*/
bool unify(Type *t1, Type *t2) {
  // Dereference any type variables until we reach a non-variable type or an
  // unbound variable
  while (t1->kind == T_VAR && t1->data.T_VAR != NULL) {
    t1 = (Type *)t1->data.T_VAR;
  }
  while (t2->kind == T_VAR && t2->data.T_VAR != NULL) {
    t2 = (Type *)t2->data.T_VAR;
  }

  // If both are the same type variable, they're already unified
  if (t1 == t2)
    return true;

  // If t1 is a variable, make it point to t2
  if (t1->kind == T_VAR) {
    if (occurs_check(t1, t2))
      return false; // Occur check
    t1->data.T_VAR = (char *)t2;
    return true;
  }

  // If t2 is a variable, make it point to t1
  if (t2->kind == T_VAR) {
    if (occurs_check(t2, t1))
      return false; // Occur check
    t2->data.T_VAR = (char *)t1;
    return true;
  }

  // If both are function types, unify their components
  if (t1->kind == T_FN && t2->kind == T_FN) {
    return unify(t1->data.T_FN.from, t2->data.T_FN.from) &&
           unify(t1->data.T_FN.to, t2->data.T_FN.to);
  }

  // If both are constructor types, check if they're the same constructor and
  // unify their arguments
  if (t1->kind == T_CONS && t2->kind == T_CONS) {
    if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0 ||
        t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
      return false;
    }
    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      if (!unify(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i])) {
        return false;
      }
    }
    return true;
  }

  // If both are the same concrete type (e.g., both T_INT), they unify
  if (t1->kind == t2->kind)
    return true;

  // If we get here, the types don't unify
  return false;
}

// Helper function for occur check
bool occurs_check(Type *var, Type *type) {
  if (type->kind == T_VAR) {
    return var == type || (type->data.T_VAR != NULL &&
                           occurs_check(var, (Type *)type->data.T_VAR));
  } else if (type->kind == T_CONS) {
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (occurs_check(var, type->data.T_CONS.args[i])) {
        return true;
      }
    }
  } else if (type->kind == T_FN) {
    return occurs_check(var, type->data.T_FN.from) ||
           occurs_check(var, type->data.T_FN.to);
  }
  return false;
}

// Function to get the final type (following all variable bindings)
Type *get_final_type(Type *t) {
  while (t->kind == T_VAR && t->data.T_VAR != NULL) {
    t = (Type *)t->data.T_VAR;
  }
  return t;
}

// Helper function to check if two types are equal
bool types_equal(Type *t1, Type *t2) {
  if (t1 == t2)
    return true;
  if (t1->kind != t2->kind)
    return false;

  switch (t1->kind) {
  case T_VAR:
    return strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0;
  case T_CONS:
    if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0 ||
        t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
      return false;
    }
    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      if (!types_equal(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i])) {
        return false;
      }
    }
    return true;
  case T_FN:
    return types_equal(t1->data.T_FN.from, t2->data.T_FN.from) &&
           types_equal(t1->data.T_FN.to, t2->data.T_FN.to);
  default:
    return true; // For primitive types like INT, BOOL, etc.
  }
}

// Helper function to compose substitutions
Substitution *compose_substitutions(Substitution *s1, Substitution *s2) {
  if (s1 == NULL)
    return s2;
  if (s2 == NULL)
    return s1;

  Substitution *result = s1;
  Substitution *current = s2;

  while (current != NULL) {
    apply_substitution(s1, current->type);
    Substitution *new_subst =
        create_substitution(current->var_name, current->type);
    new_subst->next = result;
    result = new_subst;
    current = current->next;
  }

  return result;
}

void print_substitution(Substitution *sub) {
  if (!sub) {
    return;
  }
  printf("sub: %s\n", sub->var_name);
  print_type(sub->type);
  if (sub->next) {
    printf(" -> ");
    return print_substitution(sub->next);
  }
}

// Implementation of instantiate
Type *instantiate(TypeScheme *scheme) {
  Substitution *subst = NULL;
  for (int i = 0; i < scheme->num_variables; i++) {
    Type *fresh_var = create_type_variable(fresh_name());
    Substitution *new_subst =
        create_substitution(scheme->variables[i], fresh_var);
    new_subst->next = subst;
    subst = new_subst;
  }

  Type *new_type = malloc(sizeof(Type));
  memcpy(new_type, scheme->type, sizeof(Type));
  apply_substitution(subst, new_type);

  while (subst != NULL) {
    Substitution *next = subst->next;
    // free(subst->var_name);
    // free_type(subst->type);
    // free(subst);
    subst = next;
  }

  return new_type;
}

// Helper function to check if a type variable is in a list
bool var_in_list(char *var, char **list, int list_size) {
  for (int i = 0; i < list_size; i++) {
    if (strcmp(var, list[i]) == 0)
      return true;
  }
  return false;
}

// Helper function to get free variables in a type
void get_free_vars(Type *type, char **vars, int *num_vars, int max_vars) {
  if (!type) {
    return;
  }
  switch (type->kind) {
  case T_VAR:
    if (!var_in_list(type->data.T_VAR, vars, *num_vars) &&
        *num_vars < max_vars) {
      vars[(*num_vars)++] = strdup(type->data.T_VAR);
    }
    break;
  case T_CONS:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      get_free_vars(type->data.T_CONS.args[i], vars, num_vars, max_vars);
    }
    break;
  case T_FN:
    get_free_vars(type->data.T_FN.from, vars, num_vars, max_vars);
    get_free_vars(type->data.T_FN.to, vars, num_vars, max_vars);
    break;
  }
}

// Implementation of generalize
TypeScheme *generalize(TypeEnv env, Type *type) {
  char *free_vars[100];
  int num_free_vars = 0;
  get_free_vars(type, free_vars, &num_free_vars, 100);

  char *gen_vars[100];
  int num_gen_vars = 0;
  for (int i = 0; i < num_free_vars; i++) {
    if (!lookup_env(env, free_vars[i])) {
      gen_vars[num_gen_vars++] = free_vars[i];
    } else {
      free(free_vars[i]);
    }
  }

  return create_type_scheme(gen_vars, num_gen_vars, type);
}

// Helper functions
bool is_numeric_type(Type *type) {
  return type->kind == T_INT || type->kind == T_NUM;
}

bool is_type_variable(Type *type) { return type->kind == T_VAR; }
// Helper function to get the most general numeric type
Type *get_general_numeric_type(Type *t1, Type *t2) {
  if (t1->kind == T_NUM || t2->kind == T_NUM) {
    return &t_num;
  }
  return &t_int;
}

static Type *type_id(Ast *id) {
  if (id->tag == AST_VOID) {
    return &t_void;
  }
  if (id->tag != AST_IDENTIFIER) {
    return NULL;
  }
  char *id_chars = id->data.AST_IDENTIFIER.value;

  if (strcmp(id_chars, "int") == 0) {
    return &t_int;
  } else if (strcmp(id_chars, "double") == 0) {
    return &t_num;
  } else if (strcmp(id_chars, "bool") == 0) {
    return &t_bool;
  } else if (strcmp(id_chars, "string") == 0) {
    return &t_string;
  }

  return NULL;
}

// Helper function to create a function type with multiple parameters
Type *create_multi_param_function_type(Type **param_types, size_t param_count,
                                       Type *return_type) {
  Type *func_type = return_type;
  for (int i = param_count - 1; i >= 0; i--) {
    func_type = create_function_type(param_types[i], func_type);
  }
  return func_type;
}

bool identical_literals(Type *t1, Type *t2) {
  // Check for identical literal types
  if (t1->kind == t2->kind) {
    switch (t1->kind) {
    case T_INT:
    case T_NUM:
    case T_STRING:
    case T_BOOL:
    case T_VOID:
      return true; // No substitution needed for identical literal types
    }
  }
  return false;
}

Type *get_fn_return(Type *func) {
  if (func == NULL || func->kind != T_FN) {
    return NULL;
  }
  Type *to = func->data.T_FN.to;
  while (to->kind == T_FN && to->data.T_FN.to) {
    to = to->data.T_FN.to;
  }
  return to;
}
// Helper function to get the nth parameter type from a function type
Type *get_nth_param_type(Type *func_type, size_t n) {
  while (n > 0 && func_type->kind == T_FN) {
    func_type = func_type->data.T_FN.to;
    n--;
  }
  return (func_type->kind == T_FN) ? func_type->data.T_FN.from : NULL;
}

// Implementation of infer
//
Type *infer(TypeEnv env, Ast *ast) {
  if (!ast) {
    return NULL;
  }
  switch (ast->tag) {

  case AST_BODY: {
    Type *body_type = NULL;
    TypeEnv current_env = env;
    for (size_t i = 0; i < ast->data.AST_BODY.len; i++) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      body_type = infer(current_env, stmt);
      // If the statement is a let-binding, we need to extend the environment
      if (stmt->tag == AST_LET) {
        TypeScheme *value_scheme = generalize(current_env, body_type);
        current_env = extend_env(current_env, stmt->data.AST_LET.name.chars,
                                 value_scheme);
      }
    }

    ast->md = body_type;
    return body_type;
  }
  case AST_IDENTIFIER: {
    if (ast_is_placeholder(ast)) {
      return NULL;
    }

    TypeScheme *scheme = lookup_env(env, ast->data.AST_IDENTIFIER.value);
    if (scheme == NULL) {
      fprintf(stderr, "Typecheck Error: unbound variable %s",
              ast->data.AST_IDENTIFIER.value);
      return NULL;
    }
    Type *type = instantiate(scheme);
    ast->md = type;
    return type;
  }

  case AST_LET: {
    Type *value_type = infer(env, ast->data.AST_LET.expr);
    TypeScheme *value_scheme = generalize(env, value_type);
    TypeEnv new_env =
        extend_env(env, ast->data.AST_LET.name.chars, value_scheme);

    if (ast->data.AST_LET.in_expr) {
      Type *body_type = infer(new_env, ast->data.AST_LET.in_expr);
      ast->md = body_type;
      return body_type;
    }
    ast->md = value_type;
    return value_type;
  }

  case AST_INT: {
    Type *int_type = &t_int;
    ast->md = int_type;
    return int_type;
  }

  case AST_NUMBER: {
    Type *num_type = &t_num;
    ast->md = num_type;
    return num_type;
  }

  case AST_STRING: {
    Type *str_type = &t_string;
    ast->md = str_type;
    return str_type;
  }

  case AST_BOOL: {
    Type *bool_type = &t_bool;
    ast->md = bool_type;
    return bool_type;
  }

  case AST_VOID: {
    Type *void_type = &t_void;
    ast->md = void_type;
    return void_type;
  }

  case AST_BINOP: {
    Type *lt = infer(env, ast->data.AST_BINOP.left);
    Type *rt = infer(env, ast->data.AST_BINOP.right);

    if (lt == NULL || rt == NULL) {
      // Error in inferring types of operands
      return NULL;
    }

    Type *res;
    if (is_numeric_type(lt) && is_numeric_type(rt)) {
      token_type op = ast->data.AST_BINOP.op;
      if (op >= TOKEN_PLUS && op <= TOKEN_MODULO) {
        // Arithmetic operators
        res = lt->kind >= rt->kind ? lt : rt;
        ast->md = res;
        return res;
      } else if (op >= TOKEN_LT && op <= TOKEN_NOT_EQUAL) {
        // Comparison operators
        res = &t_bool;
        ast->md = res;
        return res;
      }
    }

    if (is_type_variable(lt) && is_numeric_type(rt)) {
      ast->md = lt;
      return lt;
    }

    if (is_type_variable(rt) && is_numeric_type(lt)) {
      ast->md = rt;
      return rt;
    }

    // Default case: return left type
    ast->md = lt;
    return lt;
  }
  // case AST_LAMBDA: {
  //   size_t param_count = ast->data.AST_LAMBDA.len;
  //   Type **param_types = malloc(param_count * sizeof(Type *));
  //   TypeEnv new_env = env;
  //
  //   // Create fresh type variables for parameters
  //   for (size_t i = 0; i < param_count; i++) {
  //     param_types[i] = create_type_variable(fresh_name());
  //     new_env = extend_env(new_env, ast->data.AST_LAMBDA.params[i].chars,
  //                          create_type_scheme(NULL, 0, param_types[i]));
  //   }
  //
  //   // Handle potential recursion
  //   Type *func_type = NULL;
  //   if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
  //     func_type = create_type_variable(fresh_name());
  //     new_env = extend_env(new_env, ast->data.AST_LAMBDA.fn_name.chars,
  //                          create_type_scheme(NULL, 0, func_type));
  //   }
  //
  //   // Infer the type of the body
  //   Type *body_type = infer(new_env, ast->data.AST_LAMBDA.body);
  //   if (body_type == NULL) {
  //     // Clean up and return error
  //     for (size_t i = 0; i < param_count; i++) {
  //       free_type(param_types[i]);
  //     }
  //     free(param_types);
  //     if (func_type)
  //       free_type(func_type);
  //     return NULL;
  //   }
  //
  //   // Create the function type
  //   Type *inferred_func_type =
  //       create_multi_param_function_type(param_types, param_count,
  //       body_type);
  //
  //   // Unify parameter types with the types inferred from the body
  //   Substitution *subst = NULL;
  //   for (size_t i = 0; i < param_count; i++) {
  //     Type *inferred_param_type = get_nth_param_type(inferred_func_type, i);
  //     if (inferred_param_type != NULL) {
  //       Substitution *param_subst = unify(param_types[i],
  //       inferred_param_type); if (param_subst != NULL) {
  //         subst = compose_substitutions(subst, param_subst);
  //       }
  //     }
  //   }
  //
  //   // Apply the substitution to the inferred function type
  //   if (subst != NULL) {
  //     apply_substitution(subst, inferred_func_type);
  //   }
  //
  //   // If it's a recursive function, unify the inferred type with the assumed
  //   // type
  //   if (func_type) {
  //     Substitution *rec_subst = unify(func_type, inferred_func_type);
  //     if (rec_subst == NULL) {
  //       // Unification failed - clean up and return error
  //       free_type(inferred_func_type);
  //       free_type(func_type);
  //       for (size_t i = 0; i < param_count; i++) {
  //         free_type(param_types[i]);
  //       }
  //       free(param_types);
  //       return NULL;
  //     }
  //     apply_substitution(rec_subst, inferred_func_type);
  //     subst = compose_substitutions(subst, rec_subst);
  //   }
  //
  //   // Store the inferred type in the AST node
  //   ast->md = inferred_func_type;
  //
  //   // Clean up
  //   for (size_t i = 0; i < param_count; i++) {
  //     free_type(param_types[i]);
  //   }
  //   free(param_types);
  //   if (func_type)
  //     free_type(func_type);
  //
  //   return inferred_func_type;
  // }
  case AST_LAMBDA: {
    size_t param_count = ast->data.AST_LAMBDA.len;
    Type **param_types = malloc(param_count * sizeof(Type *));
    TypeEnv new_env = env;

    // Create fresh type variables for parameters
    for (size_t i = 0; i < param_count; i++) {
      param_types[i] = create_type_variable(fresh_name());
      printf("param type %s: ", ast->data.AST_LAMBDA.params[i].chars);
      print_type(param_types[i]);
      printf("\n");
      new_env = extend_env(new_env, ast->data.AST_LAMBDA.params[i].chars,
                           create_type_scheme(NULL, 0, param_types[i]));
    }

    // Create a type variable for the return type
    Type *return_type = create_type_variable(fresh_name());

    // Create the function type with the return type variable
    Type *func_type =
        create_multi_param_function_type(param_types, param_count, return_type);

    // Handle potential recursion
    if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
      // Add the function to the environment with its type
      new_env = extend_env(new_env, ast->data.AST_LAMBDA.fn_name.chars,
                           create_type_scheme(NULL, 0, func_type));
    }

    // Infer the type of the body
    //
    Type *body_type = infer(new_env, ast->data.AST_LAMBDA.body);
    print_env(new_env);

    if (body_type == NULL) {
      fprintf(stderr, "Typecheck lambda error: cannot infer type of fn body\n");
      return NULL;
    }

    Substitution *subst = unify(return_type, body_type);

    if (subst == NULL) {
      return NULL;
    }

    // Apply the substitution to the function type
    apply_substitution(subst, func_type);

    // Store the inferred type in the AST node
    ast->md = func_type;

    return func_type;
  }
  case AST_APPLICATION: {
    Type *func_type = infer(env, ast->data.AST_APPLICATION.function);

    if (func_type == NULL) {
      return NULL; // Error in function type inference
    }

    // Infer types of all arguments
    Type **arg_types = malloc(ast->data.AST_APPLICATION.len * sizeof(Type *));
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      arg_types[i] = infer(env, ast->data.AST_APPLICATION.args + i);
      if (arg_types[i] == NULL) {
        fprintf(stderr, "arg type null");
        // Clean up and return error
        for (int j = 0; j < i; j++) {
          // free_type(arg_types[j]);
        }
        // free(arg_types);
        return NULL;
      }
    }

    // Create fresh type variables for the result
    Type *result_type = create_type_variable(fresh_name());

    // Create the expected function type based on inferred argument types and
    // result
    Type *expected_type = create_multi_param_function_type(
        arg_types, ast->data.AST_APPLICATION.len, result_type);

    // Unify the actual function type with the expected type
    // printf("Unify the actual function type with the expected type\n");
    Substitution *subst = unify(func_type, expected_type);
    if (subst == NULL) {
      return NULL;
    }

    // Apply the substitution to the result type
    apply_substitution(subst, result_type);

    // Handle currying: if the function expects more arguments, return a new
    // function type
    Type *current_type = func_type;
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      if (current_type->kind != T_FN) {
        // Error: trying to apply too many arguments
        // free_type(result_type);
        // free_type(expected_type);
        for (int j = 0; j < ast->data.AST_APPLICATION.len; j++) {
          // free_type(arg_types[j]);
        }
        free(arg_types);
        while (subst != NULL) {
          Substitution *next = subst->next;
          // free(subst->var_name);
          // free(subst);
          subst = next;
        }
        return NULL;
      }
      current_type = current_type->data.T_FN.to;
    }

    Type *final_type;
    if (current_type->kind == T_FN) {
      final_type = current_type;
    } else {
      final_type = result_type;
    }

    // Store the inferred type in the AST node
    ast->md = final_type;

    // Clean up
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      // free_type(arg_types[i]);
    }
    // free(arg_types);
    while (subst != NULL) {
      Substitution *next = subst->next;
      // free(subst->var_name);
      // free(subst);
      subst = next;
    }

    return final_type;
  }
  case AST_EXTERN_FN: {
    int param_count = ast->data.AST_EXTERN_FN.len - 1;
    Type **params = malloc(param_count * sizeof(Type *));
    for (int i = 0; i < param_count; i++) {
      params[i] = type_id(ast->data.AST_EXTERN_FN.signature_types + i);
    }

    Type *ex_t = create_multi_param_function_type(
        params, param_count,
        type_id(ast->data.AST_EXTERN_FN.signature_types + param_count));
    ast->md = ex_t;
    return ex_t;
  }

  case AST_MATCH: {
    // Infer the type of the expression being matched
    Type *expr_type = infer(env, ast->data.AST_MATCH.expr);
    printf("match expr type: \n");
    print_ast(ast->data.AST_MATCH.expr);
    printf(" : ");
    print_type(expr_type);
    printf("\n");

    if (expr_type == NULL) {
      fprintf(stderr, "Typecheck match expr with expr failed");
      return NULL; // Error in expression type inference
    }

    Type *result_type = NULL;
    Substitution *subst = NULL;
    Type *pattern_type = NULL; // To store the most specific pattern type

    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      Ast *test_ast = ast->data.AST_MATCH.branches + (2 * i);
      Type *test_type = infer(env, test_ast);
      if (test_type == NULL && !ast_is_placeholder(test_ast)) {
        fprintf(stderr, "Typecheck pattern %d test expr failed\n", i);
        return NULL; // Error in test expression type inference
      }

      // Update pattern_type with the most specific non-placeholder pattern
      if (!ast_is_placeholder(test_ast)) {
        if (pattern_type == NULL || is_more_specific(test_type, pattern_type)) {
          pattern_type = test_type;
        }
      }

      // Unify the test type with the expression type
      if (!ast_is_placeholder(test_ast) &&
          !identical_literals(expr_type, test_type)) {

        printf("expr with test: ");
        print_type(expr_type);
        print_type(test_type);
        printf("\n");

        Substitution *test_subst = unify(expr_type, test_type);

        printf("test subst: ");
        print_substitution(test_subst);
        printf("\nexpr: ");
        print_type(expr_type);
        printf("\n");

        if (test_subst == NULL) {
          fprintf(stderr, "Typecheck pattern %d test unification failed\n", i);
          return NULL; // Unification failed: pattern doesn't match expression
                       // type
        }
        // Apply the substitution to the expression type
        apply_substitution(test_subst, expr_type);
        subst = compose_substitutions(subst, test_subst);
      }

      // Infer type of the branch body
      Type *body_type = infer(env, ast->data.AST_MATCH.branches + (2 * i + 1));
      if (body_type == NULL) {
        fprintf(stderr, "Typecheck pattern body %d expr failed\n", i);
        return NULL; // Error in body type inference
      }

      if (result_type == NULL) {
        // This is the first branch, so its type becomes the result type
        result_type = body_type;
      } else {
        // Unify this branch's type with the result type
        if (!identical_literals(result_type, body_type)) {
          Substitution *body_subst = unify(result_type, body_type);
          if (body_subst == NULL) {
            fprintf(stderr, "Typecheck pattern %d body unification failed\n",
                    i);
            return NULL; // Unification failed: inconsistent branch types
          }
          // Apply the substitution to the result type
          apply_substitution(body_subst, result_type);
          subst = compose_substitutions(subst, body_subst);
        }
      }
    }

    print_type(expr_type);
    print_type(pattern_type);
    printf("\n");
    // Unify the expression type with the most specific pattern type
    if (pattern_type != NULL && !identical_literals(expr_type, pattern_type)) {
      printf("final expr type:");
      Substitution *final_subst = unify(expr_type, pattern_type);
      if (final_subst == NULL) {
        fprintf(stderr, "Final unification of match expression type failed\n");
        return NULL;
      }
      subst = compose_substitutions(subst, final_subst);
    }

    // Apply the final substitution to the result type and expression type
    if (subst) {
      apply_substitution(subst, result_type);
      apply_substitution(subst, expr_type);
    }

    // Store the result type in the AST node
    ast->md = result_type;

    // Also store the inferred type of the matched expression
    ast->data.AST_MATCH.expr->md = expr_type;

    return result_type;
  }

  default:
    fprintf(stderr, "unhandled node type %d\n", ast->tag);
    // Unhandled AST node type
    return NULL;
  }
}

// Helper function to print types (for debugging)
void print_type(Type *type) {
  if (type == NULL) {
    printf("NULL");
    return;
  }

  switch (type->kind) {

  case T_INT:
    printf("Int");
    break;

  case T_NUM:
    printf("Double");
    break;

  case T_BOOL:
    printf("Bool");
    break;

  case T_STRING:
    printf("String");
    break;

  case T_VOID:
    printf("()");
    break;

  case T_VAR:
    printf("%s", type->data.T_VAR);
    break;

  case T_CONS:
    printf("%s", type->data.T_CONS.name);
    if (type->data.T_CONS.num_args > 0) {
      printf("(");
      for (int i = 0; i < type->data.T_CONS.num_args; i++) {
        if (i > 0)
          printf(", ");
        print_type(type->data.T_CONS.args[i]);
      }
      printf(")");
    }
    break;

  case T_FN:
    printf("(");
    print_type(type->data.T_FN.from);
    printf(" -> ");
    print_type(type->data.T_FN.to);
    printf(")");
    break;
  default:
    printf("Unknown");
    break;
  }
}

// Helper function to print type schemes (for debugging)
void print_type_scheme(TypeScheme *scheme) {
  if (scheme == NULL) {
    printf("NULL");
    return;
  }

  if (scheme->num_variables > 0) {
    printf("forall ");
    for (int i = 0; i < scheme->num_variables; i++) {
      if (i > 0)
        printf(", ");
      printf("%s", scheme->variables[i]);
    }
    printf(". ");
  }

  print_type(scheme->type);
}

// Helper function to print the type environment (for debugging)
void print_type_env(TypeEnv env) {
  while (env != NULL) {
    printf("%s : ", env->name);
    print_type_scheme(env->scheme);
    printf("\n");
    env = env->next;
  }
}

// Main function to perform type inference on an AST
Type *infer_ast(TypeEnv env, Ast *ast) {
  // TypeEnv env = NULL;
  // Add initial environment entries (e.g., built-in functions)
  // env = extend_env(env, "+", create_type_scheme(...));
  // env = extend_env(env, "-", create_type_scheme(...));
  // ...

  Type *result = infer(env, ast);

  if (result == NULL) {
    fprintf(stderr, "Type inference failed\n");
  } else {
    // printf("Inferred type: ");
    // print_type(result);
    // printf("\n");
  }

  // Clean up the environment
  while (env != NULL) {
    TypeEnvNode *next = env->next;
    // free(env->name);
    // free_type_scheme(env->scheme);
    // free(env);
    env = next;
  }

  return result;
}
