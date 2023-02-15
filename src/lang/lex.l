%{
#include "parse.tab.h"
#include <stdlib.h>
void yyerror(char *);
%}
%x EXPECT_STRING
%%

[a-zA-Z_][a-zA-Z_0-9]* {
                          yylval.string = strdup(yytext);
                          return IDENTIFIER;
                        }

[-]?[1-9][0-9]*         {
                          sscanf(yytext, "%d", &yylval.integer);
                          return INTEGER;
                        }

[-]?([0-9]*[.])?[0-9]+  {
                          sscanf(yytext, "%lf", &yylval.number);
                          return NUMBER;
                        }

->                      { return PIPE; }

[-+()=/*%^,\n]          { return *yytext; }
          

[ \t\r]   ;             /* skip whitespace */

\"	{BEGIN(EXPECT_STRING); return DOUBLE_QUOTE;}
<EXPECT_STRING>([^\\\"]|\\.)*  {
	yylval.string = strdup(yytext);
	return STRING;
}
<EXPECT_STRING>\" {BEGIN(INITIAL); return DOUBLE_QUOTE;}

.                       yyerror("Unknown character");


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
