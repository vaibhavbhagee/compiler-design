#include "localOpt.hpp"
#include <string>
#include <cstring>

void printMap(std::map<std::string, VALUE_TYPE> &propMap) {
    for (auto it = propMap.begin(); it != propMap.end(); ++it)
    {
        printf("%s %s\n", (it->first).c_str(), LLVMPrintValueToString(it->second));
    }
}

void localDeadCodeRemoval(BLOCK_TYPE block) {

    // iterate over all the instructions
    std::vector<VALUE_TYPE> deadInstrs;
    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);

    while (currInstruction != NULL) {
        /*
            - LHS of assign not used again ever
            - Allocated variable never used
        */
        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            case LLVMStore:
            {
                VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 1);
                LLVMUseRef first_use = LLVMGetFirstUse(lhs);
                LLVMUseRef next = LLVMGetNextUse(first_use);
                
                if (next == NULL) { // no further usage
                    deadInstrs.push_back(currInstruction);
                }
            }
            break;

            case LLVMAlloca:
            {
                LLVMUseRef first_use = LLVMGetFirstUse(currInstruction);
                if (first_use == NULL) { // no usage of allocated var
                    deadInstrs.push_back(currInstruction);
                    break;
                }

                // no further usage
                LLVMUseRef next = LLVMGetNextUse(first_use);                
                if (next == NULL) {
                    deadInstrs.push_back(currInstruction);
                }
            }
            break;
            
        }
        currInstruction = LLVMGetNextInstruction(currInstruction);
    }

    // remove all dead instructions
    for (auto deadInstr : deadInstrs) {
        LLVMInstructionEraseFromParent(deadInstr);
    }
}

bool constantFold(BLOCK_TYPE block, VALUE_TYPE currInstruction) {
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

            if (LLVMIsConstant(op1) && LLVMIsConstant(op2)) { // if both constants, evaluate result and replace
                VALUE_TYPE newInst = LLVMBuildBinOp(globalBuilder, (LLVMOpcode)opcode, op1, op2, "");
                LLVMReplaceAllUsesWith(currInstruction, newInst);
                return true;
            }
        }
        break;
    }
    return false;
}

void localConstantPropagation(BLOCK_TYPE block) {
    std::map<std::string, VALUE_TYPE> propMap;

    // iterate over all the instructions
    std::vector<VALUE_TYPE> deadInstrs;
    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);

    while (currInstruction != NULL) {

        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            case LLVMStore: // if rhs is constant, update value in map
            {
                VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 1);
                VALUE_TYPE rhs = LLVMGetOperand(currInstruction, 0);
                std::string valName(LLVMGetValueName(lhs));

                if (LLVMIsConstant(rhs)) {
                	propMap[valName] = rhs;
                }
            }
            break;

            case LLVMLoad: // if lhs is in map, get value from that and remove load
            {
                VALUE_TYPE lhs = LLVMGetOperand(currInstruction, 0);
                std::string valName(LLVMGetValueName(lhs));
                std::string currInstName(LLVMGetValueName(currInstruction));

                if (propMap.find(valName) != propMap.end()) {
                	propMap[currInstName] = propMap[valName];
                	deadInstrs.push_back(currInstruction);
                }
            }
            break;

            case LLVMAlloca:
            case LLVMGetElementPtr:
            break;

            default:
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
        if (constantFold(block, currInstruction)) {
            deadInstrs.push_back(currInstruction);
        }
        
        currInstruction = LLVMGetNextInstruction(currInstruction);
    }

    // remove all dead instructions
    for (auto deadInstr : deadInstrs) {
        LLVMInstructionEraseFromParent(deadInstr);
    }
}



void localOptBasicBlock(BLOCK_TYPE basicBlock, int passes) {

    for (int i = 0; i < passes; i++) {
        localDeadCodeRemoval(basicBlock);
        localConstantPropagation(basicBlock);
        // constantFold(basicBlock);
    }

}

void localOpt(LLVMModuleRef mod) {
    // Get first function of the module ref

    VALUE_TYPE currFunction = LLVMGetFirstFunction(mod);

    while (currFunction != NULL) {
        // Get all the basic blocks of this function and local optimise
        BLOCK_TYPE currBlock = LLVMGetFirstBasicBlock(currFunction);

        while (currBlock != NULL) {
            localOptBasicBlock(currBlock, 1);
            currBlock = LLVMGetNextBasicBlock(currBlock);
        }

        currFunction = LLVMGetNextFunction(currFunction);
    }
}