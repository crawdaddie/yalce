#ifndef _LANG_LEXER_H
#define _LANG_LEXER_H

typedef enum {
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
  TOKEN_LT,
  TOKEN_GT,
  TOKEN_LTE,
  TOKEN_GTE,

  TOKEN_NL,   // statement terminator
  TOKEN_PIPE, // special operator
  //
  TOKEN_IDENTIFIER,

  TOKEN_STRING, // literal
  TOKEN_NUMBER,
  TOKEN_INTEGER,

  TOKEN_FN, // keywords
  TOKEN_RETURN,
  /* TOKEN_PRINT, */
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_LET,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_IMPORT,
  TOKEN_FROM,
  TOKEN_PUB,
  TOKEN_NIL, // end keywords

  TOKEN_COMMENT,

  TOKEN_WS,
  TOKEN_ERROR,
  TOKEN_EOF,
} token_type;

typedef struct keyword {
  token_type kw;
  const char *match;
} keyword;

static keyword keywords[TOKEN_NIL - TOKEN_FN + 1] = {
    {TOKEN_FN, "fn"},
    {TOKEN_RETURN, "return"},
    /* {TOKEN_PRINT, "print"}, */
    {TOKEN_TRUE, "true"},
    {TOKEN_FALSE, "false"},
    {TOKEN_LET, "let"},
    {TOKEN_IF, "if"},
    {TOKEN_ELSE, "else"},
    {TOKEN_WHILE, "while"},
    {TOKEN_IMPORT, "import"},
    {TOKEN_FROM, "from"},
    {TOKEN_PUB, "pub"},
    {TOKEN_NIL, "nil"}};

typedef union literal {
  const char *vstr;
  int vint;
  double vfloat;
  const char *vident;
  void *null;
} literal;

typedef struct token {
  token_type type;
  literal as;
} token;

void init_scanner(const char *source);
token scan_token();
void print_token(token token);

void print_token_type(token_type token_type);

typedef struct {
  int line;
  int col_offset;
} line_info;
const char *get_scanner_current();
line_info get_line_info();

#endif
