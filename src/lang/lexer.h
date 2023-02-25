#ifndef _LANG_LEXER_H
#define _LANG_LEXER_H

enum token_type {
  TOKEN_START, // dummy token
  TOKEN_LP,    // parens
  TOKEN_RP,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT, // OPERATORS
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_BANG,
  TOKEN_MODULO,
  TOKEN_SLASH,
  TOKEN_STAR,
  TOKEN_ASSIGNMENT,
  TOKEN_EQUALITY,
  TOKEN_NL,   // statement terminator
  TOKEN_PIPE, // special operator
  TOKEN_IDENTIFIER,
  TOKEN_STRING, // literal
  TOKEN_NUMBER,
  TOKEN_INTEGER,
  TOKEN_FN, // keywords
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
