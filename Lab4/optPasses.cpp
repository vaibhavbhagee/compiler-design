#include "optPasses.hpp"
#include "utils.hpp"
#include <string>
#include <cstring>
#include <map>
#include <unordered_set>
#include <set>

void copyPropagationFunction(VALUE_TYPE currFunction, LLVMBuilderRef globalBuilder);
void copyPropagationBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap, std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs);
void copyPropagationAnalysisBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::map<std::string, VALUE_TYPE>& propMap, std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs);

void cseFunction(VALUE_TYPE currFunction, LLVMBuilderRef globalBuilder);

void cseBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::unordered_set<BLOCK_TYPE> &path, std::set<VALUE_TYPE> propMap, std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs);

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
        // copyPropagationFunction(currFunction, globalBuilder);

        // cseFunction(currFunction, globalBuilder);
        currFunction = LLVMGetNextFunction(currFunction);
    }
    LLVMFinalizeFunctionPassManager(funcPassManager);

    printModule(mod, "ssa.txt");

    currFunction = LLVMGetFirstFunction(mod);
    while (currFunction != NULL) {
        cseFunction(currFunction, globalBuilder);
        currFunction = LLVMGetNextFunction(currFunction);
    }
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

void cseFunction(VALUE_TYPE currFunction, LLVMBuilderRef globalBuilder) {

    BLOCK_TYPE currBlock = LLVMGetEntryBasicBlock(currFunction);
    
    std::set<VALUE_TYPE> propMap;
    std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > deadInstrs;
    std::unordered_set<BLOCK_TYPE> path;

    if (currBlock != NULL)
        cseBasicBlock(currBlock, globalBuilder, path, propMap, deadInstrs);

    for (auto it = deadInstrs.begin(); it != deadInstrs.end(); ++it)
    {
        for (auto deadInstr : (it->second)) {
            LLVMInstructionEraseFromParent(deadInstr);
        }
    }
}

void cseBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, std::unordered_set<BLOCK_TYPE> &path, std::set<VALUE_TYPE> propMap, std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs) {
    VALUE_TYPE term = LLVMGetBasicBlockTerminator(block);
    // printf("%s %s\n", "Terminator: ", LLVMPrintValueToString(term));

    if (term == NULL)
    {
        printf("%s\n", "NULL TERMINATOR!");
        return;
    }

    // Code for CSE

    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);

    while (currInstruction != NULL) {

        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            case LLVMAdd:
            case LLVMFAdd:
            case LLVMSub:
            case LLVMFSub:
            case LLVMMul:
            case LLVMFMul:
            case LLVMSDiv:
            case LLVMFDiv:
            case LLVMSRem:
            case LLVMFRem:
            case LLVMAnd:
            case LLVMOr:
            case LLVMXor:
            {
                // get the operands
                VALUE_TYPE op1 = LLVMGetOperand(currInstruction, 0);
                VALUE_TYPE op2 = LLVMGetOperand(currInstruction, 1);

                bool ischanged = false;

                for (auto it = propMap.begin(); it != propMap.end(); ++it)
                {
                    int checkOp = LLVMGetInstructionOpcode(*it);
                    VALUE_TYPE checkOp1 = LLVMGetOperand(*it, 0);
                    VALUE_TYPE checkOp2 = LLVMGetOperand(*it, 1);

                    if (opcode != checkOp || op1 != checkOp1 || op2 != checkOp2) continue;

                    LLVMReplaceAllUsesWith(currInstruction, *it);
                    ischanged = true;
                    deadInstrs[block].push_back(currInstruction);
                    break;
                }

                if (!ischanged) propMap.insert(currInstruction);
            }
            break;

            default:
                break;
        }
        
        currInstruction = LLVMGetNextInstruction(currInstruction);
    }

    // Code for DFS

    int numSucc = LLVMGetNumSuccessors(term);

    for (int i = 0; i < numSucc; ++i)
    {
        BLOCK_TYPE succBlk = LLVMGetSuccessor(term, i);

        if (path.find(succBlk) != path.end()) {
            printf("%s\n", "Successor lies at a back edge");
            continue;
        }

        path.insert(block);
        cseBasicBlock(succBlk, globalBuilder, path, propMap, deadInstrs);
        path.erase(block);
    }
}