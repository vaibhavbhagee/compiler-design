#ifndef TREENODE_H
#define TREENODE_H

#include <string>
#include <vector>

class treeNode {
	public:
		std::string type;
		std::vector<treeNode*> children;

		treeNode() {
			type = "";
		}

		treeNode(std::string n) {
			type = n;
		}

		treeNode(treeNode &t) {
			type = t.type;
			children = t.children;
		}

		treeNode(treeNode *t) {
			type = t->type;
			children = t->children;
		}
};

class ConstNode : public treeNode {
	public:
		std::string name;
		int ival;
		float fval;
		std::string sval;

		ConstNode() {
			type = "Const";
		}

		ConstNode(int i) {
			type = "Const";
			name = "INT";
			ival = i;
		}

		ConstNode(float f) {
			type = "Const";
			name = "FLOAT";
			fval = f;
		}

		ConstNode(std::string s) {
			type = "Const";
			name = "STRING";
			sval = s;
		}
};

class IdentNode : public treeNode {
	public:
		std::string name;

		IdentNode() {
			type = "Ident";
		}

		IdentNode(std::string n) {
			type = "Ident";
			name = n;
		}
};

#endif