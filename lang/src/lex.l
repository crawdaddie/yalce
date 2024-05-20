%{
#include <stdlib.h>
#include "parse.h"
#include "y.tab.h"
%}

%%

"fn"                    return FN;
"let"                   return LET;
"()"                    return VOID;
"extern"                return EXTERN;
"true"                  return TRUE;
"false"                 return FALSE;
";;"                    return DOUBLE_SEMICOLON;
"in"                    return IN;
[a-zA-Z_][a-zA-Z0-9_]*  {
                          yylval.vident = strdup(yytext); 
                          return IDENTIFIER;
                        }
\"([^\"\n]|\\.)*\"      {
                          int len = yyleng - 2;
                          yylval.vstr = strndup(yytext + 1, len); // remove surrounding " chars
                          return STRING;
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
"|>"            return PIPE;
"->"            { return ARROW; }
[ \t\n]+        ;       /* ignore whitespace */
"#".*           ;       /* ignore comments starting with # */

.               { char err[64]; snprintf(err, sizeof(err), "Unknown character: %s", yytext); yyerror(err); }
%%
int yywrap(void) {
    return 1;
}
