#include "parse.h"
#include "serde.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Parser *parser;
void print_current() { print_token(parser->current); }
void print_previous() { print_token(parser->previous); }

void init_parser(Parser *_parser, Lexer *lexer) {
  parser = _parser;
  parser->lexer = lexer;
  advance();
}

token advance() {
  parser->previous = parser->current;
  parser->current = scan_token(parser->lexer);
  return parser->current;
}

bool check(enum token_type type) { return parser->current.type == type; }

static bool match(enum token_type type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

static bool is_terminator() {
  switch (parser->current.type) {
  case TOKEN_DOUBLE_SEMICOLON:
  case TOKEN_NL:
  case TOKEN_EOF:
    advance();
    return true;
  default:
    return false;
  }
}

static bool is_binop_token() {
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
  case TOKEN_GTE:
    return true;
  // case TOKEN_RP: return true;
  default:
    return false;
  }
}

Ast *Ast_new(enum ast_tag tag) {
  Ast *node = malloc(sizeof(Ast));
  node->tag = tag;
  return node;
}

static Ast *parse_expression();

static Ast *parse_grouping() {

  match(TOKEN_LP);
  Ast *first = parse_expression();

  if (check(TOKEN_COMMA)) {
    advance();
    Ast *tuple = Ast_new(AST_TUPLE);
    tuple->data.AST_TUPLE.len = 1;
    tuple->data.AST_TUPLE.members = malloc(sizeof(Ast *));
    tuple->data.AST_TUPLE.members[0] = first;

    while (!match(TOKEN_RP)) {
      size_t index = tuple->data.AST_TUPLE.len;
      tuple->data.AST_TUPLE.len++;
      tuple->data.AST_TUPLE.members[index] = parse_expression();
      match(TOKEN_COMMA);
    }
    return tuple;
  }

  match(TOKEN_RP);
  // printf("finish grouping: ");
  // print_ast(first);
  // printf("\n");

  return first;
}
#define AST_LITERAL_PREFIX(type, val)                                          \
  Ast *prefix = Ast_new(type);                                                 \
  prefix->data.type.value = val

static Ast *parse_prefix() {
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
    AST_LITERAL_PREFIX(AST_IDENTIFIER, parser->current.as.vident);
    advance();
    return prefix;
  }
  // case TOKEN_FN: {
  //   return parse_fn_declaration();
  // }
  case TOKEN_BANG: {
    Ast *prefix = Ast_new(AST_UNOP);
    prefix->data.AST_UNOP.op = parser->current.type;
    advance();
    prefix->data.AST_UNOP.expr = parse_expression();
    return prefix;
  }
  case TOKEN_LP: {
    return parse_grouping();
  }
  case TOKEN_IF:

  default: {
    // advance();
    return NULL;
  }
  }
}
//
// fn token_to_precedence(tok: &Token) -> Precedence {
//     match tok {
//     }
// }

// clang-format off
ParserPrecedence token_to_precedence(token tok) {
  switch (tok.type) {
    case TOKEN_ASSIGNMENT:  return PREC_ASSIGNMENT;

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
    case TOKEN_RP:
    case TOKEN_DOT:         return PREC_NONE;

    case TOKEN_BANG:        return PREC_UNARY;
    case TOKEN_LOGICAL_OR:  return PREC_OR;
    case TOKEN_LOGICAL_AND: return PREC_AND;
    default:                return PREC_NONE;
  }
}
// clang-format on

static Ast *parse_precedence(ParserPrecedence precedence) {
  Ast *left = parse_prefix();

  while (precedence < token_to_precedence(parser->current)) {
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
      binop->data.AST_BINOP.right =
          parse_precedence(token_to_precedence(parser->previous));

      left = binop;
      break;
    }
    default:
      return left;
    }
  }

  match(TOKEN_RP);
  return left;
}

Ast *nest_applications(Ast *application, Ast *item) {

  if (application->data.AST_APPLICATION.applicable == NULL) {
    application->data.AST_APPLICATION.applicable = item;
    return application;
  }

  if (application->data.AST_APPLICATION.arg == NULL) {
    application->data.AST_APPLICATION.arg = item;
    return application;
  }

  Ast *new_app = Ast_new(AST_APPLICATION);
  new_app->data.AST_APPLICATION.applicable = application;
  new_app->data.AST_APPLICATION.arg = item;

  return new_app;
}

static Ast *reduce_degenerate_application(Ast *application) {
  if (application && application->data.AST_APPLICATION.arg == NULL) {
    return application->data.AST_APPLICATION.applicable;
  }

  return application;
}
static bool application_is_empty(Ast *app) {
  return app->data.AST_APPLICATION.applicable == NULL &&
         app->data.AST_APPLICATION.arg == NULL;
}
static bool is_applicable(Ast *item) {
  return (item->tag != AST_FN_DECLARATION) && (item->tag != AST_IDENTIFIER) &&
         (item->tag != AST_LAMBDA);
}

static Ast *parse_expression() {

  Ast *application = Ast_new(AST_APPLICATION);

  while (!is_terminator() && !is_binop_token()) {
    Ast *item = parse_precedence(PREC_NONE);
    if (application_is_empty(application) && is_applicable(item)) {
      free(application);
      return item;
    }
    application = nest_applications(application, item);
  }
  // parser->application = NULL;

  return reduce_degenerate_application(application);
}

static Ast *parse_statement() { return parse_expression(); }

static void push_statement(Ast *body, Ast *stmt) {
  if (stmt) {
    Ast **members = body->data.AST_BODY.stmts;
    body->data.AST_BODY.len++;
    int len = body->data.AST_BODY.len;

    body->data.AST_BODY.stmts = realloc(members, sizeof(Ast *) * len);
    body->data.AST_BODY.stmts[len - 1] = stmt;
  }
}

Ast *parse_body(Ast *body) {
  while (!check(TOKEN_EOF)) {
    push_statement(body, parse_statement());
  }

  return body;
}
