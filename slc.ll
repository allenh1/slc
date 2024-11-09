%option noyywrap
%option bison-bridge
%option bison-locations
%option yylineno
%option nounput
%{
#include <cstdio>

/* remove register keyword */
#define register

#include <cstring>
#include <string>
#include "slc_bison.hh"
#define YY_DECL int yylex(YYSTYPE * yylval_param, YYLTYPE * yylloc_param)
#define YY_USER_ACTION \
    yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column; \
    for(int i = 0; yytext[i] != '\0'; i++) { \
        if(yytext[i] == '\n') { \
            yylloc->last_line++; \
            yylloc->last_column = 0; \
        } \
        else { \
            yylloc->last_column++; \
        } \
    }
%}

%x LINE_COMMENT

%%

[[:space:]] {/* Ignore space */}

<INITIAL>";;" {BEGIN(LINE_COMMENT);}
<LINE_COMMENT>[^\n]+ {}
<LINE_COMMENT>[\n] {BEGIN(INITIAL);}

"lambda" {return LAMBDA;}
"let" {return LET;}
"defun" {return DEFUN;}
"int" {return INT;}
"bool" {return BOOL;}
"float" {return FLOAT;}
"string" {return STRING;}
"list" {return LIST;}
"print" {return PRINT;}
"nil" {return NIL;}
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
"extern" {return EXTERN;}
"for" {return FOR;}
"in" {return IN;}
"set" {return SET;}
"do" {return DO;}
"collect" {return COLLECT;}
"loop" {return LOOP;}
"when" {return WHEN;}
"return" {return RETURN;}

">" {return GREATER;}
"<" {return LESS;}
">=" {return GREATER_EQ;}
"<=" {return LESS_EQ;}
"=" {return EQUAL;}

[a-zA-Z_][a-zA-Z_0-9]* {yylval->sval = strdup(yytext); return IDENTIFIER;}
[0-9]+\.[0-9]+ 	{yylval->fval = atof(yytext); return FLOAT;}
[0-9]+		{yylval->ival = atoi(yytext); return INT;}
\"(\\.|[^"\\])*\" {yylval->sval = strdup(yytext); return STR;}

%%
