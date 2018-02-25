%{
#include <cstdio>
#include <iostream>
#include "treeNode.hpp"
using namespace std;

// stuff from flex that bison needs to know about:
extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;
 
void yyerror(const char *s);

treeNode ASTree("ROOT", 0);
%}

%define api.value.type {struct treeNode}

%token	IDENTIFIER I_CONSTANT F_CONSTANT STRING_LITERAL FUNC_NAME SIZEOF
%token	PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token	AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token	SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token	XOR_ASSIGN OR_ASSIGN
%token	TYPEDEF_NAME ENUMERATION_CONSTANT

%token	TYPEDEF EXTERN STATIC AUTO REGISTER INLINE
%token	CONST RESTRICT VOLATILE
%token	BOOL CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE VOID
%token	COMPLEX IMAGINARY 
%token	STRUCT UNION ENUM ELLIPSIS

%token	CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token	ALIGNAS ALIGNOF ATOMIC GENERIC NORETURN STATIC_ASSERT THREAD_LOCAL

%start start_state
%%

start_state
	: translation_unit {ASTree.children.push_back($1);}
	;

translation_unit
	: external_declaration {treeNode temp = treeNode("tu"); temp.children.push_back($1); $$ = temp;}
	| translation_unit external_declaration {treeNode temp = treeNode("tu"); temp.children.push_back($1); temp.children.push_back($2); $$ = temp;}
	;

external_declaration
	: function_definition {treeNode temp = treeNode("ext"); $$ = temp;}
	| declaration {treeNode temp = treeNode("ext"); $$ = temp;}
	;

declaration
	: declaration_specifiers init_declarator_list ';'
	;

declaration_specifiers
	: storage_class_specifier type_specifier
	| type_specifier
	;

storage_class_specifier
	: EXTERN
	| STATIC
	| REGISTER
	;

type_specifier
	: VOID
	| CHAR
	| INT
	| FLOAT
	;

init_declarator_list
	: init_declarator
	| init_declarator_list ',' init_declarator
	;

init_declarator
	: declarator
	;

declarator
	: pointer direct_declarator
	| direct_declarator
	;

direct_declarator
	: IDENTIFIER
	| IDENTIFIER '[' I_CONSTANT ']'
	| IDENTIFIER '(' parameter_type_list ')'
	;

pointer
	: '*' pointer /* Check if multiple pointers need to be allowed*/
	| '*'
	;

function_definition
	: declaration_specifiers declarator compound_statement
	;

parameter_type_list
	: parameter_list
	| VOID
	;

parameter_list
	: parameter_declaration
	| parameter_list ',' parameter_declaration
	;

parameter_declaration
	: type_specifier IDENTIFIER
	| type_specifier pointer IDENTIFIER
	;

compound_statement
	: '{' '}'
	| '{'  block_item_list '}'
	;

block_item_list
	: block_item
	| block_item_list block_item
	;

block_item
	: declaration
	| statement
	;

statement
	: compound_statement
	| expression_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	;

expression_statement
	: ';'
	| expression ';'
	;

selection_statement
	: IF '(' expression ')' statement ELSE statement
	| IF '(' expression ')' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement
	| DO statement WHILE '(' expression ')' ';'
	| FOR '(' expression_statement expression_statement expression ')' statement
	;

jump_statement
	: RETURN ';'
	| RETURN expression ';'
	;

expression
	: assignment_expression
	;

assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression
	;

assignment_operator
	: '='
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression OR_OP logical_and_expression
	;

logical_and_expression
	: exclusive_or_expression
	| logical_and_expression AND_OP exclusive_or_expression
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression
	;

equality_expression
	: relational_expression
	| equality_expression EQ_OP relational_expression
	| equality_expression NE_OP relational_expression
	;

relational_expression
	: additive_expression
	| relational_expression '<' additive_expression
	| relational_expression '>' additive_expression
	| relational_expression LE_OP additive_expression
	| relational_expression GE_OP additive_expression
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
	;

multiplicative_expression
	: unary_expression
	| multiplicative_expression '*' unary_expression
	| multiplicative_expression '/' unary_expression
	| multiplicative_expression '%' unary_expression
	;

unary_expression
	: postfix_expression
	| INC_OP unary_expression
	| DEC_OP unary_expression
	| unary_operator unary_expression
	;

unary_operator
	: '&'
	| '*'
	| '+'
	| '-'
	| '!'
	;

postfix_expression
	: primary_expression
	| postfix_expression '[' expression ']'
	| postfix_expression '(' ')'
	| postfix_expression '(' argument_expression_list ')'
	| postfix_expression INC_OP
	| postfix_expression DEC_OP
	;

primary_expression
	: IDENTIFIER
	| constant
	| '(' expression ')'
	;

constant
	: I_CONSTANT		/* includes character_constant */
	| F_CONSTANT
	;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression
	;

%%
#include <stdio.h>

void yyerror(const char *s)
{
	fflush(stdout);
	fprintf(stderr, "*** %s\n", s);
}
