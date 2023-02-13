#include "lexer.h"

void report_error(int line, int ptr, char *msg, char *input_line) {
  printf("Error: %s\n", msg);
  if (input_line) {

    printf("%d | %s", line, input_line);
    printf("%*c^-----\n", ptr + 4, ' ');
  }
}

token create_token(enum token_type type, literal *lex) {

  token tail = {type};
  if (lex) {
    tail.literal = *lex;
  }
  return tail;
}

token create_identifier(char *str) {
  for (int i = 0; i < NUM_KEYWORDS; i++) {
    keyword kw = keywords[i];
    if (strcmp(str, kw.match) == 0) {
      literal lit = {.vident = kw.match};
      token token = create_token(kw.kw, &lit);
      return token;
    }
  }

  literal lit = {.vident = str};
  token token = create_token(IDENTIFIER, &lit);
  return token;
}

int seek_char(char *input, char c) {
  int seek = 0;
  while (*(input + 1 + seek) != c) {
    seek++;
  }
  return seek + 1;
}

int seek_identifier(char *input) {
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

int parse_num(char *input, token *tok) {
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
    *tok = create_token(INTEGER, &lit);
  } else {
    // float
    double val;
    sscanf(input, "%lf", &val);
    literal lit = {.vfloat = val};
    *tok = create_token(NUMBER, &lit);
  }
  return seek;
}
int compare_ahead(char *input, int num, char c) { return *(input + num) == c; }

typedef int (*token_matcher)(char *input, token *tail);

static int _LP_MATCHER(char *input, token *tail) {
  if (*input == '(') {
    *tail = create_token(LP, NULL);
    return 1;
  }
  return 0;
}

static int _RP_MATCHER(char *input, token *tail) {
  if (*input == ')') {
    *tail = create_token(RP, NULL);
    return 1;
  }
  return 0;
}

static int _COMMA_MATCHER(char *input, token *tail) {
  if (*input == ',') {
    *tail = create_token(COMMA, NULL);
    return 1;
  }
  return 0;
}

static int _DOT_MATCHER(char *input, token *tail) {
  if (*input == '.') {
    *tail = create_token(DOT, NULL);
    return 1;
  }
  return 0;
}

static int _EQL_MATCHER(char *input, token *tail) {
  if (strncmp(input, "==", 2) == 0) {
    *tail = create_token(EQUALITY, NULL);
    return 2;
  }
  return 0;
}
static int _ASSIGN_MATCHER(char *input, token *tail) {
  if (*input == '=') {
    *tail = create_token(ASSIGNMENT, NULL);
    return 1;
  }
  return 0;
}

static int _PIPE_MATCHER(char *input, token *tail) {
  if (strncmp(input, "->", 2) == 0) {
    *tail = create_token(PIPE, NULL);
    return 2;
  }
  return 0;
}
static int _MINUS_MATCHER(char *input, token *tail) {
  if (*input == '-') {
    *tail = create_token(MINUS, NULL);
    return 1;
  }
  return 0;
}

static int _PLUS_MATCHER(char *input, token *tail) {
  if (*input == '+') {
    *tail = create_token(PLUS, NULL);
    return 1;
  }
  return 0;
}

static int _SLASH_MATCHER(char *input, token *tail) {
  if (*input == '/') {
    *tail = create_token(SLASH, NULL);
    return 1;
  }
  return 0;
}

static int _STAR_MATCHER(char *input, token *tail) {
  if (*input == '*') {
    *tail = create_token(STAR, NULL);
    return 1;
  }
  return 0;
}
static int _COMMENT_MATCHER(char *input, token *tail) {
  if (*input == '#') {
    int len = strlen(input);
    *tail = (token){-1};
    return len - 1;
  }
  return 0;
}
static int _WS_MATCHER(char *input, token *tail) {
  char c = *input;
  if (c == ' ' || c == '\r' || c == '\t') {
    *tail = (token){-1}; // skip token
    return 1;
  }
  return 0;
}
static int _NL_MATCHER(char *input, token *tail) {
  if (*input == '\n') {
    *tail = create_token(NL, NULL);
    return 1;
  }
  return 0;
}

static int _STRING_MATCHER(char *input, token *tail) {

  if (*input == '"') {
    int offset = seek_char(input, '"');
    char *str = malloc(offset * sizeof(char));
    strncpy(str, input + 1, offset - 1);
    literal lit = {.vstr = str};
    *tail = create_token(STRING, &lit);
    return offset + 1;
  }
  return 0;
}
static int _NUMBER_MATCHER(char *input, token *tail) {

  token token = {};
  int offset = 0;
  if (isdigit(*input) || *input == '-') {
    if ((offset = parse_num(input, &token)) == 0) {
      return 0;
    }
    *tail = token;
    return offset;
  }
  return 0;
}
static int _MATCH_IDENTIFIER(char *input, token *tail) {
  int offset = 0;
  if ((offset = seek_identifier(input)) != 0) {
    char *str = malloc((offset + 1) * sizeof(char));
    strncpy(str, input, offset);
    *tail = create_identifier(str);
    return offset;
  }
  return 0;
}

static token_matcher matchers[17] = {
    &_LP_MATCHER,      &_RP_MATCHER,     &_COMMA_MATCHER,  &_DOT_MATCHER,
    &_EQL_MATCHER,     &_ASSIGN_MATCHER, &_PIPE_MATCHER,   &_MINUS_MATCHER,
    &_PLUS_MATCHER,    &_SLASH_MATCHER,  &_STAR_MATCHER,   &_COMMENT_MATCHER,
    &_WS_MATCHER,      &_NL_MATCHER,     &_STRING_MATCHER, &_NUMBER_MATCHER,
    &_MATCH_IDENTIFIER};

int lexer(char *input, int line, void (*process_token)(token token)) {
  int ptr = 0;
  int len = strlen(input);
  token tail = create_token(START, NULL);
  while (ptr < len) {
    for (int i = 0; i < 17; i++) {
      token_matcher matcher = matchers[i];
      int result = matcher(input + ptr, &tail);
      if (result > 0) {
        ptr += result;
        break;
      }
      if (result < 0) {
        // lexer error!
        return 0;
      }
    }
    if (tail.type != -1) {
      process_token(tail);
    };
  }
  return 1;
}

token get_start_token() {
  token head = {};
  head.type = START;
  return head;
}
