#include "compiler.h"
#include "dbg.h"
#include "lexer.h"
#include "util.h"
typedef struct {
  token current;
  token previous;
  bool had_error;
  bool panic_mode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;
Chunk *compiling_chunk;
static Chunk *current_chunk() { return compiling_chunk; }
static void error_at(token *token, const char *message) {
  if (parser.panic_mode)
    return;
  parser.panic_mode = true;
  // TODO: add line info
  fprintf(stderr, "Error");
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {

    line_info linfo = get_line_info();
    fprintf(stderr, " at %d:%d", linfo.line, linfo.col_offset);
    // nothing
  } else {
    // nothing
  }
  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}
static void error(const char *message) { error_at(&parser.previous, message); }
static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}
static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scan_token();

    if (parser.current.type != TOKEN_ERROR)
      break;
    error_at_current(get_scanner_current());
  }
}

static void consume(enum token_type type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}
static bool check(enum token_type type) { return parser.current.type == type; }
static bool match(enum token_type type) {
  if (!check(type))
    return false;
  advance();
  return true;
}
static void emit_byte(uint8_t byte) { write_chunk(current_chunk(), byte); }
static void emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}
static uint8_t make_constant(Value value) {
  int constant = add_constant(current_chunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk");
    return 0;
  }
  return (uint8_t)constant;
}

static void expression();
static void statement();
static void declaration();
static ParseRule *get_rule(enum token_type type);
static void parse_precedence(Precedence precedence);

static void number() {
  Value value = parser.previous.type == TOKEN_INTEGER
                    ? INTEGER_VAL(parser.previous.as.vint)
                    : NUMBER_VAL(parser.previous.as.vfloat);
  emit_bytes(OP_CONSTANT, make_constant(value));
}

static void integer() {
  Value value = INTEGER_VAL(parser.previous.as.vint);
  emit_bytes(OP_CONSTANT, make_constant(value));
}
static void grouping() {
  expression();
  consume(TOKEN_RP, "Expect ')' after expression");
}
static void unary() {
  enum token_type op_type = parser.previous.type;
  parse_precedence(PREC_UNARY);
  switch (op_type) {
  case TOKEN_BANG:
    emit_byte(OP_NOT);
    break;

  case TOKEN_MINUS:
    emit_byte(OP_NEGATE);
    break;
  default:
    return;
  }
}
static void parse_literal() {
  switch (parser.previous.type) {
  case TOKEN_FALSE:
    emit_byte(OP_FALSE);
    break;

  case TOKEN_TRUE:
    emit_byte(OP_TRUE);
    break;

  case TOKEN_NIL:
    emit_byte(OP_NIL);
    break;
  default:
    break;
  }
}

static uint8_t argument_list() {
  uint8_t arg_count = 0;
  if (!check(TOKEN_RP)) {
    do {
      expression();
      if (arg_count == 255) {

        error("Can't have more than 255 arguments.");
      }
      arg_count++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RP, "Expect ')' after arguments.");
  return arg_count;
}
static void call() {
  uint8_t arg_count = argument_list();
  emit_bytes(OP_CALL, arg_count);
}

static void binary() {
  enum token_type op_type = parser.previous.type;
  ParseRule *rule = get_rule(op_type);
  parse_precedence((Precedence)(rule->precedence + 1));

  switch (op_type) {
  case TOKEN_PLUS:
    emit_byte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emit_byte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emit_byte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emit_byte(OP_DIVIDE);
    break;
  case TOKEN_EQUALITY:
    emit_byte(OP_EQUAL);
    break;

  default:
    return;
  }
}
ParseRule rules[] = {
    [TOKEN_LP] = {grouping, call, PREC_CALL},
    [TOKEN_RP] = {NULL, NULL, PREC_NONE},
    /* [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    /* [TOKEN_DOT] = {NULL, dot, PREC_CALL}, */
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    /* [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    /* [TOKEN_BANG] = {unary, NULL, PREC_NONE}, */
    /* [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_ASSIGNMENT] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON}, */
    /* [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON}, */
    /* [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON}, */
    /* [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON}, */
    /* [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE}, */
    /* [TOKEN_STRING] = {string, NULL, PREC_NONE}, */
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_INTEGER] = {number, NULL, PREC_NONE},
    /* [TOKEN_AND] = {NULL, and_, PREC_AND}, */
    /* [TOKEN_CLASS] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_ELSE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_FALSE] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_FOR] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_FUN] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_IF] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_NIL] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_OR] = {NULL, or_, PREC_OR}, */
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    /* [TOKEN_RETURN] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_SUPER] = {super_, NULL, PREC_NONE}, */
    /* [TOKEN_THIS] = {this_, NULL, PREC_NONE}, */
    /* [TOKEN_TRUE] = {literal, NULL, PREC_NONE}, */
    /* [TOKEN_VAR] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_WHILE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *get_rule(enum token_type type) { return &(rules[type]); }

static void parse_precedence(Precedence precedence) {
  advance();

  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expected expression");
    return;
  }
  prefix_rule();
  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();

    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    infix_rule();
  }
};

static void expression() { parse_precedence(PREC_ASSIGNMENT); }

bool compile(const char *source, Chunk *chunk) {
  init_scanner(source);
  compiling_chunk = chunk;
  token token;

  advance();
  expression();
  emit_byte(OP_RETURN);
  return !parser.had_error;
}
