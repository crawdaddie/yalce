%{
#include "parse.tab.h"
#include <stdlib.h>
#include "value.h"
%}
%%
"print"                 { return PRINT; }

[a-zA-Z_][a-zA-Z_0-9]*  {
                          yylval.string = strdup(yytext);
                          return IDENTIFIER;
                        }

[-]?[1-9][0-9]*         {
                          int num;
                          sscanf(yytext, "%d", &num);
                          yylval.value = INTEGER_VAL(num);
                          return INTEGER;
                        }

[-]?([0-9]*[.])?[0-9]+  {
                          double num;
                          sscanf(yytext, "%lf", &num);
                          yylval.value = NUMBER_VAL(num);
                          return NUMBER;
                        }

"->"                    { return PIPE; }

[-+()=/*%^,:{}\n]       { return *yytext; }

"=="                    { return EQUALS; } 

"fn"                    { return FN; }
          
[ \t\r]   ;             /* skip whitespace */

"//".*\n                

"/*"[^*/]*"*/"          

\"([^\\\"]|\\.)*\"      {
                          int size = strlen(yytext);
                          yylval.string = strndup(yytext + 1, size - 2);
                          return STRING;
                        }



.                       { yyerror("Unknown character"); };


%%

void set_input_string(const char *input) {
  yy_scan_string(input);
}
void end_lexical_scan(void) {
  yy_delete_buffer(YY_CURRENT_BUFFER);
}

int yywrap(void) {
    return 1;
}
