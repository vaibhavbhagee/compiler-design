// #include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include "treeNode.hpp"
#include "c.tab.hpp"

extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;
extern "C" treeNode ASTree;

static void usage()
{
  printf("Usage: cc <prog.c>\n");
}



void printTree(treeNode* tree) {
  if (tree == NULL) {
    return;
  }
  std::cout << tree->name << std::endl;

  char depth[2056];
  int di;

  for (auto child:tree->children) {
    std::cout << depth << "  --";

    // push
    depth[di++] = ' ';
    depth[di++] = '|';
    depth[di++] = ' ';
    depth[di++] = ' ';
    depth[di] = 0;

    printTree(child);

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

  // std::cout << ASTree.name << std::endl;
  printTree(&ASTree);
  std::cout << "retv = " << ret_val << std::endl;
  exit(0);
}
