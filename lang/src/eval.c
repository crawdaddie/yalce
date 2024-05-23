#include "eval.h"
#include "arithmetic.h"
#include "extern.h"
#include "ht.h"
#include "value.h"
#include <stdint.h>
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
  case AST_LIST: {
    int len = ast->data.AST_LIST.len;
    if (len == 0) {
      return (Value){VALUE_LIST,
                     .value = {.vlist = &(IntList){.items = NULL, .len = 0}}};
    }
    val.type = VALUE_LIST;
    Ast first_el_ast = ast->data.AST_LIST.items[0];
    Value first_el_value = eval(&first_el_ast, stack, stack_ptr);

    switch (first_el_value.type) {
    case VALUE_INT: {
      val.value.vlist = malloc(sizeof(IntList));
      ((IntList *)val.value.vlist)->type = VALUE_INT;
      ((IntList *)val.value.vlist)->items = malloc(sizeof(int) * len);
      ((IntList *)val.value.vlist)->items[0] = first_el_value.value.vint;
      ((IntList *)val.value.vlist)->len = len;
      for (int i = 1; i < len; i++) {
        ((IntList *)val.value.vlist)->items[i] =
            eval(ast->data.AST_LIST.items + i, stack, stack_ptr).value.vint;
      }
      break;
    }

    case VALUE_BOOL: {
      val.value.vlist = malloc(sizeof(IntList));
      ((IntList *)val.value.vlist)->type = VALUE_BOOL;
      ((IntList *)val.value.vlist)->items = malloc(sizeof(bool) * len);
      ((IntList *)val.value.vlist)->items[0] = first_el_value.value.vbool;
      ((IntList *)val.value.vlist)->len = len;
      for (int i = 1; i < len; i++) {
        ((IntList *)val.value.vlist)->items[i] =
            eval(ast->data.AST_LIST.items + i, stack, stack_ptr).value.vbool;
      }

      break;
    }
    case VALUE_NUMBER: {
      val.value.vlist = malloc(sizeof(NumberList));
      ((NumberList *)val.value.vlist)->type = VALUE_NUMBER;
      ((NumberList *)val.value.vlist)->items = malloc(sizeof(double) * len);
      ((NumberList *)val.value.vlist)->items[0] = first_el_value.value.vnum;
      ((NumberList *)val.value.vlist)->len = len;
      for (int i = 1; i < len; i++) {
        ((NumberList *)val.value.vlist)->items[i] =
            eval(ast->data.AST_LIST.items + i, stack, stack_ptr).value.vnum;
      }

      break;
    }
    case VALUE_STRING: {
      val.value.vlist = malloc(sizeof(StringList));
      ((StringList *)val.value.vlist)->type = VALUE_STRING;
      ((StringList *)val.value.vlist)->items = malloc(sizeof(char *) * len);
      ((StringList *)val.value.vlist)->lens = malloc(sizeof(int) * len);
      ((StringList *)val.value.vlist)->hashes = malloc(sizeof(uint64_t) * len);
      ((StringList *)val.value.vlist)->items[0] =
          first_el_value.value.vstr.chars;

      ((StringList *)val.value.vlist)->lens[0] =
          first_el_value.value.vstr.length;

      ((StringList *)val.value.vlist)->hashes[0] =
          first_el_value.value.vstr.hash;

      ((StringList *)val.value.vlist)->len = len;
      for (int i = 1; i < len; i++) {
        Value str_val = eval(ast->data.AST_LIST.items + i, stack, stack_ptr);
        ((StringList *)val.value.vlist)->items[i] = str_val.value.vstr.chars;
        ((StringList *)val.value.vlist)->lens[i] = str_val.value.vstr.length;
        ((StringList *)val.value.vlist)->hashes[i] = str_val.value.vstr.hash;
      }

      break;
    }
    case VALUE_LIST: {
      val.value.vlist = malloc(sizeof(ObjList));
      ((ObjList *)val.value.vlist)->type = VALUE_LIST;
      ((ObjList *)val.value.vlist)->items = malloc(sizeof(void *) * len);
      ((ObjList *)val.value.vlist)->items[0] = first_el_value.value.vlist;
      ((ObjList *)val.value.vlist)->len = len;
      for (int i = 1; i < len; i++) {
        ((ObjList *)val.value.vlist)->items[i] =
            eval(ast->data.AST_LIST.items + i, stack, stack_ptr).value.vlist;
      }

      break;
    }
    case VALUE_OBJ: {
      val.value.vlist = malloc(sizeof(ObjList));

      ((ObjList *)val.value.vlist)->type = VALUE_LIST;
      ((ObjList *)val.value.vlist)->items = malloc(sizeof(void *) * len);
      ((ObjList *)val.value.vlist)->items[0] = first_el_value.value.vobj;
      ((ObjList *)val.value.vlist)->len = len;
      for (int i = 1; i < len; i++) {
        ((ObjList *)val.value.vlist)->items[i] =
            eval(ast->data.AST_LIST.items + i, stack, stack_ptr).value.vobj;
      }

      break;
    }
    }

    return val;
  }
  default:
    return val;
  }
}
