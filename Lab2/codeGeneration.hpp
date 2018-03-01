#ifndef CODEGENERATION_H
#define CODEGENERATION_H

#include <map>
#include <string>
#include <stack>

#include "llvmHeaders.hpp"
#include "treeNode.hpp"

// static LLVMContext TheContext;
static IRBuilder<> Builder(getGlobalContext());
static Module* TheModule;
// static std::map<std::string, Value *> NamedValues;
extern std::stack< std::map<std::string, Value*> > contextStack;

extern void codegen(treeNode* AST);

#endif