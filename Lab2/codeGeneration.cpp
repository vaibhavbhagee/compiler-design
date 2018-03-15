#include "codeGeneration.hpp"

/*
 * CONTAINS CODE RELATED TO CODE GENERATION
 * 
 * REQUIRES:
 * A) VARIABLES AND FUNCTIONS DECLARED BEFORE USE
 * B) TYPE CHECKING HAS BEEN DONE
 *
 */

std::stack< std::map<std::string, FUNCTION_TYPE> > funcSymTable;
std::stack< std::map<std::string, VALUE_TYPE> > symTable;
std::stack<LLVMContextRef> contextStack;

VALUE_TYPE treeNode::codegen() {

	if (type == "start") {
		
	}

}

VALUE_TYPE IdentNode::codegen() {

}

VALUE_TYPE ConstNode::codegen() {

	if (name == "INT") {
		return LLVMConstInt(LLVMInt32TypeInContext(contextStack.top()), ival, false);
	}
	else if (name == "FLOAT") {
		return LLVMConstReal(LLVMFloatTypeInContext(contextStack.top()), fval);
	}
	else { // String constants
		return LLVMConstStringInContext(contextStack.top(), sval.c_str(), sval.length(), false);
	}

}

FUNCTION_TYPE FuncNode::code_generate() {

}

void codegen(treeNode* AST) {
	// AST->codegen();
}