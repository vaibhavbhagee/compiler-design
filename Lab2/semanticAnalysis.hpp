#include "treeNode.hpp"
#include <deque>
#include <stack>
#include <unordered_map>
#include <algorithm>

#include <iostream>

class scope {
	public:
		// variables and array in scope
		int init;
		std::deque<std::string> symbols;
		std::deque<std::string> types;
		std::deque<int> sizes;

	scope() {
		init = 0;
	}

	scope(scope* old) {
		init = old->init;
		symbols = old->symbols;
		types = old->types;
		sizes = old->sizes;
	}

	int find_symbol(std::string Id) {
		auto it = std::find(symbols.begin(), symbols.end(), Id);
		if (it != symbols.end()) {
			return std::distance(symbols.begin(), it);
		}
	}

	void add_symbol(std::string Id, std::string t, int asize=0) {
		symbols.push_front(Id);
		types.push_front(t);
		sizes.push_front(asize);
	}

	bool check_scope(std::string Id, int size=0) {
		for (int i = 0; i < symbols.size(); i++) {
			if (symbols[i] == Id && sizes[i] == size) {
				return true;
			}
		}
		return false;
	}
};
	
bool checkDeclUse(treeNode* node, std::string curr_type, std::stack<scope> &scopes,
 std::unordered_map<std::string, std::vector<std::string> > &functions) {

	std::string type = node->type;
	if (type == "decl") {
		bool chk = checkDeclUse(node->children[1], node->children[0]->type, scopes, functions);
		return chk;
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
			std::vector<std::string> fparams;
			//iterate over params - each param_decl
			for (auto child : node->children[1]->children) { 
				fparams.push_back(child->children[0]->type);
				// push in the parameter names - push into scope of function later
				fparams.push_back(((IdentNode*)(child->children[1]))->name);
			}
			//return type is last param
			fparams.push_back(curr_type); 
			functions[fname] = fparams;
			return true;
		}
		else {
			// check if declared exactly as previously - likely a func implementation
			std::vector<std::string> old_params = it->second;
			std::vector<treeNode*> new_params = node->children[1]->children;
			bool chk2 = (old_params.size() - 1)/2 == new_params.size();
			if (chk2) {
				for (int i = 0; i < new_params.size(); i++) {
					if (old_params[2 * i] != new_params[i]->children[0]->type) {
						//std::cout << fname << " " << old_params[2 * i + 1] << " " << new_params[i]->children[0]->type << std::endl;
						chk2 = false;
						break;
					}
				}
			}

			if (chk2) {
				return true;
			}
			else {
				// TODO - Overloading
				return false;
			}
		}

	}
	else if (type == "FUNC") {
		bool isDeclared = checkDeclUse(node->children[1], node->children[0]->type, scopes, functions);
		/* 
		   now if signatures match/ or function not declared before, 
		   the argument list of function should have argument types,
		   and identifiers, which are put into current scope
		*/
		scopes.push(scope(&scopes.top()));
		std::string fname = ((IdentNode*)(node->children[1]->children[0]))->name;
		treeNode* params = node->children[1]->children[1];
		if (params->children.size()) { // for void functions
			for (auto child : params->children) {
				scopes.top().add_symbol(((IdentNode*)(child->children[1]))->name, child->children[0]->type);
			}			
		}

		bool chk = isDeclared && checkDeclUse(node->children[2], curr_type, scopes, functions);
		scopes.pop();
		// std::cout << type << fname << " " << chk << std::endl;
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
	// else if (type == "FOR") {
	// 	scopes.push(scope(&scopes.top()));
	// 	bool chk = true;
	// 	scopes.pop();
	// 	return chk;
	// }
	else if (type == "Ident") {
		// for variable
		std::string name = ((IdentNode*)(node))->name;
		bool chk = scopes.top().check_scope(name);

		// for function
		if (!chk) {
			auto it = functions.find(name);
			if (it == functions.end()) { // not declared
				return false;
			}
			else {	//check no. of arguments
				if (node->children.size()){ //incase of no args
					int numargs = node->children[0]->children.size();
					return (it->second.size() - 1)/2 == numargs;
				}
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
		// std::cout << type << " " << res << std::endl;
		return res;
	}
}