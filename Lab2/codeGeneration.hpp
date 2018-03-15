#ifndef CODEGENERATION_H
#define CODEGENERATION_H

#include <map>
#include <string>
#include <stack>

#include "llvmHeaders.hpp"
#include "treeNode.hpp"

// static LLVMContext TheContext;
extern LLVMModuleRef mod;
extern LLVMBuilderRef builder;
// static std::map<std::string, Value *> NamedValues;
extern std::stack< std::map<std::string, VALUE_TYPE> > contextStack;

extern void codegen(treeNode* AST);

#endif