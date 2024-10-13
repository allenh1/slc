%{
#include <cstdio>
#include <cstdlib>
#include <string>
#include <asw/slc_node.hpp>
#include "slc_bison.hh"

int yylex(YYSTYPE *, YYLTYPE *);
int yyparse(asw::slc::node * root);
extern FILE * yyin;
void yyerror(YYLTYPE *, asw::slc::node *, char const *);
extern char * yytext;
%}

%token	<ival>		INT
%token	<fval> 		FLOAT
%token	<sval>		STR IDENTIFIER
%token			PLUS MINUS TIMES DIVIDE NIL
%token  		IF NOT LIST DEFUN IMPORT OR AND XOR
%token  		CAR CDR CONS LAMBDA BOOL STRING SQUOTE
%token 			LET LPAREN RPAREN LBRACKET RBRACKET COLON
%token			GREATER LESS GREATER_EQ LESS_EQ EQUAL COMMA
%code requires {#include <asw/slc_node.hpp>}
%code requires {#include <string>}
%define api.pure full
%locations
%parse-param {asw::slc::node * root}
%union {
    int ival;
    double fval;
    char * sval;
    asw::slc::type_id type_id;
    asw::slc::op_id op_id;
    asw::slc::node * node;
    asw::slc::definition * def;
    asw::slc::variable_definition * var_def;
    asw::slc::function_definition * func_def;
    asw::slc::expression * expr;
    asw::slc::list * exprs;
    asw::slc::simple_expression * sexpr;
    asw::slc::formal * formal;
    asw::slc::function_body * func_body;
}

%type	<node>  	stmt
%type	<type_id>	type
%type	<op_id>	        bin_op list_op unary_op
%type	<def>		definition
%type	<var_def>	variable_definition
%type	<func_def>	function_definition
%type	<expr>		expression
%type	<exprs>		expressions
%type	<sexpr>		sexpr
%type	<formal>	formal
%type	<func_body>	body
%start program
%%
program:        program stmt
		{
		    root->add_child($2);
		}
	|	stmt
		{
		    root->add_child($1);
		}
	;

stmt:	        definition
		{
		    $$ = $1;
		}
	|	expression
		{
		    $$ = $1;
		}
		;


definition:	variable_definition { $$ = $1; }
	|	function_definition { $$ = $1; }
	;

variable_definition:
		LPAREN LET IDENTIFIER expression RPAREN
		{
		    $$ = new asw::slc::variable_definition();
		    $$->add_child($4);
		    $$->set_name($3);
		    $$->set_location(@3.last_line, @3.last_column, yytext);
		    // fprintf(stderr, "define variable '%s'\n", $3);
		    free($3);
		}
		;

function_definition:
		LPAREN DEFUN IDENTIFIER LBRACKET IDENTIFIER RBRACKET body RPAREN
		{
		    /* this declaration has an input list with no bindings */
		    $$ = new asw::slc::function_definition();
		    $$->set_location(@3.last_line, @3.last_column, yytext);
		    $$->set_name($3);
		    free($3);
		    $$->set_body($7);
		    auto * tmp = new asw::slc::variable();
		    tmp->set_name($5);
		    free($5);
		    $$->set_argument_list(tmp);
		}
	|       LPAREN DEFUN IDENTIFIER body RPAREN
		{
		    /* this declaration has no parameters */
		    $$ = new asw::slc::function_definition();
		    $$->set_location(@3.last_line, @3.last_column, yytext);
		    $$->set_name($3);
		    free($3);
		    $$->set_body($4);
		}
	|	LPAREN DEFUN IDENTIFIER LPAREN formals RPAREN body RPAREN
		{
		    /* this declaration has structural bindings on each specified formal */
		}
        ;

formals:	formals COMMA formal
	|	formal;

formal:		IDENTIFIER COLON type
		{
		  $$ = new asw::slc::formal();
		  $$->set_location(@1.last_line, @1.last_column, yytext);
		  $$->set_name($1);
		  $$->set_type($3);
		  free($1);
		}
	;
type:		INT {$$ = asw::slc::type_id::INT;}
	|	BOOL {$$ = asw::slc::type_id::BOOL;}
	|       FLOAT {$$ = asw::slc::type_id::FLOAT;}
	|	STRING {$$ = asw::slc::type_id::STRING;}
	|	LIST {$$ = asw::slc::type_id::LIST;}
	|	LAMBDA {$$ = asw::slc::type_id::LAMBDA;}
	|	IDENTIFIER {$$ = asw::slc::type_id::VARIABLE;}
	;

body:	        stmt body
		{
		    $2->add_child($1);
		    $$ = $2;
		}
	|	expression  // function body must end in an expression
		{
		    $$ = new asw::slc::function_body();
		    $$->set_name(std::string("expression_") + std::to_string(@1.last_line) + "_" + std::to_string(@1.last_column));
		    $$->set_location(@1.last_line, @1.last_column, yytext);
		    $$->set_return_expression($1);
		}
	;

bin_op:       	GREATER {$$ = asw::slc::op_id::GREATER;}
	|	LESS {$$ = asw::slc::op_id::LESS;}
	|	GREATER_EQ {$$ = asw::slc::op_id::GREATER_EQ;}
	|	LESS_EQ {$$ = asw::slc::op_id::LESS_EQ;}
	|	EQUAL {$$ = asw::slc::op_id::EQUAL;}
	;

list_op:	TIMES {$$ = asw::slc::op_id::TIMES;}
	|	DIVIDE {$$ = asw::slc::op_id::DIVIDE;}
	|	PLUS {$$ = asw::slc::op_id::PLUS;}
	|       MINUS {$$ = asw::slc::op_id::MINUS;}
      	|	AND {$$ = asw::slc::op_id::AND;}
	|	OR {$$ = asw::slc::op_id::OR;}
	|	XOR {$$ = asw::slc::op_id::XOR;}
	|	CONS {$$ = asw::slc::op_id::CONS;}
	|	CDR {$$ = asw::slc::op_id::CDR;}
	|	CAR {$$ = asw::slc::op_id::CAR;}
	;

unary_op:       NOT {$$ = asw::slc::op_id::NOT;}
	;

expressions:	expressions expression
		{
		    asw::slc::list * iter = $1;
		    /* go to the end of the list */
		    for (; iter->get_tail() != nullptr; iter = iter->get_tail());
		    iter->set_tail(new asw::slc::list());
		    iter->get_tail()->set_location(@1.last_line, @1.last_column, yytext);
		    iter->get_tail()->set_head($2);
		    $$ = $1;
		}
	|       expression
		{
		    $$ = new asw::slc::list();
		    $$->set_location(@1.last_line, @1.last_column, yytext);
		    $$->set_head($1);
		}
	;

expression:	LPAREN LAMBDA formals body RPAREN
	|	LPAREN IF expression expression expression RPAREN
		{
		    $$ = new asw::slc::if_expr();
		    $$->set_location(@2.last_line, @2.last_column, yytext);
		    $$->set_name(
			std::string("if_stmt_") +
			std::to_string(@3.first_line) + "_" + std::to_string(@3.first_column));
		    $$->add_child($3);
		    $$->add_child($4);
		    $$->add_child($5);
		}
	|	LPAREN bin_op expression expression RPAREN
		{
		    $$ = new asw::slc::binary_op();
		    $$->set_name(
			std::string("bin_op_") +
			std::to_string(@3.first_line) + "_" + std::to_string(@4.first_column));
		    ((asw::slc::binary_op *)$$)->set_op($2);
		    $$->add_child($3);
		    $$->add_child($4);
	        }
	|	LPAREN unary_op expression RPAREN
		{
		    $$ = new asw::slc::unary_op();
		    $$->add_child($3);
		    ((asw::slc::unary_op *)$$)->set_op($2);
		}
	|       LPAREN list_op expressions RPAREN
		{
		    $$ = new asw::slc::list_op();
		    $$->set_name(
			std::string("list_op_") +
			std::to_string(@$.first_line) + "_" + std::to_string(@$.first_column));
		    $$->add_child($3);
		    ((asw::slc::list_op *)$$)->set_op($2);
		}
	|	LPAREN IDENTIFIER expressions RPAREN
		{
		    $$ = new asw::slc::function_call();
		    $$->set_name($2);
		    $$->add_child($3);
		    free($2);
		}
	|	LPAREN IDENTIFIER expression RPAREN
		{
		    $$ = new asw::slc::function_call();
		    $$->set_name($2);
		    $$->add_child($3);
		    free($2);
		}
	|	SQUOTE LPAREN expressions RPAREN
		{
		    $$ = $3;
		}
	|	sexpr
		{
		    $$ = $1;
		}
	;

sexpr:	        IDENTIFIER
		{
		    $$ = new asw::slc::variable();
		    $$->set_location(@1.first_line, @1.first_column, yytext);
		    $$->set_name($1);
		    free($1);
		}
	|	FLOAT
		{
		    auto * lit = new asw::slc::literal();
		    lit->set_value($1);
		    lit->set_tid(asw::slc::type_id::FLOAT);
		    $$ = lit;
		}
	|	INT
		{
		    auto * lit = new asw::slc::literal();
		    lit->set_value($1);
		    lit->set_tid(asw::slc::type_id::INT);
		    $$ = lit;
		}
	|	STR
		{
		    auto * lit = new asw::slc::literal();
		    lit->set_value(std::string($1));
		    lit->set_tid(asw::slc::type_id::STRING);
		    $$ = lit;
		    free($1);
		}
	|	NIL
		{
		    auto * lit = new asw::slc::literal();
		    lit->set_tid(asw::slc::type_id::NIL);
		    $$ = lit;
		}
	;

%%

void yyerror(YYLTYPE * locp, asw::slc::node *, char const * msg) {
  fprintf(stderr, "error: %d:%d: '%s'\n", locp->last_line, locp->last_column, msg);
  exit(1);
}
