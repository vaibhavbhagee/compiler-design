#ifndef TREENODE_H
#define TREENODE_H
#include <string>

class treeNode {
	public:
		int name;
		int value;
		treeNode* children;

		treeNode() {
			name = 0;
			value = 0;
			children = NULL;
		}

		treeNode(int n) {
			name = n;
		}

		treeNode(int n, int v) {
			name = n;
			value = v;
		}
};

#endif