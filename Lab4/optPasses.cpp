#include "optPasses.hpp"
#include <string>
#include <cstring>
#include <map>

void copyPropagationFunction(VALUE_TYPE currFunction, LLVMBuilderRef globalBuilder);
void copyPropagationBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap, std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs);
void copyPropagationAnalysisBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap, std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs);

void optPasses(LLVMModuleRef mod, LLVMBuilderRef globalBuilder) {
    // prepare function pass manager and all the passes
    PASSMANAGER_TYPE funcPassManager = LLVMCreateFunctionPassManagerForModule(mod);

    // Mem2Reg - full SSA
    LLVMAddPromoteMemoryToRegisterPass(funcPassManager);
    
    // add passes - const fold, cse, copy prop, dce, code motion(pre)
    for (int i = 0; i < 2; i++) {
        // LLVMAddConstantPropagationPass(funcPassManager);
        // LLVMAddBitTrackingDCEPass(funcPassManager);
        // LLVMAddDeadStoreEliminationPass(funcPassManager);
        // LLVMAddGVNPass(funcPassManager);
        // LLVMAddIndVarSimplifyPass(funcPassManager);
        // LLVMAddInstructionCombiningPass(funcPassManager);
        // LLVMAddLICMPass(funcPassManager);
        // LLVMAddLoopDeletionPass(funcPassManager);
        // LLVMAddMemCpyOptPass(funcPassManager);
        // LLVMAddEarlyCSEPass(funcPassManager);
    }
    
    // Initialise the pass manager
    LLVMInitializeFunctionPassManager(funcPassManager);

    // iterate over all functions
    VALUE_TYPE currFunction = LLVMGetFirstFunction(mod);
    while (currFunction != NULL) {
        LLVMRunFunctionPassManager(funcPassManager, currFunction);
        copyPropagationFunction(currFunction, globalBuilder);
        currFunction = LLVMGetNextFunction(currFunction);
    }
    LLVMFinalizeFunctionPassManager(funcPassManager);
}

void copyPropagationFunction(VALUE_TYPE currFunction, LLVMBuilderRef globalBuilder) {
    BLOCK_TYPE currBlock = LLVMGetFirstBasicBlock(currFunction);
    std::map<std::string, VALUE_TYPE> propMap;
    std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > deadInstrs;

    while (currBlock != NULL) {
        copyPropagationAnalysisBasicBlock(currBlock, globalBuilder, propMap, deadInstrs);
        currBlock = LLVMGetNextBasicBlock(currBlock);
    }

    currBlock = LLVMGetFirstBasicBlock(currFunction);

    while (currBlock != NULL) {
        copyPropagationBasicBlock(currBlock, globalBuilder, propMap, deadInstrs);
        currBlock = LLVMGetNextBasicBlock(currBlock);
    }

    for (auto it = deadInstrs.begin(); it != deadInstrs.end(); ++it)
    {
        for (auto deadInstr : (it->second)) {
            LLVMInstructionEraseFromParent(deadInstr);
        }
    }
}

void copyPropagationAnalysisBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap, std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs) {
    // iterate over all the instructions
    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);

    while (currInstruction != NULL) {

        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            case LLVMLoad:
            {
                VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 0);
                
                std::string valName(LLVMGetValueName(lhs));
                std::string currInstName(LLVMGetValueName(currInstruction));

                // printf("%s, %s, %s\n", LLVMPrintValueToString(lhs), currInstName.c_str(), valName.c_str());
                // LLVMPrintValueToString(lhs);
                // LLVMPrintValueToString(rhs);

                propMap[currInstName] = lhs;

                // LLVMReplaceAllUsesWith(lhs, rhs);
                deadInstrs[block].push_back(currInstruction);
            }
            break;
            default:
                break;
        }
        
        currInstruction = LLVMGetNextInstruction(currInstruction);
    }
}

void copyPropagationBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap, std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs) {
    // iterate over all the instructions
    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);

    while (currInstruction != NULL) {

        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            case LLVMLoad:
            break;

            // case LLVMAlloca:
            // case LLVMGetElementPtr:
            // break;

            default:
            {
                // replace with constants in each operand
                int numOps = LLVMGetNumOperands(currInstruction);
                for (int i = 0; i < numOps; ++i) {
                    VALUE_TYPE op = LLVMGetOperand(currInstruction, i);
                    std::string opName(LLVMGetValueName(op));
                    // printf("%s\n", LLVMPrintValueToString(op)/*, opName.c_str()*//*, valName.c_str()*/);
                    if (propMap.find(opName) != propMap.end()) {
                        LLVMSetOperand(currInstruction, i, propMap[opName]);
                    }
                }
            }
        }        
        currInstruction = LLVMGetNextInstruction(currInstruction);
    }
}