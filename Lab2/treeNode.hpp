#ifndef TREENODE_H
#define TREENODE_H

#include <string>
#include <vector>

class treeNode {
	public:
		std::string type;
		std::vector<treeNode> children;

		treeNode() {
			type = "";
		}

		treeNode(std::string n) {
			type = n;
		}
};

class ConstNode : public treeNode {
	public:
		int ival;
		float fval;
		std::string sval;

		ConstNode() {
			type = "Const";
		}

		ConstNode(int i) {
			type = "Const";
			ival = i;
		}

		ConstNode(float f) {
			type = "Const";
			fval = f;
		}

		ConstNode(std::string s) {
			type = "Const";
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