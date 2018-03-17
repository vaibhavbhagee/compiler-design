#include "codeGeneration.hpp"

/*
 * CONTAINS CODE RELATED TO CODE GENERATION
 * 
 * REQUIRES:
 * A) VARIABLES AND FUNCTIONS DECLARED BEFORE USE
 * B) TYPE CHECKING HAS BEEN DONE
 *
 */

// symbol tables - function names, function args, local/global variables
std::stack< std::map<std::string, FUNCTION_TYPE> > funcSymTable;
std::stack< std::map<std::string, VALUE_TYPE> > argSymTable;
std::stack< std::map<std::string, VALUE_TYPE> > symTable;

// stack of contexts/builders - operate in concert
std::stack<LLVMContextRef> contextStack;
std::stack<LLVMBuilderRef> builderStack;

// conditional operators
LLVMIntPredicate toIntPred[6] = {LLVMIntEQ, LLVMIntNE, LLVMIntSGT, LLVMIntSLT, LLVMIntSGE, LLVMIntSLE};
LLVMRealPredicate toRealPred[6] = {LLVMRealOEQ, LLVMRealONE, LLVMRealOGT, LLVMRealOLT, LLVMRealOGE, LLVMRealOLE};


VALUE_TYPE searchInTable(std::string varName, 
	std::stack< std::map<std::string, VALUE_TYPE> > &symTable, bool stopFirst=false) {

	if (symTable.empty())
		return NULL;

	std::map< std::string, VALUE_TYPE> topContext = symTable.top();

	if (topContext.find(varName) != topContext.end())
		return topContext[varName];
	else {
		if (stopFirst) {
			return NULL;
		}
		symTable.pop();
		VALUE_TYPE found = searchInTable(varName, symTable);
		symTable.push(topContext);
		return found;
	}
}

FUNCTION_TYPE searchInFuncSymTable(std::string varName, std::stack< std::map<std::string, VALUE_TYPE> > &funcSymTable) {

	std::map< std::string, FUNCTION_TYPE> topContext = funcSymTable.top();

	if (topContext.find(varName) != topContext.end())
		return topContext[varName];
	else {
		return NULL;
	}
}


LLVMModuleRef mod;

LLVMTypeRef stringToLLVMType(std::string typeName, LLVMContextRef c) {
	if (typeName == "INT") return LLVMInt32TypeInContext(c);
	else if (typeName == "FLOAT") return LLVMFloatTypeInContext(c);
	else if (typeName == "VOID") return LLVMVoidTypeInContext(c);
	else if (typeName == "CHAR") return LLVMInt8TypeInContext(c); //TODO: Check this
	else return NULL;
}


VALUE_TYPE codegenBinExp(treeNode* lhs, treeNode* rhs, LLVMOpcode Op, bool logical=false) {

	LLVMBuilderRef currBuilder = builderStack.top();

	LLVMValueRef lhsVal = lhs->codegen();
	LLVMValueRef rhsVal = rhs->codegen();

	// printf("%d %d\n",LLVMGetTypeKind(LLVMTypeOf(lhsVal)) == LLVMIntegerTypeKind,
	//   LLVMGetTypeKind(LLVMTypeOf(rhsVal)) == LLVMIntegerTypeKind);

	if ((LLVMGetTypeKind(LLVMTypeOf(lhsVal)) == LLVMIntegerTypeKind &&
	 LLVMGetTypeKind(LLVMTypeOf(rhsVal)) == LLVMIntegerTypeKind) || logical) {

		return LLVMBuildBinOp(currBuilder, Op, lhsVal, rhsVal, "int_op");
	}
	else {
		return LLVMBuildBinOp(currBuilder, ((LLVMOpcode)(Op + 1)), lhsVal, rhsVal, "flt_op");
	}
}


VALUE_TYPE codegenCondExp(treeNode* lhs, treeNode* rhs, int Op, bool logical=false) {

	LLVMBuilderRef currBuilder = builderStack.top();

	LLVMValueRef lhsVal = lhs->codegen();
	LLVMValueRef rhsVal = rhs->codegen();

	if ((LLVMGetTypeKind(LLVMTypeOf(lhsVal)) == LLVMIntegerTypeKind &&
	 LLVMGetTypeKind(LLVMTypeOf(rhsVal)) == LLVMIntegerTypeKind) || logical) {

		return LLVMBuildICmp(currBuilder, toIntPred[Op], lhsVal, rhsVal, "int_cmp");
	}
	else {
		return LLVMBuildFCmp(currBuilder, toRealPred[Op], lhsVal, rhsVal, "flt_cmp");
	}
}


VALUE_TYPE treeNode::codegen() {

	LLVMContextRef currContext = contextStack.top();
	LLVMBuilderRef currBuilder = builderStack.top();

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
		
		std::map<std::string, VALUE_TYPE> newVariableMap;

		std::vector<std::string> fparams;
		
		LLVMTypeRef funcRetType = stringToLLVMType(children[0]->type, currContext);
		FUNCTION_TYPE funcHeader = ((FunctionNode*)children[1])->codegen( true, funcRetType );

		int index = 0;
		for (auto child : children[1]->children[1]->children) { 
			std::string varName = (((IdentNode*)child->children[1])->name);
			newVariableMap[varName] = LLVMGetParam(funcHeader, index++);
		}

		argSymTable.push(newVariableMap);

		LLVMBasicBlockRef entry = LLVMAppendBasicBlock(funcHeader, "entry");
		LLVMPositionBuilderAtEnd(currBuilder, entry);

		((FuncBlockNode*)children[2])->codegen( funcRetType, funcHeader );

		argSymTable.pop();

	    return NULL;
	}
	else if (type == "RETURN") { //TODO: Check if you have to led here or return as is
		
		LLVMValueRef rhsVal = children[0]->codegen(); //TODO: Codegen is called in a similar way

		std::string rightType = children[0]->type;

		if (rightType == "Ident") {
			std::string identName = ((IdentNode*)children[0])->name;

			if (searchInTable(identName, symTable) != NULL){// is a local/global variable
				rhsVal = LLVMBuildLoad(currBuilder, rhsVal, "load-temp");
			}
		}

		return rhsVal;
	}
	else if (type == "ASSIGN") { // TODO: Check where to load and where to return as is
		LLVMValueRef varPlace = children[0]->codegen();
		LLVMValueRef rhsVal = children[1]->codegen(); //TODO: Codegen is called in a similar way

		std::string rightType = children[1]->type;

		if (rightType == "Ident") {
			std::string identName = ((IdentNode*)children[1])->name;

			if (searchInTable(identName, symTable) != NULL){
				rhsVal = LLVMBuildLoad(currBuilder, rhsVal, "load-temp");
			}
		}

		return LLVMBuildStore(currBuilder, rhsVal, varPlace); 
	}
	// binary ops
	else if (type == "PLUS") {
		return codegenBinExp(children[0], children[1], LLVMAdd);
	}
	else if (type == "MINUS") {
		return codegenBinExp(children[0], children[1], LLVMSub);
	}
	else if (type == "MULT") {
		return codegenBinExp(children[0], children[1], LLVMMul);
	}
	else if (type == "DIV") {
		return codegenBinExp(children[0], children[1], LLVMSDiv);
	}
	else if (type == "MOD") {
		return codegenBinExp(children[0], children[1], LLVMSRem);
	}
	else if (type == "OR") {
		return codegenBinExp(children[0], children[1], LLVMOr, true);
	}
	else if (type == "AND") {
		return codegenBinExp(children[0], children[1], LLVMAnd, true);
	}
	else if (type == "XOR") {
		return codegenBinExp(children[0], children[1], LLVMXor, true);
	}

	// condition operators
	else if (type == "EQ") {
		return codegenCondExp(children[0], children[1], 0);
	}
	else if (type == "NEQ") {
		return codegenCondExp(children[0], children[1], 1);
	}
	else if (type == "GT") {
		return codegenCondExp(children[0], children[1], 2);
	}
	else if (type == "LT") {
		return codegenCondExp(children[0], children[1], 3);
	}
	else if (type == "GTE") {
		return codegenCondExp(children[0], children[1], 4);
	}
	else if (type == "LTE") {
		return codegenCondExp(children[0], children[1], 5);
	}

	// unary operators
	else if (type == "INC") {
		ConstNode* temp = new ConstNode(1);
		return codegenBinExp(children[0], temp, LLVMAdd, true);
	}
	else if (type == "DEC") {
		ConstNode* temp = new ConstNode(1);
		return codegenBinExp(children[0], temp, LLVMSub, true);
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

VALUE_TYPE IdentNode::codegen() {
	std::string identName = name;
	LLVMBuilderRef currBuilder = builderStack.top();

	// Search in variable sym table
	VALUE_TYPE foundVal = searchInTable(identName, symTable);

	if (foundVal != NULL) {	// Found in variable sym table
		return foundVal;
	}

	// Search in function args - only the first level
	foundVal = searchInTable(identName, argSymTable, true);

	if (foundVal != NULL) {	// its a function argument
		return foundVal;
	}
	else {
		// check in func sym table

		VALUE_TYPE foundInFunc = searchInFuncSymTable(identName, funcSymTable);

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
					LLVMValueRef generated_code = child->codegen();

					if (child->children.size() == 0) { // No more children => a variable
						std::string varName = ((IdentNode*)(child))->name;
						VALUE_TYPE foundVal = searchInTable(varName, symTable);
						if (foundVal != NULL) {
							generated_code = LLVMBuildLoad(currBuilder, generated_code, "load-temp");
						}
					}
					codeForIdents.push_back(generated_code);
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
			else if (childType == "ASSIGN") {
				children[i]->codegen();
			}
			else { // binops and unary ops
				children[i]->codegen();
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
	std::map<std::string, VALUE_TYPE> argsMap;
	std::map<std::string, FUNCTION_TYPE> functionMap;

	contextStack.push(globalContext);
	builderStack.push(globalBuilder);
	symTable.push(variableMap);
	argSymTable.push(argsMap);
	funcSymTable.push(functionMap);

	mod = LLVMModuleCreateWithNameInContext("my_module", globalContext);

	AST->codegen();

	FILE *f = fopen("generated_code.txt", "w");
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
	argSymTable.pop();
	funcSymTable.pop();

    LLVMDisposeBuilder(globalBuilder);
    LLVMContextDispose(globalContext);
}