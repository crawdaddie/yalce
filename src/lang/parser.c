#include "parser.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
  token current;
  token previous;
} Parser;

static Parser parser;

static void advance() {
  parser.previous = parser.current;
  parser.current = scan_token();
}

static void consume(token_type type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  printf("Parse Error: %s", message);

  /* error_at_current(message); */
}

static bool consume_optional(token_type type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return true;
  }
  /* printf("Parse Error: %s", message); */
  return false;

  /* error_at_current(message); */
}

static bool check(token_type type) { return parser.current.type == type; }
static bool match(token_type type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

static NExpression *expression();
static NExpressionStatement *begin_scope() { return NULL; }
static NExpressionStatement *block_scope_body() { return NULL; }
static NExpressionStatement *end_scope() { return NULL; }

static void program(NBlock *block);

typedef NExpression *(*PrefixFn)(bool can_assign);
typedef NExpression *(*InfixFn)(bool can_assign, NExpression *prev_expr);

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_PIPE,       // ->
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef struct {
  PrefixFn prefix;
  InfixFn infix;
  Precedence precedence;
} ParseRule;

/* PrefixFn grouping; */
/* InfixFn call; */
static NExpression *call(bool can_assign, NIdentifier *fn_ident) {
  NMethodCall *method_call = new NMethodCall(*fn_ident);
  // printf("call func %s\n", fn_ident->name.c_str());
  do {
    NExpression *expr = expression();
    if (expr) {
      method_call->arguments.push_back(expr);
    }

  } while (match(TOKEN_COMMA));

  consume(TOKEN_RP, "Expect ')' after method call");
  return method_call;
}
/* PrefixFn unary; */
/* ParseFn parse_binary; */
/* PrefixFn string; */
/* ParseFn number; */
/* PrefixFn parse_literal; */

static NIdentifier *id(const char *name) { return new NIdentifier(name); }

static NIdentifier *parse_ident(const char *msg) {
  consume(TOKEN_IDENTIFIER, msg);
  return id(parser.previous.as.vstr);
}
static NBlock *parse_block() {
  NBlock *block = new NBlock();

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    if (check(TOKEN_NL)) {
      advance();
      continue;
    }
    program(block);
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block");
  return block;
}

static VariableList *parse_param_list() {
  VariableList *ParamList = new VariableList();
  do {
    NIdentifier *param_type = parse_ident("Expect type for parameter");
    NIdentifier *param_name = parse_ident("Expect parameter");
    NVariableDeclaration *param =
        new NVariableDeclaration(*param_type, *param_name);
    ParamList->push_back(param);
  } while (match(TOKEN_COMMA));
  return ParamList;
}

/*
NExpression *parse_anonymous_function(bool can_assign) {

  consume(TOKEN_LP, "Expect '(' after function declaration");
  VariableList *ParamList = new VariableList();
  if (!check(TOKEN_RP)) {
    do {
      NIdentifier *param_type =
          parse_ident("Expect type for function parameter");
      NIdentifier *param_name = parse_ident("Expect function parameter");
      NVariableDeclaration *param =
          new NVariableDeclaration(*param_type, *param_name);
      ParamList->push_back(param);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RP, "Expect ')' after function parameters");
  NIdentifier *return_type = parse_ident("expect return type");
  consume(TOKEN_LEFT_BRACE, "Expect '{' after function prototype");
  NBlock *body = parse_block();

  return new NAnonFunctionDeclaration(*return_type, *ParamList, *body);
}
*/

NExpression *number(bool can_assign) {
  if (parser.previous.type == TOKEN_INTEGER) {
    return new NInteger(parser.previous.as.vint);
  }
  return new NDouble(parser.previous.as.vfloat);
}

NExpression *parse_binary(bool can_assign, NExpression *prev_expr) {
  token_type op = parser.previous.type;
  NExpression *next_expr = expression();
  return new NBinaryOperator(op, *prev_expr, *next_expr);
}

NExpression *unary(bool can_assign) {
  token_type op = parser.previous.type;
  NExpression *next_expr = expression();
  return new NUnaryOperator(op, *next_expr);
}

NExpression *grouping(bool can_assign) {
  NExpression *next_expr = expression();
  consume(TOKEN_RP, "Expect ')' after expression");
  return next_expr;
}
NExpression *variable(bool can_assign) {
  token var_token = parser.previous;
  NIdentifier *id = new NIdentifier(var_token.as.vstr);
  if (match(TOKEN_ASSIGNMENT)) {
    NExpression *assignment_expr = expression();
    if (!assignment_expr) {
      return id;
    }
    consume(TOKEN_NL, "expect \\n after assignment");
    return new NAssignment(*id, *assignment_expr);
  }
  return id;
}

NExpression *string(bool can_assign) {
  return new NString(parser.previous.as.vstr);
}
NExpression *parse_literal(bool can_assign) {
  switch (parser.previous.type) {
  case TOKEN_TRUE: {
    return new NBool(true);
  }
  case TOKEN_FALSE: {
    return new NBool(false);
  }
  case TOKEN_NIL: {
    return new NNil();
  }
  default:
    return NULL;
  }
}
NExpression *parse_assignment(bool can_assign, NIdentifier *prev_expr) {
  NExpression *next_expr = expression();
  /* return new NAssignment(prev_expr, next_expr); */
  return NULL;
}

ParseRule rules[] = {
    [TOKEN_LP] = {grouping, (InfixFn)call, PREC_CALL},
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
    [TOKEN_PIPE] = {NULL, parse_binary, PREC_PIPE},
    /* [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_ASSIGNMENT] = {NULL, (InfixFn)parse_assignment, PREC_NONE}, */
    /* [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON}, */
    /* [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON}, */
    [TOKEN_LT] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_GT] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_LTE] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_GTE] = {NULL, parse_binary, PREC_COMPARISON},
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
    /* [TOKEN_FN] = {parse_anonymous_function, NULL, PREC_NONE}, */
    /* [TOKEN_IF] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_NIL] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_OR] = {NULL, or_, PREC_OR}, */
    /* [TOKEN_PRINT] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_RETURN] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_SUPER] = {super_, NULL, PREC_NONE}, */
    /* [TOKEN_THIS] = {this_, NULL, PREC_NONE}, */
    /* [TOKEN_TRUE] = {literal, NULL, PREC_NONE}, */
    /* [TOKEN_VAR] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_WHILE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *get_rule(token_type type) { return &(rules[type]); }
static NExpression *parse_precedence(Precedence precedence) {

  PrefixFn prefix_rule = get_rule(parser.current.type)->prefix;
  if (prefix_rule == NULL) {
    print_token(parser.current);
    return NULL;
  }
  advance();
  NExpression *infix_expr = prefix_rule(true);
  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();

    InfixFn infix_rule = get_rule(parser.previous.type)->infix;

    infix_expr = infix_rule(true, infix_expr);
  }
  return infix_expr;
}
static NStatement *named_function_declaration() {

  NIdentifier *name = parse_ident("expect function name");

  consume(TOKEN_LP, "Expect '(' after function declaration");
  VariableList *ParamList = new VariableList();
  if (!check(TOKEN_RP)) {

    do {
      NIdentifier *param_type =
          parse_ident("Expect type for function parameter");
      NIdentifier *param_name = parse_ident("Expect function parameter");
      NVariableDeclaration *param =
          new NVariableDeclaration(*param_type, *param_name);
      ParamList->push_back(param);
    } while (match(TOKEN_COMMA));
  }
  printf("params %zu\n", ParamList->size());
  consume(TOKEN_RP, "Expect ')' after function parameters");
  NIdentifier *return_type = parse_ident("expect return type");
  consume(TOKEN_LEFT_BRACE, "Expect '{' after function prototype");
  NBlock *body = parse_block();

  return new NFunctionDeclaration(*return_type, *name, *ParamList, *body);
}

static NStatement *var_declaration() {

  NIdentifier *type = parse_ident("Expect type identifier");
  NIdentifier *id = parse_ident("Expect identifier");

  if (match(TOKEN_ASSIGNMENT)) {
    NExpression *assignment_expr = expression();
    NVariableDeclaration *decl =
        new NVariableDeclaration(*type, *id, assignment_expr);

    consume(TOKEN_NL, "Expect '\\n' after variable declaration");
    return decl;
  }

  NVariableDeclaration *decl = new NVariableDeclaration(*type, *id);
  consume(TOKEN_NL, "Expect '\\n' after variable declaration");
  return decl;
}

static NStatement *return_statement() {
  NExpression *return_expr = expression();
  return new NReturnStatement(*return_expr);
}
static NStatement *if_statement() { return NULL; }

static NStatement *while_statement() { return NULL; }

static NExpression *expression() { return parse_precedence(PREC_ASSIGNMENT); }
static NStatement *import_module() {
  std::set<NIdentifier *> members;
  if (check(TOKEN_IDENTIFIER)) {

    do {
      NIdentifier *member = parse_ident("Expect imported member identifier");
      members.insert(member);
    } while (match(TOKEN_COMMA));

    consume(TOKEN_FROM, "Expect 'from' after member import list");
  }
  consume(TOKEN_STRING, "Expect module name after import");

  std::string name = parser.previous.as.vstr;
  consume(TOKEN_NL, "Expect \\n after import");

  return new NImport(name, members);
}

static NStatement *publish_statement() {
  NStatementList *stmt_list = new NStatementList();

  if (match(TOKEN_FN)) {
    NFunctionDeclaration *decl =
        (NFunctionDeclaration *)named_function_declaration();
    stmt_list->statements.push_back(decl);
    stmt_list->statements.push_back(new NPublish(decl->id));
  } else if (match(TOKEN_LET)) {
    NVariableDeclaration *decl = (NVariableDeclaration *)var_declaration();
    stmt_list->statements.push_back(decl);
    stmt_list->statements.push_back(new NPublish(decl->id));
  } else if (match(TOKEN_IDENTIFIER)) {
    token id_token = parser.previous;
    if (match(TOKEN_ASSIGNMENT)) {
      NAssignment *expr = (NAssignment *)variable(true);
      stmt_list->statements.push_back((NStatement *)expr);
      stmt_list->statements.push_back(new NPublish(expr->id));
    } else {
      NIdentifier *id = new NIdentifier(id_token.as.vstr);
      stmt_list->statements.push_back(new NPublish(*id));
      consume(TOKEN_NL, "Expect \\n after publish statement");
    }
  }

  return (NStatement *)stmt_list;
}

static NStatement *parse_statement() {
  if (match(TOKEN_IF)) {
    return if_statement();
  } else if (match(TOKEN_WHILE)) {
    return while_statement();
  } else if (match(TOKEN_RETURN)) {
    return return_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    NBlock *ret = parse_block();
    end_scope();
    return new NExpressionStatement(*ret);
  } else if (match(TOKEN_LET)) {
    return var_declaration();
  } else if (match(TOKEN_FN)) {
    return named_function_declaration();
  } else if (match(TOKEN_IMPORT)) {
    return import_module();
  } else if (match(TOKEN_PUB)) {
    return publish_statement();
  } else {
    return (NStatement *)expression();
  }
  return NULL;
}

static void program(NBlock *block) {
  NStatement *statement = parse_statement();
  if (statement) {
    block->statements.push_back(statement);
  }
}

NBlock *parse(const char *input) {
  init_scanner(input);
  advance();
  NBlock *program_block = new NBlock();
  while (!match(TOKEN_EOF)) {
    if (check(TOKEN_NL)) {
      advance();
    }
    program(program_block);
    advance();
  }
  return program_block;
}
