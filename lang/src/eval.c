#include "eval.h"
#include "arithmetic.h"
#include "eval_function.h"
#include "eval_list.h"
#include "ht.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>

static bool process_match_branch(Value predicate, Ast *branch, Value *result,
                                 ht *stack, int stack_ptr) {
  Ast *test_ast = branch;
  Ast *body = branch + 1;
  if (test_ast->tag >= AST_INT && test_ast->tag <= AST_BOOL) {
    Value l = eval(test_ast, stack, stack_ptr);
    Value is_eq = eq_ops(l, predicate);

    if (is_eq.value.vbool) {
      *result = eval(body, stack, stack_ptr + 1);
      return true;
    }
  } else if (test_ast->tag == AST_PLACEHOLDER_ID) {
    *result = eval(body, stack, stack_ptr + 1);
    return true;
  }
  return false;
}

Value eval(Ast *ast, ht *stack, int stack_ptr) {
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
      val = eval(stmt, stack, stack_ptr);
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
      return VOID;
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
      val = eq_ops(l, r);
      break;
    }
    case TOKEN_NOT_EQUAL: {
      val = neq_ops(l, r);
      break;
    }
    }
    return val;
  }
  case AST_LAMBDA: {
    val = eval_lambda_declaration(ast, stack, stack_ptr);
    return val;
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
      return VOID;
    }

    return *res;
  }

  case AST_APPLICATION: {
    val = eval_application(ast, stack, stack_ptr);
    return val;
  }
  case AST_LIST: {
    val = eval_list(ast, stack, stack_ptr);
    return val;
  }
  case AST_MATCH: {
    // val = eval_list(ast, stack, stack_ptr);
    Value predicate = eval(ast->data.AST_MATCH.expr, stack, stack_ptr);
    // print_value(&predicate);
    // printf("\n");
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {

      if (process_match_branch(predicate,
                               ast->data.AST_MATCH.branches + (i * 2), &val,
                               stack, stack_ptr)) {
        return val;
      }
    }
    // printf("\n");

    return VOID;
  }
  default:

    return val;
  }
}
