#include "serde.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_ast(Ast *ast) {
  char *buf = malloc(sizeof(char) * 300);
  printf("%s\n", ast_to_sexpr(ast, buf));
  free(buf);
}

static bool is_term(ast_tag tag) {
  return tag <= AST_IDENTIFIER;
  // return (tag == AST_INT || tag == AST_NUMBER || tag == AST_BOOL || tag ==
  // AST_STRING || tag == AST_IDENTIFIER);
}

char *ast_to_sexpr(Ast *ast, char *buffer) {
  if (!ast) {
    buffer = strcat(buffer, "null");
    return buffer;
  }

  switch (ast->tag) {
  case AST_BODY: {
    for (size_t i = 0; i < ast->data.AST_BODY.len; ++i) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      buffer = strcat(buffer, "\n");
      buffer = ast_to_sexpr(stmt, buffer);
    }
    break;
  }

  case AST_LET: {
    buffer = strcat(buffer, "(let ");
    buffer = strcat(buffer, ast->data.AST_LET.name.chars);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_LET.expr, buffer);
    buffer = strcat(buffer, ")");
    break;
  }

  case AST_NUMBER: {

    char buf[100];
    sprintf(buf, "%f", ast->data.AST_NUMBER.value);
    buffer = strcat(buffer, buf);
    break;
  }

  case AST_INT: {

    char buf[100];
    sprintf(buf, "%d", ast->data.AST_INT.value);
    buffer = strcat(buffer, buf);
    break;
  }

  case AST_STRING: {

    buffer = strcat(buffer, "\"");
    buffer = strcat(buffer, ast->data.AST_STRING.value);
    buffer = strcat(buffer, "\"");
    break;
  }

  case AST_BOOL: {
    if (ast->data.AST_BOOL.value) {
      buffer = strcat(buffer, "true");
    } else {
      buffer = strcat(buffer, "false");
    }
    break;
  }

  case AST_VOID: {
    buffer = strcat(buffer, "()");
    break;
  }
  case AST_IDENTIFIER: {
    buffer = strcat(buffer, ast->data.AST_IDENTIFIER.value);
    break;
  }

  case AST_PLACEHOLDER_ID: {
    buffer = strcat(buffer, "_");
    break;
  }

  case AST_APPLICATION: {
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      buffer = strcat(buffer, "(");
    }
    buffer = ast_to_sexpr(ast->data.AST_APPLICATION.function, buffer);

    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      buffer = strcat(buffer, " ");
      buffer = ast_to_sexpr(ast->data.AST_APPLICATION.args + i, buffer);
      buffer = strcat(buffer, ")");
    }

    break;
  }

  case AST_BINOP: {
    buffer = strcat(buffer, "(");
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS:
      buffer = strcat(buffer, "+ ");
      break;
    case TOKEN_MINUS:
      buffer = strcat(buffer, "- ");
      break;
    case TOKEN_STAR:
      buffer = strcat(buffer, "* ");
      break;
    case TOKEN_SLASH:
      buffer = strcat(buffer, "/ ");
      break;
    case TOKEN_MODULO:
      buffer = strcat(buffer, "% ");
      break;
    case TOKEN_LT:
      buffer = strcat(buffer, "< ");
      break;
    case TOKEN_GT:
      buffer = strcat(buffer, "> ");
      break;
    case TOKEN_GTE:
      buffer = strcat(buffer, ">= ");
      break;
    case TOKEN_LTE:
      buffer = strcat(buffer, "<= ");
      break;
    case TOKEN_NOT_EQUAL:
      buffer = strcat(buffer, "!= ");
      break;
    case TOKEN_EQUALITY:
      buffer = strcat(buffer, "== ");
      break;
    }

    buffer = ast_to_sexpr(ast->data.AST_BINOP.left, buffer);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_BINOP.right, buffer);
    buffer = strcat(buffer, ")");
    break;
  }

  case AST_LAMBDA: {
    buffer = strcat(buffer, "(");
    if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
      buffer = strcat(buffer, ast->data.AST_LAMBDA.fn_name.chars);
      buffer = strcat(buffer, " ");
    }
    if (ast->data.AST_LAMBDA.len == 0) {
      buffer = strcat(buffer, "() ");
    } else {
      for (int i = 0; i < ast->data.AST_LAMBDA.len; i++) {
        buffer = strcat(buffer, ast->data.AST_LAMBDA.params[i].chars);
        buffer = strcat(buffer, " ");
      }
    }

    buffer = strcat(buffer, "-> ");
    buffer = ast_to_sexpr(ast->data.AST_LAMBDA.body, buffer);
    buffer = strcat(buffer, ")\n");

    break;
  }

  case AST_EXTERN_FN_DECLARATION: {
    buffer = strcat(buffer, "(extern ");
    if (ast->data.AST_EXTERN_FN_DECLARATION.fn_name.chars != NULL) {
      buffer =
          strcat(buffer, ast->data.AST_EXTERN_FN_DECLARATION.fn_name.chars);
      buffer = strcat(buffer, " ");
    }
    if (ast->data.AST_EXTERN_FN_DECLARATION.len == 0) {
      buffer = strcat(buffer, "() ");
    } else {
      for (int i = 0; i < ast->data.AST_EXTERN_FN_DECLARATION.len; i++) {
        buffer =
            strcat(buffer, ast->data.AST_EXTERN_FN_DECLARATION.params[i].chars);
        buffer = strcat(buffer, " ");
      }
    }

    buffer = strcat(buffer, "-> ");

    buffer =
        strcat(buffer, ast->data.AST_EXTERN_FN_DECLARATION.return_type.chars);
    buffer = strcat(buffer, ")\n");

    break;
  }

  case AST_LAMBDA_ARGS: {
    break;
  }
  case AST_LIST: {

    buffer = strcat(buffer, "[");
    int len = ast->data.AST_LIST.len;
    for (int i = 0; i < len; i++) {
      buffer = ast_to_sexpr(ast->data.AST_LIST.items + i, buffer);
      if (i < len - 1) {
        buffer = strcat(buffer, ", ");
      }
    }

    buffer = strcat(buffer, "]");
    break;
  }

  case AST_TUPLE: {
    buffer = strcat(buffer, "(");
    int len = ast->data.AST_LIST.len;
    for (int i = 0; i < len; i++) {
      buffer = ast_to_sexpr(ast->data.AST_LIST.items + i, buffer);
      if (i < len - 1 || len == 1) {
        buffer = strcat(buffer, ", ");
      }
    }

    buffer = strcat(buffer, ")");
    break;
  }

  case AST_MATCH: {

    buffer = strcat(buffer, "(match ");
    buffer = ast_to_sexpr(ast->data.AST_MATCH.expr, buffer);

    buffer = strcat(buffer, " with\n");
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {

      buffer = strcat(buffer, "\t");
      buffer = ast_to_sexpr(ast->data.AST_MATCH.branches + (i * 2), buffer);

      buffer = strcat(buffer, " -> ");

      buffer = ast_to_sexpr(ast->data.AST_MATCH.branches + (i * 2) + 1, buffer);

      buffer = strcat(buffer, "\n");
    }

    buffer = strcat(buffer, ")");
    break;
  }
  case AST_META: {
    buffer = strcat(buffer, "(");
    buffer = strcat(buffer, ast->data.AST_META.value);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_META.next, buffer);

    buffer = strcat(buffer, ")");
    break;
  }

  default: {
    // Handle unsupported node types or other errors
    break;
  }
  }
  return buffer;
}

void print_value(Value *val) {
  if (!val) {
    printf("()");
    return;
  }

  switch (val->type) {
  case VALUE_INT:
    printf("%d", val->value.vint);
    break;

  case VALUE_NUMBER:
    printf("%f", val->value.vnum);
    break;

  case VALUE_STRING:
    printf("%s", val->value.vstr.chars);
    break;

  case VALUE_BOOL:
    printf("%s", val->value.vbool ? "true" : "false");
    break;

  case VALUE_VOID:
    printf("()");
    break;

  case VALUE_FN:
    printf("function %s %p", val->value.function.fn_name, val);
    if (val->value.function.num_partial_args < val->value.function.len) {

      printf(" [");
      for (int i = 0; i < val->value.function.num_partial_args; i++) {
        print_value(val->value.function.partial_args + i);
      }
      printf("]");
      printf(" (%d / %d)", val->value.function.num_partial_args,
             val->value.function.len);
    }
    break;

  // case VALUE_PARTIAL_FN:
  //   if (val->value.partial_fn.type == FN) {
  //     printf("partial function %s %d/%d",
  //            val->value.partial_fn.function.fn.fn_name,
  //            val->value.partial_fn.num_partial_args,
  //            val->value.partial_fn.function.fn.len);
  //     break;
  //   } else {
  //
  //     printf("partial native function %d/%d",
  //            val->value.partial_fn.num_partial_args,
  //            val->value.partial_fn.function.native_fn.len);
  //     break;
  //   }
  //
  case VALUE_NATIVE_FN:
    printf("native function %p", val);

    if (val->value.native_fn.num_partial_args < val->value.function.len) {

      printf(" [");
      for (int i = 0; i < val->value.function.num_partial_args; i++) {
        print_value(val->value.function.partial_args + i);
      }
      printf("]");
      printf(" (%d / %d)", val->value.function.num_partial_args,
             val->value.function.len);
    }
    break;

  case VALUE_TYPE:

    printf("type %d", val->value.type);
    break;

  case VALUE_LIST:

    printf("[");

    int len = _list_length(1, val);
    for (int i = 0; i < len; i++) {
      Value v = list_nth(i, val);
      print_value(&v);
      if (i < len - 1) {
        printf(", ");
      }
    }
    printf("]");
    break;

  case VALUE_SYNTH_NODE:
    printf("synth node (%p)", val->value.vobj);
    break;

  case VALUE_OBJ:
    printf("void * (%p)", val->value.vobj);
    break;

  default:
    printf("unknown value type %d %d", val->type, val->value.type);
  }
}
