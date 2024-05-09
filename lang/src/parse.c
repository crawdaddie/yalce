#include "parse.h"
#include "lex.h"
#include "serde.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
Parser *parser;

void init_parser(Parser *_parser, Lexer *lexer) {
  parser = _parser;
  parser->lexer = lexer;
}

void advance() {
  parser->previous = parser->current;
  parser->current = scan_token(parser->lexer);
}

bool check(enum token_type type) { return parser->current.type == type; }
static bool match(enum token_type type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}
Ast *Ast_new(enum ast_tag tag) {
  Ast *node = malloc(sizeof(Ast));
  node->tag = tag;
  return node;
}

static Ast *parse_expression();

static Ast *parse_let_statement() {
  advance();
  advance();
  if (!check(TOKEN_ASSIGNMENT)) {
    fprintf(stderr, "Error: assignment = must follow let\n");
    return NULL;
  }

  Ast *ast_let = Ast_new(AST_LET);
  // char *id = strdup(parser->previous.as.vstr);
  char *id = parser->previous.as.vstr;
  advance();
  ast_let->data.AST_LET.name = id;
  ast_let->data.AST_LET.expr = parse_expression(NULL);
  return ast_let;
}


static Ast *parse_statement() {
  token tok = parser->current;
  switch (tok.type) {
  case TOKEN_LET: {
    return parse_let_statement();
  }
  default: {
    return parse_expression();
  }
  }
}

static void push_statement(Ast *body) {
  Ast *stmt = parse_statement();
  if (stmt) {
    Ast **members = body->data.AST_BODY.members;
    body->data.AST_BODY.len++;
    int len = body->data.AST_BODY.len;

    body->data.AST_BODY.members = realloc(members, sizeof(Ast *) * len);
    body->data.AST_BODY.members[len - 1] = stmt;
  }
}

Ast *parse_body(Ast *body) {
  if (body == NULL) {
    body = Ast_new(AST_BODY);
    body->data.AST_BODY.len = 0;
    body->data.AST_BODY.members = malloc(sizeof(Ast *));
  }

  advance();
  while (!check(TOKEN_EOF)) {
    push_statement(body);
  }
  return body;
}

Ast *body_return(Ast *body) {
  if (body->data.AST_BODY.len == 0 || body->data.AST_BODY.members == NULL) {
    return NULL;
  }
  return body->data.AST_BODY.members[body->data.AST_BODY.len - 1];
}

// -------- parse expressions --------------
ParserPrecedence token_to_precedence(token tok) {
  switch (tok.type) {
  case TOKEN_ASSIGNMENT:
    return PREC_ASSIGNMENT;
  case TOKEN_EQUALITY:
  case TOKEN_NOT_EQUAL:   return PREC_EQUALITY;
  case TOKEN_LT:
  case TOKEN_LTE:
  case TOKEN_GT:
  case TOKEN_GTE:         return PREC_COMPARISON;
  case TOKEN_PLUS:
  case TOKEN_MINUS:       return PREC_TERM;
  case TOKEN_SLASH:
  case TOKEN_STAR:
  case TOKEN_MODULO:      return PREC_FACTOR;
  case TOKEN_LEFT_SQ:     return PREC_INDEX;
  case TOKEN_LP:
  case TOKEN_DOT:         return PREC_CALL;
  case TOKEN_RP:          return PREC_NONE;
  case TOKEN_BANG:        return PREC_UNARY;
  case TOKEN_LOGICAL_OR:  return PREC_OR;
  case TOKEN_LOGICAL_AND: return PREC_AND;
  default:                return PREC_NONE;
  }
}
static Ast *parse_prefix();
static Ast *parse_infix_expr() { return NULL; }

static Ast *parse_precedence(Ast *prefix, ParserPrecedence precedence) {
  Ast *left = prefix;
  // printf("precedence: %d\ncurrent token to prec: %d\n", precedence, token_to_precedence(parser->current));
  // print_token(parser->current);
  // printf("\nleft: ");
  // print_ser_ast(left);
  // printf("\n");
  // printf("-----\n");


  while (precedence <= token_to_precedence(parser->current)) {
    switch (parser->current.type) {

    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_SLASH:
    case TOKEN_STAR:
    case TOKEN_MODULO:
    case TOKEN_EQUALITY:
    case TOKEN_NOT_EQUAL:
    case TOKEN_LT:
    case TOKEN_LTE:
    case TOKEN_GT:
    case TOKEN_GTE: {
      Ast *binop = Ast_new(AST_BINOP);
      binop->data.AST_BINOP.op = parser->current.type;
      binop->data.AST_BINOP.left = left;

      advance();
      binop->data.AST_BINOP.right = parse_expression();
      left = binop;
      break;
    }
    default:
      return left;
    }
  }
  return left;
};


static Ast *parse_expression() {
  while (check(TOKEN_NL)) {
    advance();
  }

  // prefix
  Ast *prefix = parse_prefix();
  Ast *ast = parse_precedence(prefix, PREC_ASSIGNMENT);
  return ast;
}

static void push_arg(Ast *fn_application, Ast *arg) {
  if (arg) {
    Ast **args = fn_application->data.AST_APPLICATION.args;
    fn_application->data.AST_APPLICATION.len++;
    int len = fn_application->data.AST_APPLICATION.len;

    fn_application->data.AST_APPLICATION.args =
        realloc(args, sizeof(Ast *) * len);
    fn_application->data.AST_APPLICATION.args[len - 1] = arg;
  }
}

static void push_tuple_member(Ast *tuple, Ast *member) {
  if (member) {
    Ast **args = tuple->data.AST_TUPLE.members;
    tuple->data.AST_TUPLE.len++;
    int len = tuple->data.AST_TUPLE.len;

    tuple->data.AST_TUPLE.members = realloc(args, sizeof(Ast *) * len);
    tuple->data.AST_TUPLE.members[len - 1] = member;
  }
}

#define AST_LITERAL_PREFIX(type, val)                                          \
  Ast *prefix = Ast_new(type);                                                 \
  prefix->data.type.value = val

static bool is_terminator() {
  return check(TOKEN_RP) || check(TOKEN_DOUBLE_SEMICOLON) || check(TOKEN_NL);
}
static Ast *parse_grouping() {
  advance();
  Ast *first = parse_expression(NULL);
  if (check(TOKEN_COMMA)) {
    Ast *tuple = Ast_new(AST_TUPLE);
    tuple->data.AST_TUPLE.len = 1;
    tuple->data.AST_TUPLE.members = malloc(sizeof(Ast *));
    tuple->data.AST_TUPLE.members[0] = first;
    advance();

    while (!check(TOKEN_RP)) {
      push_tuple_member(tuple, parse_expression());
      check(TOKEN_COMMA);
    }
    return tuple;
  }
  if (parser->current.type == TOKEN_RP) {
    advance();
    return first;
  }
  return NULL;
}

static Ast *parse_prefix() {
  // printf("prefix: ");
  // print_token(parser->current);
  switch (parser->current.type) {
  case TOKEN_INTEGER: {
    AST_LITERAL_PREFIX(AST_INT, parser->current.as.vint);
    advance();
    return prefix;
  }
  case TOKEN_NUMBER: {
    AST_LITERAL_PREFIX(AST_NUMBER, parser->current.as.vfloat);
    advance();
    return prefix;
  }
  case TOKEN_STRING: {
    AST_LITERAL_PREFIX(AST_STRING, parser->current.as.vstr);
    advance();
    return prefix;
  }
  case TOKEN_TRUE: {
    AST_LITERAL_PREFIX(AST_BOOL, true);
    advance();
    return prefix;
  }
  case TOKEN_FALSE: {
    AST_LITERAL_PREFIX(AST_BOOL, false);
    advance();
    return prefix;
  }
  case TOKEN_IDENTIFIER: {
    Ast *prefix = Ast_new(AST_APPLICATION);
    prefix->data.AST_APPLICATION.len = 0;
    prefix->data.AST_APPLICATION.args = malloc(sizeof(Ast *));

    Ast *id = Ast_new(AST_IDENTIFIER);
    id->data.AST_IDENTIFIER.value = parser->current.as.vident;
    push_arg(prefix, id);
    advance();

    while (!is_terminator()) {
      Ast *arg = parse_expression(NULL);
      push_arg(prefix, arg);
    }

    return prefix;
  }
  case TOKEN_BANG: {
    Ast *prefix = Ast_new(AST_UNOP);
    prefix->data.AST_UNOP.op = parser->current.type;
    advance();
    prefix->data.AST_UNOP.expr = parse_expression(NULL);
    return prefix;
  }
  case TOKEN_LP: {
    return parse_grouping();
  }
  case TOKEN_IF:
  default: {
    advance();
    return NULL;
  }
  }
}
