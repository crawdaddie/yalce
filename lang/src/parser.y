%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parse.h"
#include "serde.h"

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
    LexId vident;               /* identifier */
    LexString vstr;                 /* string */
    int vint;                   /* int val */
    double vfloat;
};

%token <vint>   INTEGER
%token <vfloat> NUMBER
%token <vident> IDENTIFIER
%token <vstr>   STRING
%token TRUE FALSE
%token WHILE IF PRINT PIPE
%token EXTERN
%token LET
%token FN
%token ARROW
%token VOID
%token DOUBLE_SEMICOLON
%token IN 

%nonassoc IFX
%nonassoc ELSE

%left PIPE
%left MODULO
%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'

%nonassoc UMINUS

%type <ast_node_ptr> stmt expr stmt_list stmt_list_opt application lambda_expr lambda_args


%%

program:
  stmt_list_opt           {
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
  expr                        { $$ = $1; }
  | LET IDENTIFIER '=' expr   { $$ = ast_let($2, $4); }
  | LET VOID '=' expr         { $$ = $4; }
  ;

stmt_list_opt:
    stmt_list                 { $$ = $1; }
  | /* empty */               { $$ = NULL; }
  ;

stmt_list:
    stmt                      { $$ = $1; }
  | stmt_list ';' stmt        { $$ = parse_stmt_list($1, $3); }
  | stmt_list                 { $$ = $1; } // To handle optional ';' at the end
  ;


expr:
    INTEGER               { $$ = AST_CONST(AST_INT, $1); }
  | NUMBER                { $$ = AST_CONST(AST_NUMBER, $1); }
  | STRING                { $$ = ast_string($1); }
  | TRUE                  { $$ = AST_CONST(AST_BOOL, true); }
  | FALSE                 { $$ = AST_CONST(AST_BOOL, false); }
  | IDENTIFIER            { $$ = ast_identifier($1); }
  | VOID                  { $$ = ast_void(); }
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
  | expr PIPE expr        { $$ = ast_application($3, $1); }
  | '(' expr ')'          { $$ = $2; }
  | lambda_expr           { $$ = $1; }
  | application           { $$ = $1; }
  ;

lambda_expr:
    FN lambda_args ARROW stmt_list_opt  { $$ = ast_lambda($2, $4); }
  | FN VOID ARROW stmt_list_opt         { $$ = ast_lambda(NULL, $4); }
  ;



lambda_args:
    IDENTIFIER              { $$ = ast_arg_list($1); }
  | lambda_args IDENTIFIER  { $$ = ast_arg_list_push($1, $2); }
  ;


application:
    IDENTIFIER expr         { $$ = ast_application(ast_identifier($1), $2); }
  | application expr        { $$ = ast_application($1, $2); }
  ;
%%


void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at line %d near '%s'\n", s, yylineno, yytext);
}
