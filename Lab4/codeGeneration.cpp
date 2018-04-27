#include "codeGeneration.hpp"
#include "localOpt.hpp"

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
LLVMBuilderRef globalBuilder;

// symbol tables - function names, function args, local/global variables
std::stack< std::map<std::string, FUNCTION_TYPE> > funcSymTable;
std::stack< std::map<std::string, VALUE_TYPE> > argSymTable;
std::stack< std::map<std::string, VALUE_TYPE> > symTable;
std::stack< std::map<std::string, VALUE_TYPE> > arrSymTable;

// stack of contexts/builders - operate in concert
std::stack<LLVMContextRef> contextStack;
std::stack<LLVMBuilderRef> builderStack;

// conditional operators
LLVMIntPredicate toIntPred[6] = {LLVMIntEQ, LLVMIntNE, LLVMIntSGT, LLVMIntSLT, LLVMIntSGE, LLVMIntSLE};
LLVMRealPredicate toRealPred[6] = {LLVMRealOEQ, LLVMRealONE, LLVMRealOGT, LLVMRealOLT, LLVMRealOGE, LLVMRealOLE};

VALUE_TYPE useArray(treeNode* node, VALUE_TYPE array);
VALUE_TYPE loadValueifNeeded(treeNode* node, VALUE_TYPE prev_val);


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

DATATYPE_TYPE searchInArrTable(std::string varName, 
	std::stack< std::map<std::string, DATATYPE_TYPE> > &symTable, bool stopFirst=false) {

	if (symTable.empty())
		return NULL;

	std::map< std::string, DATATYPE_TYPE> topContext = symTable.top();

	if (topContext.find(varName) != topContext.end())
		return topContext[varName];
	else {
		if (stopFirst) {
			return NULL;
		}
		symTable.pop();
		DATATYPE_TYPE found = searchInArrTable(varName, symTable);
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

DATATYPE_TYPE stringToLLVMType(std::string typeName, LLVMContextRef c) {
	if (typeName == "INT") return LLVMInt32TypeInContext(c);
	else if (typeName == "FLOAT") return LLVMFloatTypeInContext(c);
	else if (typeName == "VOID") return LLVMVoidTypeInContext(c);
	else if (typeName == "CHAR") return LLVMInt8TypeInContext(c); //TODO: Check this
	else if (typeName.find("*") != std::string::npos) {
		typeName.pop_back();
		DATATYPE_TYPE base = stringToLLVMType(typeName, c);
		return LLVMPointerType(base, 0);
	}
	else return NULL;
}


VALUE_TYPE useArray(treeNode* node, VALUE_TYPE array) {
	LLVMBuilderRef currBuilder = builderStack.top();

	// get the code for index and load if needed
	ConstNode* indexNode = (ConstNode*)(node->children[0]->children[0]);
	std::string ind = std::to_string(indexNode->ival);
	VALUE_TYPE index = indexNode->codegen();
	index = loadValueifNeeded(indexNode, index);

	// get the name and tag
	std::string name = ((IdentNode*)node)->name;
	std::string tag = name + "_" + ind + "_";

	// Check if access made from a pointer variable or an array variable
	VALUE_TYPE arrFoundVal = searchInTable(name, arrSymTable);
	VALUE_TYPE base_ptr = NULL;
	VALUE_TYPE element_ptr = NULL;

	if (arrFoundVal != NULL) { // get pointer to element of array at index 
		base_ptr = LLVMBuildStructGEP(currBuilder, array, 0, "_load_ptr_val");
		element_ptr = LLVMBuildInBoundsGEP(currBuilder, base_ptr, &index, 1, tag.c_str());
	}
	else {
		base_ptr = array;
		base_ptr = LLVMBuildLoad(currBuilder, array, "_load_ptr_val"); // loads the base pointer from pointer to pointer
		element_ptr = LLVMBuildGEP(currBuilder, base_ptr, &index, 1, tag.c_str()); // calculates the actual pointer of the offset
	}

	return element_ptr;
}

VALUE_TYPE loadValueifNeeded(treeNode* node, VALUE_TYPE prev_val) {
	// loads the value if its an id or a dereference, to be used for lhs vs rhs issues
	LLVMBuilderRef currBuilder = builderStack.top();

	// check for node variable and load accordingly
	if (node->type == "Ident" && node->children.size() == 0) {

		std::string varName = ((IdentNode*)node)->name;
		VALUE_TYPE arrFoundVal = searchInTable(varName, arrSymTable);

		if (arrFoundVal == NULL) // not an array variable
		{
			// Search in variable sym table
			VALUE_TYPE foundVal = searchInTable(varName, symTable);
			if (foundVal != NULL) {	// Found in variable sym table
				std::string tag = "load_" + varName + "_";
				return LLVMBuildLoad(currBuilder, prev_val, tag.c_str());
			}
		}
		else { // if an array variable - likely a param to a function without [] - calculate base ptr and return
			LLVMBuilderRef currBuilder = builderStack.top();
			VALUE_TYPE generated_code = LLVMBuildStructGEP(currBuilder, arrFoundVal, 0, "_arr_base_ptr");
			VALUE_TYPE indices = {LLVMConstInt(LLVMInt32TypeInContext(contextStack.top()), 0, false)};
			return LLVMBuildGEP(currBuilder, generated_code, &indices, 1, "_arr_first_elt_ptr");	
		}
	}
	//load array value
	else if (node->type == "Ident" && node->children[0]->type == "[ ]") {
		return LLVMBuildLoad(currBuilder, prev_val, "array_deref_");
	}
	// load pointer value
	else if (node->type == "DEREF") {
		// LLVMValueRef element_ptr = usePtr(node, prev_val);	
		return LLVMBuildLoad(currBuilder, prev_val, "ptr_deref");
	}

	return prev_val;
}

VALUE_TYPE codegenBinExp(treeNode* lhs, treeNode* rhs, LLVMOpcode Op, bool logical=false) {

	LLVMBuilderRef currBuilder = builderStack.top();

	VALUE_TYPE lhsVal = lhs->codegen();
	lhsVal = loadValueifNeeded(lhs, lhsVal);
	
	VALUE_TYPE rhsVal = rhs->codegen();
	rhsVal = loadValueifNeeded(rhs, rhsVal);

	if (LLVMGetTypeKind(LLVMTypeOf(lhsVal)) == LLVMFloatTypeKind &&
	 LLVMGetTypeKind(LLVMTypeOf(rhsVal)) == LLVMFloatTypeKind && !logical){

		return LLVMBuildBinOp(currBuilder, ((LLVMOpcode)(Op + 1)), lhsVal, rhsVal, "_flt_op");
	}
	else if (LLVMGetTypeKind(LLVMTypeOf(lhsVal)) == LLVMPointerTypeKind && LLVMGetTypeKind(LLVMTypeOf(rhsVal)) == LLVMIntegerTypeKind) {
		VALUE_TYPE indices = {rhsVal};

		return LLVMBuildGEP(currBuilder, lhsVal, &indices, 1, "_ptr_arith");
	}
	else if (LLVMGetTypeKind(LLVMTypeOf(rhsVal)) == LLVMPointerTypeKind && LLVMGetTypeKind(LLVMTypeOf(lhsVal)) == LLVMIntegerTypeKind) {
		VALUE_TYPE indices = {lhsVal};

		return LLVMBuildGEP(currBuilder, rhsVal, &indices, 1, "_ptr_arith");
	}
	else {
		return LLVMBuildBinOp(currBuilder, Op, lhsVal, rhsVal, "_int_op");
	}
}

VALUE_TYPE constShift(treeNode* node, ConstNode* cons, LLVMOpcode Op) {

	LLVMBuilderRef currBuilder = builderStack.top();

	VALUE_TYPE constVal = cons->codegen();
	std::string varName = ((IdentNode*)(node))->name;
	VALUE_TYPE Ident = node->codegen();
	VALUE_TYPE IdentVal = loadValueifNeeded(node, Ident);

	std::string tag = varName + "_const_op";
	VALUE_TYPE var_incr = LLVMBuildBinOp(currBuilder, Op, IdentVal, constVal, tag.c_str());

	return LLVMBuildStore(currBuilder, var_incr, Ident);
}


VALUE_TYPE codegenCondExp(treeNode* lhs, treeNode* rhs, int Op, bool logical=false) {

	LLVMBuilderRef currBuilder = builderStack.top();

	VALUE_TYPE lhsVal = lhs->codegen();
	lhsVal = loadValueifNeeded(lhs, lhsVal);
	
	VALUE_TYPE rhsVal = rhs->codegen();
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
		
		DATATYPE_TYPE funcRetType = stringToLLVMType(children[0]->type, currContext);
		FUNCTION_TYPE funcHeader = ((FunctionNode*)children[1])->codegen( true, funcRetType );

		int index = 0;
		for (auto child : children[1]->children[1]->children) { 
			std::string varName = (((IdentNode*)child->children[1])->name);
			newVariableMap[varName] = LLVMGetParam(funcHeader, index++);
		}

		argSymTable.push(newVariableMap);

		BLOCK_TYPE entry = LLVMAppendBasicBlock(funcHeader, "entry");
		LLVMPositionBuilderAtEnd(currBuilder, entry);

		((FuncBlockNode*)children[2])->codegen( funcRetType, funcHeader );

		argSymTable.pop();

	    return NULL;
	}
	else if (type == "RETURN") { //TODO: Check if you have to load here or return as is
		
		VALUE_TYPE rhsVal = children[0]->codegen();
		rhsVal = loadValueifNeeded(children[0], rhsVal);

		return rhsVal;
	}
	else if (type == "ASSIGN") {
		treeNode* temp = children[0];
		while (temp->type != "Ident") { // for pointer
			temp = temp->children[0];
		}
		std::string varName = ((IdentNode*)temp)->name;
		VALUE_TYPE varPlace = children[0]->codegen();

		VALUE_TYPE rhsVal = children[1]->codegen();
		rhsVal = loadValueifNeeded(children[1], rhsVal);

		// if (children[0]->children.size() > 0 && children[0]->children[0]->type == "[ ]") { 
		// 	// storage in array
		// 	VALUE_TYPE element_ptr_actual = useArray(children[0], varPlace);
		// 	return LLVMBuildStore(currBuilder, rhsVal, element_ptr_actual);
		// }
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
		VALUE_TYPE derefVal = children[0]->codegen();
		derefVal = loadValueifNeeded(children[0], derefVal);
		return derefVal;
	}
	else if (type == "REF") {
		VALUE_TYPE refVal = children[0]->codegen();
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

VALUE_TYPE InitDeclNode::codegen(bool isGlobalContext, DATATYPE_TYPE type) {
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

VALUE_TYPE getGlobalDeclaration(LLVMModuleRef mod, DATATYPE_TYPE type, std::string varName) {
	VALUE_TYPE allocate = LLVMAddGlobal(mod, type, varName.c_str());	
	LLVMSetLinkage(allocate, LLVMCommonLinkage);
	LLVMSetGlobalConstant(allocate, 0);
	return allocate;
}

VALUE_TYPE getTypeZero(DATATYPE_TYPE type, std::string array_name="", int array_length=0) {

	LLVMTypeKind type_kind = LLVMGetTypeKind(type);
	if (type_kind ==  LLVMIntegerTypeKind) {
		return LLVMConstInt(LLVMInt32TypeInContext(contextStack.top()), 0, false);
	}
	else if (type_kind ==  LLVMFloatTypeKind) {
		return LLVMConstReal(LLVMFloatTypeInContext(contextStack.top()), 0.0);
	}
	else if (type_kind ==  LLVMPointerTypeKind) {
		return LLVMConstPointerNull(type);
	}
	else if (type_kind ==  LLVMArrayTypeKind) {
        // global array
		// VALUE_TYPE* init_vals = new VALUE_TYPE[array_length];
		// // DATATYPE_TYPE elem_type = searchInArrTable(array_name, arrSymTable);
		// DATATYPE_TYPE elem_type = LLVMInt32Type();
		// for (int i = 0; i < array_length; i++) {
		// 	init_vals[i] = getTypeZero(elem_type);
		// }
		// return LLVMConstArray(elem_type, init_vals, array_length);
		// // return NULL;
		// // return LLVMConstNull(type);
		// // return LLVMConstStruct(NULL, 0, true);
	}
	return NULL;
}

VALUE_TYPE VariableNode::codegen(bool isGlobalContext, DATATYPE_TYPE type) {

	std::string varName = ((IdentNode*)children[0])->name;
	if (isGlobalContext) {
		VALUE_TYPE allocate = getGlobalDeclaration(mod, type, varName.c_str());
		LLVMSetInitializer (allocate, getTypeZero(type));

		// Add to the symbol table
		symTable.top()[varName] = allocate;
		return allocate;
	}
	LLVMBuilderRef currBuilder = builderStack.top();
	VALUE_TYPE allocate = LLVMBuildAlloca(currBuilder, type, varName.c_str());

	// Add to the symbol table
	symTable.top()[varName] = allocate;
	return allocate;
}

VALUE_TYPE ArrayNode::codegen(bool isGlobalContext, DATATYPE_TYPE type) {

	std::string varName = ((IdentNode*)children[0])->name;
	int arrayLen = ((ConstNode*)children[1])->ival;
	DATATYPE_TYPE arrayType = LLVMArrayType(type, arrayLen);

	if (isGlobalContext) {
		VALUE_TYPE allocate = getGlobalDeclaration(mod, arrayType, varName.c_str());
		LLVMSetInitializer (allocate, getTypeZero(type, varName, arrayLen));

		symTable.top()[varName] = allocate;

		arrSymTable.top()[varName] = allocate;
		return allocate;
	}

	LLVMBuilderRef currBuilder = builderStack.top();

	// Trial: Get function header entry
	BLOCK_TYPE curBlk = LLVMGetInsertBlock(currBuilder); // TODO: Check if this gives the current block
	VALUE_TYPE parentFuncHeader = LLVMGetBasicBlockParent(curBlk);

	// TODO: Look at the third parameter
	VALUE_TYPE allocate = LLVMBuildArrayAlloca(currBuilder, arrayType, NULL /*This could probably be the function header value*/, varName.c_str());

	symTable.top()[varName] = allocate;

	arrSymTable.top()[varName] = allocate;

	return allocate;	
}

VALUE_TYPE PointerNode::codegen(bool isGlobalContext, DATATYPE_TYPE type) {

	// TODO: Check about address space, Default is 0

	std::string childType = children[0]->type;
	VALUE_TYPE varPlace;

	if (childType == "POINTER") {
		DATATYPE_TYPE pointerType = LLVMPointerType(type, 0);
		varPlace = ((PointerNode*)children[0])->codegen(isGlobalContext, pointerType);
	}
	else if (childType == "VARIABLE") {
		DATATYPE_TYPE pointerType = LLVMPointerType(type, 0);
		varPlace = ((VariableNode*)children[0])->codegen(isGlobalContext, pointerType);	
	}
	else // FUNCTION
	{
		DATATYPE_TYPE pointerType = LLVMPointerType(type, 0);
		return ((FunctionNode*)children[0])->codegen(isGlobalContext, pointerType);
	}
	LLVMSetAlignment(varPlace, 8);
	return varPlace;
}

VALUE_TYPE IdentNode::codegen() {
	std::string identName = name;
	LLVMBuilderRef currBuilder = builderStack.top();

	// Search in variable sym table
	VALUE_TYPE foundVal = searchInTable(identName, symTable);

	if (foundVal != NULL) {	// Found in variable sym table
		// printf("%s\n", identName.c_str());
		if (children.size() == 0) { // variable and hence return the value
			return foundVal;
		}
		else { // should be an array access

			return useArray(this, foundVal);
		}
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

				VALUE_TYPE *temp;

				std::string tag = identName + "_ret_";
				return LLVMBuildCall(currBuilder, foundInFunc, temp, 0, tag.c_str());
			} 
			else {
				// Function takes args

				std::vector<VALUE_TYPE> codeForIdents;

				for(auto child: children[0]->children) { // TODO: ADD LOAD RELATED INFO HERE AS WELL
					VALUE_TYPE generated_code = child->codegen();
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
		std::string temp_sval = sval;
		temp_sval.pop_back();
		temp_sval = temp_sval.substr(1);

		LLVMBuilderRef currBuilder = builderStack.top();
		VALUE_TYPE array = LLVMBuildGlobalStringPtr(currBuilder, temp_sval.c_str(), "const_string");

		return array;
	}
}

FUNCTION_TYPE FunctionNode::codegen(bool isGlobalContext, DATATYPE_TYPE type) {

	std::string funcName = ((IdentNode*)children[0])->name;

	std::string childType = children[1]->type;

	std::map<std::string, FUNCTION_TYPE> topmostSymbolTable = funcSymTable.top();

	if (topmostSymbolTable.find(funcName) != topmostSymbolTable.end())
		return topmostSymbolTable[funcName];

	if (childType == "VOID") {
		// Return a function with no params
		DATATYPE_TYPE param_types[] = {};
	    DATATYPE_TYPE retType = LLVMFunctionType(type, param_types, 0, 0);
	    VALUE_TYPE funcDecl = LLVMAddFunction(mod, funcName.c_str(), retType);

	    // Add to function symbol table
	    funcSymTable.top()[funcName] = funcDecl;

	    return funcDecl;
	}
	else {
		std::vector<DATATYPE_TYPE> fparams;
		LLVMContextRef currContext = contextStack.top();
		bool isVariadic = ((ParamNode*)children[1])->isVariadic;

		for (auto child : children[1]->children) {
			std::string childType = child->children[0]->type;
			fparams.push_back(stringToLLVMType(childType, currContext));
		}

		DATATYPE_TYPE* paramTypeList = fparams.data();

		DATATYPE_TYPE retType = LLVMFunctionType(type, paramTypeList, fparams.size(), isVariadic);
	    VALUE_TYPE funcDecl = LLVMAddFunction(mod, funcName.c_str(), retType);

	    // Add to function symbol table
	    funcSymTable.top()[funcName] = funcDecl;

	    return funcDecl;
	}
}

VALUE_TYPE BranchNode::codegen(DATATYPE_TYPE retType) {

	LLVMBuilderRef currBuilder = builderStack.top();
	LLVMContextRef currContext = contextStack.top();

	BLOCK_TYPE curBlk = LLVMGetInsertBlock(currBuilder); // TODO: Check if this gives the current block
	VALUE_TYPE parentFuncHeader = LLVMGetBasicBlockParent(curBlk);

	if (type == "IF") {

		BLOCK_TYPE endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		BLOCK_TYPE then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[1])->codegen(retType, endif);

		LLVMPositionBuilderAtEnd(currBuilder, curBlk); // add to the current block
		VALUE_TYPE conditionCode = children[0]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, endif);		

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}
	else if (type == "ITE") {

		BLOCK_TYPE endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		BLOCK_TYPE then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then
		BLOCK_TYPE else_blk = LLVMAppendBasicBlock(parentFuncHeader, "else"); // basic block for else

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[1])->codegen(retType, endif);

		LLVMPositionBuilderAtEnd(currBuilder, else_blk); // generated code for else
		((CondBlockNode*)children[2])->codegen(retType, endif);

		LLVMPositionBuilderAtEnd(currBuilder, curBlk); // add to the current block
		VALUE_TYPE conditionCode = children[0]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, else_blk);		

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}
	else if (type == "WHILE") {

		BLOCK_TYPE endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		BLOCK_TYPE then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then
		BLOCK_TYPE else_blk = LLVMAppendBasicBlock(parentFuncHeader, "else"); // basic block for else

		// jump to the else block first
		LLVMBuildBr(currBuilder, else_blk);

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[1])->codegen(retType, else_blk); // goes back to the if condition again

		LLVMPositionBuilderAtEnd(currBuilder, else_blk); // add to the else block
		VALUE_TYPE conditionCode = children[0]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, endif);		

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}
	else if (type == "DO-WHILE") {

		BLOCK_TYPE endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		BLOCK_TYPE then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then
		BLOCK_TYPE else_blk = LLVMAppendBasicBlock(parentFuncHeader, "else"); // basic block for else

		// jump to the then block first
		LLVMBuildBr(currBuilder, then_blk);

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[0])->codegen(retType, else_blk); // goes back to the if condition again

		LLVMPositionBuilderAtEnd(currBuilder, else_blk); // add to the else block
		VALUE_TYPE conditionCode = children[1]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, endif);		

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}
	else if (type == "FOR") {

		//Initialization expression
		children[0]->codegen();

		// Now code for the actual loop
		BLOCK_TYPE endif = LLVMAppendBasicBlock(parentFuncHeader, "endif"); // basic block for endif
		BLOCK_TYPE then_blk = LLVMAppendBasicBlock(parentFuncHeader, "then"); // basic block for then
		BLOCK_TYPE else_blk = LLVMAppendBasicBlock(parentFuncHeader, "else"); // basic block for else
		BLOCK_TYPE increment_blk = LLVMAppendBasicBlock(parentFuncHeader, "increment"); // basic block for increment

		// jump to the else block first
		LLVMBuildBr(currBuilder, else_blk);

		LLVMPositionBuilderAtEnd(currBuilder, then_blk); // generated code for then
		((CondBlockNode*)children[3])->codegen(retType, increment_blk); // goes back to the if condition again

		LLVMPositionBuilderAtEnd(currBuilder, else_blk); // add to the else block
		VALUE_TYPE conditionCode = children[1]->codegen(); // generated code for if
		LLVMBuildCondBr(currBuilder, conditionCode, then_blk, endif);	

		LLVMPositionBuilderAtEnd(currBuilder, increment_blk); // add to the increment block
		children[2]->codegen(); // update expression
		LLVMBuildBr(currBuilder, else_blk);

		LLVMPositionBuilderAtEnd(currBuilder, endif); // add to the endif block
	}

	return NULL;
}

void callBlockInst(treeNode* t, LLVMBuilderRef currBuilder, DATATYPE_TYPE retType) {
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

void pushSymTables() {
	std::map<std::string, VALUE_TYPE> varMap;
	std::map<std::string, VALUE_TYPE> arrMap;
	symTable.push(varMap);
	arrSymTable.push(arrMap);
}

void popSymTables() {
	symTable.pop();
	arrSymTable.pop();
}

VALUE_TYPE FuncBlockNode::codegen(DATATYPE_TYPE retType, VALUE_TYPE funcHeader) {

	pushSymTables();

	LLVMBuilderRef currBuilder = builderStack.top();
	LLVMContextRef currContext = contextStack.top();
	callBlockInst((treeNode*)this, currBuilder, retType);

	popSymTables();

	return NULL;
}

VALUE_TYPE CondBlockNode::codegen(DATATYPE_TYPE retTypeIfReqd, BLOCK_TYPE afterDest) {

	pushSymTables();

	LLVMBuilderRef currBuilder = builderStack.top();
	LLVMContextRef currContext = contextStack.top();
	callBlockInst((treeNode*)this, currBuilder, retTypeIfReqd);
	LLVMBuildBr(currBuilder, afterDest);

	popSymTables();

	return NULL;
}

void printModule(LLVMModuleRef mod, std::string fname) {
    FILE *f = fopen(fname.c_str(), "w");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    fprintf(f, "%s\n", LLVMPrintModuleToString(mod));

    fclose(f);
}

void codegen(treeNode* AST) {
	LLVMContextRef globalContext = LLVMGetGlobalContext();
	globalBuilder = LLVMCreateBuilderInContext(globalContext);
	std::map<std::string, VALUE_TYPE> variableMap;
	std::map<std::string, VALUE_TYPE> arrValMap;
	std::map<std::string, VALUE_TYPE> argsMap;
	std::map<std::string, FUNCTION_TYPE> functionMap;

	contextStack.push(globalContext);
	builderStack.push(globalBuilder);
	symTable.push(variableMap);
	arrSymTable.push(arrValMap);
	argSymTable.push(argsMap);
	funcSymTable.push(functionMap);

	mod = LLVMModuleCreateWithNameInContext("my_module", globalContext);

	AST->codegen();
    printModule(mod, "generated_code.txt");

	localOpt(mod);
    printModule(mod, "optimised_code.txt");

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