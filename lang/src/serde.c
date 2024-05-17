#include "serde.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_ast(Ast *ast) {
  char *buf = malloc(sizeof(char));
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
    break;
  }

  case AST_NUMBER: {

    char buf[12];
    sprintf(buf, "%f", ast->data.AST_NUMBER.value);
    buffer = strcat(buffer, buf);
    break;
  }

  case AST_INT: {

    char buf[12];
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
  case AST_IDENTIFIER: {
    buffer = strcat(buffer, ast->data.AST_IDENTIFIER.value);
    break;
  }

  case AST_APPLICATION: {
    buffer = strcat(buffer, "(");
    buffer = ast_to_sexpr(ast->data.AST_APPLICATION.function, buffer);

    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_APPLICATION.arg, buffer);
    buffer = strcat(buffer, ")");
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

  case AST_FN_DECLARATION: {
    break;
  }

  default: {
    // Handle unsupported node types or other errors
    break;
  }
  }
  return buffer;
}
