#include "codeGeneration.hpp"

/*
 * CONTAINS CODE RELATED TO CODE GENERATION
 * 
 * REQUIRES:
 * A) VARIABLES AND FUNCTIONS DECLARED BEFORE USE
 * B) TYPE CHECKING HAS BEEN DONE
 *
 */

std::stack< std::map<std::string, VALUE_TYPE> > contextStack;

VALUE_TYPE treeNode::codegen() {

}

VALUE_TYPE IdentNode::codegen() {

}

VALUE_TYPE ConstNode::codegen() {

}

FUNCTION_TYPE FuncNode::code_generate() {

}

void codegen(treeNode* AST) {
	// AST->codegen();
}