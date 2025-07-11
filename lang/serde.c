#include "serde.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_ast(Ast *ast) {
  char *buf = malloc(sizeof(char) * 500);
  printf("%s\n", ast_to_sexpr(ast, buf));
  // printf(COLOR_RESET);
  free(buf);
}

void print_ast_err(Ast *ast) {
  char *buf = malloc(sizeof(char) * 500);
  fprintf(stderr, "%s\n", ast_to_sexpr(ast, buf));
  // fprintf(stderr, COLOR_RESET);
  free(buf);
}

char *ast_to_sexpr(Ast *ast, char *buffer) {
  // buffer = strcat(buffer, COLOR_RED);
  if (!ast) {
    buffer = strcat(buffer, "null");
    return buffer;
  }

  switch (ast->tag) {
  case AST_BODY: {
    for (size_t i = 0; i < ast->data.AST_BODY.len; ++i) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      buffer = ast_to_sexpr(stmt, buffer);
      if (i < ast->data.AST_BODY.len - 1) {
        buffer = strcat(buffer, "\n");
      }
    }
    break;
  }

  case AST_LET: {
    buffer = strcat(buffer, "(let ");
    buffer = ast_to_sexpr(ast->data.AST_LET.binding, buffer);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_LET.expr, buffer);
    buffer = strcat(buffer, ")");

    if (ast->data.AST_LET.in_expr) {
      buffer = strcat(buffer, " : ");
      buffer = ast_to_sexpr(ast->data.AST_LET.in_expr, buffer);
    }
    break;
  }
  case AST_TYPE_DECL: {
    buffer = strcat(buffer, "(let ");
    buffer = ast_to_sexpr(ast->data.AST_LET.binding, buffer);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_LET.expr, buffer);
    buffer = strcat(buffer, ")");

    if (ast->data.AST_LET.in_expr) {
      buffer = strcat(buffer, " : ");
      buffer = ast_to_sexpr(ast->data.AST_LET.in_expr, buffer);
    }
    break;
  }

  case AST_DOUBLE: {

    char buf[100];
    sprintf(buf, "%f", ast->data.AST_DOUBLE.value);
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
    buffer = strncat(buffer, ast->data.AST_STRING.value,
                     ast->data.AST_STRING.length);
    buffer = strcat(buffer, "\"");
    break;
  }

  case AST_CHAR: {

    buffer = strcat(buffer, "'");
    buffer = strcat(buffer, &(ast->data.AST_CHAR.value));
    buffer = strcat(buffer, "'");
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

  case AST_UNOP: {
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_STAR: {

      buffer = strcat(buffer, "(deref ");

      buffer = ast_to_sexpr(ast->data.AST_BINOP.right, buffer);
      buffer = strcat(buffer, ")");

      break;
    }

    case TOKEN_AMPERSAND: {
      buffer = strcat(buffer, "(addr_of ");
      buffer = ast_to_sexpr(ast->data.AST_BINOP.right, buffer);
      buffer = strcat(buffer, ")");
      break;
    }
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
    case TOKEN_DOUBLE_COLON:
      buffer = strcat(buffer, ":: ");
      break;

    case TOKEN_OF:
      buffer = strcat(buffer, "cons ");
      break;
    }

    buffer = ast_to_sexpr(ast->data.AST_BINOP.left, buffer);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_BINOP.right, buffer);
    buffer = strcat(buffer, ")");
    break;
  }

  case AST_RECORD_ACCESS: {
    buffer = strcat(buffer, "(");
    buffer = strcat(buffer, ". ");

    buffer = ast_to_sexpr(ast->data.AST_RECORD_ACCESS.record, buffer);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_RECORD_ACCESS.member, buffer);
    buffer = strcat(buffer, ")");
    break;
  }

  case AST_MODULE: {
    buffer = strcat(buffer, "Module (");
    if (ast->data.AST_LAMBDA.is_coroutine) {
      buffer = strcat(buffer, "[coroutine] ");
    }
    if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
      buffer = strcat(buffer, ast->data.AST_LAMBDA.fn_name.chars);
      buffer = strcat(buffer, " ");
    }
    if (ast->data.AST_LAMBDA.len == 0) {
      buffer = strcat(buffer, "() ");
    } else {

      AST_LIST_ITER(ast->data.AST_LAMBDA.params, ({
                      buffer = ast_to_sexpr(l->ast, buffer);
                      buffer = strcat(buffer, " ");
                    }));
    }

    buffer = strcat(buffer, "-> \n");
    buffer = ast_to_sexpr(ast->data.AST_LAMBDA.body, buffer);
    buffer = strcat(buffer, ")\n");

    break;
  }
  case AST_TRAIT_IMPL: {
    buffer = strcat(buffer, ast->data.AST_TRAIT_IMPL.trait_name.chars);
    buffer = strcat(buffer, " for ");
    buffer = strcat(buffer, ast->data.AST_TRAIT_IMPL.type.chars);

    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_TRAIT_IMPL.impl, buffer);

    break;
  }
  case AST_IMPORT: {
    buffer = strcat(buffer, "import ");
    if (ast->data.AST_IMPORT.import_all) {

      buffer = strcat(buffer, " * ");
    }

    buffer = strcat(buffer, ast->data.AST_IMPORT.fully_qualified_name);
    buffer = strcat(buffer, " as ");
    buffer = strcat(buffer, ast->data.AST_IMPORT.identifier);
    break;
  }

  case AST_LAMBDA: {
    buffer = strcat(buffer, "(");
    if (ast->data.AST_LAMBDA.is_coroutine) {
      buffer = strcat(buffer, "[coroutine] ");
    }
    if (ast->data.AST_LAMBDA.fn_name.chars != NULL) {
      buffer = strcat(buffer, ast->data.AST_LAMBDA.fn_name.chars);
      buffer = strcat(buffer, " ");
    }
    if (ast->data.AST_LAMBDA.len == 0) {
      buffer = strcat(buffer, "() ");
    } else {
      AST_LIST_ITER(ast->data.AST_LAMBDA.params, ({
                      buffer = ast_to_sexpr(l->ast, buffer);
                      buffer = strcat(buffer, " ");
                    }));
    }

    buffer = strcat(buffer, "-> \n");
    buffer = ast_to_sexpr(ast->data.AST_LAMBDA.body, buffer);
    buffer = strcat(buffer, ")\n");

    break;
  }

  case AST_EXTERN_VARIANTS: {

    buffer = strcat(buffer, "[\n\t");
    int len = ast->data.AST_LIST.len;
    for (int i = 0; i < len; i++) {
      buffer = ast_to_sexpr(ast->data.AST_LIST.items + i, buffer);
      if (i < len - 1) {
        buffer = strcat(buffer, ",\n\t");
      }
    }

    buffer = strcat(buffer, "\n]");
    break;
  }

  case AST_EXTERN_FN: {
    // printf("serde extern fn\n");
    buffer = strcat(buffer, "(extern ");

    if (ast->data.AST_EXTERN_FN.fn_name.chars != NULL) {
      buffer = strcat(buffer, ast->data.AST_EXTERN_FN.fn_name.chars);
      buffer = strcat(buffer, " ");
    }
    buffer = ast_to_sexpr(ast->data.AST_EXTERN_FN.signature_types, buffer);
    buffer = strcat(buffer, ")");

    break;
  }
  //
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

  case AST_ARRAY: {
    buffer = strcat(buffer, "[|");
    int len = ast->data.AST_LIST.len;
    for (int i = 0; i < len; i++) {
      buffer = ast_to_sexpr(ast->data.AST_LIST.items + i, buffer);
      if (i < len - 1) {
        buffer = strcat(buffer, ", ");
      }
    }

    buffer = strcat(buffer, "|]");
    break;
  }
  case AST_FN_SIGNATURE: {

    buffer = strcat(buffer, "(");
    int len = ast->data.AST_LIST.len;
    for (int i = 0; i < len; i++) {
      buffer = ast_to_sexpr(ast->data.AST_LIST.items + i, buffer);
      if (i < len - 1) {
        buffer = strcat(buffer, " -> ");
      }
    }

    buffer = strcat(buffer, ")");
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

  case AST_FMT_STRING: {
    buffer = strcat(buffer, "str(");
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

  case AST_MATCH_GUARD_CLAUSE: {
    buffer = ast_to_sexpr(ast->data.AST_MATCH_GUARD_CLAUSE.test_expr, buffer);
    buffer = strcat(buffer, " if ");
    buffer = ast_to_sexpr(ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr, buffer);
    break;
  }

  case AST_YIELD: {
    buffer = strcat(buffer, "(yield ");
    buffer = ast_to_sexpr(ast->data.AST_YIELD.expr, buffer);
    buffer = strcat(buffer, ")");
    break;
  }

  case AST_SPREAD_OP: {
    buffer = strcat(buffer, "(... ");
    buffer = ast_to_sexpr(ast->data.AST_SPREAD_OP.expr, buffer);
    buffer = strcat(buffer, ")");
    break;
  }
  case AST_EMPTY_LIST: {
    buffer = strcat(buffer, ast->data.AST_EMPTY_LIST.type_id.chars);
    buffer = strcat(buffer, "[]");
    break;
  }

  case AST_RANGE_EXPRESSION: {
    buffer = strcat(buffer, "(range ");
    buffer = ast_to_sexpr(ast->data.AST_RANGE_EXPRESSION.from, buffer);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_RANGE_EXPRESSION.to, buffer);
    buffer = strcat(buffer, ")");
    break;
  }

  case AST_LOOP: {
    buffer = strcat(buffer, "(for ");
    buffer = ast_to_sexpr(ast->data.AST_LET.binding, buffer);

    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_LET.expr, buffer);
    buffer = strcat(buffer, " ");
    buffer = ast_to_sexpr(ast->data.AST_LET.in_expr, buffer);
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
