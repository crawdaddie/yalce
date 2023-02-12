#include "lang.h"
#define DEBUG 1

execution_ctx *create_execution_ctx() {
  return calloc(sizeof(execution_ctx), 1);
}
void process_token(execution_ctx *execution_ctx, token token) {}

void report_error(int line, int ptr, char *msg, char *input_line) {
  printf("Error: %s\n", msg);
  if (input_line) {
    printf("%d | %s\n", line, input_line);
    printf("%*c^-----\n", ptr + 4, ' ');
  }
}

void print_token(token tok) {
  switch (tok.type) {
  case START:
    printf("[>]");
    break;
  case LP:
    printf("[(]");
    break;
  case RP:
    printf("[)]");
    break;
  case COMMA:
    printf("[,]");
    break;
  case DOT:
    printf("[.]");
    break;
  case MINUS:
    printf("[-]");
    break;
  case PLUS:
    printf("[+]");
    break;
  case SLASH:
    printf("[/]");
    break;
  case STAR:
    printf("[*]");
    break;
  case NL:
    printf("\n");
    break;
  case PIPE:
    printf("[->]");
    break;
  case IDENTIFIER:
    printf("[%s]", tok.literal.vident);
    break;
  case STRING:
    printf("[%s]", tok.literal.vstr);
    break;
  case NUMBER:
    printf("[%lf]", tok.literal.vfloat);
    break;
  case INTEGER:
    printf("[%d]", tok.literal.vint);
    break;
  case FN:
    printf("[%s]", tok.literal.vident);
    break;
  case PRINT:
    printf("[%s]", tok.literal.vident);
    break;
  default:
    printf("[]");
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

void append_token(token token, execution_ctx *execution_ctx) {
  if (DEBUG) {
    print_token(token);
  }
  process_token(execution_ctx, token);
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

int lexer(char *input, execution_ctx *execution_ctx) {
  append_token(create_token(START, NULL), execution_ctx);
  int ptr = 0;
  int line = 0;
  int len = strlen(input);

  char *cur_input = input + ptr;

  while (ptr < len) {

    cur_input = input + ptr;
    char c = *(cur_input);
    switch (c) {
    case '(':
      append_token(create_token(LP, NULL), execution_ctx);
      ptr++;
      break;
    case ')':
      append_token(create_token(RP, NULL), execution_ctx);
      ptr++;

      break;
    case ',':
      append_token(create_token(COMMA, NULL), execution_ctx);
      ptr++;
      break;
    case '.':
      append_token(create_token(DOT, NULL), execution_ctx);
      ptr++;
      break;
    case '-': {
      if (*(cur_input + 1) == '>') {
        append_token(create_token(PIPE, NULL), execution_ctx);
        ptr += 2;
        break;
      }
      if (isdigit(*(cur_input + 1))) {
        int offset = 0;
        token token = {};
        if ((offset = parse_num(cur_input, &token)) == 0) {
          report_error(line, ptr, "error parsing number", input);
        } else {
          append_token(token, execution_ctx);
          ptr += offset;
        }
        break;
      }
      append_token(create_token(MINUS, NULL), execution_ctx);

      ptr++;
      break;
    }
    case '+':
      append_token(create_token(PLUS, NULL), execution_ctx);

      ptr++;
      break;
    case '/':
      append_token(create_token(SLASH, NULL), execution_ctx);

      ptr++;
      break;
    case '*':
      append_token(create_token(STAR, NULL), execution_ctx);

      ptr++;
      break;
    case '#': {
      ptr += seek_char(cur_input, '\n');
      break;
    }
    case '"': {
      int offset = seek_char(cur_input, '"');
      int substr_len = offset;
      char *str = malloc(substr_len * sizeof(char));
      strncpy(str, cur_input + 1, substr_len - 1);
      literal lit = {.vstr = str};
      append_token(create_token(STRING, &lit), execution_ctx);
      ptr += offset + 1;
      break;
    }

    case ' ':
      ptr++;
      break;
    case '\r':
      ptr++;
      break;
    case '\t':
      ptr++;
      break;
    case '\n':
      append_token(create_token(NL, NULL), execution_ctx);
      line++;
      ptr++;
      break;

    case '\0':
      ptr++;
      break;

    default: {
      int offset = 0;
      char *msg;
      if (isdigit(c)) {
        token token = {};
        if ((offset = parse_num(cur_input, &token)) == 0) {
          report_error(line, ptr, "error parsing number", input);
          return 0;
        }
        append_token(token, execution_ctx);
        ptr += offset;
        break;
      }
      if ((offset = seek_identifier(cur_input)) != 0) {
        char *str = malloc((offset + 1) * sizeof(char));
        strncpy(str, cur_input, offset);
        append_token(create_identifier(str), execution_ctx);
        ptr += offset;
        break;
      } else {
        report_error(line, ptr, "failed to parse identifier", input);
        return 0;
      }

      asprintf(&msg, "unexpected character '%c'", c);
      report_error(line, ptr, msg, input);
      return 0;
    }
    }
  }
  return 1;
}
token get_start_token() {
  token head = {};
  head.type = START;
  return head;
}
