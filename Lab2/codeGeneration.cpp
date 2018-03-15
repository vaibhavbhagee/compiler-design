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
std::stack<LLVMBuilderRef> builderStack;

LLVMTypeRef stringToLLVMType(std::string typeName, LLVMContextRef c) {
	if (typeName == "INT") return LLVMInt32TypeInContext(c);
	else if (typeName == "FLOAT") return LLVMFloatTypeInContext(c);
	else if (typeName == "VOID") return LLVMVoidTypeInContext(c);
	else if (typeName == "CHAR") return LLVMInt32TypeInContext(c); //TODO: Check this
	else return NULL;
}

VALUE_TYPE treeNode::codegen() {

	// if (type == "start") {
	// 	// Call code gen for all the children
	// }

}

VALUE_TYPE DeclNode::codegen() {
	std::string type = children[0]->type;
	LLVMContextRef currContext = contextStack.top();
	return ((InitDeclNode*)children[1])->codegen( stringToLLVMType(type, currContext) );
}

VALUE_TYPE InitDeclNode::codegen(LLVMTypeRef type) {

}

VALUE_TYPE VariableNode::codegen() {

}

VALUE_TYPE ArrayNode::codegen() {

}

VALUE_TYPE PointerNode::codegen() {

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

FUNCTION_TYPE FunctionNode::code_generate() {

}

void codegen(treeNode* AST) {
	// AST->codegen();
}