%{
#include <stdlib.h>
#include "parse.h"
#include "y.tab.h"
#include "string_proc.h"
%}
%option yylineno
%x FSTRING
ESCAPE          \\(['"?\\abfnrtv]|[0-7]{1,3}|x[0-9a-fA-F]+)
CHAR            '([^'\\\n]|{ESCAPE})'
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
"import"                return *yytext;
"."                     return *yytext;
"type"                  return *yytext;
"of"                    return *yytext;
"await"                 return *yytext;
"async"                 return ASYNC;
"&"                     return *yytext;
"*"                     return *yytext;

[a-zA-Z_][a-zA-Z0-9_]*  {

                          int len = yyleng;
                          yylval.vident.chars = strdup(yytext); 
                          yylval.vident.length = len;
                          yylval.vident.hash = hash_string(yylval.vident.chars, len);
                          return IDENTIFIER;
                        }

@[a-zA-Z_][a-zA-Z0-9_]* {

                          int len = yyleng;
                          yylval.vident.chars = strdup(yytext); 
                          yylval.vident.length = len;
                          yylval.vident.hash = hash_string(yylval.vident.chars, len);
                          return META_IDENTIFIER;
                        }

"_"                     return *yytext;



"`"                     { BEGIN(FSTRING); return FSTRING_START; }
<FSTRING>"`"            { BEGIN(INITIAL); return FSTRING_END; }
<FSTRING>"{"            { BEGIN(INITIAL); return FSTRING_INTERP_START; }
<FSTRING>[^`{]+         { 
                          int len = yyleng;
                          char *raw_str = strndup(yytext, len); // remove surrounding " chars
                          char *processed_str = process_escapes(raw_str, len); // process escape sequences
                          free(raw_str);
                          yylval.vstr.chars = processed_str;
                          yylval.vstr.length = strlen(processed_str);
                          yylval.vstr.hash = hash_string(processed_str, yylval.vstr.length);
                          return FSTRING_TEXT;
                        }
"}"                     { 
                          if (YYSTATE == FSTRING) {
                            return FSTRING_INTERP_END;
                          } else {
                            BEGIN(FSTRING); 
                            return FSTRING_INTERP_END;
                          }
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


-?0                     { 
                          yylval.vint = atoi(yytext);
                          return INTEGER;
                        }

-?[1-9][0-9]*f          { 
                          yylval.vdouble = atof(yytext);
                          return DOUBLE;
                        }

-?[1-9][0-9]*           { 
                          yylval.vint = atoi(yytext);
                          return INTEGER;
                        }

0x[0-9a-fA-F]+          { 
                          yylval.vint = (int)strtol(yytext, NULL, 16);
                          return INTEGER;
                        }

-?[0-9]*\.([0-9]*[1-9]|[0-9][0-9]*)? {
                        yylval.vdouble = atof(yytext);
                        return DOUBLE;
                        }

"|>"                    return PIPE;

[-()<>=+*/;{},|]        {
                          /*printf("operator '%s'", yytext);*/
                          return *yytext;
                        }

"%"             return MODULO;
">="            return GE;
"<="            return LE;
"=="            return EQ;
"!="            return NE;
"::"            return DOUBLE_COLON;
":"             return *yytext;
"->"            return ARROW;
[ \t\n]+        ;       /* ignore whitespace */
"#".*           ;       /* ignore comments starting with # */

^%.*            {
                  /*printf("possible macro %s %p %d %d\n", yytext, ast_root, ast_root->tag, ast_root->data.AST_BODY.len);
                  print_ast(ast_root);
                  printf("should insert ast nodes\n");
                  */
                  handle_macro(ast_root, yytext + 1);
                };       
  
{CHAR}          { 
                    /* Matched a character literal */
                    /* You might want to return a token here, e.g., return CHAR_LITERAL; */
                    yylval.vchar = *(yytext + 1);
                    return TOK_CHAR;
                }
.               { char err[64]; snprintf(err, sizeof(err), "Unknown character: %s", yytext); yyerror(err); }
%%
int yywrap(void) {
    return 1;
}
