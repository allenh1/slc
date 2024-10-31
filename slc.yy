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
%token  		CAR CDR CONS LAMBDA BOOL STRING SQUOTE EXTERN
%token 			LET LPAREN RPAREN LBRACKET RBRACKET COLON PRINT
%token			GREATER LESS GREATER_EQ LESS_EQ EQUAL COMMA
%code requires {#include <asw/slc_node.hpp>}
%code requires {#include <asw/type_info.hpp>}
%code requires {#include <string>}
%define api.pure full
%locations
%parse-param {asw::slc::node * root}
%union {
    int ival;
    double fval;
    char * sval;
    asw::slc::type_info * type_id;
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
    asw::slc::formals * formals;
    asw::slc::extern_function * exdef;
}

%type	<node>  	stmt
%type	<type_id>	type primitive
%type	<op_id>	        bin_op list_op unary_op
%type	<def>		definition
%type	<var_def>	variable_definition
%type	<func_def>	function_definition
%type	<expr>		expression
%type	<exprs>		expressions
%type	<sexpr>		sexpr
%type	<formal>	formal
%type	<formals>	formals
%type	<func_body>	body
%type	<exdef>		extern_definition
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
	|	extern_definition { $$ = $1; }
	;

extern_definition:
		LPAREN EXTERN type IDENTIFIER LPAREN formals RPAREN RPAREN
		{
		    $$ = new asw::slc::extern_function();
		    $$->set_name($4);
		    free($4);
		    $$->set_formals($6);
                    delete $6;
		    $$->set_type($3);
                }
        |       LPAREN EXTERN type IDENTIFIER RPAREN
		{
		    $$ = new asw::slc::extern_function();
		    $$->set_name($4);
		    free($4);
		    $$->set_type($3);
                }
	;

variable_definition:
		LPAREN LET IDENTIFIER expression RPAREN
		{
		    $$ = new asw::slc::variable_definition();
		    $$->add_child($4);
		    $$->set_name($3);
		    $$->set_location(@3.first_line, @3.first_column, yytext);
		    free($3);
		}
		;

function_definition:
	     	LPAREN DEFUN IDENTIFIER body RPAREN
		{
		    /* this declaration has no parameters */
		    $$ = new asw::slc::function_definition();
		    $$->set_location(@3.first_line, @3.first_column, yytext);
		    $$->set_name($3);
		    free($3);
		    $$->set_body($4);
		}
	|	LPAREN DEFUN IDENTIFIER LPAREN formals RPAREN body RPAREN
		{
		    $$ = new asw::slc::function_definition();
		    $$->set_location(@3.first_line, @3.first_column, yytext);
		    $$->set_name($3);
		    free($3);
		    $$->set_formals($5);
		    delete $5;
		    $$->set_body($7);
		}
	 |	LPAREN LAMBDA formals body RPAREN
		{
		    $$ = new asw::slc::function_definition();
		    $$->set_location(@3.first_line, @3.first_column, yytext);
		    $$->set_name(
			std::string("lambda_") + std::to_string(@2.first_line) + "_" +
			std::to_string(@2.first_column));
		    $$->set_formals($3);
		    $$->set_body($4);
		    delete $3;
		}

        ;

formals:	formals COMMA formal
		{
		    $1->push_back($3);
		    $$ = $1;
		}
	|	formal
		{
		  $$ = new asw::slc::formals();
		  $$->push_back($1);
		}

formal:		IDENTIFIER COLON type
		{
		  $$ = new asw::slc::formal();
		  $$->set_location(@1.first_line, @1.first_column, yytext);
		  $$->set_name($1);
		  $$->set_type($3);
		  free($1);
		}
	;

type:	        primitive
		{
		    $$ = $1;
		}
	|	LIST LESS type GREATER
		{
		    $$ = new asw::slc::type_info();
		    $$->type = asw::slc::type_id::LIST;
		    $$->subtype = $3;
		}
		;

primitive:
		INT
		{
		    $$ = new asw::slc::type_info();
		    $$->type = asw::slc::type_id::INT;
		}
	|	BOOL
		{
		    $$ = new asw::slc::type_info();
		    $$->type = asw::slc::type_id::BOOL;
		}
	|       FLOAT
		{
		    $$ = new asw::slc::type_info();
		    $$->type = asw::slc::type_id::FLOAT;
		}
	|	STRING
		{
		    $$ = new asw::slc::type_info();
		    $$->type = asw::slc::type_id::STRING;
		}
	|	LAMBDA
		{
		    $$ = new asw::slc::type_info();
		    $$->type = asw::slc::type_id::LAMBDA;
		}
	;

body:	        stmt body
		{
		    $2->prepend_child($1);
		    $$ = $2;
		}
	|	expression  // function body must end in an expression
		{
		    $$ = new asw::slc::function_body();
		    $$->set_name(std::string("expression_") + std::to_string(@1.first_line) + "_" + std::to_string(@1.first_column));
		    $$->set_location(@1.first_line, @1.first_column, yytext);
		    $$->set_return_expression($1);
		}
	;

bin_op:       	GREATER {$$ = asw::slc::op_id::GREATER;}
	|	LESS {$$ = asw::slc::op_id::LESS;}
	|	GREATER_EQ {$$ = asw::slc::op_id::GREATER_EQ;}
	|	LESS_EQ {$$ = asw::slc::op_id::LESS_EQ;}
	|	EQUAL {$$ = asw::slc::op_id::EQUAL;}
	|	CONS {$$ = asw::slc::op_id::CONS;}
	;

list_op:	TIMES {$$ = asw::slc::op_id::TIMES;}
	|	DIVIDE {$$ = asw::slc::op_id::DIVIDE;}
	|	PLUS {$$ = asw::slc::op_id::PLUS;}
	|       MINUS {$$ = asw::slc::op_id::MINUS;}
      	|	AND {$$ = asw::slc::op_id::AND;}
	|	OR {$$ = asw::slc::op_id::OR;}
	|	XOR {$$ = asw::slc::op_id::XOR;}
        |       PRINT {$$ = asw::slc::op_id::PRINT;}
	;

unary_op:       NOT {$$ = asw::slc::op_id::NOT;}
	|	CAR {$$ = asw::slc::op_id::CAR;}
	|	CDR {$$ = asw::slc::op_id::CDR;}
	;

expressions:	expressions expression
		{
		    asw::slc::list * iter = $1;
		    /* go to the end of the list */
		    for (; iter->get_tail() != nullptr; iter = iter->get_tail());
		    iter->set_tail(new asw::slc::list());
		    iter->get_tail()->set_location(@1.first_line, @1.first_column, yytext);
		    iter->get_tail()->set_type(asw::slc::type_id::LIST);
		    iter->get_tail()->set_head($2);
		    $$ = $1;
		}
	|       expression
		{
		    $$ = new asw::slc::list();
		    $$->set_type(asw::slc::type_id::LIST);
		    $$->set_location(@1.first_line, @1.first_column, yytext);
		    $$->set_head($1);
		}
	;

expression:
 		LPAREN IF expression expression expression RPAREN
		{
		    $$ = new asw::slc::if_expr();
		    $$->set_location(@2.first_line, @2.first_column, yytext);
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
		    $$->set_location(@2.first_line, @2.first_column, yytext);
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
		    $$->set_location(@2.first_line, @2.first_column, yytext);
		    $$->add_child($3);
		    ((asw::slc::unary_op *)$$)->set_op($2);
		}
	|       LPAREN list_op expressions RPAREN
		{
		    $$ = new asw::slc::list_op();
		    $$->set_location(@2.first_line, @2.first_column, yytext);
		    $$->set_name(
			std::string("list_op_") +
			std::to_string(@$.first_line) + "_" + std::to_string(@$.first_column));
		    $$->add_child($3);
		    ((asw::slc::list_op *)$$)->set_op($2);
		}
	|	LPAREN IDENTIFIER expressions RPAREN
		{
		    $$ = new asw::slc::function_call();
		    $$->set_location(@2.first_line, @2.first_column, yytext);
		    $$->set_name($2);
		    free($2);
                    /* convert list to parameters */
                    if (!$3->is_list()) {
			$$->add_child($3);
		    } else {
			auto * slc_list = $3->as_list();
			for (; nullptr != slc_list; ) {
			    /* transfer child to this node */
			    $$->add_child(slc_list->get_head());
			    slc_list->remove_child(slc_list->get_head(), false);
			    slc_list = slc_list->get_tail();
                        }
		        delete $3;
                    }
		}
        |       LPAREN IDENTIFIER RPAREN
                {
		    $$ = new asw::slc::function_call();
		    $$->set_location(@2.first_line, @2.first_column, yytext);
		    $$->set_name($2);
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
		    ((asw::slc::variable *)$$)->set_type(asw::slc::type_id::VARIABLE);
		    free($1);
		}
	|	FLOAT
		{
		    auto * lit = new asw::slc::literal();
		    lit->set_value($1);
		    lit->set_type(asw::slc::type_id::FLOAT);
		    $$ = lit;
		    $$->set_location(@1.first_line, @1.first_column, yytext);
		}
	|	INT
		{
		    auto * lit = new asw::slc::literal();
		    lit->set_value($1);
		    lit->set_type(asw::slc::type_id::INT);
		    $$ = lit;
		    $$->set_location(@1.first_line, @1.first_column, yytext);
		}
	|	STR
		{
		    auto * lit = new asw::slc::literal();
                    std::string as_string = std::string($1);
                    free($1);
                    as_string.erase(std::begin(as_string));
                    as_string.pop_back();
		    lit->set_value(as_string);
		    lit->set_type(asw::slc::type_id::STRING);
		    $$ = lit;
		    /* minus 1 to include the quote character */
		    $$->set_location(@1.first_line, @1.first_column - 1, yytext);
		}
	|	NIL
		{
		    auto * lit = new asw::slc::literal();
		    lit->set_type(asw::slc::type_id::NIL);
		    $$ = lit;
		    $$->set_location(@1.first_line, @1.first_column, yytext);
		}
	;

%%

void yyerror(YYLTYPE * locp, asw::slc::node *, char const * msg) {
  fprintf(stderr, "error: %d:%d: '%s'\n", locp->first_line, locp->first_column, msg);
  exit(1);
}
