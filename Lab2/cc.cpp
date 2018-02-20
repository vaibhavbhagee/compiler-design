#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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
  int ret = yyparse();

  printf("%d\n", ASTree.name);
  printf("retv = %d\n", ret);
  exit(0);
}
