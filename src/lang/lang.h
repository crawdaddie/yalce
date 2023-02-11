#ifndef _LANG_H
#define _LANG_H
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

  // arithmetic
  DOT,
  MINUS,
  PLUS,
  /* SEMICOLON, */
  SLASH,
  STAR,

  // statement terminator
  NL,

  // special operators
  PIPE,

  IDENTIFIER,
  STRING,
  NUMBER,
  INTEGER,

  // keywords
  FN,
  PRINT
};
#define NUM_KEYWORDS 2
typedef struct keyword {
  enum token_type kw;
  char *match;
} keyword;

static keyword keywords[NUM_KEYWORDS] = {{FN, "fn"}, {PRINT, "print"}};
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

typedef struct ctx {

} ctx;

ctx *create_ctx();

void process_token(ctx *ctx, token token);

int parse_string(char *input);
void report_error(int line, int ptr, char *msg, char *input_line);

token get_start_token();
token create_token(enum token_type type, literal *lex_unit);

void print_tokens(token *head);
void debug_tokens(token *head);
void free_tokens(token *head);

int seek_char(char *input, char c);
int parse_num(char *input, token *tok);

int lexer(char *input, ctx *ctx); // returns tail

void append_token(token token, ctx *ctx);
void print_token(token tok);
#endif
