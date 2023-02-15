%code requires {
#include <stdio.h>
#include <stdlib.h>
#include "sym.h"
#include "value.h"
#include "obj.h"
#include "util.h"
int yylex(void);

void set_input_string(const char* in);
void end_lexical_scan(void);

/* This function parses a string */
int parse_line(const char* in, int line);

}
/* TODO: add lineno to errors */
%union {
  double number;
  int integer;
  char *string;
  Object obj;
  Value value;
}

%token<number> NUMBER
%token<integer> INTEGER
%token<string> IDENTIFIER
%token<string> STRING
%token PIPE
%token DOUBLE_QUOTE

%left '+' '-'
%left '*' '/'
%type <number> nexpr
%type <integer> iexpr
%type <string> sexpr
%type <number>statement
%%

program:
        program statement '\n' 
        | '\n'
        | /* NULL */
        ;

statement:
        nexpr                  
        | IDENTIFIER '=' nexpr  { table_set($1, $3);}
        | IDENTIFIER '=' iexpr  { table_set($1, (double)$3);}
        | iexpr                  
        | sexpr                 
        | IDENTIFIER            { printf("%s> %lf\n", $1, table_get($1));} 
        ;

nexpr:
        IDENTIFIER           { $$ = table_get($1);}
        | nexpr '+' nexpr    { $$ = $1 + $3; }
        | nexpr '-' nexpr    { $$ = $1 - $3; }
        | nexpr '*' nexpr    { $$ = $1 * $3; }
        | nexpr '/' nexpr    { $$ = divf($1, $3); }
        | '(' nexpr ')'      { $$ = $2; }
        | nexpr '+' iexpr    { $$ = $1 + (double)$3; }
        | nexpr '-' iexpr    { $$ = $1 - (double)$3; }
        | nexpr '*' iexpr    { $$ = $1 * (double)$3; }
        | nexpr '/' iexpr    { $$ = divf($1, (double)$3); }
        | iexpr '+' nexpr    { $$ = (double)$1 + $3; }
        | iexpr '-' nexpr    { $$ = (double)$1 - $3; }
        | iexpr '*' nexpr    { $$ = (double)$1 * $3; }
        | iexpr '/' nexpr    { $$ = divf((double) $1, $3); }
        | NUMBER
        ;

sexpr:
        DOUBLE_QUOTE STRING DOUBLE_QUOTE {$$ = $2;}
        | sexpr '+' sexpr {$$ = strconcat($1, $3);}
    



iexpr:
        IDENTIFIER            
        | iexpr '+' iexpr     { $$ = $1 + $3; }
        | iexpr '-' iexpr     { $$ = $1 - $3; }
        | iexpr '*' iexpr     { $$ = $1 * $3; }
        | iexpr '%' iexpr     { $$ = $1 % $3; }
        | iexpr '/' iexpr     { $$ = divi($1, $3); }
        | '(' iexpr ')'       { $$ = $2; }
        | INTEGER
        ;
%%


/* Declarations */
void set_input_string(const char* in);
void end_lexical_scan(void);

/* This function parses a string */
int parse_line(const char* in, int line) {
  set_input_string(in);
  int rv = yyparse();
  end_lexical_scan();
  return rv;
}
