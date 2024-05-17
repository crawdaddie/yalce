#include "lex.h"
#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define create_literal_token(_token_type, _literal)                            \
  (token){.type = _token_type, .as = lit};

#define create_symbol_token(_token_type)                                       \
  (token) { .type = _token_type }

void print_token(token token) {
  // line_info l = get_line_info();
  switch (token.type) {

  case TOKEN_START: {
    printf("start");
    break;
  }

  case TOKEN_BAR: {
    printf("|");
    break;
  }

  case TOKEN_LP: {

    printf("(");
    break;
  }
  case TOKEN_RP: {

    printf(")");
    break;
  }

  case TOKEN_LEFT_BRACE: {

    printf("{");
    break;
  }
  case TOKEN_RIGHT_BRACE: {

    printf("}");
    break;
  }

  case TOKEN_LEFT_SQ: {

    printf("[");
    break;
  }
  case TOKEN_RIGHT_SQ: {

    printf("]");
    break;
  }
  case TOKEN_COMMA: {

    printf(",");
    break;
  }
  case TOKEN_DOT: {

    printf(".");
    break;
  }
  case TOKEN_MINUS: {

    printf("-");
    break;
  }
  case TOKEN_PLUS: {

    printf("+");
    break;
  }

  case TOKEN_BANG: {

    printf("!");
    break;
  }

  case TOKEN_MODULO: {

    printf("%%");
    break;
  }
  case TOKEN_SLASH: {

    printf("/");
    break;
  }
  case TOKEN_STAR: {

    printf("*");
    break;
  }
  case TOKEN_ASSIGNMENT: {

    printf("=");

    break;
  }
  case TOKEN_EQUALITY: {

    printf("==");
    break;
  }
  case TOKEN_NL: {

    printf("\\n");
    break;
  }
  case TOKEN_PIPE: {

    printf("|>");
    break;
  }

  case TOKEN_ARROW: {

    printf("->");
    break;
  }
  case TOKEN_IDENTIFIER: {

    printf("%s", token.as.vident);
    break;
  }
  case TOKEN_STRING: {

    printf("%s", token.as.vstr);
    break;
  }
  case TOKEN_NUMBER: {

    printf("%lf", token.as.vfloat);
    break;
  }
  case TOKEN_INTEGER: {

    printf("%d", token.as.vint);
    break;
  }
  case TOKEN_TRUE: {

    printf("true");
    break;
  }
  case TOKEN_FALSE: {

    printf("false");
    break;
  }
  case TOKEN_FN: {

    printf("fn");
    break;
  }
  /* case TOKEN_PRINT: { */
  /*  */
  /*   printf("print"); */
  /*   break; */
  /* } */
  case TOKEN_ERROR: {

    printf("err");
    break;
  }
  case TOKEN_EOF: {

    printf("\\0");
    break;
  }

  case TOKEN_DOUBLE_SEMICOLON: {

    printf(";;");
    break;
  }
  case TOKEN_LET: {

    printf("let");
    break;
  }

  case TOKEN_LT: {

    printf("<");
    break;
  }
  case TOKEN_GT: {

    printf(">");
    break;
  }
  case TOKEN_LTE: {

    printf("<=");
    break;
  }
  case TOKEN_GTE: {

    printf(">=");
    break;
  }
  case TOKEN_AMPERSAND: {

    printf("&");
    break;
  }

  case TOKEN_QUESTION: {

    printf("?");
    break;
  }

  case TOKEN_COLON: {

    printf(":");
    break;
  }
  }
}

Lexer lexer;

void _init_lexer(const char *source) {
  lexer.start = source;
  lexer.current = source;
  lexer.line = 0;
  lexer.col_offset = 0;
}

void init_lexer(const char *source, Lexer *lexer) {
  lexer->start = source;
  lexer->current = source;
  lexer->line = 0;
  lexer->col_offset = 0;
}

token create_identifier(char *str) {
  for (int i = 0; i < NUM_KEYWORDS; i++) {
    keyword kw = keywords[i];
    if (strcmp(str, kw.match) == 0) {
      literal lit = {.vident = kw.match};
      token token = {.type = kw.kw, .as = lit};
      return token;
    }
  }

  literal lit = {.vident = str};
  token token = {.type = TOKEN_IDENTIFIER, .as = lit};
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
    fprintf(stderr, "failure compiling regex %d\n", s);
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
    *tail = (token){TOKEN_NL};
    lexer.line++;
    lexer.col_offset = 0;
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

int parse_num(const char *input, token *tok, int mul) {
  int seek = 0;
  int num_dots = 0;
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
    literal lit = {.vint = val * mul};
    *tok = (token){.type = TOKEN_INTEGER, .as = lit};
  } else {
    // float
    double val;
    sscanf(input, "%lf", &val);
    literal lit = {.vfloat = val * mul};
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
    *tail = create_symbol_token(TOKEN_LEFT_SQ);
    return 1;
  case ']':
    *tail = create_symbol_token(TOKEN_RIGHT_SQ);
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
  if (strncmp(input, "...", 3) == 0) {
    *tail = create_symbol_token(TOKEN_TRIPLE_DOT);
    return 3;
  }
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

static int _ARROW_MATCHER(const char *input, token *tail) {
  if (strncmp(input, "->", 2) == 0) {
    *tail = create_symbol_token(TOKEN_ARROW);
    return 2;
  }
  return 0;
}

static int _MINUS_MATCHER(const char *input, token *tail) {
  if (*input == '-') {
    if (isdigit(*(input + 1))) {
      token tok;
      input++;
      int offset = 1 + parse_num(input, tail, -1);
      return offset;
    }
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
    if (*(input + 1) == '=') {
      *tail = create_symbol_token(TOKEN_NOT_EQUAL);
      return 2;
    }
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

static int _QUESTION_MATCHER(const char *input, token *tail) {
  if (*input == '?') {
    *tail = create_symbol_token(TOKEN_QUESTION);
    return 1;
  }
  return 0;
}

static int _COLON_MATCHER(const char *input, token *tail) {
  if (*input == ':') {
    *tail = create_symbol_token(TOKEN_COLON);
    return 1;
  }
  return 0;
}

static int _SEMICOLON_MATCHER(const char *input, token *tail) {
  if (*input == ';') {

    if (*(input + 1) == ';') {

      *tail = create_symbol_token(TOKEN_DOUBLE_SEMICOLON);
      return 2;
    }
    *tail = create_symbol_token(TOKEN_SEMICOLON);
    return 1;
  }
  return 0;
}

/*
static int _BAR_MATCHER(const char *input, token *tail) {
  if (*input == '|') {
    *tail = create_symbol_token(TOKEN_BAR);
    return 1;
  }
  return 0;
}
*/

static int _BAR_MATCHER(const char *input, token *tail) {
  if (*input == '|') {
    if (*(input + 1) == '>') {
      *tail = create_symbol_token(TOKEN_PIPE);
      return 2;
    }
    *tail = create_symbol_token(TOKEN_BAR);
    return 1;
  }
  return 0;
}
//
// static int _BAR_MATCHER(const char *input, token *tail) {
//   if (strncmp(input, "||", 2) == 0) {
//     *tail = create_symbol_token(TOKEN_LOGICAL_OR);
//     return 2;
//   }
//   if (*input == '|') {
//     *tail = create_symbol_token(TOKEN_BAR);
//     return 1;
//   }
//   return 0;
// }

static int _AND_MATCHER(const char *input, token *tail) {
  if (strncmp(input, "&&", 2) == 0) {
    *tail = create_symbol_token(TOKEN_LOGICAL_AND);
    return 2;
  }
  if (*input == '&') {
    *tail = create_symbol_token(TOKEN_AMPERSAND);
    return 1;
  }
  return 0;
}

static int _STRING_MATCHER(const char *input, token *tail) {

  if (*input == '"') {
    int offset = seek_char(input, '"');
    char *str = malloc((offset) * sizeof(char));
    int x = 0;
    while (x < offset) {
      // strlcpy(str, input + 1, offset);
      char c = *(input + 1 + x);
      if (c == '\\' && *(input + 1 + x + 1) == 'n') {
        str[x] = '\n';
        x++;
      } else {
        str[x] = c;
      }
      x++;
    }
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
    if ((offset = parse_num(input, &token, 1)) == 0) {
      return 0;
    }
    *tail = token;
    return offset;
  }
  return 0;
}

/*
static size_t _strlcpy(char *dst, const char *src, size_t size) {
  size_t src_len = strlen(src);
  size_t copy_len =
      (size > 0) ? size - 1 : 0; // Ensure space for null-terminator

  if (src_len < copy_len) {
    // If the source fits completely in the destination buffer,
    // we can just use memcpy and manually null-terminate.
    memcpy(dst, src, src_len);
    dst[src_len] = '\0'; // Null-terminate the destination buffer
  } else if (copy_len > 0) {
    // If there's some space in the destination buffer, copy as much as
    // possible.
    memcpy(dst, src, copy_len);
    dst[copy_len] = '\0'; // Null-terminate the destination buffer
  }

  return src_len; // Return the length of the source string (not including
                  // null-terminator).
}
*/

static int _IDENTIFIER_MATCHER(const char *input, token *tail) {
  int offset = 0;
  if ((offset = seek_identifier(input)) != 0) {

    char *str = malloc((offset + 1) * sizeof(char));
    strlcpy(str, input, offset + 1);
    *tail = create_identifier(str);
    return offset;
  }
  return 0;
}

#define NUM_MATCHERS 23
static token_matcher matchers[NUM_MATCHERS] = {
    _COLON_MATCHER,   _SEMICOLON_MATCHER, _QUESTION_MATCHER,
    _BRACKET_MATCHER, _COMMA_MATCHER,     _DOT_MATCHER,
    _EQL_MATCHER,     _LESS_THAN_MATCHER, _GREATER_THAN_MATCHER,
    _ASSIGN_MATCHER,  _ARROW_MATCHER,     _MINUS_MATCHER,
    _BANG_MATCHER,    _MODULO_MATCHER,    _PLUS_MATCHER,
    _SLASH_MATCHER,   _STAR_MATCHER,      _NL_MATCHER,
    _STRING_MATCHER,  _NUMBER_MATCHER,    _IDENTIFIER_MATCHER,
    _BAR_MATCHER,     _AND_MATCHER,
};

#define error_token(msg)                                                       \
  {                                                                            \
    .type = TOKEN_ERROR, .as = {.vstr = msg }                                  \
  }

token scan_token(Lexer *lexer) {
  lexer->start = lexer->current;
  if (*lexer->current == '\0') {
    return (token){TOKEN_EOF};
  }
  int whitespace = _WS_MATCHER(lexer->current);
  int comment = _COMMENT_MATCHER(lexer->current + whitespace);
  lexer->current += whitespace + comment;
  lexer->col_offset += whitespace + comment;

  token tail = {};

  for (int i = 0; i < NUM_MATCHERS; i++) {
    token_matcher matcher = matchers[i];
    int result = matcher(lexer->current, &tail);

    if (result > 0) {
      lexer->current += result;
      lexer->col_offset += result;
      return tail;
    }
  }
  lexer->current++;
  return (token){.type = TOKEN_ERROR, .as = {.vstr = "unexpected character"}};

  return (token){TOKEN_EOF};
}
