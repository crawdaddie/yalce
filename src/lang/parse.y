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
  Value value;
}

%token<value> NUMBER
%token<value> INTEGER
%token<string> IDENTIFIER
%token<string> STRING
%token PIPE EQUALS FN LET


%left '+' '-'
%left '*' '/'
%type <value> expr
%type <value> sexpr
%type <value> nexpr
%type <value> statement
%type <value> lexpr
%%

program:
        program statement 
        | '\n'
        | /* NULL */
        ;

statement:
        expr                  
        | IDENTIFIER '=' expr { table_set($1, $3);}
        | IDENTIFIER          { print_value(table_get($1));} 
        | '\n'
        ;


expr:
        IDENTIFIER         { $$ = table_get($1);}
        | '(' expr ')'     { $$ = $2; }
        | nexpr
        | sexpr
        | expr EQUALS expr { printf("equality\n");}
        | IDENTIFIER '(' lexpr ')' { printf("call function %s\n", $1);}
        | lexpr
        ;

lexpr:
     expr ',' { printf("list? n"); $$ = $1; }
     | lexpr expr { printf("list expr > 1 el "); print_value($1); print_value($2);}
     ;

nexpr: 
        | expr '+' expr    { $$ = nadd($1, $3); }
        | expr '-' expr    { $$ = nsub($1, $3); }
        | expr '*' expr    { $$ = nmul($1, $3); }
        | expr '/' expr    { $$ = ndiv($1, $3); }
        | INTEGER
        | NUMBER
        ;

sexpr:
        STRING {$$ = make_string($1);}
        /* | sexpr '+' sexpr {$$ = strconcat($1, $3);} */
    


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
