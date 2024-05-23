#include "eval.h"
#include "arithmetic.h"
#include "extern.h"
#include "ht.h"
#include "serde.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>

static Value call_function(Value *fn, Value *args, int num_args, ht *stack) {
  int stack_ptr = fn->value.function.scope_ptr + 1;

  ht *fn_scope = stack + stack_ptr;

  for (int i = 0; i < fn->value.function.len; i++) {
    ObjString param_id = fn->value.function.params[i];
    ht_set_hash(fn_scope, param_id.chars, param_id.hash, (args + i));
  }

  if (fn->value.function.fn_name != NULL) {

    const char *fn_name = fn->value.function.fn_name;
    uint64_t hash = hash_string(fn_name, strlen(fn_name));

    Value recursive_ref = {VALUE_RECURSIVE_REF,
                           .value = {.recursive_ref = fn->value.function}};
    ht_set_hash(fn_scope, fn_name, hash, &recursive_ref);
  }
  Value return_val = eval(fn->value.function.body, stack, stack_ptr);
  ht_reinit(fn_scope);
  return return_val;
}

Value eval(Ast *ast, ht *stack, int stack_ptr) {
  if (!ast) {
    return (Value){VALUE_VOID};
  }

  Value val;
  val.type = VALUE_VOID;
  switch (ast->tag) {

  case AST_BODY: {
    // Value *final;
    for (size_t i = 0; i < ast->data.AST_BODY.len; ++i) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      val = eval(stmt, stack + stack_ptr, stack_ptr);
    }
    return val;
  }
  case AST_LET: {
    ObjString name = ast->data.AST_LET.name;

    Value *expr = malloc(sizeof(Value));
    *expr = eval(ast->data.AST_LET.expr, stack, stack_ptr);
    ht_set_hash(stack + stack_ptr, name.chars, name.hash, expr);
    return *expr;
  }

  case AST_NUMBER: {
    val = NUM(ast->data.AST_NUMBER.value);
    return val;
  }

  case AST_INT: {
    val = INT(ast->data.AST_INT.value);
    return val;
  }

  case AST_STRING: {
    char *chars = ast->data.AST_STRING.value;
    int length = ast->data.AST_STRING.length;
    ObjString vstr = (ObjString){
        .chars = chars, .length = length, .hash = hash_string(chars, length)};
    val = STRING(vstr);
    return val;
  }

  case AST_BOOL: {
    val = BOOL(ast->data.AST_BOOL.value);
    return val;
  }

  case AST_VOID: {
    val.type = VALUE_VOID;
    return val;
  }
  case AST_BINOP: {
    Value l = eval(ast->data.AST_BINOP.left, stack, stack_ptr);
    Value r = eval(ast->data.AST_BINOP.right, stack, stack_ptr);

    if (l.type == VALUE_VOID || r.type == VALUE_VOID) {
      return (Value){VALUE_VOID};
    }

    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS: {
      val = add_ops(l, r, &val);
      break;
    }
    case TOKEN_MINUS: {
      val = sub_ops(l, r, &val);
      break;
    }
    case TOKEN_STAR: {

      val = mul_ops(l, r, &val);
      break;
    }
    case TOKEN_SLASH: {
      val = div_ops(l, r, &val);
      break;
    }
    case TOKEN_MODULO: {
      val = modulo_ops(l, r, &val);
      break;
    }
    case TOKEN_LT: {
      val = lt_ops(l, r, &val);
      break;
    }
    case TOKEN_LTE: {
      val = lte_ops(l, r, &val);
      break;
    }
    case TOKEN_GT: {
      val = gt_ops(l, r, &val);
      break;
    }
    case TOKEN_GTE: {
      val = gte_ops(l, r, &val);
      break;
    }
    case TOKEN_EQUALITY: {
      val = eq_ops(l, r, &val);
      break;
    }
    case TOKEN_NOT_EQUAL: {
      val = neq_ops(l, r, &val);
      break;
    }
    }
    return val;
  }
  case AST_LAMBDA: {
    val.type = VALUE_FN;
    val.value.function.len = ast->data.AST_LAMBDA.len;
    val.value.function.params = ast->data.AST_LAMBDA.params;
    val.value.function.fn_name = ast->data.AST_LAMBDA.fn_name.chars;
    val.value.function.body = ast->data.AST_LAMBDA.body;
    val.value.function.scope_ptr = stack_ptr;
    return val;
  }

  case AST_EXTERN_FN_DECLARATION: {
    return *register_external_symbol(ast, stack, stack_ptr);
  }

  case AST_IDENTIFIER: {
    char *chars = ast->data.AST_IDENTIFIER.value;
    int length = ast->data.AST_IDENTIFIER.length;
    ObjString key = {chars, length, hash_string(chars, length)};
    Value *res = NULL;

    int ptr = stack_ptr;

    while (ptr >= 0 &&
           !((res = (Value *)ht_get_hash(stack + ptr, key.chars, key.hash)))) {
      ptr--;
    }
    if (!res) {
      return (Value){VALUE_VOID};
    }

    return *res;
  }

  case AST_APPLICATION: {
    Value func = eval(ast->data.AST_APPLICATION.function, stack, stack_ptr);

    if (func.type == VALUE_FN &&
        (func.value.function.len == ast->data.AST_APPLICATION.len ||
         func.value.function.len == 0 &&
             ast->data.AST_APPLICATION.args[0]->tag == AST_VOID)) {

      int len = func.value.function.len || ast->data.AST_APPLICATION.len;

      Value *arg_vals = malloc(sizeof(Value) * len);
      for (int i = 0; i < len; i++) {
        *(arg_vals + i) =
            eval(ast->data.AST_APPLICATION.args[i], stack, stack_ptr);
      }
      return call_function(&func, arg_vals, len, stack);
    }

    if (func.type == VALUE_EXTERN_FN &&
        func.value.extern_fn.len == ast->data.AST_APPLICATION.len) {

      Value *input_vals = malloc(sizeof(Value) * ast->data.AST_APPLICATION.len);
      for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
        Value v = eval(ast->data.AST_APPLICATION.args[i], stack, stack_ptr);
        input_vals[i] = v;
      }

      call_external_function(&func, input_vals, ast->data.AST_APPLICATION.len,
                             &val);
      return val;
    }

    if (func.type == VALUE_NATIVE_FN &&
        func.value.native_fn.len == ast->data.AST_APPLICATION.len) {
      int len = func.value.native_fn.len;

      Value *input_vals = malloc(sizeof(Value) * ast->data.AST_APPLICATION.len);
      for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
        Value v = eval(ast->data.AST_APPLICATION.args[i], stack, stack_ptr);
        input_vals[i] = v;
      }
      val = func.value.native_fn.handle(len, input_vals);

      return val;
    }
    return val;
  }
  default:
    return val;
  }
}
