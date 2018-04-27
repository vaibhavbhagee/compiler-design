%code requires {
	#include <iostream>
	#include "treeNode.hpp"
}

%{
#include <cstdio>
#include <iostream>

#include "treeNode.hpp"

using namespace std;

// stuff from flex that bison needs to know about:
extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;
extern "C" int yylineno;
 
void yyerror(const char *s);

treeNode *ASTree = NULL;

%}

%locations

// All the semantic values symbols can have
%union {
	int ival;
	float fval;
	char* sval;
	treeNode* node;
	ConstNode* cnode;
	IdentNode* inode;
}

// semantic values of terminals
%token	<sval> IDENTIFIER STRING_LITERAL
%token  <ival> I_CONSTANT 
%token  <fval> F_CONSTANT 
%token  FUNC_NAME ELLIPSIS
%token 	SIZEOF
%token	PTR_OP LEFT_OP RIGHT_OP 
%token  INC_OP DEC_OP LE_OP GE_OP EQ_OP NE_OP
%token	AND_OP OR_OP 
%token  MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token	SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token	XOR_ASSIGN OR_ASSIGN
%token	TYPEDEF_NAME ENUMERATION_CONSTANT

%token	TYPEDEF AUTO INLINE
%token	CONST RESTRICT VOLATILE
%token  EXTERN STATIC REGISTER
%token  SHORT LONG SIGNED UNSIGNED DOUBLE
%token	BOOL CHAR INT FLOAT VOID
%token	COMPLEX IMAGINARY 
%token	STRUCT UNION ENUM

%token	CASE DEFAULT SWITCH GOTO CONTINUE BREAK 
%token IF ELSE WHILE DO FOR RETURN

%token	ALIGNAS ALIGNOF ATOMIC GENERIC NORETURN STATIC_ASSERT THREAD_LOCAL

// semantic values of non-terminals
%type <node> start_state translation_unit external_declaration declaration declaration_specifiers storage_class_specifier 
%type <node> type_specifier init_declarator_list init_declarator declarator direct_declarator pointer function_definition
%type <node> parameter_list parameter_declaration parameter_type_list block_item block_item_list argument_expression_list
%type <node> statement selection_statement iteration_statement compound_statement expression_statement expression jump_statement
%type <node> unary_operator unary_expression relational_expression additive_expression multiplicative_expression
%type <node> logical_or_expression logical_and_expression exclusive_or_expression equality_expression and_expression
%type <node> conditional_expression assignment_expression assignment_operator postfix_expression primary_expression
%type <node> constant

%start start_state
%%

start_state
	: translation_unit 							{treeNode *rootNode = new treeNode("start"); rootNode->children = $1->children; $$ = rootNode; ASTree = $$;}
	;

translation_unit
	: external_declaration 						{treeNode *temp = new treeNode("tu"); temp->children.push_back($1); $$ = temp;}
	| translation_unit external_declaration 	{	treeNode *temp = new treeNode("tu"); 
													for(int i = 0; i<$1->children.size(); i++)
													{
														temp->children.push_back($1->children[i]);
													} 
													temp->children.push_back($2); 
													$$ = temp;
												}
	;

external_declaration
	: function_definition 						{$$ = $1;}
	| declaration 								{$$ = $1;}
	;

declaration
	: declaration_specifiers init_declarator_list ';' 	{	
															DeclNode *temp = new DeclNode("decl"); 
															for (int i = 0; i<$1->children.size(); i++)
															{
																temp->children.push_back($1->children[i]);
															}
															temp->children.push_back($2); 
															$$ = temp;
														}
	;

declaration_specifiers
	: storage_class_specifier type_specifier 	{treeNode *temp = new treeNode("decl_spec"); temp->children.push_back($1); temp->children.push_back($2); $$ = temp;}
	| type_specifier						 	{treeNode *temp = new treeNode("decl_spec"); temp->children.push_back($1); $$ = temp;}
	;

storage_class_specifier
	: EXTERN 									{treeNode *temp = new treeNode("EXTERN"); $$ = temp;}
	| STATIC 									{treeNode *temp = new treeNode("STATIC"); $$ = temp;}
	| REGISTER  								{treeNode *temp = new treeNode("REGISTER"); $$ = temp;}
	;

type_specifier
	: VOID 										{treeNode *temp = new treeNode("VOID"); $$ = temp;}
	| CHAR 										{treeNode *temp = new treeNode("CHAR"); $$ = temp;}
	| INT 										{treeNode *temp = new treeNode("INT"); $$ = temp;}
	| FLOAT 									{treeNode *temp = new treeNode("FLOAT"); $$ = temp;}
	;

init_declarator_list
	: init_declarator 							{InitDeclNode *temp = new InitDeclNode("init_decl"); temp->children.push_back($1); $$ = temp;}
	| init_declarator_list ',' init_declarator  {
													InitDeclNode *temp = new InitDeclNode("init_decl"); 
													for (int i = 0; i<$1->children.size(); i++)
													{
														temp->children.push_back($1->children[i]);
													}
													temp->children.push_back($3); 
													$$ = temp;
												}
	;

init_declarator
	: declarator 								{$$ = $1;}
	;

declarator
	: pointer direct_declarator 				{
													treeNode *temp2 = $1;
													while (temp2->children.size()) {
														temp2 = temp2->children[0];
													}
													temp2->children.push_back($2); 
													$$ = $1;
												}
	| direct_declarator							{$$ = $1;}
	;

direct_declarator
	: IDENTIFIER								{IdentNode *temp = new IdentNode($1); VariableNode *arr = new VariableNode("VARIABLE"); arr->children.push_back(temp); $$ = arr;}
	| IDENTIFIER '[' I_CONSTANT ']'				{
													IdentNode *temp = new IdentNode($1);
													ConstNode *temp2 = new ConstNode($3);
													ArrayNode *arr = new ArrayNode("ARRAY");
													arr->children.push_back(temp); arr->children.push_back(temp2);
													$$ = arr;
												}
	| IDENTIFIER '(' parameter_type_list ')'	{
													IdentNode *temp = new IdentNode($1);
	 												FunctionNode *arr = new FunctionNode("FUNCTION");
													arr->children.push_back(temp); arr->children.push_back($3);
													$$ = arr;
		 										}
	;

pointer											
	: '*'										{PointerNode *temp = new PointerNode("POINTER"); $$ = temp;}
	| '*' pointer 								{PointerNode *temp = new PointerNode("POINTER"); temp->children.push_back($2); $$ = temp;}
	;

function_definition
	: declaration_specifiers declarator compound_statement 		{
																	treeNode *temp = new treeNode("FUNC"); 
																	for (int i = 0; i<$1->children.size(); i++)
																	{
																		temp->children.push_back($1->children[i]);
																	} 
																	temp->children.push_back($2); 
																	FuncBlockNode *fbn = new FuncBlockNode($3); 
																	temp->children.push_back(fbn); 
																	$$ = temp;
																}
	;

parameter_type_list
	: parameter_list ',' ELLIPSIS				{((ParamNode*)$1)->isVariadic = 1; $$ = $1;}
	| parameter_list 							{$$ = $1;}
	| VOID 										{treeNode *temp = new treeNode("VOID"); $$ = temp;}
	;

parameter_list
	: parameter_declaration  					{ParamNode *temp = new ParamNode("params"); temp->children.push_back($1); $$ = temp;}
	| parameter_list ',' parameter_declaration  {
													ParamNode *temp = new ParamNode("params"); 
													for (int i = 0; i<$1->children.size(); i++)
													{
														temp->children.push_back($1->children[i]);
													}
													temp->children.push_back($3); 
													$$ = temp;
												}
	;

parameter_declaration							
	: type_specifier IDENTIFIER					{treeNode *temp = new treeNode("param_decl"); temp->children.push_back($1); temp->children.push_back(new IdentNode($2)); $$ = temp;}
	| type_specifier pointer IDENTIFIER 		{
													treeNode *temp = new treeNode("param_decl");
													$1->type += "*";
													
													// add * to type
													treeNode *temp2 = $2;
													while (temp2->children.size()) {
														$1->type += "*";
														temp2 = temp2->children[0];
													}

													temp->children.push_back($1); 
													temp->children.push_back(new IdentNode($3)); 
													$$ = temp;
												}
	;

compound_statement
	: '{' '}'									{treeNode *temp = new treeNode("BLOCK"); $$ = temp;}
	| '{'  block_item_list '}'					{$$ = $2;}
	;

block_item_list
	: block_item 								{treeNode *temp = new treeNode("BLOCK"); temp->children.push_back($1); $$ = temp;}
	| block_item_list block_item 				{	treeNode *temp = new treeNode("BLOCK"); 
													for (int i = 0; i < $1->children.size(); i++) {
														temp->children.push_back($1->children[i]);
													} 
													temp->children.push_back($2); 
													$$ = temp;
												}
	;

block_item
	: declaration 								{$$ = $1;}
	| statement 								{$$ = $1;}
	;

statement
	: compound_statement 						{$$ = $1;}
	| expression_statement  					{$$ = $1;}
	| selection_statement  					 	{$$ = $1;}
	| iteration_statement   					{$$ = $1;}
	| jump_statement        					{$$ = $1;}
	;

expression_statement
	: ';'										{treeNode *temp = new treeNode(";"); $$ = temp;}
	| expression ';'							{$$ = $1;}
	;

selection_statement
	: IF '(' expression ')' compound_statement ELSE compound_statement  								{
																											BranchNode *temp = new BranchNode("ITE"); 
																											treeNode *temp2 = new treeNode("CONDITION"); 
																											temp2->children.push_back($3); 
																											temp->children.push_back(temp2); 

																											CondBlockNode *cbn5 = new CondBlockNode($5);
																											CondBlockNode *cbn7 = new CondBlockNode($7);
																											
																											temp->children.push_back(cbn5);  
																											temp->children.push_back(cbn7); 
																											$$ = temp;
																										}
	| IF '(' expression ')' compound_statement 				  								{
																								BranchNode *temp = new BranchNode("IF"); 
																								treeNode *temp2 = new treeNode("CONDITION"); 
																								temp2->children.push_back($3);  
																								temp->children.push_back(temp2); 

																								CondBlockNode *cbn5 = new CondBlockNode($5);
																								
																								temp->children.push_back(cbn5); 
																								$$ = temp;
																							}
	;

iteration_statement
	: WHILE '(' expression ')' compound_statement 											{
																								BranchNode *temp = new BranchNode("WHILE"); 
																								treeNode *temp2 = new treeNode("CONDITION"); 
																								temp2->children.push_back($3); 
																								temp->children.push_back(temp2); 

																								CondBlockNode *cbn5 = new CondBlockNode($5);
																								
																								temp->children.push_back(cbn5); 
																								$$ = temp;
																							}
	| DO compound_statement WHILE '(' expression ')' ';'										{
																									BranchNode *temp = new BranchNode("DO-WHILE"); 

																									CondBlockNode *cbn2 = new CondBlockNode($2);
																									temp->children.push_back(cbn2); 

																									treeNode *temp2 = new treeNode("CONDITION"); 
																									temp2->children.push_back($5); 
																									temp->children.push_back(temp2); 
																									$$ = temp;
																								}
	| FOR '(' expression_statement expression_statement expression ')' compound_statement   {
																								BranchNode *temp = new BranchNode("FOR"); 
																								temp->children.push_back($3);  
																								temp->children.push_back($4); 
																								temp->children.push_back($5);  

																								CondBlockNode *cbn7 = new CondBlockNode($7);
																								temp->children.push_back(cbn7); 
																								$$ = temp;
																							}
	;

jump_statement
	: RETURN ';'														{treeNode *temp = new treeNode("RETURN"); $$ = temp;}
	| RETURN expression ';'												{treeNode *temp = new treeNode("RETURN"); temp->children.push_back($2); $$ = temp;}
	;

expression
	: assignment_expression 											{$$ = $1;}
	;

assignment_expression
	: conditional_expression											{$$ = $1;}
	| unary_expression assignment_operator assignment_expression		{treeNode *temp = new treeNode("ASSIGN"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

assignment_operator	
	: '='																{treeNode *temp = new treeNode("="); $$ = temp;}
	;

conditional_expression
	: logical_or_expression												{$$ = $1;}
	| logical_or_expression '?' expression ':' conditional_expression   {treeNode *temp = new treeNode("conditional"); temp->children.push_back($1); temp->children.push_back($3); temp->children.push_back($5); $$ = temp;}
	;

logical_or_expression
	: logical_and_expression								{$$ = $1;}
	| logical_or_expression OR_OP logical_and_expression	{treeNode *temp = new treeNode("OR"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

logical_and_expression
	: exclusive_or_expression								{$$ = $1;}
	| logical_and_expression AND_OP exclusive_or_expression	{treeNode *temp = new treeNode("AND"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

exclusive_or_expression
	: and_expression 										{$$ = $1;}
	| exclusive_or_expression '^' and_expression 			{treeNode *temp = new treeNode("XOR"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

and_expression
	: equality_expression 									{$$ = $1;}
	| and_expression '&' equality_expression 				{treeNode *temp = new treeNode("&"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

equality_expression
	: relational_expression 								{$$ = $1;}
	| equality_expression EQ_OP relational_expression		{treeNode *temp = new treeNode("EQ"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	| equality_expression NE_OP relational_expression		{treeNode *temp = new treeNode("NEQ"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

relational_expression
	: additive_expression									{$$ = $1;}
	| relational_expression '<' additive_expression			{treeNode *temp = new treeNode("LT"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	| relational_expression '>' additive_expression 		{treeNode *temp = new treeNode("GT"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	| relational_expression LE_OP additive_expression 		{treeNode *temp = new treeNode("LEQ"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	| relational_expression GE_OP additive_expression 		{treeNode *temp = new treeNode("GEQ"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

additive_expression
	: multiplicative_expression								{$$ = $1;}
	| additive_expression '+' multiplicative_expression 	{treeNode *temp = new treeNode("PLUS"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	| additive_expression '-' multiplicative_expression 	{treeNode *temp = new treeNode("MINUS"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

multiplicative_expression
	: unary_expression										{$$ = $1;}
	| multiplicative_expression '*' unary_expression 		{treeNode *temp = new treeNode("MULT"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	| multiplicative_expression '/' unary_expression		{treeNode *temp = new treeNode("DIV"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	| multiplicative_expression '%' unary_expression		{treeNode *temp = new treeNode("MOD"); temp->children.push_back($1); temp->children.push_back($3); $$ = temp;}
	;

unary_expression
	: postfix_expression									{$$ = $1;}
	| INC_OP unary_expression								{treeNode *temp = new treeNode("INC"); temp->children.push_back($2); $$ = temp;}
	| DEC_OP unary_expression								{treeNode *temp = new treeNode("DEC"); temp->children.push_back($2); $$ = temp;}
	| unary_operator unary_expression						{$1->children.push_back($2); $$ = $1;}
	;

unary_operator
	: '&'													{treeNode *temp = new treeNode("REF"); $$ = temp;}
	| '*'													{treeNode *temp = new treeNode("DEREF"); $$ = temp;}
	| '+'													{treeNode *temp = new treeNode("+"); $$ = temp;}
	| '-'													{treeNode *temp = new treeNode("-"); $$ = temp;}
	| '!'													{treeNode *temp = new treeNode("NOT"); $$ = temp;}
	;

postfix_expression
	: primary_expression									{$$ = $1;}
	| postfix_expression '[' expression ']'					{treeNode *temp = new treeNode("[ ]"); temp->children.push_back($3); $1->children.push_back(temp); $$ = $1;}
	| postfix_expression '(' ')'							{$$ = $1;}
	| postfix_expression '(' argument_expression_list ')'	{$1->children.push_back($3); $$ = $1;}
	| postfix_expression INC_OP								{treeNode *temp = new treeNode("INC"); temp->children.push_back($1); $$ = temp;}
	| postfix_expression DEC_OP								{treeNode *temp = new treeNode("DEC"); temp->children.push_back($1); $$ = temp;}
	;

primary_expression
	: IDENTIFIER											{$$ = new IdentNode($1);}
	| constant 												{$$ = $1;}
	| '(' expression ')'									{$$ = $2;}
	;

constant
	: I_CONSTANT /* includes character_constant */			{ConstNode *temp = new ConstNode($1); $$ = temp;}
	| F_CONSTANT								  			{ConstNode *temp = new ConstNode($1); $$ = temp;}
	| STRING_LITERAL								  		{ConstNode *temp = new ConstNode($1); $$ = temp;}
	;

argument_expression_list
	: assignment_expression									{treeNode *temp = new treeNode("arg_list"); temp->children.push_back($1); $$ = temp;}
	| argument_expression_list ',' assignment_expression	{
																treeNode *temp = new treeNode("arg_list"); 
																for (int i = 0; i<$1->children.size(); i++)
																{
																	temp->children.push_back($1->children[i]);
																}
																temp->children.push_back($3); 
																$$ = temp;
															}
	;

%%
#include <stdio.h>

void yyerror(const char *s)
{
	fflush(stdout);
	fprintf(stderr, "%s in line %d\n", s, yylineno);
}
