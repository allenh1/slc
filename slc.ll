%option noyywrap
%{
#include <cstdio>

#define YY_DECL int yylex()

/* remove register keyword */
#define register

#include <cstring>
#include <string>
#include "slc_bison.hh"

%}

%%

[ \t\n\v];      // ignore all whitespace
";;"[^\n];      // ignore comments

"lambda" {return LAMBDA;}
"let" {return LET;}
"defun" {return DEFUN;}
"int" {return INT;}
"bool" {return BOOL;}
"float" {return FLOAT;}
"string" {return STRING;}
"list" {return LIST;}
"(" {return LPAREN;}
")" {return RPAREN;}
"[" {return LBRACKET;}
"]" {return RBRACKET;}
":" {return COLON;}
"," {return COMMA;}
"'" {return SQUOTE;}

"if" {return IF;}

"+" {return PLUS;}
"-" {return MINUS;}
"*" {return TIMES;}
"cons" {return CONS;}
"cdr" {return CDR;}
"car" {return CAR;}
"xor" {return XOR;}
"or" {return OR;}
"and" {return AND;}
"not" {return NOT;}

">" {return GREATER;}
"<" {return LESS;}
">=" {return GREATER_EQ;}
"<=" {return LESS_EQ;}
"=" {return EQUAL;}

[a-zA-Z_]+ {yylval.sval = strdup(yytext); return IDENTIFIER;}
[0-9]+\.[0-9]+ 	{yylval.fval = atof(yytext); return FLOAT;}
[0-9]+		{yylval.ival = atoi(yytext); return INT;}
\"(\\.|[^"\\])*\" {yylval.sval = strdup(yytext); return STR;}

%%
