#ifndef CODEGENERATION_H
#define CODEGENERATION_H

// #include "llvm/ADT/APFloat.h"
// #include "llvm/ADT/STLExtras.h"
// #include "llvm/IR/BasicBlock.h"
// #include "llvm/IR/Constants.h"
// #include "llvm/IR/DerivedTypes.h"
// #include "llvm/IR/Function.h"
// #include "llvm/IR/IRBuilder.h"
// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
// #include "llvm/IR/Type.h"
// #include "llvm/IR/Verifier.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include <map>
#include <string>

#include "treeNode.hpp"

using namespace llvm;

// static LLVMContext TheContext;
static IRBuilder<> Builder(getGlobalContext());
static Module* TheModule;
static std::map<std::string, Value *> NamedValues;

extern void codegen(treeNode* AST);

#endif