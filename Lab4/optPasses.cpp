#include "optPasses.hpp"
#include "utils.hpp"
#include <string>
#include <cstring>
#include <map>
#include <unordered_set>
#include <set>

void printMap(std::map<std::string, VALUE_TYPE> &propMap) {
    for (auto it = propMap.begin(); it != propMap.end(); ++it)
    {
        printf("%s %s\n", (it->first).c_str(), LLVMPrintValueToString(it->second));
    }
}

void printMap2(std::map<VALUE_TYPE, VALUE_TYPE> &propMap) {
    for (auto it = propMap.begin(); it != propMap.end(); ++it)
    {
        printf("%s %s\n", LLVMPrintValueToString(it->first), LLVMPrintValueToString(it->second));
    }
}


void copyPropagationBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder, 
    std::unordered_set<BLOCK_TYPE> &path, 
    std::map<std::string, VALUE_TYPE>& propMap, 
    std::map<VALUE_TYPE, VALUE_TYPE> &varLoadedVarMap, 
    std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs) {

    VALUE_TYPE term = LLVMGetBasicBlockTerminator(block);
    printf("%s %s\n", "Terminator: ", LLVMPrintValueToString(term));
    if (term == NULL) {
        printf("%s\n", "NULL TERMINATOR!");
        return;
    }

    // iterate over all the instructions
    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);
    while (currInstruction != NULL) {

        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            case LLVMLoad: // assuming ssa, track loads of the same mem location
            {
                VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 0);
                std::string lhsValName(LLVMGetValueName(lhs));
                std::string currInstName(LLVMGetValueName(currInstruction));

                propMap[currInstName] = lhs;

                // first load - keep it
                if (varLoadedVarMap.find(lhs) == varLoadedVarMap.end()) {
                    varLoadedVarMap[lhs] = currInstruction;
                }
                else { // discard subsequent loads, use the previous loaded variable
                    deadInstrs[block].push_back(currInstruction);
                }
            }
            break;

            case LLVMCall: // assuming that memory locations changed by calls only
            {
                // for function args, assume their mem locations are clobbered by the call
                for (int i = 0; i < LLVMGetNumOperands(currInstruction); i++) {
                    VALUE_TYPE operand = LLVMGetOperand(currInstruction, i);
                    std::string opName(LLVMGetValueName(operand));
                    if (propMap.find(opName) != propMap.end()) {
                        LLVMSetOperand(currInstruction, i, varLoadedVarMap[propMap[opName]]);
                    }

                    if (varLoadedVarMap.find(operand) != varLoadedVarMap.end()) {
                        // erase from the map, so next loaded var is propagated ahead
                        printf("%s\n", LLVMPrintValueToString(operand));
                        varLoadedVarMap.erase(operand);
                    }
                }
            }
            break;

            default:
            {
                // replace with loaded value in each operand
                int numOps = LLVMGetNumOperands(currInstruction);
                for (int i = 0; i < numOps; ++i) {
                    VALUE_TYPE operand = LLVMGetOperand(currInstruction, i);
                    std::string opName(LLVMGetValueName(operand));

                    if (propMap.find(opName) != propMap.end()) {
                        LLVMSetOperand(currInstruction, i, varLoadedVarMap[propMap[opName]]);
                    }
                }
            }
            break;
        }
        currInstruction = LLVMGetNextInstruction(currInstruction);
    }

    // DFS
    int numSucc = LLVMGetNumSuccessors(term);
    for (int i = 0; i < numSucc; ++i) {
        BLOCK_TYPE succBlk = LLVMGetSuccessor(term, i);

        if (path.find(succBlk) != path.end()) {
            printf("%s\n", "Successor lies at a back edge");
            continue;
        }

        path.insert(block);
        copyPropagationBasicBlock(succBlk, globalBuilder, path, propMap, varLoadedVarMap, deadInstrs);
        path.erase(block);
    }
}

void copyPropagationFunction(VALUE_TYPE currFunction, LLVMBuilderRef globalBuilder) {
    BLOCK_TYPE currBlock = LLVMGetEntryBasicBlock(currFunction);

    std::unordered_set<BLOCK_TYPE> path;
    std::map<std::string, VALUE_TYPE> propMap;
    std::map<VALUE_TYPE, VALUE_TYPE> varLoadedVarMap;
    std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > deadInstrs;

    while (currBlock != NULL) {
        copyPropagationBasicBlock(currBlock, globalBuilder, path, propMap, varLoadedVarMap, deadInstrs);
    }

    for (auto it = deadInstrs.begin(); it != deadInstrs.end(); ++it) {
        for (auto deadInstr : (it->second)) {
            LLVMInstructionEraseFromParent(deadInstr);
        }
    }
}

void cseBasicBlock(BLOCK_TYPE block, LLVMBuilderRef globalBuilder,
    std::unordered_set<BLOCK_TYPE> &path, 
    std::set<VALUE_TYPE> propMap, 
    std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > &deadInstrs) {

    VALUE_TYPE term = LLVMGetBasicBlockTerminator(block);
    if (term == NULL) {
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
    for (int i = 0; i < numSucc; ++i) {
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

void cseFunction(VALUE_TYPE currFunction, LLVMBuilderRef globalBuilder) {
    BLOCK_TYPE currBlock = LLVMGetEntryBasicBlock(currFunction);
    
    std::unordered_set<BLOCK_TYPE> path;
    std::set<VALUE_TYPE> propMap;
    std::map<BLOCK_TYPE, std::vector<VALUE_TYPE> > deadInstrs;

    if (currBlock != NULL)
        cseBasicBlock(currBlock, globalBuilder, path, propMap, deadInstrs);

    for (auto it = deadInstrs.begin(); it != deadInstrs.end(); ++it) {
        for (auto deadInstr : (it->second)) {
            LLVMInstructionEraseFromParent(deadInstr);
        }
    }
}

void optPasses(LLVMModuleRef mod, LLVMBuilderRef globalBuilder) {
    // prepare function pass manager and all the passes
    PASSMANAGER_TYPE funcPassManager = LLVMCreateFunctionPassManagerForModule(mod);

    // Mem2Reg - full SSA
    LLVMAddPromoteMemoryToRegisterPass(funcPassManager);

    // add passes - Mem2Reg, const fold, cse, copy prop, dce, code motion(pre)
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
        // cseFunction(currFunction, globalBuilder);
        currFunction = LLVMGetNextFunction(currFunction);
    }
    LLVMFinalizeFunctionPassManager(funcPassManager);
}