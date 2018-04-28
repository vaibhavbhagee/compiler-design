#include "optPasses.hpp"
#include "Scalar.h"

void optPasses(LLVMModuleRef mod) {
    // prepare function pass manager and all the passes
    PASSMANAGER_TYPE funcPassManager = LLVMCreateFunctionPassManagerForModule(mod);

    // add passes - Mem2Reg, const fold, cse, copy prop, dce, code motion(pre)
    for (int i = 0; i < 2; i++) {
        LLVMAddPromoteMemoryToRegisterPass(funcPassManager);
        LLVMAddConstantPropagationPass(funcPassManager);
        LLVMAddBitTrackingDCEPass(funcPassManager);
        LLVMAddDeadStoreEliminationPass(funcPassManager);
        LLVMAddGVNPass(funcPassManager);
        LLVMAddIndVarSimplifyPass(funcPassManager);
        LLVMAddInstructionCombiningPass(funcPassManager);
        LLVMAddLICMPass(funcPassManager);
        LLVMAddLoopDeletionPass(funcPassManager);
        LLVMAddMemCpyOptPass(funcPassManager);
        LLVMAddEarlyCSEPass(funcPassManager);
    }
    
    // Initialise the pass manager
    LLVMInitializeFunctionPassManager(funcPassManager);

    // iterate over all functions
    VALUE_TYPE currFunction = LLVMGetFirstFunction(mod);
    while (currFunction != NULL) {
        LLVMRunFunctionPassManager(funcPassManager, currFunction);
        currFunction = LLVMGetNextFunction(currFunction);
    }
    LLVMFinalizeFunctionPassManager(funcPassManager);
}