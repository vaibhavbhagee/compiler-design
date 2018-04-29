#include "optPasses.hpp"
#include <string>
#include <cstring>
#include <map>

void copyPropagationFunction(VALUE_TYPE currFunction, LLVMBuilderRef globalBuilder);
void copyPropagationBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap);
void copyPropagationAnalysisBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap);

void optPasses(LLVMModuleRef mod, LLVMBuilderRef globalBuilder) {
    // prepare function pass manager and all the passes
    PASSMANAGER_TYPE funcPassManager = LLVMCreateFunctionPassManagerForModule(mod);

    // add passes - Mem2Reg, const fold, cse, copy prop, dce, code motion(pre)
    for (int i = 0; i < 2; i++) {
        LLVMAddPromoteMemoryToRegisterPass(funcPassManager);
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

    while (currBlock != NULL) {
        copyPropagationAnalysisBasicBlock(currBlock, globalBuilder, propMap);
        currBlock = LLVMGetNextBasicBlock(currBlock);
    }

    currBlock = LLVMGetFirstBasicBlock(currFunction);

    while (currBlock != NULL) {
        copyPropagationBasicBlock(currBlock, globalBuilder, propMap);
        currBlock = LLVMGetNextBasicBlock(currBlock);
    }
}

void copyPropagationAnalysisBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap) {
    // std::map<std::string, VALUE_TYPE> propMap;

    // iterate over all the instructions
    std::vector<VALUE_TYPE> deadInstrs;
    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);

    while (currInstruction != NULL) {

        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            // case LLVMStore: // if rhs is constant, update value in map
            // {
            //     VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 1);
            //     VALUE_TYPE rhs = LLVMGetOperand(currInstruction, 0);
            //     std::string valName(LLVMGetValueName(lhs));

            //     // if (LLVMIsConstant(rhs)) {
            //         propMap[valName] = rhs;
            //     // }
            // }
            // break;

            case LLVMLoad: // if lhs is in map, get value from that and remove load
            {
                VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 0);
                VALUE_TYPE rhs = LLVMGetOperand(currInstruction, 1);
                std::string valName(LLVMGetValueName(lhs));
                std::string currInstName(LLVMGetValueName(currInstruction));

                // if (propMap.find(valName) != propMap.end()) {
                //     propMap[currInstName] = propMap[valName];
                //     deadInstrs.push_back(currInstruction);
                // }

                propMap[valName] = rhs;

                // LLVMReplaceAllUsesWith(lhs, rhs);
                deadInstrs.push_back(currInstruction);
            }
            break;

            // case LLVMAlloca:
            // case LLVMGetElementPtr:
            // break;

            default:
                break;
            // {
            //     // replace with constants in each operand
            //     int numOps = LLVMGetNumOperands(currInstruction);
            //     for (int i = 0; i < numOps; ++i) {
            //         VALUE_TYPE op = LLVMGetOperand(currInstruction, i);
            //         std::string opName(LLVMGetValueName(op));

            //         if (propMap.find(opName) != propMap.end()) {
            //             LLVMSetOperand(currInstruction, i, propMap[opName]);
            //         }
            //     }
            // }
        }

        // fold constants
        // if (constantFold(block, currInstruction, globalBuilder)) {
        //     deadInstrs.push_back(currInstruction);
        // }
        
        currInstruction = LLVMGetNextInstruction(currInstruction);
    }

    // remove all dead instructions
    for (auto deadInstr : deadInstrs) {
        LLVMInstructionEraseFromParent(deadInstr);
    }
}

void copyPropagationBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap) {
    // std::map<std::string, VALUE_TYPE> propMap;

    // iterate over all the instructions
    // std::vector<VALUE_TYPE> deadInstrs;
    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);

    while (currInstruction != NULL) {

        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            // case LLVMStore: // if rhs is constant, update value in map
            // {
            //     VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 1);
            //     VALUE_TYPE rhs = LLVMGetOperand(currInstruction, 0);
            //     std::string valName(LLVMGetValueName(lhs));

            //     // if (LLVMIsConstant(rhs)) {
            //         propMap[valName] = rhs;
            //     // }
            // }
            // break;

            case LLVMLoad: // if lhs is in map, get value from that and remove load
            // {
            //     VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 0);
            //     VALUE_TYPE rhs = LLVMGetOperand(currInstruction, 1);
            //     std::string valName(LLVMGetValueName(lhs));
            //     std::string currInstName(LLVMGetValueName(currInstruction));

            //     // if (propMap.find(valName) != propMap.end()) {
            //     //     propMap[currInstName] = propMap[valName];
            //     //     deadInstrs.push_back(currInstruction);
            //     // }

            //     propMap[valName] = rhs;

            //     LLVMReplaceAllUsesWith(lhs, rhs);
            //     deadInstrs.push_back(currInstruction);
            // }
            break;

            // case LLVMAlloca:
            // case LLVMGetElementPtr:
            // break;

            default:
                break;
            {
                // replace with constants in each operand
                int numOps = LLVMGetNumOperands(currInstruction);
                for (int i = 0; i < numOps; ++i) {
                    VALUE_TYPE op = LLVMGetOperand(currInstruction, i);
                    std::string opName(LLVMGetValueName(op));

                    if (propMap.find(opName) != propMap.end()) {
                        LLVMSetOperand(currInstruction, i, propMap[opName]);
                    }
                }
            }
        }

        // fold constants
        // if (constantFold(block, currInstruction, globalBuilder)) {
        //     deadInstrs.push_back(currInstruction);
        // }
        
        currInstruction = LLVMGetNextInstruction(currInstruction);
    }

    // remove all dead instructions
    // for (auto deadInstr : deadInstrs) {
    //     LLVMInstructionEraseFromParent(deadInstr);
    // }
}