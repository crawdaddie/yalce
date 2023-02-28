#include "compiler.h"
#include "dbg.h"
#include "lexer.h"
#include "util.h"
typedef struct {
  token current;
  token previous;
  bool had_error;
  bool panic_mode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool can_assign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  token name;
  int depth;
} Local;

typedef enum {
  TYPE_FUNCTION,
  TYPE_SCRIPT,
} FunctionType;

typedef struct {
  /* ObjFunction *function; */
  /* FunctionType type; */
  Local locals[UINT8_COUNT];
  int local_count;
  int scope_depth;
} Compiler;

Parser parser;
Compiler *current = NULL;
Chunk *compiling_chunk;

static void init_compiler(Compiler *compiler, FunctionType type) {
  /* compiler->type = type; */
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  /* compiler->function = make_function(); */
  current = compiler;
  /* Local *local = &current->locals[current->local_count++]; */
  /* local->depth = 0; */
  /* local->name = (token){TOKEN_STRING, {.vstr = ""}}; */
}

static Chunk *current_chunk() {
  /* return &current->function->chunk; */
  return compiling_chunk;
}
static void error_at(token *token, const char *message) {
  if (parser.panic_mode) {
    /* return; */
  }
  parser.panic_mode = true;
  // TODO: add line info
  fprintf(stderr, "Error");
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {

    line_info linfo = get_line_info();
    fprintf(stderr, " at %d:%d", linfo.line, linfo.col_offset);
    // nothing
  } else {
    // nothing
  }
  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}
static void error(const char *message) { error_at(&parser.previous, message); }
static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}
static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scan_token();

    if (parser.current.type != TOKEN_ERROR)
      break;
    error_at_current(get_scanner_current());
  }
}

static void consume(enum token_type type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}
static bool check(enum token_type type) { return parser.current.type == type; }
static bool match(enum token_type type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}
static void emit_byte(uint8_t byte) { write_chunk(current_chunk(), byte); }
static void emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}
static uint8_t make_constant(Value value) {
  int constant = add_constant(current_chunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk");
    return 0;
  }
  return (uint8_t)constant;
}

static void expression();
static void statement();
static void declaration();
static void program();
static void variable(bool can_assign);
static ParseRule *get_rule(enum token_type type);
static void parse_precedence(Precedence precedence);

static void number(bool can_assign) {
  Value value = parser.previous.type == TOKEN_INTEGER
                    ? INTEGER_VAL(parser.previous.as.vint)
                    : NUMBER_VAL(parser.previous.as.vfloat);
  emit_bytes(OP_CONSTANT, make_constant(value));
}

static void integer(bool can_assign) {
  Value value = INTEGER_VAL(parser.previous.as.vint);
  emit_bytes(OP_CONSTANT, make_constant(value));
}
static void grouping(bool can_assign) {
  expression();
  consume(TOKEN_RP, "Expect ')' after expression");
}
static void unary(bool can_assign) {
  enum token_type op_type = parser.previous.type;
  parse_precedence(PREC_UNARY);
  switch (op_type) {
  case TOKEN_BANG:
    emit_byte(OP_NOT);
    break;

  case TOKEN_MINUS:
    emit_byte(OP_NEGATE);
    break;
  default:
    return;
  }
}
static void parse_literal(bool can_assign) {
  switch (parser.previous.type) {
  case TOKEN_FALSE:
    emit_byte(OP_FALSE);
    break;

  case TOKEN_TRUE:
    emit_byte(OP_TRUE);
    break;

  case TOKEN_NIL:
    emit_byte(OP_NIL);
    break;
  default:
    break;
  }
}

static uint8_t argument_list() {
  uint8_t arg_count = 0;
  if (!check(TOKEN_RP)) {
    do {
      expression();
      if (arg_count == 255) {

        error("Can't have more than 255 arguments.");
      }
      arg_count++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RP, "Expect ')' after arguments.");
  return arg_count;
}
static void call(bool can_assign) {
  uint8_t arg_count = argument_list();
  emit_bytes(OP_CALL, arg_count);
}

static void parse_binary(bool can_assign) {
  enum token_type op_type = parser.previous.type;
  ParseRule *rule = get_rule(op_type);
  parse_precedence((Precedence)(rule->precedence + 1));
  switch (op_type) {
  case TOKEN_PLUS:
    emit_byte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emit_byte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emit_byte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emit_byte(OP_DIVIDE);
    break;
  case TOKEN_EQUALITY:
    emit_byte(OP_EQUAL);
    break;
  case TOKEN_MODULO:
    emit_byte(OP_MODULO);
    break;
  case TOKEN_LT:
    emit_byte(OP_LT);
    break;
  case TOKEN_GT:
    emit_byte(OP_GT);
    break;
  case TOKEN_LTE:
    emit_byte(OP_LTE);
    break;
  case TOKEN_GTE:
    emit_byte(OP_GTE);
    break;
  default:
    return;
  }
}
static void string(bool can_assign) {
  Object *str = (Object *)make_string(parser.previous.as.vstr);
  Value val = {VAL_OBJ, {.object = str}};
  emit_bytes(OP_CONSTANT, make_constant((val)));
}

ParseRule rules[] = {
    [TOKEN_LP] = {grouping, call, PREC_CALL},
    [TOKEN_RP] = {NULL, NULL, PREC_NONE},
    /* [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    /* [TOKEN_DOT] = {NULL, dot, PREC_CALL}, */
    [TOKEN_MINUS] = {unary, parse_binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, parse_binary, PREC_TERM},
    [TOKEN_MODULO] = {NULL, parse_binary, PREC_TERM},
    [TOKEN_NL] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, parse_binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, parse_binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    /* [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_ASSIGNMENT] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON}, */
    /* [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON}, */
    [TOKEN_LT] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_GT] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_LTE] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_GTE] = {NULL, parse_binary, PREC_COMPARISON},
    /* [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON}, */
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_INTEGER] = {number, NULL, PREC_NONE},
    /* [TOKEN_AND] = {NULL, and_, PREC_AND}, */
    /* [TOKEN_CLASS] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_ELSE] = {NULL, NULL, PREC_NONE}, */

    [TOKEN_TRUE] = {parse_literal, NULL, PREC_NONE},
    [TOKEN_FALSE] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_FOR] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_FUN] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_IF] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_NIL] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_OR] = {NULL, or_, PREC_OR}, */
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    /* [TOKEN_RETURN] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_SUPER] = {super_, NULL, PREC_NONE}, */
    /* [TOKEN_THIS] = {this_, NULL, PREC_NONE}, */
    /* [TOKEN_TRUE] = {literal, NULL, PREC_NONE}, */
    /* [TOKEN_VAR] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_WHILE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *get_rule(enum token_type type) { return &(rules[type]); }

static void parse_precedence(Precedence precedence) {
  while (check(TOKEN_NL)) {
    advance();
  }
  advance();

  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expected expression ");
    printf("  ");
    print_token(parser.previous);
    return;
  }
  bool can_assign =
      precedence <=
      PREC_ASSIGNMENT; // guard against expressions like a * b = c + d
  prefix_rule(can_assign);
  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();

    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }
  if (can_assign && match(TOKEN_ASSIGNMENT)) {
    error("Invalid assignment target");
  }
};

static void expression() { parse_precedence(PREC_ASSIGNMENT); }

static void print_statement() {
  expression();
  consume(TOKEN_NL, "Expect \\n after statement");
  emit_byte(OP_PRINT);
}
static void synchronize() {
  parser.panic_mode = false;
  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_NL)
      return;
    switch (parser.current.type) {
    /* case TOKEN_CLASS: */
    /* case TOKEN_FUN: */
    /* case TOKEN_VAR: */
    /* case TOKEN_FOR: */
    /* case TOKEN_IF: */
    /* case TOKEN_WHILE: */
    case TOKEN_PRINT:
      /* case TOKEN_RETURN: */
      return;

    default:; // Do nothing.
    }

    advance();
  }
}
static void begin_scope() { current->scope_depth++; }

static void end_scope() {

  current->scope_depth--;
  while (current->local_count > 0 &&
         current->locals[current->local_count - 1].depth >
             current->scope_depth) {
    emit_byte(OP_POP);
    current->local_count--;
  }
}
static void block() {

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    if (check(TOKEN_NL)) {
      advance();
      continue;
    }
    program();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block");
}
static uint8_t identifier_constant(token name) {
  Object *str = (Object *)make_string(name.as.vstr);

  Value val = {VAL_OBJ, {.object = str}};
  return make_constant(val);
}
static bool identifiers_equal(token *a, token *b) {
  int a_len = strlen(a->as.vstr);
  if (a_len != strlen(b->as.vstr))
    return false;

  return memcmp(a->as.vstr, b->as.vstr, a_len) == 0;
}
static void add_local(token name) {
  if (current->local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }
  Local *local = &current->locals[current->local_count++];
  local->name = name;
  local->depth = -1;
}
static void declare_variable() {
  if (current->scope_depth == 0)
    return;

  token *name = &parser.previous;
  for (int i = current->local_count - 1; i >= 0; i--) {
    Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scope_depth) {
      break;
    }

    if (identifiers_equal(name, &local->name)) {
      error("There already exists a variable with this name in this scope.");
    }
  }
  add_local(*name);
}
static uint8_t parse_var(const char *error_msg) {

  consume(TOKEN_IDENTIFIER, error_msg);
  declare_variable();
  if (current->scope_depth > 0) {
    return 0;
  }
  return identifier_constant(parser.previous);
}
static void mark_initialized() {
  current->locals[current->local_count - 1].depth = current->scope_depth;
}
static void define_var(uint8_t global) {
  if (current->scope_depth > 0) {
    mark_initialized();
    return;
  }
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void var_declaration() {
  uint8_t global = parse_var("Expect variable name");

  if (match(TOKEN_ASSIGNMENT)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }
  consume(TOKEN_NL, "Expect \\n after variable declaration");
  define_var(global);
}
static int resolve_local(Compiler *compiler, token *name) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (identifiers_equal(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}
static void named_variable(token name, bool can_assign) {
  uint8_t get_op, set_op;
  int arg = resolve_local(current, &name);
  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;

  } else {
    arg = identifier_constant(name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  if (can_assign && match(TOKEN_ASSIGNMENT)) {
    expression();
    emit_bytes(set_op, (uint8_t)arg);
  } else {
    emit_bytes(get_op, (uint8_t)arg);
  }
}
static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}
static int emit_jump(uint8_t instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return current_chunk()->count - 2;
}
static void patch_jump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = current_chunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  current_chunk()->code[offset] = (jump >> 8) & 0xff;
  current_chunk()->code[offset + 1] = jump & 0xff;
}

static void if_statement() {
  consume(TOKEN_LP, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RP, "Expect ')' after condition.");
  int then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);

  statement();
  int else_jump = emit_jump(OP_JUMP);

  patch_jump(then_jump);
  emit_byte(OP_POP);
  if (match(TOKEN_ELSE)) {
    statement();
  }
  patch_jump(else_jump);
}
static void emit_loop(int loopStart) {
  emit_byte(OP_LOOP);

  int offset = current_chunk()->count - loopStart + 2;
  if (offset > UINT16_MAX)
    error("Loop body too large.");

  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}
static void while_statement() {
  int loop_start = current_chunk()->count;
  consume(TOKEN_LP, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RP, "Expect ')' after condition.");

  int exitJump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();
  emit_loop(loop_start);

  patch_jump(exitJump);
  emit_byte(OP_POP);
}
static void statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_IF)) {
    if_statement();
  } else if (match(TOKEN_WHILE)) {
    while_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else if (match(TOKEN_LET)) {
    var_declaration();
  } else {
    expression();
  }
}

static void program() {
  if (check(TOKEN_NL)) {
    advance();
    return;
  }
  statement();
}

void end_compiler() {
  emit_byte(OP_RETURN);
  /*   ObjFunction *function = current->function; */
  /* #ifdef DEBUG_PRINT_CODE */
  /*   if (!parser.hadError) { */
  /*     disassemble_chunk(current_chunk(), function->name != NULL */
  /*                                            ? function->name->chars */
  /*                                            : "<script>"); */
  /*   } */
  /* #endif */
  /*   return function; */
}

bool compile(const char *source, Chunk *chunk) {

  init_scanner(source);
  Compiler compiler;
  init_compiler(&compiler, TYPE_SCRIPT);

  parser.had_error = false;
  parser.panic_mode = false;
  compiling_chunk = chunk;
  token token;

  advance();
  /* expression(); */
  while (!match(TOKEN_EOF)) {
    program();
  }
  end_compiler();
  /* ObjFunction *function = end_compiler(); */
  /* return parser.had_error ? NULL : function; */
  return !parser.had_error;
}
