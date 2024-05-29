#include "eval.h"
#include "arithmetic.h"
#include "eval_function.h"
#include "eval_list.h"
#include "ht.h"
#include "serde.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>

static Value lookup_id(const char *id, int length, ht *stack, int stack_ptr) {

  ObjString key = {.chars = id, length, hash_string(id, length)};
  Value *res = NULL;

  int ptr = stack_ptr;

  while (ptr >= 0 &&
         !((res = (Value *)ht_get_hash(stack + ptr, key.chars, key.hash)))) {
    ptr--;
  }

  if (!res) {
    return VOID;
  }
  // print_value(res);
  return *res;
}

static bool process_match_branch(Value predicate, Ast *branch, Value *result,
                                 ht *stack, int stack_ptr,
                                 val_bind_fn_t val_bind) {
  Ast *test_ast = branch;
  Ast *body = branch + 1;
  if (test_ast->tag >= AST_INT && test_ast->tag <= AST_BOOL) {
    Value l = eval(test_ast, stack, stack_ptr, val_bind);
    Value is_eq = eq_ops(l, predicate);

    if (is_eq.value.vbool) {
      *result = eval(body, stack, stack_ptr + 1, val_bind);
      return true;
    }
  } else if (test_ast->tag == AST_PLACEHOLDER_ID) {
    *result = eval(body, stack, stack_ptr + 1, val_bind);
    return true;
  } else if (test_ast->tag == AST_IDENTIFIER) {
    Value l = eval(test_ast, stack, stack_ptr, val_bind);
    Value is_eq = eq_ops(l, predicate);

    if (is_eq.value.vbool) {
      *result = eval(body, stack, stack_ptr + 1, val_bind);
      return true;
    }
  }

  return false;
}
static Value eval_match(Ast *ast, ht *stack, int stack_ptr,
                        val_bind_fn_t val_bind) {

  Value predicate = eval(ast->data.AST_MATCH.expr, stack, stack_ptr, val_bind);

  for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
    Ast *test_ast = ast->data.AST_MATCH.branches + (i * 2);
    Ast *body = test_ast + 1;

    if (test_ast->tag >= AST_INT && test_ast->tag <= AST_BOOL) {
      Value l = eval(test_ast, stack, stack_ptr, val_bind);
      Value is_eq = eq_ops(l, predicate);

      if (is_eq.value.vbool) {
        return eval(body, stack, stack_ptr + 1, val_bind);
      }
    } else if (test_ast->tag == AST_PLACEHOLDER_ID) {
      return eval(body, stack, stack_ptr + 1, val_bind);
    } else if (test_ast->tag == AST_IDENTIFIER) {
      Value l = eval(test_ast, stack, stack_ptr, val_bind);
      Value is_eq = eq_ops(l, predicate);

      if (is_eq.value.vbool) {
        return eval(body, stack, stack_ptr + 1, val_bind);
      }
    }
  }
  return VOID;
}

Value eval(Ast *ast, ht *stack, int stack_ptr, val_bind_fn_t val_bind) {
  // print_ast(ast);
  if (!ast) {
    return VOID;
  }

  Value val;
  val.type = VALUE_VOID;
  switch (ast->tag) {

  case AST_BODY: {
    for (size_t i = 0; i < ast->data.AST_BODY.len; ++i) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      val = eval(stmt, stack, stack_ptr, val_bind);
    }
    break;
  }
  case AST_LET: {
    ObjString name = ast->data.AST_LET.name;

    Value *expr = malloc(sizeof(Value));
    *expr = eval(ast->data.AST_LET.expr, stack, stack_ptr, val_bind);
    ht_set_hash(stack + stack_ptr, name.chars, name.hash, expr);
    val = *expr;
    break;
  }

  case AST_NUMBER: {
    val = NUM(ast->data.AST_NUMBER.value);
    break;
  }

  case AST_INT: {
    val = INT(ast->data.AST_INT.value);
    break;
  }

  case AST_STRING: {
    char *chars = ast->data.AST_STRING.value;
    int length = ast->data.AST_STRING.length;
    ObjString vstr = (ObjString){
        .chars = chars, .length = length, .hash = hash_string(chars, length)};
    val = STRING(vstr);
    break;
  }

  case AST_BOOL: {
    val = BOOL(ast->data.AST_BOOL.value);
    break;
  }

  case AST_VOID: {
    val.type = VALUE_VOID;
    break;
  }
  case AST_BINOP: {
    Value l = eval(ast->data.AST_BINOP.left, stack, stack_ptr, val_bind);
    Value r = eval(ast->data.AST_BINOP.right, stack, stack_ptr, val_bind);

    if (l.type == VALUE_VOID || r.type == VALUE_VOID) {
      val = VOID;
      break;
    }

    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS: {
      val = add_ops(l, r);
      break;
    }
    case TOKEN_MINUS: {
      val = sub_ops(l, r);
      break;
    }
    case TOKEN_STAR: {

      val = mul_ops(l, r);
      break;
    }
    case TOKEN_SLASH: {
      val = div_ops(l, r);
      break;
    }
    case TOKEN_MODULO: {
      val = modulo_ops(l, r);
      break;
    }
    case TOKEN_LT: {
      val = lt_ops(l, r);
      break;
    }
    case TOKEN_LTE: {
      val = lte_ops(l, r);
      break;
    }
    case TOKEN_GT: {
      val = gt_ops(l, r);
      break;
    }
    case TOKEN_GTE: {
      val = gte_ops(l, r);
      break;
    }
    case TOKEN_EQUALITY: {
      val = eq_ops(l, r);
      break;
    }
    case TOKEN_NOT_EQUAL: {
      val = neq_ops(l, r);
      break;
    }
    }
    break;
  }
  case AST_LAMBDA: {
    val = eval_lambda_declaration(ast, stack, stack_ptr);
    break;
  }

  case AST_IDENTIFIER: {
    char *chars = ast->data.AST_IDENTIFIER.value;
    int length = ast->data.AST_IDENTIFIER.length;
    val = lookup_id(chars, length, stack, stack_ptr);
    if (val.type == VALUE_VOID) {
      fprintf(stderr, "Error: value %s not found\n", chars);
    }
    break;
  }

  case AST_APPLICATION: {
    val = eval_application(ast, stack, stack_ptr, val_bind);
    // return val;
    break;
  }
  case AST_LIST: {
    val = eval_list(ast, stack, stack_ptr, val_bind);
    break;
  }
  case AST_MATCH: {
    val = eval_match(ast, stack, stack_ptr, val_bind);
    break;
  }
  case AST_META: {
    Value meta_fn_ = lookup_id(ast->data.AST_META.value,
                               ast->data.AST_META.length, stack, stack_ptr);

    meta_fn_t meta_fn = meta_fn_.value.vmeta_fn;
    val = meta_fn(ast->data.AST_META.next, stack, stack_ptr);
    break;
  }
  default:
    // return val;
    break;
  }
  if (val_bind != NULL) {
    val_bind(val);
  }
  // printf("\neval:");
  // print_ast(ast);
  // printf("\n");
  // print_value(&val);
  return val;
}
