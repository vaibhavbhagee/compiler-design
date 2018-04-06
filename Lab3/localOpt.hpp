#ifndef LOCAL_OPT_H
#define LOCAL_OPT_H

// #include "codeGeneration.hpp"
#include <map>
#include <string>
#include <stack>

#include "llvmHeaders.hpp"
#include "treeNode.hpp"

extern void localOpt(LLVMModuleRef mod);
extern void localOptBasicBlock(BLOCK_TYPE basicBlock);

#endif