%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parse.h"

/* prototypes */
extern void yyerror(const char *s);
/* Define global variable for the root of AST */

extern int yylineno;
extern char *yytext;


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
    char *vident;               /* identifier */
    char *vstr;                 /* string */
    int vint;                   /* int val */
    double vfloat;
};

%token <vint>   INTEGER
%token <vfloat> NUMBER
%token <vident> IDENTIFIER
%token <vstr>   STRING
%token TRUE FALSE
%token WHILE IF PRINT PIPE
%token LET
%token FN
%token ARROW

%nonassoc IFX
%nonassoc ELSE

%left MODULO
%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'
%left PIPE

%nonassoc UMINUS

%type <ast_node_ptr> stmt expr stmt_list application pipe_expr lambda_expr lambda_args

%%

program:
  function                {return 0;}
  ;

function:
  function stmt           {
                            if (ast_root == NULL) {
                              ast_root = Ast_new(AST_BODY);
                              ast_root->data.AST_BODY.len = 0;
                              ast_root->data.AST_BODY.stmts = malloc(sizeof(Ast *));
                            }
                            Ast_body_push(ast_root, $2);
                          }
  | /* NULL */
  ;

stmt:
    ';'                            { $$ = opr(';', 2, NULL, NULL); }
  | expr ';'                       { $$ = $1; }
  | LET IDENTIFIER '=' expr ';'    { $$ = ast_let($2, $4); }
  | WHILE '(' expr ')' stmt        { $$ = opr(WHILE, 2, $3, $5); }
  | IF '(' expr ')' stmt %prec IFX { $$ = opr(IF, 2, $3, $5); }
  | IF '(' expr ')' stmt ELSE stmt { $$ = opr(IF, 3, $3, $5, $7); }
  | '{' stmt_list '}'              { $$ = $2; }
  ;

stmt_list:
    stmt                  { $$ = $1; }
  | stmt_list stmt        { $$ = opr(';', 2, $1, $2); }
  ;
expr:
    INTEGER               { $$ = AST_CONST(AST_INT, $1); }
  | NUMBER                { $$ = AST_CONST(AST_NUMBER, $1); }
  | STRING                { $$ = AST_CONST(AST_STRING, $1); }
  | TRUE                  { $$ = AST_CONST(AST_BOOL, true); }
  | FALSE                 { $$ = AST_CONST(AST_BOOL, false); }
  | IDENTIFIER            { $$ = ast_identifier($1); }
  | '-' expr %prec UMINUS { $$ = ast_unop(TOKEN_MINUS, $2); }
  | expr '+' expr         { $$ = ast_binop(TOKEN_PLUS, $1, $3); }
  | expr '-' expr         { $$ = ast_binop(TOKEN_MINUS, $1, $3); }
  | expr '*' expr         { $$ = ast_binop(TOKEN_STAR, $1, $3); }
  | expr '/' expr         { $$ = ast_binop(TOKEN_SLASH, $1, $3); }
  | expr MODULO expr      { $$ = ast_binop(TOKEN_MODULO, $1, $3); }
  | expr '<' expr         { $$ = ast_binop(TOKEN_LT, $1, $3); }
  | expr '>' expr         { $$ = ast_binop(TOKEN_GT, $1, $3); }
  | expr GE expr          { $$ = ast_binop(TOKEN_GTE, $1, $3); }
  | expr LE expr          { $$ = ast_binop(TOKEN_LTE, $1, $3); }
  | expr NE expr          { $$ = ast_binop(TOKEN_NOT_EQUAL, $1, $3); }
  | expr EQ expr          { $$ = ast_binop(TOKEN_EQUALITY, $1, $3); }
  | '(' expr ')'          { $$ = $2; }
  | lambda_expr           { $$ = $1; }
  | pipe_expr             { $$ = $1; }
  ;

lambda_expr:
  FN lambda_args %prec ARROW  {
                                $$ = ast_lambda($2, NULL);
                              }
  ;

lambda_args:
    IDENTIFIER              { $$ = ast_arg_list($1); }
  | lambda_args IDENTIFIER  { $$ = ast_arg_list_push($1, $2); }
  ;

pipe_expr:
    expr PIPE pipe_expr   { $$ = ast_application($3, $1); }
  | expr PIPE application { $$ = ast_application($3, $1); }
  | expr PIPE expr        { $$ = ast_application($3, $1); }
  | application
  ;

application:
    IDENTIFIER expr       { $$ = ast_application(ast_identifier($1), $2); }
  | application expr      { $$ = ast_application($1, $2); }
  ;
%%


void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at line %d near '%s'\n", s, yylineno, yytext);
}
