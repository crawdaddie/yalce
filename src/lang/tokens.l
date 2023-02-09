%{
  #include "parser.h"

  int yylex();
  void yyerror(char *s);
%}

delim         [ t]
digit         [0-9]
number        [-]?{digit}*[.]?{digit}+
hex           0[Xx][0-9A-Fa-f]+
ident         [A-Za-z_][A-Za-z0-9_]*
comment       "#"[^\n\r]*
ws            [ \r\n\t]*
list [^,]+ 

%%


"exit"      {return exit_command;}
"print"     {return print;}
"loop"      {return loop;}

[a-zA-Z]    {yylval.id = yytext[0]; return identifier;}
{number}    {sscanf(yytext, "%lf", &yylval.num); return number;}
[-]      {return yytext[0];}
[+]      {return yytext[0];}
[*]      {return yytext[0];}
[/]      {return yytext[0];}
[=]      {return yytext[0];}
[;]      {return yytext[0];}
[ \t\n]     ;
.           {ECHO; yyerror("unexpected character");}

%%

int yywrap(void) {
  return 1;
}

void set_input_string(const char* in) {
  yy_scan_string(in);
}

void end_lexical_scan(void) {
  yy_delete_buffer(YY_CURRENT_BUFFER);
}
