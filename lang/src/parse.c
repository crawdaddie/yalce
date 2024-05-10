#include "parse.h"
#include "serde.h"
#include <stdio.h>
#include <stdlib.h>

Parser *parser;
void print_current() { print_token(parser->current); }
void print_previous() { print_token(parser->previous); }

void init_parser(Parser *_parser, Lexer *lexer) {
  parser = _parser;
  parser->lexer = lexer;
  advance();
}

void advance() {
  parser->previous = parser->current;
  parser->current = scan_token(parser->lexer);
}

bool check(enum token_type type) { return parser->current.type == type; }

static bool is_terminator() {
  return check(TOKEN_RP) || check(TOKEN_DOUBLE_SEMICOLON) || check(TOKEN_NL);
}

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

Ast *nest_applications(Ast *application, Ast *item) {

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

static Ast *reduce_nested_applications(Ast *application) {
  if (application && application->data.AST_APPLICATION.arg == NULL) {
    return application->data.AST_APPLICATION.applicable;
  }

  return application;
}
static Ast *parse_grouping() { return NULL; }

#define AST_LITERAL_PREFIX(type, val)                                          \
  Ast *prefix = Ast_new(type);                                                 \
  prefix->data.type.value = val

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
  // case TOKEN_LP: {
  //   return parse_grouping();
  // }
  case TOKEN_IF:
  default: {
    advance();
    return NULL;
  }
  }
}
// clang-format off
// static ParserPrecedence precedence_rules[] = {
//   [TOKEN_ASSIGNMENT]  = PREC_ASSIGNMENT,
//   [TOKEN_EQUALITY]    = PREC_EQUALITY,
//   [TOKEN_NOT_EQUAL]   = PREC_EQUALITY,
//   [TOKEN_LT]          = PREC_COMPARISON,
//   [TOKEN_LTE]         = PREC_COMPARISON,
//   [TOKEN_GT]          = PREC_COMPARISON,
//   [TOKEN_GTE]         = PREC_COMPARISON,
//   [TOKEN_PLUS]        = PREC_TERM,
//   [TOKEN_MINUS]       = PREC_TERM,
//   [TOKEN_SLASH]       = PREC_FACTOR,
//   [TOKEN_STAR]        = PREC_FACTOR,
//   [TOKEN_MODULO]      = PREC_FACTOR,
//   [TOKEN_LEFT_SQ]     = PREC_INDEX
// };

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
  case TOKEN_DOT:         return PREC_CALL;

  case TOKEN_RP:          return PREC_NONE;

  case TOKEN_BANG:        return PREC_UNARY;

  case TOKEN_LOGICAL_OR:  return PREC_OR;

  case TOKEN_LOGICAL_AND: return PREC_AND;

  default:                return PREC_NONE;
  }
}
// clang-format on

static Ast *parse_precedence(Ast *prefix, ParserPrecedence precedence) {
  Ast *left = prefix;

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
  Ast *prefix = parse_prefix();
  if (prefix) {
    prefix = parse_precedence(prefix, PREC_ASSIGNMENT);
  }
  return prefix;
}

static Ast *parse_expression() {
  // Ast *application = NULL;
  // while (!match(TOKEN_DOUBLE_SEMICOLON) && !match(TOKEN_NL)) {
  //   Ast *expr = _parse_expression();
  //
  //   if (expr) {
  //     application = nest_applications(application, expr);
  //   }
  // }
  // return reduce_nested_applications(application);

  Ast *expr = _parse_expression();
  return expr;
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
