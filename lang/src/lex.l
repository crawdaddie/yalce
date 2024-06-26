%{
#include <stdlib.h>
#include "parse.h"
#include "y.tab.h"
#include "string_proc.h"
%}
%option yylineno

%%
"fn"                    return FN;
"let"                   return LET;
"in"                    return IN;
"and"                   return AND;
"()"                    return TOK_VOID;
"extern"                return EXTERN;
"true"                  return TRUE;
"false"                 return FALSE;
"..."                   return TRIPLE_DOT;
"match"                 return MATCH;
"with"                  return WITH;
"["                     return *yytext;
"]"                     return *yytext;
"_"                     return *yytext;
[a-zA-Z_][a-zA-Z0-9_]*  {

                          int len = yyleng;
                          yylval.vident.chars = strdup(yytext); 
                          yylval.vident.length = len;
                          yylval.vident.hash = hash_string(yylval.vident.chars, len);
                          return IDENTIFIER;
                        }

@[a-zA-Z_][a-zA-Z0-9_]*  {

                          int len = yyleng;
                          yylval.vident.chars = strdup(yytext); 
                          yylval.vident.length = len;
                          yylval.vident.hash = hash_string(yylval.vident.chars, len);
                          return META_IDENTIFIER;
                        }
\"([^\"\n]|\\.)*\"      {
                          int len = yyleng - 2;
                          char *raw_str = strndup(yytext + 1, len); // remove surrounding " chars
                          char *processed_str = process_escapes(raw_str, len); // process escape sequences
                          free(raw_str);
                          yylval.vstr.chars = processed_str;
                          yylval.vstr.length = strlen(processed_str);
                          yylval.vstr.hash = hash_string(processed_str, yylval.vstr.length);
                          return TOK_STRING;
                        }

\`([^\"\n]|\\.)*\`      {
                          int len = yyleng - 2;
                          char *raw_str = strndup(yytext + 1, len); // remove surrounding " chars
                          char *processed_str = process_escapes(raw_str, len); // process escape sequences
                          free(raw_str);
                          yylval.vstr.chars = processed_str;
                          yylval.vstr.length = strlen(processed_str);
                          yylval.vstr.hash = hash_string(processed_str, yylval.vstr.length);
                          return FSTRING;
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
-?[0-9]*\.([0-9]*[1-9]|[0-9][0-9]*)? {
                        yylval.vfloat = atof(yytext);
                        return NUMBER;
                        }

"|>"                    return PIPE;

[-()<>=+*/;{},|]         {
                          /*printf("operator '%s'", yytext);*/
                          return *yytext;
                        }
"%"             return MODULO;
">="            return GE;
"<="            return LE;
"=="            return EQ;
"!="            return NE;
"->"            { return ARROW; }
[ \t\n]+        ;       /* ignore whitespace */
"#".*           ;       /* ignore comments starting with # */

.               { char err[64]; snprintf(err, sizeof(err), "Unknown character: %s", yytext); yyerror(err); }
%%
int yywrap(void) {
    return 1;
}
