// #include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include "semanticAnalysis.hpp"
#include "c.tab.hpp"
#include "codeGeneration.hpp"

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
    std::string name = ((ConstNode*)tree)->name;
    if (name == "INT"){
      std::cout << ((ConstNode*)tree)->ival << std::endl;
    }
    else if (name == "FLOAT") {
      std::cout << ((ConstNode*)tree)->fval << std::endl;
    }
    else {
      std::cout << ((ConstNode*)tree)->sval << std::endl;
    }
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

  // Parsing and AST generation
  int ret_val = yyparse();
  printTree(ASTree);

  // Semantic analysis to ensure a) Declaration before use and b) Type checking
  ret_val = semanticCheck(ASTree);
  std::cout << "retv = " << !ret_val << std::endl;

  // LLVM IR Code generation
  TheModule = new Module("coge_gen_output", getGlobalContext());
  codegen(ASTree);
  TheModule->dump();
  delete TheModule;

  exit(0);
}
