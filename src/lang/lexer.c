#include "lexer.h"
#include "common.h"
#include <ctype.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_token(token token) {
  line_info l = get_line_info();
  switch (token.type) {

  case TOKEN_START: {
    printf("[start]");
    break;
  }

  case TOKEN_LP: {

    printf("[(]");
    break;
  }
  case TOKEN_RP: {

    printf("[)]");
    break;
  }

  case TOKEN_LEFT_BRACE: {

    printf("[{]");
    break;
  }
  case TOKEN_RIGHT_BRACE: {

    printf("[}]");
    break;
  }

  case TOKEN_LEFT_SB: {

    printf("[[]");
    break;
  }

  case TOKEN_RIGHT_SB: {

    printf("[]]");
    break;
  }
  case TOKEN_COMMA: {

    printf("[,]");
    break;
  }
  case TOKEN_DOT: {

    printf("[.]");
    break;
  }
  case TOKEN_MINUS: {

    printf("[-]");
    break;
  }
  case TOKEN_PLUS: {

    printf("[+]");
    break;
  }

  case TOKEN_BANG: {

    printf("[!]");
    break;
  }

  case TOKEN_MODULO: {

    printf("[%%]");
    break;
  }
  case TOKEN_SLASH: {

    printf("[/]");
    break;
  }
  case TOKEN_STAR: {

    printf("[*]");
    break;
  }
  case TOKEN_ASSIGNMENT: {

    printf("[=]");

    break;
  }
  case TOKEN_EQUALITY: {

    printf("[==]");
    break;
  }
  case TOKEN_NL: {

    printf("[\\n]");
    break;
  }
  case TOKEN_PIPE: {

    printf("[->]");
    break;
  }
  case TOKEN_IDENTIFIER: {

    printf("[%s]", token.as.vident);
    break;
  }
  case TOKEN_STRING: {

    printf("[%s]", token.as.vstr);
    break;
  }
  case TOKEN_NUMBER: {

    printf("[%lf]", token.as.vfloat);
    break;
  }
  case TOKEN_INTEGER: {

    printf("[%d]", token.as.vint);
    break;
  }
  case TOKEN_TRUE: {

    printf("[true]");
    break;
  }
  case TOKEN_FALSE: {

    printf("[false]");
    break;
  }
  case TOKEN_FN: {

    printf("[fn]");
    break;
  }
  /* case TOKEN_PRINT: { */
  /*  */
  /*   printf("[print]"); */
  /*   break; */
  /* } */
  case TOKEN_ERROR: {

    printf("[err]");
    break;
  }
  case TOKEN_EOF: {

    printf("[\\0]");
    break;
  }

  case TOKEN_LET: {

    printf("[let]");
    break;
  }
  }
}

typedef struct {
  const char *start;
  const char *current;
  int line;
  int col_offset;
} Scanner;

Scanner scanner;
void init_scanner(char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 0;
}

static token create_literal_token(enum token_type type, literal lex_unit) {
  token token;
  token.type = type;
  token.as = lex_unit;
  return token;
}

static token create_symbol_token(enum token_type type) {
  token token;
  token.type = type;
  return token;
}

token create_identifier(char *str) {
  for (int i = 0; i < TOKEN_NIL - TOKEN_FN + 1; i++) {
    keyword kw = keywords[i];
    if (strcmp(str, kw.match) == 0) {
      literal lit = {.vident = kw.match};
      token token = create_literal_token(kw.kw, lit);
      return token;
    }
  }

  literal lit = {.vident = str};
  token token = create_literal_token(TOKEN_IDENTIFIER, lit);
  return token;
}

int seek_char(const char *input, char c) {
  int seek = 0;
  while (*(input + 1 + seek) != c) {
    seek++;
  }
  return seek + 1;
}

int seek_identifier(const char *input) {
  regex_t regex;
  int s = regcomp(&regex, "^[a-zA-Z_][a-zA-Z_0-9]*", 0);
  if (s) {
    return 0;
  }

  regmatch_t match;
  s = regexec(&regex, input, 1, &match, 0);
  if (s) {
    return 0;
  }

  regfree(&regex);
  return match.rm_eo;
}

int parse_num(const char *input, token *tok) {
  int seek = 0;
  int num_dots = 0;
  if (*input == '-') {
    seek++;
  }
  char c = *(input + seek);
  while (isdigit(c) || c == '.') {
    if (c == '.') {
      num_dots++;
      if (num_dots == 2) {
        return 0;
      }
    }

    seek++;
    c = *(input + seek);
  }
  if (num_dots == 0) {
    // integer
    int val;
    sscanf(input, "%d", &val);
    literal lit = {.vint = val};
    *tok = create_literal_token(TOKEN_INTEGER, lit);
  } else {
    // float
    double val;
    sscanf(input, "%lf", &val);
    literal lit = {.vfloat = val};
    *tok = create_literal_token(TOKEN_NUMBER, lit);
  }
  return seek;
}
int compare_ahead(const char *input, int num, char c) {
  return *(input + num) == c;
}

typedef int (*token_matcher)(const char *input, token *tail);

static int _LP_MATCHER(const char *input, token *tail) {
  if (*input == '(') {
    *tail = create_symbol_token(TOKEN_LP);
    return 1;
  }
  return 0;
}

static int _RP_MATCHER(const char *input, token *tail) {
  if (*input == ')') {
    *tail = create_symbol_token(TOKEN_RP);
    return 1;
  }
  return 0;
}
static int _BRACKET_MATCHER(const char *input, token *tail) {
  switch (*input) {

  case '(':
    *tail = create_symbol_token(TOKEN_LP);
    return 1;

  case ')':
    *tail = create_symbol_token(TOKEN_RP);
    return 1;
  case '{':
    *tail = create_symbol_token(TOKEN_LEFT_BRACE);
    return 1;

  case '}':
    *tail = create_symbol_token(TOKEN_RIGHT_BRACE);
    return 1;

  case '[':
    *tail = create_symbol_token(TOKEN_LEFT_SB);
    return 1;

  case ']':
    *tail = create_symbol_token(TOKEN_RIGHT_SB);
    return 1;
  default:
    return 0;
  }
}

static int _COMMA_MATCHER(const char *input, token *tail) {
  if (*input == ',') {
    *tail = create_symbol_token(TOKEN_COMMA);
    return 1;
  }
  return 0;
}

static int _DOT_MATCHER(const char *input, token *tail) {
  if (*input == '.') {
    *tail = create_symbol_token(TOKEN_DOT);
    return 1;
  }
  return 0;
}

static int _EQL_MATCHER(const char *input, token *tail) {
  if (strncmp(input, "==", 2) == 0) {
    *tail = create_symbol_token(TOKEN_EQUALITY);
    return 2;
  }
  return 0;
}
static int _ASSIGN_MATCHER(const char *input, token *tail) {
  if (*input == '=') {
    *tail = create_symbol_token(TOKEN_ASSIGNMENT);
    return 1;
  }
  return 0;
}

static int _LESS_THAN_MATCHER(const char *input, token *tail) {
  if (*input == '<') {
    if (*(input + 1) == '=') {
      *tail = create_symbol_token(TOKEN_LTE);
      return 2;
    }
    *tail = create_symbol_token(TOKEN_LT);
    return 1;
  }
  return 0;
}

static int _GREATER_THAN_MATCHER(const char *input, token *tail) {
  if (*input == '>') {
    if (*(input + 1) == '=') {
      *tail = create_symbol_token(TOKEN_GTE);
      return 2;
    }
    *tail = create_symbol_token(TOKEN_GT);
    return 1;
  }
  return 0;
}

static int _PIPE_MATCHER(const char *input, token *tail) {
  if (strncmp(input, "->", 2) == 0) {
    *tail = create_symbol_token(TOKEN_PIPE);
    return 2;
  }
  return 0;
}
static int _MINUS_MATCHER(const char *input, token *tail) {
  if (*input == '-') {
    *tail = create_symbol_token(TOKEN_MINUS);
    return 1;
  }
  return 0;
}

static int _MODULO_MATCHER(const char *input, token *tail) {
  if (*input == '%') {
    *tail = create_symbol_token(TOKEN_MODULO);
    return 1;
  }
  return 0;
}

static int _BANG_MATCHER(const char *input, token *tail) {
  if (*input == '!') {
    *tail = create_symbol_token(TOKEN_BANG);
    return 1;
  }
  return 0;
}

static int _PLUS_MATCHER(const char *input, token *tail) {
  if (*input == '+') {
    *tail = create_symbol_token(TOKEN_PLUS);
    return 1;
  }
  return 0;
}

static int _SLASH_MATCHER(const char *input, token *tail) {
  if (*input == '/') {
    *tail = create_symbol_token(TOKEN_SLASH);
    return 1;
  }
  return 0;
}

static int _STAR_MATCHER(const char *input, token *tail) {
  if (*input == '*') {
    *tail = create_symbol_token(TOKEN_STAR);
    return 1;
  }
  return 0;
}
static int _COMMENT_MATCHER(const char *input) {
  if (*input == '#') {
    int len = seek_char(input, '\n');
    return len;
  }
  return 0;
}

static int seek_ws(const char *input) {
  int seek = 0;
  char c = *(input + 1 + seek);
  while (c == ' ' || c == '\r' || c == '\t') {
    seek++;
    c = *(input + 1 + seek);
  }
  return seek + 1;
}
static int _WS_MATCHER(const char *input) {
  char c = *input;
  if (c == ' ' || c == '\r' || c == '\t') {
    int offset = seek_ws(input);
    return offset;
  }
  return 0;
}
static int _NL_MATCHER(const char *input, token *tail) {
  if (*input == '\n') {
    *tail = create_symbol_token(TOKEN_NL);
    scanner.line++;
    scanner.col_offset = 0;
    return 1;
  }
  return 0;
}

static int _STRING_MATCHER(const char *input, token *tail) {

  if (*input == '"') {
    int offset = seek_char(input, '"');
    char *str = malloc((offset) * sizeof(char));
    strlcpy(str, input + 1, offset);
    str[offset - 1] = '\0';
    literal lit = {.vstr = str};
    *tail = create_literal_token(TOKEN_STRING, lit);
    return offset + 1;
  }
  return 0;
}
static int _NUMBER_MATCHER(const char *input, token *tail) {

  token token = {};
  int offset = 0;
  if (isdigit(*input)) {
    if ((offset = parse_num(input, &token)) == 0) {
      return 0;
    }
    *tail = token;
    return offset;
  }
  return 0;
}
static int _MATCH_IDENTIFIER(const char *input, token *tail) {
  int offset = 0;
  if ((offset = seek_identifier(input)) != 0) {
    char *str = malloc((offset + 1) * sizeof(char));
    strlcpy(str, input, offset + 1);
    *tail = create_identifier(str);
    return offset;
  }
  return 0;
}

#define NUM_MATCHERS 18
static token_matcher matchers[NUM_MATCHERS] = {
    _BRACKET_MATCHER,   _COMMA_MATCHER,        _DOT_MATCHER,    _EQL_MATCHER,
    _LESS_THAN_MATCHER, _GREATER_THAN_MATCHER, _ASSIGN_MATCHER, _PIPE_MATCHER,
    _MINUS_MATCHER,     _BANG_MATCHER,         _MODULO_MATCHER, _PLUS_MATCHER,
    _SLASH_MATCHER,     _STAR_MATCHER,         _NL_MATCHER,     _STRING_MATCHER,
    _NUMBER_MATCHER,    _MATCH_IDENTIFIER};

static token error_token(char *msg) {
  return create_literal_token(TOKEN_ERROR, (literal){.vstr = msg});
}

token scan_token() {
  scanner.start = scanner.current;
  if (*scanner.current == '\0') {
    return create_symbol_token(TOKEN_EOF);
  }
  int whitespace = _WS_MATCHER(scanner.current);
  int comment = _COMMENT_MATCHER(scanner.current + whitespace);
  scanner.current += whitespace + comment;
  scanner.col_offset += whitespace + comment;
  token tail = {};
  for (int i = 0; i < NUM_MATCHERS; i++) {
    token_matcher matcher = matchers[i];
    int result = matcher(scanner.current, &tail);

    if (result > 0) {
      scanner.current += result;
      scanner.col_offset += result;
      return tail;
    }
  }
  scanner.current++;
  return error_token("unexpected character");
}
const char *get_scanner_current() { return scanner.current; }

line_info get_line_info() {
  return (line_info){scanner.line,
                     scanner.col_offset - 1 /* 0-index col offset ?? */};
}
