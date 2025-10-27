#include "./util.h"
// Print AST directly to stdout without buffering (WASM-friendly)
void print_ast_wasm(Ast *ast) {

  if (!ast) {
    printf("null");
    return;
  }

  switch (ast->tag) {
  case AST_BODY:
    for (AstList *current = ast->data.AST_BODY.stmts; current != NULL;
         current = current->next) {
      print_ast_wasm(current->ast);
      if (current->next != NULL) {
        printf("\n");
      }
    }
    break;

  case AST_LET:
    printf("(let ");
    print_ast_wasm(ast->data.AST_LET.binding);
    printf(" ");
    print_ast_wasm(ast->data.AST_LET.expr);
    printf(")");
    if (ast->data.AST_LET.in_expr) {
      printf(" : ");
      print_ast_wasm(ast->data.AST_LET.in_expr);
    }
    break;

  case AST_INT:
    printf("%d", ast->data.AST_INT.value);
    break;

  case AST_FLOAT:
    printf("%f", ast->data.AST_FLOAT.value);
    break;

  case AST_DOUBLE:
    printf("%f", ast->data.AST_DOUBLE.value);
    break;

  case AST_STRING:
    printf("\"");
    printf("%.*s", (int)ast->data.AST_STRING.length,
           ast->data.AST_STRING.value);
    printf("\"");
    break;

  case AST_CHAR:
    printf("'%c'", ast->data.AST_CHAR.value);
    break;

  case AST_BOOL:
    printf("%s", ast->data.AST_BOOL.value ? "true" : "false");
    break;

  case AST_VOID:
    printf("()");
    break;

  case AST_IDENTIFIER:
    printf("%s", ast->data.AST_IDENTIFIER.value);
    break;

  case AST_PLACEHOLDER_ID:
    printf("_");
    break;

  case AST_APPLICATION:
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      printf("(");
    }
    print_ast_wasm(ast->data.AST_APPLICATION.function);
    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      printf(" ");
      print_ast_wasm(ast->data.AST_APPLICATION.args + i);
      printf(")");
    }
    break;

  case AST_BINOP:
    printf("(");
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS:
      printf("+");
      break;
    case TOKEN_MINUS:
      printf("-");
      break;
    case TOKEN_STAR:
      printf("*");
      break;
    case TOKEN_SLASH:
      printf("/");
      break;
    case TOKEN_LT:
      printf("<");
      break;
    case TOKEN_GT:
      printf(">");
      break;
    case TOKEN_LTE:
      printf("<=");
      break;
    case TOKEN_GTE:
      printf(">=");
      break;
    case TOKEN_EQUALITY:
      printf("==");
      break;
    case TOKEN_NOT_EQUAL:
      printf("!=");
      break;
    default:
      printf("?");
      break;
    }
    printf(" ");
    print_ast_wasm(ast->data.AST_BINOP.left);
    printf(" ");
    print_ast_wasm(ast->data.AST_BINOP.right);
    printf(")");
    break;

  case AST_LAMBDA:
    printf("(fn ");
    print_ast_wasm(ast->data.AST_LAMBDA.params);
    printf(" -> ");
    print_ast_wasm(ast->data.AST_LAMBDA.body);
    printf(")");
    break;

  case AST_MATCH:
    printf("(match ");
    print_ast_wasm(ast->data.AST_MATCH.expr);
    printf(" { ... })");
    break;

  default:
    printf("(? tag=%d)", ast->tag);
    break;
  }
}
