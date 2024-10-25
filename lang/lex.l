%{
#ifndef _LEX_YY_C
#define _LEX_YY_C
#include <stdlib.h>
#include "parse.h"
#include "y.tab.h"
#include "string_proc.h"
// Add these declarations
int yycolumn = 1;
int yyprevcolumn = 1;
int yyprevlineno = 1;

long long int yyabsoluteoffset = 0; 
long long int yyprevoffset = 0; 
int in_test_section = 0;

// This macro is called at the beginning of each rule's action
#define YY_USER_ACTION \
    yyprevlineno = yylineno; \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yyprevcolumn = yycolumn; \
    yycolumn += yyleng; \
    yyprevoffset = yyabsoluteoffset; \
    yyabsoluteoffset += yyleng;
%}
%option yylineno

%x FSTRING
%x TRIPLE_FSTRING
%x TEST_SECTION

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
"if"                    return *yytext;
"await"                 return *yytext;
"async"                 return ASYNC;
"yield"                 return *yytext;
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
"```"                   { BEGIN(TRIPLE_FSTRING); return FSTRING_START; }
<TRIPLE_FSTRING>"```"   { BEGIN(INITIAL); return FSTRING_END; }
<TRIPLE_FSTRING>"{"     { BEGIN(INITIAL); return FSTRING_INTERP_START; }
<TRIPLE_FSTRING>[^`{]+  { 
                          int len = yyleng;
                          char *raw_str = strndup(yytext, len);
                          // Trim first and last newline if present

                          if (raw_str[0] == '\n') {
                            raw_str++;
                            len--;
                          }
                          if (len > 0 && raw_str[len-1] == '\n') {
                            raw_str[len-1] = '\0';
                            len--;
                          }
                          char *processed_str = process_escapes(raw_str, len);
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
"@@"                    return DOUBLE_AT;

[-()<>=+*/;{},|]        {
                          /*printf("operator '%s'", yytext);*/
                          return *yytext;
                        }
^%test          {
                  if (lex_test_block) {
                    in_test_section = 1;
                    BEGIN(TEST_SECTION);
                  } else {
                    yyterminate();  // This will stop lexing and return 0
                  }
                }

<TEST_SECTION>(.|\n)   {
                         if (lex_test_block) {
                           // Process the test section normally
                           REJECT;
                         }
                         // Otherwise, this rule won't be reached due to yyterminate()
                       }
^%.*            {
                  /*
                  printf("possible macro %s %d\n", yytext, top_level_tests);
                  handle_macro(ast_root, yytext + 1);
                  print_ast(ast_root);
                  printf("should insert ast nodes\n");
                  */
                };       

"%"             return MODULO;
">="            return GE;
"<="            return LE;
"=="            return EQ;
"!="            return NE;
"::"            return DOUBLE_COLON;
":"             return *yytext;
"->"            return ARROW;
"#".*           ;       /* ignore comments starting with # */

  
[ \t]+          ;       /* ignore whitespace */
\n              {yycolumn = 1;}      /* ignore whitespace */
{CHAR}          { 
                    /* Matched a character literal */
                    /* You might want to return a token here, e.g., return CHAR_LITERAL; */
                    yylval.vchar = *(yytext + 1);
                    return TOK_CHAR;
                }
[&$*+\-/=>@^|%<]([&$*+\-/=>@^|%<!.:\?~]*)       {
                          int len = yyleng;
                          yylval.vident.chars = strdup(yytext); 
                          yylval.vident.length = len;
                          yylval.vident.hash = hash_string(yylval.vident.chars, len);
                          return IDENTIFIER;
}
.               { char err[64]; snprintf(err, sizeof(err), "Unknown character: %s", yytext); yyerror(err); }
%%
int yywrap(void) {
    return 1;
}
#endif _LEX_YY_C
