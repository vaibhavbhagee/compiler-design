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

VALUE_TYPE searchInSymTable(std::string varName);

LLVMModuleRef mod;

LLVMTypeRef stringToLLVMType(std::string typeName, LLVMContextRef c) {
	if (typeName == "INT") return LLVMInt32TypeInContext(c);
	else if (typeName == "FLOAT") return LLVMFloatTypeInContext(c);
	else if (typeName == "VOID") return LLVMVoidTypeInContext(c);
	else if (typeName == "CHAR") return LLVMInt8TypeInContext(c); //TODO: Check this
	else return NULL;
}

VALUE_TYPE treeNode::codegen() {

	if (type == "start") {
		
		// Call code gen for all the children
		for (int i = 0; i < children.size(); ++i)
		{
			if (children[i]->type == "decl") {
				((DeclNode*)children[i])->codegen(true);
			}
			else
				children[i]->codegen();
		}
		return NULL;
	}
	else if (type == "FUNC") {
		
		LLVMContextRef newContext = contextStack.top();
		LLVMBuilderRef newBuilder = builderStack.top();
		std::map<std::string, VALUE_TYPE> newVariableMap;

		std::vector<std::string> fparams;
		
		LLVMTypeRef funcRetType = stringToLLVMType(children[0]->type, newContext);
		FUNCTION_TYPE funcHeader = ((FunctionNode*)children[1])->codegen( true, funcRetType );

		int index = 0;
		for (auto child : children[1]->children[1]->children) { 
			std::string varName = (((IdentNode*)child->children[1])->name);

			// LLVMTypeRef paramType = stringToLLVMType(child->children[0]->type, contextStack.top());

			// newVariableMap[varName] = LLVMBuildAlloca(newBuilder, paramType, varName.c_str());
			newVariableMap[varName] = LLVMGetParam(funcHeader, index++);
		}

		symTable.push(newVariableMap);

		LLVMBasicBlockRef entry = LLVMAppendBasicBlock(funcHeader, "entry");
		LLVMPositionBuilderAtEnd(newBuilder, entry);

		((FuncBlockNode*)children[2])->codegen( funcRetType, funcHeader );

		symTable.pop();

	    return NULL;
	}
	else if (type == "RETURN") { //TODO: Check if you have to led here or return as is
		
		LLVMValueRef rhsVal = children[0]->codegen(); //TODO: Codegen is called in a similar way
		LLVMBuilderRef currBuilder = builderStack.top();

		std::string rightType = children[0]->type;

		if (rightType == "Ident") {
			std::string identName= ((IdentNode*)children[0])->name;
			if (searchInSymTable(identName) != NULL)
				rhsVal = LLVMBuildLoad(currBuilder, rhsVal, "load-temp");
		}

		return rhsVal;
	}
	else if (type == "ASSIGN") { // TODO: Check where to load and where to return as is
		LLVMValueRef varPlace = children[0]->codegen();
		LLVMValueRef rhsVal = children[1]->codegen(); //TODO: Codegen is called in a similar way

		LLVMBuilderRef currBuilder = builderStack.top();

		std::string rightType = children[1]->type;

		if (rightType == "Ident") {
			std::string identName= ((IdentNode*)children[1])->name;
			printf("%s\n", identName.c_str());
			if (searchInSymTable(identName) != NULL)
				rhsVal = LLVMBuildLoad(currBuilder, rhsVal, "load-temp");
		}

		return LLVMBuildStore(currBuilder, rhsVal, varPlace); 
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
		LLVMTypeRef arrayType = LLVMArrayType(type, arrayLen);

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

VALUE_TYPE searchInSymTable(std::string varName) {

	if (symTable.empty())
		return NULL;

	std::map< std::string, VALUE_TYPE> topContext = symTable.top();

	if (topContext.find(varName) != topContext.end())
		return topContext[varName];
	else {
		symTable.pop();

		VALUE_TYPE found = searchInSymTable(varName);

		symTable.push(topContext);

		return found;
	}
}

FUNCTION_TYPE searchInFuncSymTable(std::string varName) {

	std::map< std::string, FUNCTION_TYPE> topContext = funcSymTable.top();

	if (topContext.find(varName) != topContext.end())
		return topContext[varName];
	else {
		return NULL;
	}
}

VALUE_TYPE IdentNode::codegen() {
	std::string identName = name;
	LLVMBuilderRef currBuilder = builderStack.top();

	// Search in variable sym table
	VALUE_TYPE foundVal = searchInSymTable(identName);

	if (foundVal != NULL) {
		// Found in variable sym table

		return foundVal;
	}
	else {
		// check in func sym table

		VALUE_TYPE foundInFunc = searchInFuncSymTable(identName);

		if (foundInFunc != NULL) {
			// Iterate over all the children

			if (children.size() == 0) {
				// Func takes no args

				LLVMValueRef *temp;
				return LLVMBuildCall(currBuilder, foundInFunc, temp, 0, "ident_ret");
			} else {
				// Function takes args

				std::vector<LLVMValueRef> codeForIdents;

				for(auto child: children[0]->children) { // TODO: ADD LOAD RELATED INFO HERE AS WELL
					codeForIdents.push_back(child->codegen());
				}

				return LLVMBuildCall(currBuilder, foundInFunc, codeForIdents.data(), codeForIdents.size(), "ident_ret");
			}
		}
		else {
			return NULL;
		}
	}

	return NULL;
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

	std::string funcName = ((IdentNode*)children[0])->name;

	std::string childType = children[1]->type;

	std::map<std::string, FUNCTION_TYPE> topmostSymbolTable = funcSymTable.top();

	if (topmostSymbolTable.find(funcName) != topmostSymbolTable.end())
		return topmostSymbolTable[funcName];

	if (childType == "VOID") {
		// Return a function with no params
		LLVMTypeRef param_types[] = {};
	    LLVMTypeRef retType = LLVMFunctionType(type, param_types, 0, 0);
	    LLVMValueRef funcDecl = LLVMAddFunction(mod, funcName.c_str(), retType);

	    // Add to function symbol table
	    funcSymTable.top()[funcName] = funcDecl;

	    return funcDecl;
	}
	else {
		std::vector<LLVMTypeRef> fparams;
		LLVMContextRef currContext = contextStack.top();

		for (auto child : children[1]->children) { 
			fparams.push_back(stringToLLVMType(child->children[0]->type, currContext));
		}

		LLVMTypeRef* paramTypeList = fparams.data();

		LLVMTypeRef retType = LLVMFunctionType(type, paramTypeList, fparams.size(), 0);
	    LLVMValueRef funcDecl = LLVMAddFunction(mod, funcName.c_str(), retType);

	    // Add to function symbol table
	    funcSymTable.top()[funcName] = funcDecl;

	    return funcDecl;
	}
}

VALUE_TYPE FuncBlockNode::codegen(LLVMTypeRef retType, LLVMValueRef funcHeader) {

	LLVMBuilderRef currBuilder = builderStack.top();
	LLVMContextRef currContext = contextStack.top();

	for (int i = 0; i < children.size(); ++i)
	{
		std::string childType = children[i]->type;

		if (childType == "RETURN") {
			if (retType == LLVMVoidType())
				LLVMBuildRetVoid(currBuilder);
			else {
				LLVMBuildRet(currBuilder, children[i]->codegen()); //TODO: Update if return format changes
			}
		}
		else {
			// TODO: Write code for all statements here
			if (childType == "decl") {
				((DeclNode*)children[i])->codegen(false);
			}
			else if (childType == "ASSIGN")
			{
				(children[i]->codegen());
			}
		}
	}

	return NULL;
}

VALUE_TYPE CondBlockNode::codegen(LLVMTypeRef retTypeIfReqd, LLVMBasicBlockRef afterDest) {
	// Push a new context and builder, create a basic block ref

	// LLVMBasicBlockRef entry = LLVMAppendBasicBlock(funcHeader, "entry");

	LLVMBuilderRef currBuilder = builderStack.top();
	LLVMContextRef currContext = contextStack.top();

	for (int i = 0; i < children.size(); ++i)
	{
		std::string childType = children[i]->type;

		if (childType == "RETURN") {
			if (retTypeIfReqd == LLVMVoidType())
				LLVMBuildRetVoid(currBuilder);
			else
				children[i]->codegen(); //TODO: Update if return format changes
		}
		else ;
			// TODO: Write code for all statements here
	}

	LLVMBuildBr(currBuilder, afterDest);

	return NULL;
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