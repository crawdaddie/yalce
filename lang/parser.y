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
%token <vident>  PATH_IDENTIFIER
%token <vident>  IDENTIFIER_LIST
%token <vstr>    TOK_STRING
%token <vchar>   TOK_CHAR
%token TRUE FALSE
%token PIPE
%token EXTERN
%token DOUBLE_DOT
%token LET
%token FN
%token MODULE
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



%left '|'
%left PIPE
%left DOUBLE_AT
%left DOUBLE_AMP DOUBLE_PIPE
%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'
%left MODULO
%left ':'
%right DOUBLE_COLON
%left DOUBLE_DOT
%left MATCH
%right APPLICATION
%right '.' 

%nonassoc UMINUS

%type <ast_node_ptr>
  expr
  atom_expr
  lambda_expr
  lambda_arg
  lambda_args
  list
  array
  tuple
  expr_list
  match_expr
  match_branches
  simple_expr
  let_binding
  fstring
  fstring_parts
  fstring_part
  expr_sequence
  type_decl
  type_expr
  type_atom
  fn_signature
  tuple_type
  type_args
  match_test_clause
%%


program:
    expr_sequence ';' { pctx.ast_root = parse_stmt_list(pctx.ast_root, $1); }
  | expr_sequence     { pctx.ast_root = parse_stmt_list(pctx.ast_root, $1); }
  | /* NULL */
  ;


expr:
    atom_expr
  | 'yield' expr                      { $$ = ast_yield($2); }
  | 'await' expr                      { $$ = ast_await($2); }
  | expr DOUBLE_AT expr               { $$ = ast_application($1, $3); }
  | expr atom_expr %prec APPLICATION  { $$ = ast_application($1, $2); }
  | expr '+' expr                     { $$ = ast_binop(TOKEN_PLUS, $1, $3); }
  | expr '-' expr                     { $$ = ast_binop(TOKEN_MINUS, $1, $3); }
  | expr '*' expr                     { $$ = ast_binop(TOKEN_STAR, $1, $3); }
  | expr '/' expr                     { $$ = ast_binop(TOKEN_SLASH, $1, $3); }
  | expr MODULO expr                  { $$ = ast_binop(TOKEN_MODULO, $1, $3); }
  | expr '<' expr                     { $$ = ast_binop(TOKEN_LT, $1, $3); }
  | expr '>' expr                     { $$ = ast_binop(TOKEN_GT, $1, $3); }
  | expr DOUBLE_AMP expr              { $$ = ast_binop(TOKEN_DOUBLE_AMP, $1, $3); }
  | expr DOUBLE_PIPE expr             { $$ = ast_binop(TOKEN_DOUBLE_PIPE, $1, $3); }
  | expr GE expr                      { $$ = ast_binop(TOKEN_GTE, $1, $3); }
  | expr LE expr                      { $$ = ast_binop(TOKEN_LTE, $1, $3); }
  | expr NE expr                      { $$ = ast_binop(TOKEN_NOT_EQUAL, $1, $3); }
  | expr EQ expr                      { $$ = ast_binop(TOKEN_EQUALITY, $1, $3); }
  | expr PIPE expr                    { $$ = ast_application($3, $1); }
  | expr ':' expr                     { $$ = ast_assoc($1, $3); }
  | expr DOUBLE_DOT expr              { $$ = ast_range_expression($1, $3); }
  | expr DOUBLE_COLON expr            { $$ = ast_list_prepend($1, $3); }
  | let_binding                       { $$ = $1; }
  | match_expr                        { $$ = $1; }
  | type_decl                         { $$ = $1; }
  | THUNK expr                        { $$ = ast_thunk_expr($2); }
  // | TRIPLE_DOT expr                   { $$ = ast_spread_operator($2); }
  | IDENTIFIER_LIST                   { $$ = ast_typed_empty_list($1); }
  | 'for' IDENTIFIER '=' expr IN expr   {
                                          Ast *let = ast_let(ast_identifier($2), $4, $6);
                                          let->tag = AST_LOOP;
                                          $$ = let;

                                      }
  | expr '[' expr ']'                 { $$ = array_index_expression($1, $3);}
  | expr '[' expr DOUBLE_DOT ']'      { $$ = array_offset_expression($1, $3);}
  | expr ':' '=' expr                 { $$ = ast_assignment($1, $4); }
  ;

atom_expr:
    simple_expr
  | atom_expr '.' IDENTIFIER          { $$ = ast_record_access($1, ast_identifier($3)); }
  ;

simple_expr:
    INTEGER               { $$ = AST_CONST(AST_INT, $1); }
  | DOUBLE                { $$ = AST_CONST(AST_DOUBLE, $1); }
  | FLOAT                 { $$ = AST_CONST(AST_FLOAT, $1); }
  | TOK_STRING            { $$ = ast_string($1); }
  | TRUE                  { $$ = AST_CONST(AST_BOOL, true); }
  | FALSE                 { $$ = AST_CONST(AST_BOOL, false); }
  | IDENTIFIER            { $$ = ast_identifier($1); }
  | TOK_VOID              { $$ = ast_void(); }
  | list                  { $$ = $1; }
  | array                 { $$ = $1; }
  | tuple                 { $$ = $1; }
  | fstring               { $$ = parse_fstring_expr($1); }
  | TOK_CHAR              { $$ = ast_char($1); }
  | '(' expr_sequence ')' { $$ = $2; }
  | '(' FN lambda_args ARROW expr_sequence ')' { $$ = ast_lambda($3, $5); }
  | '(' FN TOK_VOID ARROW expr_sequence ')'    { $$ = ast_void_lambda($5); }
  | '(' '+' ')'           { $$ = ast_identifier((ObjString){"+", 1}); }
  | '(' '-' ')'           { $$ = ast_identifier((ObjString){"-", 1}); }
  | '(' '*' ')'           { $$ = ast_identifier((ObjString){"*", 1}); }
  | '(' '/' ')'           { $$ = ast_identifier((ObjString){"/", 1}); }
  | '(' MODULO ')'        { $$ = ast_identifier((ObjString){"%", 1}); }
  | '(' '<' ')'           { $$ = ast_identifier((ObjString){"<", 1}); }
  | '(' '>' ')'           { $$ = ast_identifier((ObjString){">", 1}); }
  | '(' DOUBLE_AMP ')'    { $$ = ast_identifier((ObjString){"&&", 2}); }
  | '(' DOUBLE_PIPE ')'   { $$ = ast_identifier((ObjString){"||", 2}); }
  | '(' GE ')'            { $$ = ast_identifier((ObjString){">=", 2}); }
  | '(' LE ')'            { $$ = ast_identifier((ObjString){"<=", 2}); }
  | '(' NE ')'            { $$ = ast_identifier((ObjString){"!=", 2}); }
  | '(' EQ ')'            { $$ = ast_identifier((ObjString){"==", 2}); }
  | '(' PIPE ')'          { $$ = ast_identifier((ObjString){"|", 1}); }
  | '(' ':' ')'           { $$ = ast_identifier((ObjString){":", 1}); }
  | '(' DOUBLE_COLON ')'  { $$ = ast_identifier((ObjString){"::", 2}); }
  | '(' IDENTIFIER ')'            { $$ = ast_identifier($2); }
  ;


expr_sequence:
    expr                        { $$ = $1; }
  | expr_sequence ';' expr      { $$ = parse_stmt_list($1, $3); }
  ;

let_binding:
    LET TEST_ID '=' expr            { $$ = ast_test_module($4);}
  | LET IDENTIFIER '=' expr         { $$ = ast_let(ast_identifier($2), $4, NULL); }
  | LET IDENTIFIER '=' EXTERN FN fn_signature  
                                    { $$ = ast_let(ast_identifier($2), ast_extern_fn($2, $6), NULL); }

  | LET lambda_arg '=' expr         { $$ = ast_let($2, $4, NULL); }

  | LET expr_list '=' expr          { $$ = ast_let(ast_tuple($2), $4, NULL);}

  | LET MUT expr_list '=' expr      { Ast *let = ast_let(ast_tuple($3), $5, NULL);
                                      let->data.AST_LET.is_mut = true;
                                      $$ = let;
                                    }




  | LET TOK_VOID '=' expr           { $$ = $4; }
  | let_binding IN expr             {
                                      Ast *let = $1;
                                      let->data.AST_LET.in_expr = $3;
                                      $$ = let;
                                    }
  | lambda_expr                     { $$ = $1; }

  | LET '(' IDENTIFIER ')' '=' lambda_expr 
                                    {
                                      Ast *id = ast_identifier($3);
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      $$ = ast_let(id, $6, NULL);
                                    }

  | LET '(' IDENTIFIER ')' '=' expr
                                    {
                                      Ast *id = ast_identifier($3);
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      $$ = ast_let(id, $6, NULL);
                                    }
/*
  | IMPORT PATH_IDENTIFIER IN expr  { $$ = ast_let(NULL, ast_import_stmt($2, false), $4); }
  | OPEN PATH_IDENTIFIER IN expr    { $$ = ast_let(NULL, ast_import_stmt($2, true), $4); }
  */
  | IMPORT PATH_IDENTIFIER            { $$ = ast_import_stmt($2, false); }
  | OPEN PATH_IDENTIFIER              { $$ = ast_import_stmt($2, true); }
  | IMPORT IDENTIFIER                 { $$ = ast_import_stmt($2, false); }
  | OPEN IDENTIFIER                   { $$ = ast_import_stmt($2, true); }
  | LET IDENTIFIER ':' IDENTIFIER '=' lambda_expr { $$ = ast_trait_impl($4, $2, $6); }
  ;



lambda_expr:
    FN lambda_args ARROW expr_sequence ';'      { $$ = ast_lambda($2, $4); }
  | FN TOK_VOID ARROW expr_sequence ';'         { $$ = ast_void_lambda($4); }
  | MODULE lambda_args ARROW expr_sequence ';'{ $$ = ast_module(ast_lambda($2, $4)); }
  | MODULE TOK_VOID ARROW expr_sequence ';'   { $$ = ast_module(ast_lambda(NULL, $4)); }
  ;




lambda_args:
    lambda_arg                                   { $$ = ast_arg_list($1, NULL); }
  | lambda_arg '=' expr                          { $$ = ast_arg_list($1, $3); }
  | lambda_arg ':' '(' type_expr ')'             { $$ = ast_arg_list($1, $4); }
  | lambda_args lambda_arg                       { $$ = ast_arg_list_push($1, $2, NULL); }
  | lambda_args lambda_arg '=' expr              { $$ = ast_arg_list_push($1, $2, $4); }
  | lambda_args lambda_arg ':' '(' type_expr ')' { $$ = ast_arg_list_push($1, $2, $5); }
  ;

lambda_arg:
    IDENTIFIER              { $$ = ast_identifier($1); }
  | '(' expr_list ')'       { $$ = ast_tuple($2); }
  | IDENTIFIER DOUBLE_COLON lambda_arg  { $$ = ast_list_prepend(ast_identifier($1), $3); }
  | '_'                               { $$ = Ast_new(AST_PLACEHOLDER_ID); }
  ;


list:
    '[' ']'                 { $$ = ast_empty_list(); }
  | '[' expr_list ']'       { $$ = $2; }
  | '[' expr_list ',' ']'   { $$ = $2; }
  ;

array:
    '[''|' '|'']'                 { $$ = ast_empty_array(); }
  | '[''|' expr_list '|'']'       { $$ = ast_list_to_array($3); }
  | '[''|' expr_list ',' '|'']'   { $$ = ast_list_to_array($3); }
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
  | 'if' expr THEN expr ELSE expr  { $$ = ast_if_else($2, $4 ,$6);} 
  | 'if' expr THEN expr            { $$ = ast_if_else($2, $4, NULL);} 
  ;

match_test_clause:
    expr {$$ = $1;}
  | expr 'if' expr { $$ = ast_match_guard_clause($1, $3);}

match_branches:
    '|' match_test_clause ARROW expr                 {$$ = ast_match_branches(NULL, $2, $4);}
  | match_branches '|' match_test_clause ARROW expr  {$$ = ast_match_branches($1, $3, $5);}
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
    TYPE IDENTIFIER '=' type_expr {
                                    Ast *type_decl = ast_let(ast_identifier($2), $4, NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    $$ = type_decl;
                                  }

  | TYPE IDENTIFIER              {
                                      Ast *type_decl = ast_let(ast_identifier($2), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      $$ = type_decl;
                                   }

  | TYPE type_args '=' type_expr {
                                    Ast *args = $2;
                                    Ast *name = args->data.AST_LAMBDA.params;
                                    args->data.AST_LAMBDA.params = args->data.AST_LAMBDA.params + 1;
                                    args->data.AST_LAMBDA.len--;
                                    args->data.AST_LAMBDA.body = $4;
                                    Ast *type_decl = ast_let(name, args, NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    $$ = type_decl;
                                  }
  ;

type_args:
    IDENTIFIER IDENTIFIER                { $$ = ast_arg_list_push(ast_arg_list(ast_identifier($1), NULL), ast_identifier($2), NULL); }
  | type_args IDENTIFIER                 { $$ = ast_arg_list_push($1, ast_identifier($2), NULL); }

fn_signature:
    type_expr ARROW type_expr           { $$ = ast_fn_sig($1, $3); }
  | fn_signature ARROW type_expr        { $$ = ast_fn_sig_push($1, $3); }
  ;

tuple_type:
    type_atom ',' type_atom         { $$ = ast_tuple_type($1, $3); }
  | tuple_type ',' type_atom        { $$ = ast_tuple_type_push($1, $3); }
  ;

type_expr:
    type_atom                 { $$ = $1; }
  | '|' type_atom             { $$ = ast_list($2); }
  | type_expr '|' type_atom   { $$ = ast_list_push($1, $3); } 
  | fn_signature              { $$ = ast_fn_signature_of_list($1); }
  | tuple_type                { $$ = $1; }
  ;

type_atom:
    IDENTIFIER                { $$ = ast_identifier($1); }
  | IDENTIFIER '=' INTEGER    { $$ = ast_let(ast_identifier($1), AST_CONST(AST_INT, $3), NULL); } 
  | IDENTIFIER 'of' type_atom { $$ = ast_cons_decl(TOKEN_OF, ast_identifier($1), $3); } 
  | IDENTIFIER ':' type_atom  { $$ = ast_assoc(ast_identifier($1), $3); } 
  | '(' type_expr ')'         { $$ = $2; }
  | TOK_VOID                  { $$ = ast_void(); }
  | IDENTIFIER '.' IDENTIFIER { $$ = ast_record_access(ast_identifier($1), ast_identifier($3)); }
  ;
%%


void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, pctx.cur_script);
}
#endif _LANG_TAB_H
