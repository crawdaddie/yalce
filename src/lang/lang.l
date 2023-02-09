%{
  #include "parser.h"

  int yylex();
  void yyerror(char *s);
%}


DIG	[0-9]+
EOL	\n|\r\n|\n\r|\r
WS	{EOL}|[[:blank:]]
IDENT	[!$%&*+\-./:<=>?@^_~[:alnum:]]

CHAR_VAL (newline|space|[^{WS}]|x[0-9A-F]+)

%x EXPECT_CHAR
%x EXPECT_STRING
%%


"exit"      {return exit_command;}
"print"     {return print;}
"loop"      {return loop;}

"("	{return LP;}
")"	{return RP;}
"."	{return DOT;}
"'"	{return QUOTE;}
"#f"	{return FALSE_T;}
"#t"	{return TRUE_T;}
{WS}*	/* do nothing with white space */
;.*$	/* Scheme comment */
<<EOF>>	{return END_OF_FILE;}

[-]?{DIG} {
	yylval.n = strtoll(yytext, NULL, 10);
	return FIXNUM_T;
}
[-]?{DIG}?"."{DIG}	{
  yylval.d = strtod(yytext, NULL);
  return FLOATNUM_T;
}

#\\	{BEGIN(EXPECT_CHAR);}
<EXPECT_CHAR>{CHAR_VAL} {
	BEGIN(INITIAL); 
	yylval.c = *yytext;
	return CHAR_T;
}

\"	{BEGIN(EXPECT_STRING); return DOUBLE_QUOTE;}
<EXPECT_STRING>([^\\\"]|\\.)* {
	yylval.s = yytext;
	return STRING_T;
}
<EXPECT_STRING>\" {BEGIN(INITIAL); return DOUBLE_QUOTE;}

{IDENT}+  {yylval.s = yytext; return SYMBOL_T;}


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
