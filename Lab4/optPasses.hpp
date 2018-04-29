#ifndef OPT_PASSES_H
#define OPT_PASSES_H

#include "llvmHeaders.hpp"
#include "treeNode.hpp"

void optPasses(LLVMModuleRef mod, LLVMBuilderRef globalBuilder);

#endif