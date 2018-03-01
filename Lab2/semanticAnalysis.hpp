#ifndef SEMANTICANLYSIS_H
#define SEMANTICANLYSIS_H

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
		return -1;
	}

	std::string find_symbol_type(std::string Id) {
		int i = find_symbol(Id);
		if (i >= 0) {
			return types[i];
		}
		return "";
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

std::vector<std::string> getFuncDeclParams(treeNode* func_node, std::string type);
scope expandScope(scope &old_scope, treeNode* func_node);
bool matchFuncArgs(std::vector<std::string> def_params, std::vector<treeNode*> given_params);
bool matchFuncUseArgs(std::vector<std::string> def_params, std::vector<std::string> given_params);

extern bool checkDeclUse(treeNode* node, std::string curr_type, std::stack<scope> &scope);
extern std::string checkType(treeNode* node, std::string curr_type, std::stack<scope> &scope);
extern bool semanticCheck(treeNode* ASTree);

#endif