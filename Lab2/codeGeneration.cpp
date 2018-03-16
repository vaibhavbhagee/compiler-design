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

LLVMModuleRef mod;

LLVMTypeRef stringToLLVMType(std::string typeName, LLVMContextRef c) {
	if (typeName == "INT") return LLVMInt32TypeInContext(c);
	else if (typeName == "FLOAT") return LLVMFloatTypeInContext(c);
	else if (typeName == "VOID") return LLVMVoidTypeInContext(c);
	else if (typeName == "CHAR") return LLVMInt8TypeInContext(c); //TODO: Check this
	else return NULL;
}

VALUE_TYPE treeNode::codegen() {

	// if (type == "start") {
	// 	// Call code gen for all the children
	// }

	for (int i = 0; i < children.size(); ++i)
	{
		if (children[i]->type == "decl") {
			((DeclNode*)children[i])->codegen(true);
		}
		// else if (children[i]->type == "FUNC") {
		// 	((ArrayNode*)children[i])->codegen(type);
		// }
		// else if (children[i]->type == "POINTER") {
		// 	((PointerNode*)children[i])->codegen(type);
		// }
		// else if (children[i]->type == "FUNCTION") {
		// 	((FunctionNode*)children[i])->codegen(type);
		// }
	}
	return NULL;

}

VALUE_TYPE DeclNode::codegen(bool isGlobalContext) {
	
	if (!isGlobalContext) {
		std::string type = children[0]->type;
		LLVMContextRef currContext = contextStack.top();
		return ((InitDeclNode*)children[1])->codegen( isGlobalContext, stringToLLVMType(type, currContext) );	
	}
	else {
		std::string type = children[0]->type;
		LLVMContextRef currContext = contextStack.top();
		return ((InitDeclNode*)children[1])->codegen( isGlobalContext, stringToLLVMType(type, currContext) );		
	}
}

VALUE_TYPE InitDeclNode::codegen(bool isGlobalContext, LLVMTypeRef type) {
	// Call children variable, array, pointer and function node codes from here

	for (int i = 0; i < children.size(); ++i)
	{
		if (children[i]->type == "VARIABLE") {
			((VariableNode*)children[i])->codegen(isGlobalContext, type);
		}
		else if (children[i]->type == "ARRAY") {
			((ArrayNode*)children[i])->codegen(isGlobalContext,type);
		}
		else if (children[i]->type == "POINTER") {
			((PointerNode*)children[i])->codegen(isGlobalContext, type);
		}
		else if (children[i]->type == "FUNCTION") {
			((FunctionNode*)children[i])->codegen(isGlobalContext, type);
		}
	}

	return NULL;
}

VALUE_TYPE VariableNode::codegen(bool isGlobalContext, LLVMTypeRef type) {

	if (isGlobalContext)
	{
		std::string varName = ((IdentNode*)children[0])->name;

		LLVMValueRef allocate = LLVMAddGlobal(mod, type, varName.c_str());

		// Add to the symbol table
		symTable.top()[varName] = allocate;
		return allocate;
	}

	LLVMBuilderRef currBuilder = builderStack.top();
	std::string varName = ((IdentNode*)children[0])->name;

	LLVMValueRef allocate = LLVMBuildAlloca(currBuilder, type, varName.c_str());

	// Add to the symbol table
	symTable.top()[varName] = allocate;

	return allocate;
}

VALUE_TYPE ArrayNode::codegen(bool isGlobalContext, LLVMTypeRef type) {

	if (isGlobalContext)
	{
		std::string varName = ((IdentNode*)children[0])->name;
		int arrayLen= ((ConstNode*)children[1])->ival;
		LLVMTypeRef arrayType = LLVMArrayType(type, arrayLen); // TODO: See about context

		// TODO: Look at the third parameter
		LLVMValueRef allocate = LLVMAddGlobal(mod, arrayType, varName.c_str());

		symTable.top()[varName] = allocate;

		return allocate;
	}

	// LLVMBuilderRef currBuilder = builderStack.top();
	// std::string varName = ((IdentNode*)children[0])->name;
	// int arrayLen= ((IdentNode*)children[1])->ival;
	// LLVMTypeRef arrayType = LLVMArrayType(type, arrayLen) // TODO: See about context

	// // TODO: Look at the third parameter
	// LLVMValueRef allocate = LLVMBuildArrayAlloca(currBuilder, arrayType, /*What is this?*/, varName.c_str());

	// symTable.top()[varName] = allocate;

	// return allocate;	

	return NULL;
}

VALUE_TYPE PointerNode::codegen(bool isGlobalContext, LLVMTypeRef type) {

	// TODO: Check about address space, Default is 0

	std::string childType = children[0]->type;

	if (childType == "POINTER") {
		LLVMTypeRef pointerType = LLVMPointerType(type, 0);

		return ((PointerNode*)children[0])->codegen(isGlobalContext, pointerType);
	}
	else if (childType == "VARIABLE") {
		LLVMTypeRef pointerType = LLVMPointerType(type, 0);

		return ((VariableNode*)children[0])->codegen(isGlobalContext, pointerType);	
	}
	else // FUNCTION
	{
		LLVMTypeRef pointerType = LLVMPointerType(type, 0);

		return ((FunctionNode*)children[0])->codegen(isGlobalContext, pointerType);
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

FUNCTION_TYPE FunctionNode::codegen(bool isGlobalContext, LLVMTypeRef type) {

}

void codegen(treeNode* AST) {
	LLVMContextRef globalContext = LLVMGetGlobalContext();
	LLVMBuilderRef globalBuilder = LLVMCreateBuilderInContext(globalContext);
	std::map<std::string, VALUE_TYPE> variableMap;
	std::map<std::string, FUNCTION_TYPE> functionMap;

	contextStack.push(globalContext);
	builderStack.push(globalBuilder);
	symTable.push(variableMap);
	funcSymTable.push(functionMap);

	mod = LLVMModuleCreateWithNameInContext("my_module", globalContext);

	// Trial
	// LLVMTypeRef param_types[] = {};
	// LLVMTypeRef ret_type = LLVMFunctionType(LLVMVoidType(), param_types, 0, 0);
	// LLVMValueRef sum = LLVMAddFunction(mod, "sum", ret_type);

	// LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");
	// LLVMPositionBuilderAtEnd(globalBuilder, entry);

	AST->codegen();

	// LLVMBuildRetVoid(globalBuilder);

	FILE *f = fopen("code.txt", "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	fprintf(f, "%s\n", LLVMPrintModuleToString(mod));

	fclose(f);

	// TRIAL END

    // printf("%s\n", LLVMPrintModuleToString(mod));

    LLVMContextRef stackContext = contextStack.top();
	LLVMBuilderRef stackBuilder = builderStack.top();

	contextStack.pop();
	builderStack.pop();
	symTable.pop();
	funcSymTable.pop();

    LLVMDisposeBuilder(globalBuilder);
    LLVMContextDispose(globalContext);
}