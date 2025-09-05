%{
#ifndef _LANG_TAB_H
#define _LANG_TAB_H
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parse.h"
#include "serde.h"
#include "common.h"
#include <string.h>

/* prototypes */
extern void yyerror(const char *s);

extern int yylineno;
extern int yycolumn;
extern char *yytext;

/* Define global variable for the root of AST */
Ast* ast_root = NULL;
#define AST_CONST(type, val)                                            \
    ({                                                                  \
      Ast *prefix = Ast_new(type);                                      \
      prefix->data.type.value = val;                                    \
      prefix;                                                           \
    })

%}

%union {
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    float vfloat;
    char vchar;
};

%token <vint>    INTEGER
%token <vdouble> DOUBLE 
%token <vfloat>  FLOAT
%token <vident>  IDENTIFIER
%token <vident>  MACRO_IDENTIFIER
%token <vident>  PATH_IDENTIFIER
%token <vident>  IDENTIFIER_LIST
%token <vstr>    TOK_STRING
%token <vchar>   TOK_CHAR
%token TRUE FALSE
%token PIPE
%token EXTERN
%token TRIPLE_DOT
%token LET
%token MODULE
%token FN
%token MATCH
%token WITH 
%token ARROW
%token DOUBLE_COLON
%token TOK_VOID
%token IN AND
%token ASYNC
%token DOUBLE_AT
%token THUNK
%token IMPORT
%token OPEN 
%token IMPLEMENTS
%token AMPERSAND
%token TYPE
%token TEST_ID
%token MUT
%token THEN ELSE

%token FSTRING_START FSTRING_END FSTRING_INTERP_START FSTRING_INTERP_END
%token <vstr> FSTRING_TEXT

/* Precedence - from lowest to highest */
%left ';'                       /* statement separator */
%right IN                       /* let in expressions */
%left MODULE FN                 /* module and function keywords */
%left PIPE                      /* pipe operator |> */
%left DOUBLE_AT                 /* application @@ */
%left DOUBLE_PIPE               /* logical or || */
%left DOUBLE_AMP                /* logical and && */
%left EQ NE                     /* equality */
%left '<' '>' LE GE             /* comparison */
%left ':' DOUBLE_COLON          /* cons operators */
%left '+' '-'                   /* addition/subtraction */
%left '*' '/' MODULO            /* multiplication/division */
%right UMINUS                   /* unary minus */
%left APPLICATION               /* function application */
%left '.'                       /* record access */
%left '[' ']'                   /* array/list access */
%left '(' ')'                   /* parentheses */

%type <ast_node_ptr>
  program
  stmt_list
  stmt
  expr
  application
  atom
  param_list
  param
  expr_list
  match_clause_list

%%

program:
    stmt_list ';'               { parse_stmt_list(ast_root, $1); }
  | stmt_list                   { parse_stmt_list(ast_root, $1); }
  | /* empty */                 { }
  ;

stmt_list:
    stmt                        { $$ = $1; }
  | stmt_list ';' stmt          { $$ = parse_stmt_list($1, $3); }
  ;

stmt:
    expr                        { $$ = $1; }
  | LET IDENTIFIER '=' expr     { $$ = ast_let(ast_identifier($2), $4, NULL); }
  | LET IDENTIFIER '=' expr IN expr { $$ = ast_let(ast_identifier($2), $4, $6); }
  | LET '(' expr_list ')' '=' expr { $$ = ast_let(ast_tuple($3), $6, NULL); }
  | LET '(' expr_list ')' '=' expr IN expr { $$ = ast_let(ast_tuple($3), $6, $8); }
  | LET '(' IDENTIFIER ')' '=' expr {
                                      Ast *id = ast_identifier($3);
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      $$ = ast_let(id, $6, NULL);
                                    }
  | LET IDENTIFIER '=' FN param_list ARROW stmt_list 
                                { $$ = ast_let(ast_identifier($2), ast_lambda($5, $7), NULL); }
  | LET IDENTIFIER '=' MODULE param_list ARROW stmt_list
                                { printf("module with params\n"); $$ = ast_let(ast_identifier($2), ast_module(ast_lambda($5, $7)), NULL); }
  | LET IDENTIFIER '=' MODULE stmt_list ';'
                                { $$ = ast_let(ast_identifier($2), ast_module(ast_lambda(NULL, $5)), NULL); }
  | IMPORT IDENTIFIER           { $$ = ast_import_stmt($2, false); }
  | OPEN IDENTIFIER             { $$ = ast_import_stmt($2, true); }
  ;

expr:
    application                 { $$ = $1; }
  | expr '+' expr               { $$ = ast_binop(TOKEN_PLUS, $1, $3); }
  | expr '-' expr               { $$ = ast_binop(TOKEN_MINUS, $1, $3); }
  | expr '*' expr               { $$ = ast_binop(TOKEN_STAR, $1, $3); }
  | expr '/' expr               { $$ = ast_binop(TOKEN_SLASH, $1, $3); }
  | expr MODULO expr            { $$ = ast_binop(TOKEN_MODULO, $1, $3); }
  | expr '<' expr               { $$ = ast_binop(TOKEN_LT, $1, $3); }
  | expr '>' expr               { $$ = ast_binop(TOKEN_GT, $1, $3); }
  | expr DOUBLE_AMP expr        { $$ = ast_binop(TOKEN_DOUBLE_AMP, $1, $3); }
  | expr DOUBLE_PIPE expr       { $$ = ast_binop(TOKEN_DOUBLE_PIPE, $1, $3); }
  | expr GE expr                { $$ = ast_binop(TOKEN_GTE, $1, $3); }
  | expr LE expr                { $$ = ast_binop(TOKEN_LTE, $1, $3); }
  | expr NE expr                { $$ = ast_binop(TOKEN_NOT_EQUAL, $1, $3); }
  | expr EQ expr                { $$ = ast_binop(TOKEN_EQUALITY, $1, $3); }
  | expr PIPE expr              { $$ = ast_application($3, $1); }
  | expr ':' expr               { $$ = ast_assoc($1, $3); }
  | expr DOUBLE_COLON expr      { $$ = ast_list_prepend($1, $3); }
  | '-' expr %prec UMINUS       { $$ = ast_binop(TOKEN_MINUS, AST_CONST(AST_INT, 0), $2); }
  | MATCH expr WITH match_clause_list { $$ = ast_match($2, $4); }
  | 'if' expr 'then' expr 'else' expr { $$ = ast_if_else($2, $4, $6); }
  | 'if' expr 'then' expr             { $$ = ast_if_else($2, $4, NULL); }
  | FN param_list ARROW stmt_list ';' { $$ = ast_lambda($2, $4); }
  | FN TOK_VOID ARROW stmt_list ';'   { $$ = ast_void_lambda($4); }
  | '(' FN param_list ARROW stmt_list ')' { $$ = ast_lambda($3, $5); }
  | '(' FN TOK_VOID ARROW stmt_list ')'   { $$ = ast_void_lambda($5); }
  ;

application:
    atom                        { $$ = $1; }
  | application atom            { $$ = ast_application($1, $2); }
  | application DOUBLE_AT atom  { $$ = ast_application($1, $3); }
  ;

atom:
    INTEGER                     { $$ = AST_CONST(AST_INT, $1); }
  | DOUBLE                      { $$ = AST_CONST(AST_DOUBLE, $1); }
  | FLOAT                       { $$ = AST_CONST(AST_FLOAT, $1); }
  | TOK_STRING                  { $$ = ast_string($1); }
  | TOK_CHAR                    { $$ = ast_char($1); }
  | TRUE                        { $$ = AST_CONST(AST_BOOL, true); }
  | FALSE                       { $$ = AST_CONST(AST_BOOL, false); }
  | TOK_VOID                    { $$ = ast_void(); }
  | IDENTIFIER                  { $$ = ast_identifier($1); }
  | '(' expr ')'                { $$ = $2; }
  | '(' expr_list ')'           { $$ = ast_tuple($2); }
  | '[' ']'                     { $$ = ast_empty_list(); }
  | '[' expr_list ']'           { $$ = $2; }
  | '[' '|' expr_list '|' ']'   { $$ = ast_list_to_array($3); }
  | '(' '+' ')'                 { $$ = ast_identifier((ObjString){"+", 1}); }
  | '(' '-' ')'                 { $$ = ast_identifier((ObjString){"-", 1}); }
  | '(' '*' ')'                 { $$ = ast_identifier((ObjString){"*", 1}); }
  | '(' '/' ')'                 { $$ = ast_identifier((ObjString){"/", 1}); }
  | atom '.' IDENTIFIER         { $$ = ast_record_access($1, ast_identifier($3)); }
  | atom '[' expr ']'           { $$ = ast_application(ast_application(ast_identifier((ObjString){.chars = "array_at", 8}), $1), $3); }
  ;

param_list:
    param                       { printf("single param\n"); $$ = ast_arg_list($1, NULL); }
  | param param                 { printf("double param\n"); $$ = ast_arg_list_push(ast_arg_list($1, NULL), $2, NULL); }
  | param param param           { printf("triple param\n"); $$ = ast_arg_list_push(ast_arg_list_push(ast_arg_list($1, NULL), $2, NULL), $3, NULL); }
  ;

param:
    IDENTIFIER                  { $$ = ast_identifier($1); }
  | TOK_VOID                    { $$ = ast_void(); }
  | '(' expr_list ')'           { $$ = ast_tuple($2); }
  ;

expr_list:
    expr                        { $$ = ast_list($1); }
  | expr_list ',' expr          { $$ = ast_list_push($1, $3); }
  ;

match_clause_list:
    '|' expr ARROW expr                           { $$ = ast_match_branches(NULL, $2, $4); }
  | match_clause_list '|' expr ARROW expr         { $$ = ast_match_branches($1, $3, $5); }
  ;


%%

void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, _cur_script);
}
#endif _LANG_TAB_H
