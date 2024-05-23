#include "serde.h"
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

  case AST_APPLICATION: {
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      buffer = strcat(buffer, "(");
    }
    buffer = ast_to_sexpr(ast->data.AST_APPLICATION.function, buffer);

    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      buffer = strcat(buffer, " ");
      buffer = ast_to_sexpr(ast->data.AST_APPLICATION.args[i], buffer);
      buffer = strcat(buffer, ")");
    }

    break;
  }

  case AST_TUPLE: {
    buffer = strcat(buffer, "(");
    for (int i = 0; i < ast->data.AST_TUPLE.len; i++) {
      buffer = ast_to_sexpr(ast->data.AST_TUPLE.members[i], buffer);
      buffer = strcat(buffer, ",");
    }
    buffer = strcat(buffer, ")");
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
    printf("[%d]", val->value.vint);
    break;

  case VALUE_NUMBER:
    printf("[%f]", val->value.vnum);
    break;

  case VALUE_STRING:
    printf("[%s]", val->value.vstr.chars);
    break;

  case VALUE_BOOL:
    printf("[%s]", val->value.vbool ? "true" : "false");
    break;

  case VALUE_VOID:
    printf("[()]");
    break;

  case VALUE_FN:
    printf("[function [%p]]", val);
    break;

  case VALUE_EXTERN_FN:
    printf("[function [%p]]", val);
    break;

  case VALUE_TYPE:
    printf("[type %d]", val->value.type);
    break;

  default:
    printf("unknown value type %d %d", val->type, val->value.type);
  }
}
