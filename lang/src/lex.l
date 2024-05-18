%{
#include <stdlib.h>
#include "parse.h"
#include "y.tab.h"
%}

%%

"fn"                    { return FN; }
[a-zA-Z_][a-zA-Z0-9_]*  {
                          yylval.vident = strdup(yytext); // Store the matched identifier in yylval
                          return IDENTIFIER; // Return the IDENTIFIER token
                        }
\"([^\"\n]|\\.)*\"      {
                          yylval.vstr = strdup(yytext); // Store the matched string in yylval
                          return STRING; // Return the STRING token
                        }

-?0                     { 
                          yylval.vint = atoi(yytext);
                          return INTEGER;
                        }

-?[1-9][0-9]*f          { 
                          yylval.vfloat = atof(yytext);
                          return NUMBER;
                        }
-?[1-9][0-9]*           { 
                          yylval.vint = atoi(yytext);
                          return INTEGER;
                        }
-?0\.([0-9]*[1-9]|[1-9][0-9]*) {
                        yylval.vfloat = atof(yytext);
                        return NUMBER;
                     }
-?[1-9][0-9]*\.([0-9]*[1-9]|[1-9][0-9]*)? {
                        yylval.vfloat = atof(yytext);
                        return NUMBER;
                     }

[-()<>=+*/;{}.]         {
                          return *yytext;
                        }
"%"             return MODULO;


">="            return GE;
"<="            return LE;
"=="            return EQ;
"!="            return NE;
"while"         return WHILE;
"if"            return IF;
"else"          return ELSE;
"print"         return PRINT;
"|>"            return PIPE;
"true"          return TRUE;
"false"         return FALSE;
"let"           return LET;
"->"            { return ARROW; }
[ \t\n]+        ;       /* ignore whitespace */
"#".*           ;       /* ignore comments starting with # */

.               { char err[64]; snprintf(err, sizeof(err), "Unknown character: %s", yytext); yyerror(err); }
%%
int yywrap(void) {
    return 1;
}
