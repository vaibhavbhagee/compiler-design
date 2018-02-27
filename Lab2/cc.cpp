// #include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include "semanticAnalysis.hpp"
#include "c.tab.hpp"

extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;
extern "C" treeNode *ASTree;

static void usage()
{
  printf("Usage: cc <prog.c>\n");
}

// print AST - unix style
char depth[2056];
int di;
void printTree(treeNode* tree) {
  if (tree == NULL) {return;}

  if (tree->type == "Const") {
    std::cout << ((ConstNode*)tree)->ival << std::endl;
  }
  else if (tree->type == "Ident") {
    std::cout << ((IdentNode*)tree)->name << std::endl;
  }
  else {
    std::cout << tree->type << std::endl;
  }

  for (int i = 0; i < tree->children.size(); i++) {
    std::cout << depth << "  --";

    // push
    depth[di++] = ' ';
    depth[di++] = (i < tree->children.size() - 1)?'|':' ';
    depth[di++] = ' ';
    depth[di++] = ' ';
    depth[di] = 0;

    printTree(tree->children[i]);

    //pop
    depth[di -= 4] = 0;
  }
}

int
main(int argc, char **argv)
{
  if (argc != 2) {
    usage();
    exit(1);
  }
  char const *filename = argv[1];
  yyin = fopen(filename, "r");
  assert(yyin);
  int ret_val = yyparse();

  printTree(ASTree);

  // Check Declarations
  std::stack<scope> scopes;
  std::unordered_map<std::string, std::vector<std::string> > functions;
  
  ret_val = checkDeclUse(ASTree, "VOID", scopes, functions);

  // std::cout << ASTree.children[0].children[0].children.size() << std::endl;
  std::cout << "retv = " << ret_val << std::endl;
  exit(0);
}
