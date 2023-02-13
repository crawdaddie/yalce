#ifndef _LEXER_H
#define _LEXER_H
#include <ctype.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum token_type {
  // dummy token
  START,
  // parens
  LP,
  RP,

  COMMA,

  // OPERATORS
  DOT,
  MINUS,
  PLUS,
  /* SEMICOLON, */
  SLASH,
  STAR,
  ASSIGNMENT,
  EQUALITY,

  // statement terminator
  NL,

  // special operators
  PIPE,

  IDENTIFIER,

  // LITERALS
  STRING,
  NUMBER,
  INTEGER,
  TRUE,
  FALSE,

  // keywords
  FN,
  PRINT,
  _END_TOKEN,
};
typedef struct keyword {
  enum token_type kw;
  char *match;
} keyword;

#define NUM_KEYWORDS 4
static keyword keywords[NUM_KEYWORDS] = {
    {FN, "fn"}, {PRINT, "print"}, {TRUE, "true"}, {FALSE, "false"}};

typedef union literal {
  char *vstr;
  int vint;
  double vfloat;
  char *vident;
  void *null;
} literal;

typedef struct token {
  enum token_type type;
  literal literal;
} token;

int parse_string(char *input);
void report_error(int line, int ptr, char *msg, char *input_line);

token get_start_token();
token create_token(enum token_type type, literal *lex_unit);

int seek_char(char *input, char c);
int parse_num(char *input, token *tok);
int compare_ahead(char *input, int num, char c);

int lexer(char *input, int line,
          void (*process_token)(token token)); // returns tail

#endif
