%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parse.h"
#include "serde.h"
#include "common.h"

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
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vfloat;
};

%token <vint>   INTEGER
%token <vfloat> NUMBER
%token <vident> IDENTIFIER
%token <vident> META_IDENTIFIER
%token <vstr>   TOK_STRING
%token <vstr>   FSTRING
%token TRUE FALSE
%token WHILE IF PRINT PIPE
%token EXTERN
%token TRIPLE_DOT
%token LET
%token FN
%token MATCH
%token WITH 
%token ARROW
%token TOK_VOID
%token IN AND

%nonassoc IFX
%nonassoc ELSE

%left PIPE
%left MODULO
%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'

%nonassoc UMINUS

%type <ast_node_ptr>
  stmt expr stmt_list application
  lambda_expr lambda_args list tuple expr_list
  match_expr match_branches



%%

program:
  stmt_list           {
                            if (ast_root == NULL) {
                              ast_root = Ast_new(AST_BODY);
                              ast_root->data.AST_BODY.len = 0;
                              ast_root->data.AST_BODY.stmts = malloc(sizeof(Ast *));
                            }
                            ast_body_push(ast_root, $1);
                          }

  | /* NULL */
  ;

stmt:
  expr                            { $$ = $1; }
  | LET TOK_VOID '=' expr         { $$ = $4; }
  | META_IDENTIFIER LET IDENTIFIER '=' lambda_expr  
                                  { $$ = ast_meta($1, ast_let($3, $5, NULL)); }


  | LET IDENTIFIER '=' lambda_expr  { $$ = ast_let($2, $4, NULL); }
  | LET IDENTIFIER '=' expr         { $$ = ast_let($2, $4, NULL); }
  ;


stmt_list:
    stmt                        { $$ = $1; }
  | stmt_list ';' stmt          { $$ = parse_stmt_list($1, $3); }
  | '(' stmt_list ')'           { $$ = $2; }
  ;


expr:
    INTEGER               { $$ = AST_CONST(AST_INT, $1); }
  | NUMBER                { $$ = AST_CONST(AST_NUMBER, $1); }
  | TOK_STRING            { $$ = ast_string($1); }
  | TRUE                  { $$ = AST_CONST(AST_BOOL, true); }
  | FALSE                 { $$ = AST_CONST(AST_BOOL, false); }
  /*| META_IDENTIFIER expr  { $$ = ast_meta($1, $2); } */
  | IDENTIFIER            { $$ = ast_identifier($1); }
  | TOK_VOID              { $$ = ast_void(); }
  /*| '-' expr %prec UMINUS { $$ = ast_unop(TOKEN_MINUS, $2); } */
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
  | expr PIPE expr        { $$ = ast_application($3, $1); }
  /*| '(' expr ')'          { $$ = $2; } */
  | lambda_expr           { $$ = $1; }
  | application           { $$ = $1; }
  | FSTRING               { $$ = parse_format_expr($1); }
  | list                  { $$ = $1; }
  | tuple                 { $$ = $1; }
  | match_expr            { $$ = $1; }
  ;

lambda_expr:
    FN lambda_args ARROW stmt_list ';'      { $$ = ast_lambda($2, $4); }
  | FN TOK_VOID ARROW stmt_list ';'         { $$ = ast_lambda(NULL, $4); }
  | '(' FN lambda_args ARROW stmt_list ')'  { $$ = ast_lambda($3, $5); }
  | '(' FN TOK_VOID ARROW stmt_list ')'     { $$ = ast_lambda(NULL, $5); }
  ;



lambda_args:
    IDENTIFIER              { $$ = ast_arg_list($1, NULL); }
  | IDENTIFIER '=' expr     { $$ = ast_arg_list($1, $3); }
  | lambda_args IDENTIFIER  { $$ = ast_arg_list_push($1, $2, NULL); }
  | lambda_args IDENTIFIER '=' expr { $$ = ast_arg_list_push($1, $2, $4); }
  ;


application:
    IDENTIFIER expr         { $$ = ast_application(ast_identifier($1), $2); }
  /*| expr expr               { $$ = ast_application($1, $2); } */
  | application expr        { $$ = ast_application($1, $2); }
  ;

list:
    '[' ']'                 { $$ = ast_empty_list(); }
  | '[' expr_list ']'       { $$ = $2; }
  ;

tuple:
    '(' expr ')'          { $$ = $2; }
  | '(' expr_list ')'     { $$ = ast_tuple($2); }
  ;

expr_list:
    expr                  { $$ = ast_list($1); }
  | expr_list ',' expr    { $$ = ast_list_push($1, $3); }

  ;

match_expr:
    MATCH expr WITH match_branches { $$ = ast_match($2, $4); }
  ;

match_branches:
    '|' expr ARROW stmt_list                {$$ = ast_match_branches(NULL, $2, $4);}
  | match_branches '|' expr ARROW stmt_list {$$ = ast_match_branches($1, $3, $5);}
  | match_branches '|' '_' ARROW stmt_list  {$$ = ast_match_branches($1, Ast_new(AST_PLACEHOLDER_ID), $5);}
  ;
%%


void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at line %d near '%s'\n", s, yylineno, yytext);
}
