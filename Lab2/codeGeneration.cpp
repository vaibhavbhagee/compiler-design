#include "codeGeneration.hpp"

/*
 * CONTAINS CODE RELATED TO CODE GENERATION
 * 
 * REQUIRES:
 * A) VARIABLES AND FUNCTIONS DECLARED BEFORE USE
 * B) TYPE CHECKING HAS BEEN DONE
 *
 */

// global module
LLVMModuleRef mod;

// symbol tables - function names, function args, local/global variables
std::stack< std::map<std::string, FUNCTION_TYPE> > funcSymTable;
std::stack< std::map<std::string, VALUE_TYPE> > argSymTable;
std::stack< std::map<std::string, VALUE_TYPE> > symTable;
std::stack< std::map<std::string, LLVMTypeRef> > arrSymTable;

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

LLVMTypeRef searchInArrTable(std::string varName, 
	std::stack< std::map<std::string, LLVMTypeRef> > &symTable, bool stopFirst=false) {

	if (symTable.empty())
		return NULL;

	std::map< std::string, LLVMTypeRef> topContext = symTable.top();

	if (topContext.find(varName) != topContext.end())
		return topContext[varName];
	else {
		if (stopFirst) {
			return NULL;
		}
		symTable.pop();
		LLVMTypeRef found = searchInArrTable(varName, symTable);
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

LLVMTypeRef stringToLLVMType(std::string typeName, LLVMContextRef c) {
	if (typeName == "INT") return LLVMInt32TypeInContext(c);
	else if (typeName == "FLOAT") return LLVMFloatTypeInContext(c);
	else if (typeName == "VOID") return LLVMVoidTypeInContext(c);
	else if (typeName == "CHAR") return LLVMInt8TypeInContext(c); //TODO: Check this
	else if (typeName.find("*") != std::string::npos) {
		typeName.pop_back();
		LLVMTypeRef base = stringToLLVMType(typeName, c);
		return LLVMPointerType(base, 0);
	}
	else return NULL;
}

VALUE_TYPE loadValueifNeeded(treeNode* node, VALUE_TYPE prev_val) {
	// loads the value if its an id or a dereference, to be used for lhs vs rhs issues
	LLVMBuilderRef currBuilder = builderStack.top();

	// check for node variable and load accordingly
	if (node->type == "Ident" && node->children.size() == 0) {

		std::string varName = ((IdentNode*)(node))->name;

		// Search in variable sym table
		VALUE_TYPE foundVal = searchInTable(varName, symTable);
		if (foundVal != NULL) {	// Found in variable sym table
			std::string tag = "load_" + varName + "_";
			return LLVMBuildLoad(currBuilder, prev_val, tag.c_str());
		}
	}
	//load array value
	else if (node->type == "Ident" && node->children[0]->type == "[ ]") {
		LLVMValueRef index = node->children[0]->children[0]->codegen();

		std::string name = ((IdentNode*)node)->name;
		std::string tag = name + "_" + std::to_string(
			((ConstNode*)node->children[0]->children[0])->ival) + "_";

		// return LLVMBuildExtractValue(currBuilder, prev_val, index, tag.c_str());
		LLVMValueRef element_ptr = LLVMBuildInBoundsGEP(currBuilder, prev_val, &index, 1, tag.c_str());
		
		LLVMTypeRef array_type = LLVMTypeOf(prev_val);
		LLVMTypeRef elem_type = searchInArrTable(name, arrSymTable);
		LLVMTypeRef elem_type_ptr = LLVMPointerType(elem_type, 0);

		LLVMValueRef element_ptr_actual = LLVMBuildBitCast(currBuilder, element_ptr, elem_type_ptr, "cast");
		
		return LLVMBuildLoad(currBuilder, element_ptr_actual, "array_deref_");
	}
	// load pointer value
	else if (node->type == "DEREF") {
		return LLVMBuildLoad(currBuilder, prev_val, "ptr_deref");
	}

	return prev_val;
}

VALUE_TYPE codegenBinExp(treeNode* lhs, treeNode* rhs, LLVMOpcode Op, bool logical=false) {

	LLVMBuilderRef currBuilder = builderStack.top();

	LLVMValueRef lhsVal = lhs->codegen();
	lhsVal = loadValueifNeeded(lhs, lhsVal);
	
	LLVMValueRef rhsVal = rhs->codegen();
	rhsVal = loadValueifNeeded(rhs, rhsVal);

	if (LLVMGetTypeKind(LLVMTypeOf(lhsVal)) == LLVMFloatTypeKind &&
	 LLVMGetTypeKind(LLVMTypeOf(rhsVal)) == LLVMFloatTypeKind && !logical){

		return LLVMBuildBinOp(currBuilder, ((LLVMOpcode)(Op + 1)), lhsVal, rhsVal, "_flt_op");
	}
	else {
		return LLVMBuildBinOp(currBuilder, Op, lhsVal, rhsVal, "_int_op");
	}
}

VALUE_TYPE constShift(treeNode* node, ConstNode* cons, LLVMOpcode Op) {

	LLVMBuilderRef currBuilder = builderStack.top();

	LLVMValueRef constVal = cons->codegen();
	std::string varName = ((IdentNode*)(node))->name;
	LLVMValueRef Ident = node->codegen();
	LLVMValueRef IdentVal = loadValueifNeeded(node, Ident);

	std::string tag = varName + "_const_op";
	LLVMValueRef var_incr = LLVMBuildBinOp(currBuilder, Op, IdentVal, constVal, tag.c_str());

	return LLVMBuildStore(currBuilder, var_incr, Ident);
}


VALUE_TYPE codegenCondExp(treeNode* lhs, treeNode* rhs, int Op, bool logical=false) {

	LLVMBuilderRef currBuilder = builderStack.top();

	LLVMValueRef lhsVal = lhs->codegen();
	lhsVal = loadValueifNeeded(lhs, lhsVal);
	
	LLVMValueRef rhsVal = rhs->codegen();
	rhsVal = loadValueifNeeded(rhs, rhsVal);

	if (LLVMGetTypeKind(LLVMTypeOf(lhsVal)) == LLVMFloatTypeKind &&
	 LLVMGetTypeKind(LLVMTypeOf(rhsVal)) == LLVMFloatTypeKind && !logical){

		return LLVMBuildFCmp(currBuilder, toRealPred[Op], lhsVal, rhsVal, "flt_cmp");
	}
	else{
		return LLVMBuildICmp(currBuilder, toIntPred[Op], lhsVal, rhsVal, "int_cmp");
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
	else if (type == "RETURN") { //TODO: Check if you have to load here or return as is
		
		LLVMValueRef rhsVal = children[0]->codegen();
		rhsVal = loadValueifNeeded(children[0], rhsVal);

		return rhsVal;
	}
	else if (type == "ASSIGN") {
		std::string varName = ((IdentNode*)children[0])->name;
		LLVMValueRef varPlace = children[0]->codegen();

		LLVMValueRef rhsVal = children[1]->codegen();
		rhsVal = loadValueifNeeded(children[1], rhsVal);

		if (children[0]->children.size() > 0 && children[0]->children[0]->type == "[ ]") { // storage in array
			ConstNode* indexNode = ((ConstNode*)children[0]->children[0]->children[0]);
			std::string ind = std::to_string(indexNode->ival);
			LLVMValueRef index = indexNode->codegen();

			std::string tag = varName + "_" + ind + "_";
			LLVMValueRef element_ptr = LLVMBuildInBoundsGEP(currBuilder, varPlace, &index, 1, tag.c_str());
			
			LLVMTypeRef array_type = LLVMTypeOf(varPlace);
			LLVMTypeRef elem_type = searchInArrTable(varName, arrSymTable);
			LLVMTypeRef elem_type_ptr = LLVMPointerType(elem_type, 0);
			LLVMValueRef element_ptr_actual = LLVMBuildBitCast(currBuilder, element_ptr, elem_type_ptr, "cast");
			
			return LLVMBuildStore(currBuilder, rhsVal, element_ptr_actual);
			// return LLVMBuildInsertElement(currBuilder, varPlace, rhsVal, index, tag.c_str());
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
	else if (type == "CONDITION") {
		return children[0]->codegen();
	}
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
	else if (type == "GEQ") {
		return codegenCondExp(children[0], children[1], 4);
	}
	else if (type == "LEQ") {
		return codegenCondExp(children[0], children[1], 5);
	}

	// unary operators
	else if (type == "INC") {
		ConstNode* temp = new ConstNode(1);
		return constShift(children[0], temp, LLVMAdd);
	}
	else if (type == "DEC") {
		ConstNode* temp = new ConstNode(1);
		return constShift(children[0], temp, LLVMSub);
	}

	// pointer ops
	else if (type == "DEREF") {
		LLVMValueRef derefVal = children[0]->codegen();
		derefVal = loadValueifNeeded(children[0], derefVal);
		return derefVal;
	}
	else if (type == "REF") {
		LLVMValueRef refVal = children[0]->codegen();
		return refVal;
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
		arrSymTable.top()[varName] = type;

		return allocate;
	}

	LLVMBuilderRef currBuilder = builderStack.top();
	std::string varName = ((IdentNode*)children[0])->name;
	int arrayLen = ((ConstNode*)children[1])->ival;
	LLVMTypeRef arrayType = LLVMArrayType(type, arrayLen); // TODO: See about context

	// Trial: Get function header entry
	LLVMBasicBlockRef curBlk = LLVMGetInsertBlock(currBuilder); // TODO: Check if this gives the current block
	LLVMValueRef parentFuncHeader = LLVMGetBasicBlockParent(curBlk);

	// TODO: Look at the third parameter
	LLVMValueRef allocate = LLVMBuildArrayAlloca(currBuilder, arrayType, NULL /*This could probably be the function header value*/, varName.c_str());

	symTable.top()[varName] = allocate;
	arrSymTable.top()[varName] = type;

	return allocate;	

	// return NULL;
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

				std::string tag = identName + "_ret_";
				return LLVMBuildCall(currBuilder, foundInFunc, temp, 0, tag.c_str());
			} 
			else {
				// Function takes args

				std::vector<LLVMValueRef> codeForIdents;

				for(auto child: children[0]->children) { // TODO: ADD LOAD RELATED INFO HERE AS WELL
					LLVMValueRef generated_code = child->codegen();

					generated_code = loadValueifNeeded(child, generated_code);
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
		bool isVariadic = ((ParamNode*)children[1])->isVariadic;

		for (auto child : children[1]->children) {
			std::string childType = child->children[0]->type;
			fparams.push_back(stringToLLVMType(childType, currContext));
		}

		LLVMTypeRef* paramTypeList = fparams.data();

		LLVMTypeRef retType = LLVMFunctionType(type, paramTypeList, fparams.size(), isVariadic);
	    LLVMValueRef funcDecl = LLVMAddFunction(mod, funcName.c_str(), retType);

	    // Add to function symbol table
	    funcSymTable.top()[funcName] = funcDecl;

	    return funcDecl;
	}
}

VALUE_TYPE BranchNode::codegen(LLVMTypeRef retType) {

	LLVMBuilderRef currBuilder = builderStack.top();
	LLVMContextRef currContext = contextStack.top();

	LLVMBasicBlockRef curBlk = LLVMGetInsertBlock(currBuilder); // TODO: Check if this gives the current block
	LLVMValueRef parentFuncHeader = LLVMGetBasicBlockParent(curBlk);

	if (type == "IF") {

		LLVMBasicBlockRef endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[1])->codegen(retType, endif);

		LLVMPositionBuilderAtEnd(currBuilder, curBlk); // add to the current block
		LLVMValueRef conditionCode = children[0]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, endif);		

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}
	else if (type == "ITE") {

		LLVMBasicBlockRef endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then
		LLVMBasicBlockRef else_blk = LLVMAppendBasicBlock(parentFuncHeader, "else"); // basic block for else

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[1])->codegen(retType, endif);

		LLVMPositionBuilderAtEnd(currBuilder, else_blk); // generated code for else
		((CondBlockNode*)children[2])->codegen(retType, endif);

		LLVMPositionBuilderAtEnd(currBuilder, curBlk); // add to the current block
		LLVMValueRef conditionCode = children[0]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, else_blk);		

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}
	else if (type == "WHILE") {

		LLVMBasicBlockRef endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then
		LLVMBasicBlockRef else_blk = LLVMAppendBasicBlock(parentFuncHeader, "else"); // basic block for else

		// jump to the else block first
		LLVMBuildBr(currBuilder, else_blk);

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[1])->codegen(retType, else_blk); // goes back to the if condition again

		LLVMPositionBuilderAtEnd(currBuilder, else_blk); // add to the else block
		LLVMValueRef conditionCode = children[0]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, endif);		

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}
	else if (type == "DO-WHILE") {

		LLVMBasicBlockRef endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then
		LLVMBasicBlockRef else_blk = LLVMAppendBasicBlock(parentFuncHeader, "else"); // basic block for else

		// jump to the then block first
		LLVMBuildBr(currBuilder, then_blk);

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[0])->codegen(retType, else_blk); // goes back to the if condition again

		LLVMPositionBuilderAtEnd(currBuilder, else_blk); // add to the else block
		LLVMValueRef conditionCode = children[1]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, endif);		

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}
	else if (type == "FOR") {

		//Initialization expression
		children[0]->codegen();

		// Now code for the actual loop
		LLVMBasicBlockRef endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then
		LLVMBasicBlockRef else_blk = LLVMAppendBasicBlock(parentFuncHeader, "else"); // basic block for else
		LLVMBasicBlockRef increment_blk = LLVMAppendBasicBlock(parentFuncHeader, "increment"); // basic block for increment

		// jump to the else block first
		LLVMBuildBr(currBuilder, else_blk);

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[3])->codegen(retType, increment_blk); // goes back to the if condition again

		LLVMPositionBuilderAtEnd(currBuilder, else_blk); // add to the else block
		LLVMValueRef conditionCode = children[1]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, endif);	

		LLVMPositionBuilderAtEnd(currBuilder, increment_blk); // add to the increment block
		children[2]->codegen(); // update expression
		LLVMBuildBr(currBuilder, else_blk);

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}

	return NULL;
}

void callBlockInst(treeNode* t, LLVMBuilderRef currBuilder, LLVMTypeRef retType) {
	for (int i = 0; i < t->children.size(); ++i) {
		std::string childType = t->children[i]->type;

		if (childType == "RETURN") {
			if (retType == LLVMVoidType())
				LLVMBuildRetVoid(currBuilder);
			else {
				LLVMBuildRet(currBuilder, t->children[i]->codegen()); //TODO: Update if return format changes
			}
		}
		else {
			// TODO: Write code for all statements here
			if (childType == "decl") {
				((DeclNode*)t->children[i])->codegen(false);
			}
			else if (childType == "ASSIGN") {
				t->children[i]->codegen();
			}
			else if (childType == "IF" || childType == "ITE" || childType == "WHILE" || childType == "DO-WHILE" || childType == "FOR") {
				((BranchNode*)t->children[i])->codegen(retType);
			}
			else { // binops and unary ops
				t->children[i]->codegen();
			}
		}
	}
}

VALUE_TYPE FuncBlockNode::codegen(LLVMTypeRef retType, LLVMValueRef funcHeader) {

	LLVMBuilderRef currBuilder = builderStack.top();
	LLVMContextRef currContext = contextStack.top();

	callBlockInst((treeNode*)this, currBuilder, retType);

	return NULL;
}

VALUE_TYPE CondBlockNode::codegen(LLVMTypeRef retTypeIfReqd, LLVMBasicBlockRef afterDest) {

	LLVMBuilderRef currBuilder = builderStack.top();
	LLVMContextRef currContext = contextStack.top();

	callBlockInst((treeNode*)this, currBuilder, retTypeIfReqd);

	LLVMBuildBr(currBuilder, afterDest);

	return NULL;
}

void codegen(treeNode* AST) {
	LLVMContextRef globalContext = LLVMGetGlobalContext();
	LLVMBuilderRef globalBuilder = LLVMCreateBuilderInContext(globalContext);
	std::map<std::string, VALUE_TYPE> variableMap;
	std::map<std::string, LLVMTypeRef> arrTypeMap;
	std::map<std::string, VALUE_TYPE> argsMap;
	std::map<std::string, FUNCTION_TYPE> functionMap;

	contextStack.push(globalContext);
	builderStack.push(globalBuilder);
	symTable.push(variableMap);
	arrSymTable.push(arrTypeMap);
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

    LLVMContextRef stackContext = contextStack.top();
	LLVMBuilderRef stackBuilder = builderStack.top();

	contextStack.pop();
	builderStack.pop();
	symTable.pop();
	arrSymTable.pop();
	argSymTable.pop();
	funcSymTable.pop();

    LLVMDisposeBuilder(globalBuilder);
    LLVMContextDispose(globalContext);
}