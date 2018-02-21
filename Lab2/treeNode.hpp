#ifndef TREENODE_H
#define TREENODE_H

#include <string>
#include <vector>

class treeNode {
	public:
		std::string name;
		int value;
		std::vector<treeNode*> children;

		treeNode() {
			name = "";
			value = 0;
		}

		treeNode(std::string n) {
			name = n;
		}

		treeNode(std::string n, int v) {
			name = n;
			value = v;
		}
};

#endif