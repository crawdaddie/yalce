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

static void push_fn_param(Ast *fn, const char *param) {
  if (param) {
    const char **args = fn->data.AST_FN_DECLARATION.params;
    fn->data.AST_FN_DECLARATION.params_len++;
    size_t len = fn->data.AST_FN_DECLARATION.params_len;

    fn->data.AST_FN_DECLARATION.params = realloc(args, sizeof(Ast *) * len);
    fn->data.AST_FN_DECLARATION.params[len - 1] = param;
  }
}

static Ast *parse_fn_declaration() {
  Ast *fn = Ast_new(AST_FN_DECLARATION);
  fn->data.AST_FN_DECLARATION.params = malloc(sizeof(char *));
  fn->data.AST_FN_DECLARATION.params_len = 0;
  fn->data.AST_FN_DECLARATION.fn_name = NULL;
  advance();
  while (!check(TOKEN_ARROW)) {
    if (check(TOKEN_IDENTIFIER)) {
      const char *param_name = parser->current.as.vident;
      push_fn_param(fn, param_name);
      advance();
    };
  }
  Ast *body = parse_body(NULL);
  fn->data.AST_FN_DECLARATION.body = body;
  return fn;
}
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
  if (check(TOKEN_FN)) {
    ast_let->data.AST_LET.expr = parse_fn_declaration();
    ast_let->data.AST_LET.expr->data.AST_FN_DECLARATION.fn_name = id;
    return ast_let;
  }
  ast_let->data.AST_LET.expr = parse_expression();
  return ast_let;
}

Ast *handle_applications(Ast *application, Ast *item) {
  if (application == NULL) {
    application = Ast_new(AST_APPLICATION);
    application->data.AST_APPLICATION.applicable = item;
    return application;
  }
  if (application != NULL &&
      application->data.AST_APPLICATION.applicable != NULL &&
      application->data.AST_APPLICATION.arg == NULL) {
    application->data.AST_APPLICATION.arg = item;
    return application;
  }

  if (application != NULL &&
      application->data.AST_APPLICATION.applicable != NULL &&
      application->data.AST_APPLICATION.arg != NULL) {

    Ast *new_application = Ast_new(AST_APPLICATION);
    new_application->data.AST_APPLICATION.applicable = application;
    new_application->data.AST_APPLICATION.arg = item;
    return new_application;
  }
  return NULL;
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

static void push_statement(Ast *body, Ast *stmt) {
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
    Ast *stmt = parse_statement();
    push_statement(body, stmt);
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
  case TOKEN_NOT_EQUAL:
    return PREC_EQUALITY;
  case TOKEN_LT:
  case TOKEN_LTE:
  case TOKEN_GT:
  case TOKEN_GTE:
    return PREC_COMPARISON;
  case TOKEN_PLUS:
  case TOKEN_MINUS:
    return PREC_TERM;
  case TOKEN_SLASH:
  case TOKEN_STAR:
  case TOKEN_MODULO:
    return PREC_FACTOR;
  case TOKEN_LEFT_SQ:
    return PREC_INDEX;
  case TOKEN_LP:
  case TOKEN_DOT:
    return PREC_CALL;
  case TOKEN_RP:
    return PREC_NONE;
  case TOKEN_BANG:
    return PREC_UNARY;
  case TOKEN_LOGICAL_OR:
    return PREC_OR;
  case TOKEN_LOGICAL_AND:
    return PREC_AND;
  default:
    return PREC_NONE;
  }
}
static Ast *parse_prefix();

static Ast *parse_precedence(Ast *prefix, ParserPrecedence precedence) {
  Ast *left = prefix;
  // printf("precedence: %d\ncurrent token to prec: %d\n", precedence,
  // token_to_precedence(parser->current)); print_token(parser->current);
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

static Ast *_parse_expression() {
  // while (check(TOKEN_NL)) {
  //   advance();
  // }

  // prefix
  Ast *prefix = parse_prefix();
  Ast *ast = parse_precedence(prefix, PREC_ASSIGNMENT);
  return ast;
}
static Ast *reduce_application(Ast *application) {
  if (application && application->data.AST_APPLICATION.arg == NULL) {
    return application->data.AST_APPLICATION.applicable;
  }

  return application;
}

static Ast *parse_expression() {
  Ast *application = NULL;
  while (!check(TOKEN_DOUBLE_SEMICOLON) && !check(TOKEN_NL)) {
    Ast *expr = _parse_expression();
    application = handle_applications(application, expr);
  }
  advance();
  return reduce_application(application);
}

static void _push_application_param(Ast *fn_application, Ast *arg) {
  if (arg) {
    Ast **args = fn_application->data._AST_APPLICATION.args;
    fn_application->data._AST_APPLICATION.len++;
    int len = fn_application->data._AST_APPLICATION.len;

    fn_application->data._AST_APPLICATION.args =
        realloc(args, sizeof(Ast *) * len);
    fn_application->data._AST_APPLICATION.args[len - 1] = arg;
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
  Ast *first = parse_expression();
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

  return first;
}

static Ast *parse_prefix() {
  // printf("inside grouping");
  // print_token(parser->current);
  // print_ser_ast(first);
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
    // Ast *prefix = Ast_new(AST_APPLICATION);
    // prefix->data.AST_APPLICATION.len = 0;
    // prefix->data.AST_APPLICATION.args = malloc(sizeof(Ast *));
    //
    Ast *id = Ast_new(AST_IDENTIFIER);
    id->data.AST_IDENTIFIER.value = parser->current.as.vident;
    // push_application_param(prefix, id);
    advance();
    //
    // while (!is_terminator()) {
    //   Ast *arg = parse_expression();
    //   push_application_param(prefix, arg);
    // }

    return id;
  }
  case TOKEN_FN: {
    return parse_fn_declaration();
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
