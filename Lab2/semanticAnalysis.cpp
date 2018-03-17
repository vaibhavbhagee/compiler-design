#include "semanticAnalysis.hpp"

std::vector<std::string> getFuncDeclParams(treeNode* function_node, std::string type) {
	std::vector<std::string> fparams;

	//iterate over params - each param_decl
	for (auto child : function_node->children[1]->children) { 
		fparams.push_back(child->children[0]->type);
	}

	//return type is last param
	fparams.push_back(type); 
	return fparams;
}

scope expandScope(scope &old_scope, treeNode* func_node) {
	// only called when FUNCTION node inside FUNC node - ie func impl.

	// copy old scope
	scope new_scope = scope(old_scope);
	std::string fname = ((IdentNode*)(func_node->children[1]->children[0]))->name;
	
	// put all func parameters in scope
	treeNode* params = func_node->children[1]->children[1];
	if (params->children.size()) { // for void functions
		for (auto child : params->children) {
			new_scope.add_symbol( ((IdentNode*)(child->children[1]))->name, child->children[0]->type );
		}			
	}
	return new_scope;
}

bool matchFuncArgs(std::vector<std::string> def_params, std::vector<treeNode*> given_params) {
	// defined params is n+1, given is n
	bool chk2 = def_params.size() - 1 == given_params.size();
	if (!chk2) {
		return false;
	}

	for (int i = 0; i < given_params.size(); i++) {
		if (def_params[i] != given_params[i]->children[0]->type) {
			chk2 = false;
			break;
		}
	}
	return chk2;
}

bool matchFuncUseArgs(std::vector<std::string> def_params, std::vector<std::string> given_params) {
	// defined params is n+1, given is n
	bool chk2 = def_params.size() - 1 == given_params.size();
	if (!chk2) {
		return false;
	}

	for (int i = 0; i < given_params.size(); i++) {
		if (def_params[i] != given_params[i]) {
			chk2 = false;
			break;
		}
	}
	return chk2;
}
	
bool checkDeclUse(treeNode* node, std::string curr_type, std::stack<scope> &scopes,
 std::unordered_map<std::string, std::vector<std::string> > &functions) {
	
	if (node == NULL) {
		return false;
	}

	std::string type = node->type;
	if (type == "decl") {
		return checkDeclUse(node->children[1], node->children[0]->type, scopes, functions);
	}
	else if (type == "VARIABLE") {
		scopes.top().add_symbol(((IdentNode*)(node->children[0]))->name, curr_type);
		return true;
	}
	else if (type == "POINTER") {
		return checkDeclUse(node->children[0], curr_type + "*", scopes, functions);
	}
	else if (type == "ARRAY") {
		scopes.top().add_symbol(((IdentNode*)(node->children[0]))->name, curr_type, ((ConstNode*)(node->children[1]))->ival);
		return true;
	}
	else if (type == "FUNCTION") {
		std::string fname = ((IdentNode*)(node->children[0]))->name;
		auto it = functions.find(fname);
		bool chk1 = it == functions.end(); 
		if (chk1) { //NOT Declared previously
			functions[fname] = getFuncDeclParams(node, curr_type);
			return true;
		}
		else {
			// match args and the return type
			bool chk2 = matchFuncArgs(it->second, node->children[1]->children) && (it->second.back() == curr_type);
			if (chk2) {
				return true;
			}
			else {
				// TODO - Overloading
				std::cout << "The function " << fname << " has a mismatched arguments in declaration and definition" << std::endl;
				return false;
			}
		}

	}
	else if (type == "FUNC") {

		// check if function returns a pointer
		if (node->children[1]->type == "POINTER") {
			treeNode* temp = node->children[1];
			while (temp->type == "POINTER") {
				node->children[0]->type += "*";
				temp = temp->children[0];				
			}
			node->children[1] = temp;
		}

		bool isDeclared = checkDeclUse(node->children[1], node->children[0]->type, scopes, functions);
		/* 
		   now if signatures match/ or function not declared before, 
		   the argument list of function should have argument types,
		   and identifiers, which are put into current scope
		*/
		scopes.push(expandScope(scopes.top(), node));

		bool chk = isDeclared && checkDeclUse(node->children[2], curr_type, scopes, functions);
		scopes.pop(); 
		return chk;
	}
	else if (type == "WHILE") {
		scopes.push(scope(&scopes.top()));
		bool chk = checkDeclUse(node->children[1], curr_type, scopes, functions);
		scopes.pop();
		return chk;
	}
	else if (type == "DO-WHILE") {
		scopes.push(scope(&scopes.top()));
		bool chk = checkDeclUse(node->children[0], curr_type, scopes, functions);
		scopes.pop();
		return chk;
	}
	else if (type == "Ident") {
		// for variable
		std::string name = ((IdentNode*)(node))->name;
		bool chk = scopes.top().check_scope(name);

		// for function
		if (!chk) {
			auto it = functions.find(name);
			if (it == functions.end()) { // not declared
				std::cout << name << " has not been declared" << std::endl;
				return false;
			}
			else {	//check no. of arguments
				if (node->children.size()){ // in case of no args
					int numargs = node->children[0]->children.size();
					if (it->second.size() - 1 == numargs) { // correct number of args
						return checkDeclUse(node->children[0], curr_type, scopes, functions);
					}
				}
				std::cout << name << " has been called with incorrect number of arguments" << std::endl;
				return false;
			}
		}
		return chk;
	}
	else if (type == "Const") {
		return true;
	}
	else {
		if (type == "start") {
			scope new_scope = scope();
			scopes.push(new_scope);
		}
		bool res = true;
		for (auto child : node->children) {
			res &= checkDeclUse(child, curr_type, scopes, functions);
		}
		return res;
	}
}

std::string checkType(treeNode* node, std::string curr_type, std::stack<scope> &scopes,
 std::unordered_map<std::string, std::vector<std::string> > &functions) {

	/*
		Assuming this is called after checkDeclUse, we should have a
		function table and the global scope ready - i.e. unlike checkDeclUse
		"scopes" is not empty initially - consequently no start branch
	*/
	if (node == NULL) {
		return "";
	}

	std::string type = node->type;
	if (type == "decl") {
		return checkType(node->children[1], node->children[0]->type, scopes, functions);
	}
	else if (type == "VARIABLE") {
		scopes.top().add_symbol(((IdentNode*)(node->children[0]))->name, curr_type);
		return "VOID";
	}
	else if (type == "POINTER") {
		return checkType(node->children[0], curr_type + "*", scopes, functions);
	}
	else if (type == "ARRAY") {
		scopes.top().add_symbol(((IdentNode*)(node->children[0]))->name, curr_type, ((ConstNode*)(node->children[1]))->ival);
		return "VOID";
	}
	else if (type == "Ident") {
		// for variable
		std::string name = ((IdentNode*)(node))->name;
		bool chk = scopes.top().check_scope(name);

		// for function
		if (!chk) {
			auto it = functions.find(name);
			/* 
				since decl before use is checked, we assume function is defined
				check argument types and get the last args - return type
			*/
			std::vector<std::string> given_params;
			for (auto child : node->children[0]->children) { 
				given_params.push_back(checkType(child, curr_type, scopes, functions));
			}

			bool chk2 = matchFuncUseArgs(it->second, given_params);
			if (chk2){
				return (it->second).back();
			}
			else {
				std::cout << name << " has been called with incorrect argument types" << std::endl;
				return "";
			}
		}
		else {
			std::string typ = scopes.top().find_symbol_type(((IdentNode*)(node))->name);
			return typ; 
		}

	}
	else if (type == "FUNCTION") {
		return "VOID";
	}
	else if (type == "FUNC") {
		// By now we are sure that the signature in func def. and func impl. are the same
		std::string ret_type = node->children[0]->type;

		scopes.push(expandScope(scopes.top(), node));
		std::string block_ret = checkType(node->children[2], ret_type, scopes, functions);
		scopes.pop(); 
		return block_ret;
	}
	else if (type == "RETURN") {
		if (node->children.size()) {
			return checkType(node->children[0], curr_type, scopes, functions);
		}
		return "VOID";
	}
	else if (type == "Const") {
		return ((ConstNode*)(node))->name;
	}
	else if (type == "ASSIGN") {
		std::string lhs = checkType(node->children[0], curr_type, scopes, functions);
		std::string rhs = checkType(node->children[1], curr_type, scopes, functions);
		if (lhs == rhs) {
			return "VOID";
		}
		else if (lhs.find("INT") == 0 && rhs == "INT") { //pointer assigned INT
			return "VOID";
		}
		else {
			std::cout << "Incorrectly assigned type to " << ((IdentNode*)(node->children[0]))->name << std::endl;
			return "";
		}
	}
	else if (type == "INC" || type == "DEC") {
		std::string rhs = checkType(node->children[0], curr_type, scopes, functions);
		return (rhs == "INT")?"VOID":"";
	}
	else if (type == "PLUS" || type == "MINUS" || type == "MULT" || type == "DIV" || type == "MOD") {
		std::string lhs = checkType(node->children[0], curr_type, scopes, functions);
		std::string rhs = checkType(node->children[1], curr_type, scopes, functions);
		if (lhs == "INT" && rhs == lhs) {
			return "INT";
		}
		else if (lhs == "FLOAT" && rhs == "INT") {
			return "FLOAT";
		}
		else if (lhs == "INT" && rhs == "FLOAT") {
			return "FLOAT";
		}
		else if (lhs == "FLOAT" && rhs == lhs) {
			return "FLOAT";
		}
		else {
			return "";
		}
	}
	else if (type == "OR" || type == "AND" || type == "XOR") {
		std::string lhs = checkType(node->children[0], curr_type, scopes, functions);
		std::string rhs = checkType(node->children[1], curr_type, scopes, functions);
		if (lhs == "BOOL" && rhs == lhs) {
			return "BOOL";
		}
		else {
			return "";
		}
	}
	else if (type == "EQ" || type == "NEQ") {
		std::string lhs = checkType(node->children[0], curr_type, scopes, functions);
		std::string rhs = checkType(node->children[1], curr_type, scopes, functions);
		return (lhs == rhs)?"BOOL":"";
	}
	else if (type == "LT" || type == "GT" || type == "LTE" || type == "GTE" ) {
		std::string lhs = checkType(node->children[0], curr_type, scopes, functions);
		std::string rhs = checkType(node->children[1], curr_type, scopes, functions);
		return (lhs == rhs)?"BOOL":"";
	}
	else if (type == "NOT") {
		std::string lhs = checkType(node->children[0], curr_type, scopes, functions);
		return (lhs == "INT" | lhs == "BOOL")?"BOOL":"";
	}
	else if (type == "REF") {
		std::string lhs = checkType(node->children[0], curr_type, scopes, functions);
		return (lhs + "*");
	}
	else if (type == "DEREF") {
		std::string lhs = checkType(node->children[0], curr_type, scopes, functions);
		if (lhs.find("*") != std::string::npos) {
			// a pointer is dereferenced
			lhs.pop_back();
			return lhs;
		}
		return "";
	}
	else if (type == "CONDITION") {
		std::string cond = checkType(node->children[0], curr_type, scopes, functions);
		return (cond == "BOOL")?"VOID":"";
	}
	else {
		for (int i = 0; i < node->children.size(); i++) {
			auto child = node->children[i];
			std::string typ = checkType(child, curr_type, scopes, functions);
			if (typ == "") {
				return "";
			}
			if (type == "BLOCK" && child->type == "RETURN") {
				if (i == node->children.size() - 1) { // return is last in block
					if (curr_type == typ) {			  // return type matches function def
						return "VOID";
					}
				}
				return "";
			}
		}
		return "VOID";
	}
}

bool semanticCheck(treeNode* ASTree) {
	std::stack<scope> scopes;
  	std::unordered_map<std::string, std::vector<std::string> > functions;

  	bool chk1 = checkDeclUse(ASTree, "VOID", scopes, functions);
  	if (!chk1) {
  		std::cout << "Missing declarations before use" << std::endl;
  		return false;
  	}
  	std::cout << "Identifiers are declared before use" << std::endl;

  	std::string chk2 = checkType(ASTree, "VOID", scopes, functions);
  	if (chk2 == "") {
  		std::cout << "Typing errors found" << std::endl;
  		return false;
  	}
  	std::cout << "No typing errors found" << std::endl;

  	return true;
}