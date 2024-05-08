#include "serde.h"
#include <stdio.h>

void print_ser_ast(Ast *ast) {
  if (ast == NULL) {
    printf("[null]");
    return;
  }

  switch (ast->tag) {
  case AST_BODY: {
    printf("[\n");
    for (size_t i = 0; i < ast->data.AST_BODY.len; ++i) {
      Ast *stmt = ast->data.AST_BODY.members[i];
      print_ser_ast(stmt);
      printf("\n");
    }

    printf("]");
    break;
  }

  case AST_LET: {
    printf("assign %s to ", ast->data.AST_LET.name);
    print_ser_ast(ast->data.AST_LET.expr);
    break;
  }

  case AST_NUMBER: {
    printf("%f", ast->data.AST_NUMBER.value);
    break;
  }

  case AST_INT: {
    printf("%d", ast->data.AST_INT.value);
    break;
  }

  case AST_STRING: {
    printf("%s", ast->data.AST_STRING.value);
    break;
  }

  case AST_BOOL: {
    printf("%d", ast->data.AST_BOOL.value);
    break;
  }

  case AST_IDENTIFIER: {
    printf("%s", ast->data.AST_IDENTIFIER.value);
    break;
  }

  case AST_APPLICATION: {
    printf("(");
    for (size_t i = 0; i < ast->data.AST_APPLICATION.len; ++i) {
      Ast *stmt = ast->data.AST_APPLICATION.args[i];
      print_ser_ast(stmt);
      printf(" ");
    }
    printf(")"); 
    break;
  }

  case AST_TUPLE: {
    printf("(");
    for (size_t i = 0; i < ast->data.AST_TUPLE.len; ++i) {
      Ast *stmt = ast->data.AST_TUPLE.members[i];
      print_ser_ast(stmt);
      printf(", ");
    }
    printf(")"); 
    break;
  }

  case AST_BINOP: {
    printf("(");
    token tok = {.type = ast->data.AST_BINOP.op};
    print_token(tok);
    printf(" ");
    print_ser_ast(ast->data.AST_BINOP.left);
    printf(" ");
    print_ser_ast(ast->data.AST_BINOP.right);
    printf(")");
    break;
  }

  default: {
    printf("[%d]", ast->tag);
  }
  }
  // printf("\n");
}
