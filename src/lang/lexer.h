#ifndef _LANG_LEXER_H
#define _LANG_LEXER_H

enum token_type {
  // dummy token
  TOKEN_START,
  // parens
  TOKEN_LP,
  TOKEN_RP,

  TOKEN_COMMA,

  // OPERATORS
  TOKEN_DOT,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_BANG,
  TOKEN_MODULO,
  /* SEMICOLON, */
  TOKEN_SLASH,
  TOKEN_STAR,
  TOKEN_ASSIGNMENT,
  TOKEN_EQUALITY,

  // statement terminator
  TOKEN_NL,

  // special operators
  TOKEN_PIPE,

  TOKEN_IDENTIFIER,

  // LITERALS
  TOKEN_STRING,
  TOKEN_NUMBER,
  TOKEN_INTEGER,

  // keywords
  TOKEN_FN,
  TOKEN_PRINT,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_LET,
  TOKEN_NIL,

  TOKEN_COMMENT,
  TOKEN_WS,
  TOKEN_ERROR,
  TOKEN_EOF,
};

typedef struct keyword {
  enum token_type kw;
  char *match;
} keyword;

static keyword keywords[TOKEN_NIL - TOKEN_FN + 1] = {
    {TOKEN_FN, "fn"},       {TOKEN_PRINT, "print"}, {TOKEN_TRUE, "true"},
    {TOKEN_FALSE, "false"}, {TOKEN_LET, "let"},     {TOKEN_NIL, "nil"}};

typedef union literal {
  char *vstr;
  int vint;
  double vfloat;
  char *vident;
  void *null;
} literal;

typedef struct token {
  enum token_type type;
  literal as;
} token;

void init_scanner(const char *source);
token scan_token();
void print_token(token token);

typedef struct {
  int line;
  int col_offset;
} line_info;
const char *get_scanner_current();
line_info get_line_info();

#endif
