%code requires {
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "lang.h"

double symbols[52];
int yylex();
void yyerror(char *s);
double symbol_val(char symbol);
void set_symbol_val(char symbol, double val);
int symbol_idx(char token);

void set_input_string(const char* in);
void end_lexical_scan(void);

int parse_string(const char* in);
typedef struct sym_list {

} sym_list;
}



%union {
    double num;
    char id;
    sym_list list;
}

%start line

%token print
%token exit_command
%token loop

%token <num> number
%token <id> identifier

%type <num> line exp term 
%type <id> assignment

%%

line    : assignment ';'          {;}
        | exit_command ';'        {exit(EXIT_SUCCESS);}
        | print exp ';'           {printf("> %lf\n", $2);}
        | line assignment ';'     {;}
        | line print exp ';'      {printf("> %lf\n", $3);}
        | line exit_command ';'   {exit(EXIT_SUCCESS);}
        | loop exp ';'            {printf("loop %s, %lf\n", $$, $2);}
        ;

assignment : identifier '=' exp  { set_symbol_val($1,$3); }
           ;

exp     : term                   {$$ = $1;}
        | exp '+' term           {$$ = $1 + $3;}
        | exp '-' term           {$$ = $1 - $3;}
        | exp '*' term           {$$ = $1 * $3;}
        | exp '/' term           {$$ = $1 / $3;}
        ;

term    : number                 {printf("val %lf\n", $1);$$ = $1;}
        | identifier             {$$ = symbol_val($1);} 
        ;

%%

double symbol_val(char symbol) {
    int bucket = symbol_idx(symbol);
    return symbols[bucket];
}

void set_symbol_val(char symbol, double val) {
    int bucket = symbol_idx(symbol);
    symbols[bucket] = val;
}

int symbol_idx(char token) {
    int idx = -1;
    if(islower(token)) {
        idx = token - 'a' + 26;
    }
    else if(isupper(token)) {
        idx = token - 'A';
    }
    return idx;
} 

void yyerror(char *s) {
    fprintf(stderr, "%s\n", s);
}

void set_input_string(const char* in);
void end_lexical_scan(void);

/* This function parses a string */
int parse_string(const char* in) {
  set_input_string(in);
  int rv = yyparse();
  end_lexical_scan();
  return rv;
}

