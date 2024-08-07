%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parse.h"
#include "serde.h"
#include "common.h"

/* prototypes */
extern void yyerror(const char *s);

extern int yylineno;
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
    char vchar;
};

%token <vint>   INTEGER
%token <vdouble>DOUBLE 
%token <vident> IDENTIFIER
%token <vident> META_IDENTIFIER
%token <vstr>   TOK_STRING
%token <vchar>  TOK_CHAR
%token TRUE FALSE
%token PIPE
%token EXTERN
%token TRIPLE_DOT
%token LET
%token FN
%token MATCH
%token WITH 
%token ARROW
%token DOUBLE_COLON
%token TOK_VOID
%token IN AND

%token FSTRING_START FSTRING_END FSTRING_INTERP_START FSTRING_INTERP_END
%token <vstr> FSTRING_TEXT



%left '.' 
%left PIPE
%left APPLICATION 
%left MODULO
%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'
%left ':'
%left MATCH

%nonassoc UMINUS

%type <ast_node_ptr>
  expr
  application
  lambda_expr
  lambda_arg
  lambda_args
  extern_typed_signature
  extern_variants
  list
  tuple
  expr_list
  match_expr
  match_branches
  list_match_expr
  simple_expr
  let_binding
  fstring
  fstring_parts
  fstring_part
  expr_sequence
  type_decl
  type_expr
  type_atom
%%


program:
    expr_sequence ';' { parse_stmt_list(ast_root, $1); }
  | expr_sequence    { parse_stmt_list(ast_root, $1); }
  | /* NULL */
  ;



expr:
    simple_expr
  | expr '.' IDENTIFIER               { $$ = ast_record_access($1, ast_identifier($3)); }
  | expr simple_expr %prec APPLICATION { $$ = ast_application($1, $2); }
  | expr '+' expr                     { $$ = ast_binop(TOKEN_PLUS, $1, $3); }
  | expr '-' expr                     { $$ = ast_binop(TOKEN_MINUS, $1, $3); }
  | expr '*' expr                     { $$ = ast_binop(TOKEN_STAR, $1, $3); }
  | expr '/' expr                     { $$ = ast_binop(TOKEN_SLASH, $1, $3); }
  | expr MODULO expr                  { $$ = ast_binop(TOKEN_MODULO, $1, $3); }
  | expr '<' expr                     { $$ = ast_binop(TOKEN_LT, $1, $3); }
  | expr '>' expr                     { $$ = ast_binop(TOKEN_GT, $1, $3); }
  | expr GE expr                      { $$ = ast_binop(TOKEN_GTE, $1, $3); }
  | expr LE expr                      { $$ = ast_binop(TOKEN_LTE, $1, $3); }
  | expr NE expr                      { $$ = ast_binop(TOKEN_NOT_EQUAL, $1, $3); }
  | expr EQ expr                      { $$ = ast_binop(TOKEN_EQUALITY, $1, $3); }
  | expr PIPE expr                    { $$ = ast_application($3, $1); }
  | expr ':' expr                     { $$ = ast_assoc($1, $3); }
  | expr DOUBLE_COLON expr            { $$ = ast_list_prepend($1, $3); }
  | let_binding                       { $$ = $1; } | match_expr                        { $$ = $1; }
  | type_decl                         { $$ = $1; }
  ;

simple_expr:
    INTEGER               { $$ = AST_CONST(AST_INT, $1); }
  | DOUBLE                { $$ = AST_CONST(AST_DOUBLE, $1); }
  | TOK_STRING            { $$ = ast_string($1); }
  | TRUE                  { $$ = AST_CONST(AST_BOOL, true); }
  | FALSE                 { $$ = AST_CONST(AST_BOOL, false); }
  | IDENTIFIER            { $$ = ast_identifier($1); }
  | TOK_VOID              { $$ = ast_void(); }
  | list                  { $$ = $1; }
  | tuple                 { $$ = $1; }
  | fstring               { $$ = parse_fstring_expr($1); }
  | TOK_CHAR              { $$ = ast_char($1); }
  | '(' expr_sequence ')' { $$ = $2; }
  ;

expr_sequence:
    expr                        { $$ = $1; }
  | expr_sequence ';' expr      { $$ = parse_stmt_list($1, $3); }
  ;

let_binding:
    LET IDENTIFIER '=' expr         { $$ = ast_let(ast_identifier($2), $4, NULL); }
  | LET lambda_arg '=' expr         { $$ = ast_let($2, $4, NULL); }
  | LET IDENTIFIER '=' extern_typed_signature  
                                    { $$ = ast_let(ast_identifier($2), ast_extern_fn($2, $4), NULL); }


  | LET IDENTIFIER '=' '(' extern_variants ')'  
                                    {
                                      Ast *variants = $5;
                                      variants->tag = AST_EXTERN_VARIANTS;
                                      $$ = ast_let(ast_identifier($2), variants, NULL);
                                    }

  | LET TOK_VOID '=' expr           { $$ = $4; }
  | let_binding IN expr             {
                                      Ast *let = $1;
                                      let->data.AST_LET.in_expr = $3;
                                      $$ = let;
                                    }
  | lambda_expr                     { $$ = $1; }
  ;

extern_typed_signature:
    EXTERN FN expr                  { $$ = extern_typed_signature($3); }
  | extern_typed_signature ARROW expr %prec ':'
                                    { $$ = extern_typed_signature_push($1, $3); }
  ;

extern_variants:
    extern_typed_signature ':' TOK_STRING 
                                    { $$ = ast_list(ast_extern_fn($3, $1)); }

  | extern_variants ',' extern_typed_signature ':' TOK_STRING 
                                    { $$ = ast_list_push($1, ast_extern_fn($5, $3)); }

  | extern_variants ','             { $$ = $1; } /* Allow trailing comma */
  ;


lambda_expr:
    FN lambda_args ARROW expr_sequence ';'      { $$ = ast_lambda($2, $4); }
  | FN TOK_VOID ARROW expr_sequence ';'         { $$ = ast_lambda(NULL, $4); }
  | '(' FN lambda_args ARROW expr_sequence ')'  { $$ = ast_lambda($3, $5); }
  | '(' FN TOK_VOID ARROW expr_sequence ')'     { $$ = ast_lambda(NULL, $5); }
  ;




lambda_args:
    lambda_arg              { $$ = ast_arg_list($1, NULL); }
  | lambda_arg '=' expr     { $$ = ast_arg_list($1, $3); }
  | lambda_args lambda_arg  { $$ = ast_arg_list_push($1, $2, NULL); }
  | lambda_args lambda_arg '=' expr { $$ = ast_arg_list_push($1, $2, $4); }

  | lambda_arg              { $$ = ast_arg_list($1, NULL); }
  | lambda_arg ':' expr     { $$ = ast_arg_list($1, $3); }
  | lambda_args lambda_arg  { $$ = ast_arg_list_push($1, $2, NULL); }
  | lambda_args lambda_arg ':' expr { $$ = ast_arg_list_push($1, $2, $4); }
  ;

lambda_arg:
    IDENTIFIER              { $$ = ast_identifier($1); }
  | '(' expr_list ')'       { $$ = ast_tuple($2); }
  | list_match_expr         { $$ = $1; }
  ;

application:
    IDENTIFIER expr %prec APPLICATION   { $$ = ast_application(ast_identifier($1), $2); }
  /*| expr expr               { $$ = ast_application($1, $2); } */
  | application expr %prec APPLICATION  { $$ = ast_application($1, $2); }
  ;

list:
    '[' ']'                 { $$ = ast_empty_list(); }
  | '[' expr_list ']'       { $$ = $2; }
  | '[' expr_list ',' ']'       { $$ = $2; }
  ;

list_match_expr:
    IDENTIFIER DOUBLE_COLON IDENTIFIER  { $$ = ast_list_prepend(ast_identifier($1), ast_identifier($3)); }
  | IDENTIFIER DOUBLE_COLON expr        { $$ = ast_list_prepend(ast_identifier($1), $3); }
  ;

tuple:
    '(' expr ')'          { $$ = $2; }
  | '(' expr_list ')'     { $$ = ast_tuple($2); }
  | '(' expr_list ',' ')' { $$ = ast_tuple($2); }
  ;

expr_list:
    expr                  { $$ = ast_list($1); }
  | expr_list ',' expr    { $$ = ast_list_push($1, $3); }
  ;

match_expr:
    MATCH expr WITH match_branches { $$ = ast_match($2, $4); }
  ;

match_branches:
    '|' expr ARROW expr                 {$$ = ast_match_branches(NULL, $2, $4);}
  | match_branches '|' expr ARROW expr  {$$ = ast_match_branches($1, $3, $5);}
  | match_branches '|' '_' ARROW expr   {$$ = ast_match_branches($1, Ast_new(AST_PLACEHOLDER_ID), $5);}
  ;

fstring: FSTRING_START fstring_parts FSTRING_END { $$ = $2; }
  ;

fstring_parts:
  /* empty */                   { $$ = ast_empty_list(); }
  | fstring_parts fstring_part  { $$ = ast_list_push($1, $2); }
  ;

fstring_part:
    FSTRING_TEXT                                  { $$ = ast_string($1); }
  | FSTRING_INTERP_START expr FSTRING_INTERP_END  { $$ = $2; }
  ;

type_decl:
  'type' IDENTIFIER '=' type_expr {
                                    Ast *type_decl = ast_let(ast_identifier($2), $4, NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    $$ = type_decl;
                                  }
  ;

type_expr:
    type_atom                 { $$ = $1; }
  | '|' type_atom             { $$ = ast_list($2); }
  | type_expr '|' type_atom   { $$ = ast_list_push($1, $3); } 
  ;

type_atom:
    expr                      { $$ = $1; }
  | IDENTIFIER 'of' type_atom { $$ = ast_binop(TOKEN_OF, ast_identifier($1), $3); } 
  ;


%%


void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at line %d near '%s'\n", s, yylineno, yytext);
}
