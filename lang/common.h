#ifndef _LANG_COMMON_H
#define _LANG_COMMON_H
#include <stdint.h>

typedef struct {
  const char *chars;
  int length;
  uint64_t hash;
} ObjString;

uint64_t hash_string(const char *key, int length);
uint64_t hash_key(const char *key);

typedef enum token_type {
  TOKEN_START, // dummy token
  TOKEN_LP,    // parens
  TOKEN_RP,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_SQ,
  TOKEN_RIGHT_SQ,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_TRIPLE_DOT,

  // OPERATORS
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_MODULO,
  TOKEN_LT,
  TOKEN_GT,
  TOKEN_LTE,
  TOKEN_GTE,
  TOKEN_EQUALITY,
  TOKEN_NOT_EQUAL,
  // finish operators

  TOKEN_BANG,
  TOKEN_ASSIGNMENT,
  TOKEN_NL,    // statement terminator
  TOKEN_PIPE,  // special pipe operator |>
  TOKEN_ARROW, // special fn arrow operator |>
  TOKEN_IDENTIFIER,
  TOKEN_STRING, // literal
  TOKEN_NUMBER,
  TOKEN_INTEGER,
  TOKEN_FN, // keywords
  TOKEN_RETURN,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_LET,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_NIL, // end keywords
  TOKEN_COMMENT,
  TOKEN_WS,
  TOKEN_ERROR,
  TOKEN_EOF,
  TOKEN_BAR,
  TOKEN_MATCH,
  TOKEN_EXTERN,
  TOKEN_STRUCT,
  TOKEN_TYPE,
  TOKEN_IMPORT,
  TOKEN_AMPERSAND,
  TOKEN_LOGICAL_AND,
  TOKEN_LOGICAL_OR,
  TOKEN_QUESTION,
  TOKEN_COLON,
  TOKEN_DOUBLE_COLON,
  TOKEN_SEMICOLON,
  TOKEN_DOUBLE_SEMICOLON,
  TOKEN_IN,
  TOKEN_OF,
} token_type;
#endif
