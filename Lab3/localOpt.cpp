#include "localOpt.hpp"
#include <string>
#include <cstring>

void localOpt(LLVMModuleRef mod);
void localDeadCodeRemoval(BLOCK_TYPE block);
void localOptBasicBlock(BLOCK_TYPE basicBlock, int passes);
void localConstantPropagation(BLOCK_TYPE block);

void localOpt(LLVMModuleRef mod) {
	// Get first function of the module ref

	VALUE_TYPE currFunction = LLVMGetFirstFunction(mod);

	while (currFunction != NULL) {
		// Get all the basic blocks of this function and local optimise
		BLOCK_TYPE currBlock = LLVMGetFirstBasicBlock(currFunction);

		while (currBlock != NULL) {
            localOptBasicBlock(currBlock, 2);
			currBlock = LLVMGetNextBasicBlock(currBlock);
		}

		currFunction = LLVMGetNextFunction(currFunction);
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
                LLVMUseRef lhs = LLVMGetOperandUse(currInstruction, 1);
                LLVMUseRef first_use = LLVMGetFirstUse(LLVMGetUsedValue(lhs));
                LLVMUseRef next = LLVMGetNextUse(first_use);
                
                if (next == NULL) { // no further usage
                    deadInstrs.push_back(currInstruction);
                }
            }
            break;

            case LLVMAlloca:
            {
                LLVMUseRef first_use = LLVMGetFirstUse(currInstruction);
                LLVMUseRef next = LLVMGetNextUse(first_use);
                
                if (next == NULL) { // no further usage
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

void printMap(std::map<std::string, VALUE_TYPE> &propMap) {
	for (auto it = propMap.begin(); it != propMap.end(); ++it)
	{
		printf("%s %s\n", (it->first).c_str(), LLVMPrintValueToString(it->second));
	}
}

void constantFold(VALUE_TYPE currInstruction, int opcode) {
	LLVMPositionBuilderBefore(globalBuilder, currInstruction);
    // VALUE_TYPE clone = LLVMInstructionClone(currInstruction);
    std::string curInstName(LLVMGetValueName(currInstruction));
    curInstName = "new_" + curInstName;
    // printf("%s %s %d\n", curInstName.c_str(), "num ops: ", numOps );
	VALUE_TYPE newInst = LLVMBuildBinOp(globalBuilder, (LLVMOpcode)opcode, LLVMGetOperand(currInstruction, 0), LLVMGetOperand(currInstruction, 1), curInstName.c_str());
	// LLVMInsertIntoBuilder(globalBuilder, newInst);

	LLVMReplaceAllUsesWith(currInstruction, newInst);
	// deadInstrs.push_back(currInstruction);
}

void localConstantPropagation(BLOCK_TYPE block) {

	std::map<std::string, VALUE_TYPE> propMap;

    // iterate over all the instructions
    std::vector<VALUE_TYPE> deadInstrs;
    VALUE_TYPE currInstruction = LLVMGetFirstInstruction(block);

    while (currInstruction != NULL) {
        /*
            - LHS of assign not used again ever
            - Allocated variable never used
        */
    	// printf("%s\n", LLVMPrintValueToString(currInstruction));
        int opcode = LLVMGetInstructionOpcode(currInstruction);
        switch (opcode) {
            case LLVMStore:
            {
                LLVMUseRef lhs = LLVMGetOperandUse(currInstruction, 1);
                LLVMUseRef rhs = LLVMGetOperandUse(currInstruction, 0);
                VALUE_TYPE useVal = LLVMGetUsedValue(lhs);
                VALUE_TYPE maybeConstVal = LLVMGetUsedValue(rhs);
                std::string valName(LLVMGetValueName(useVal));

                if (LLVMIsConstant(maybeConstVal)) {
                	propMap[valName] = maybeConstVal;

                	// printMap(propMap);
                }
            }
            break;
            case LLVMLoad:
            {
                LLVMUseRef lhs = LLVMGetOperandUse(currInstruction, 0);
                VALUE_TYPE useVal = LLVMGetUsedValue(lhs);
                std::string valName(LLVMGetValueName(useVal));
                std::string curInstName(LLVMGetValueName(currInstruction));

                if (propMap.find(valName) != propMap.end()) {
                	propMap[curInstName] = propMap[valName];
                	deadInstrs.push_back(currInstruction);
                }
            }
            break;
            case LLVMAlloca:
            case LLVMGetElementPtr:
            break;
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
                int numOps = LLVMGetNumOperands(currInstruction);

                for (int i = 0; i < numOps; ++i) {
                	VALUE_TYPE op = LLVMGetOperand(currInstruction, i);
                	std::string opName(LLVMGetValueName(op));

                	if (propMap.find(opName) != propMap.end()) {
                		// printf("%s\n", "Replacing");
                		LLVMSetOperand(currInstruction, i, propMap[opName]);
                		// deadInstrs.push_back(currInstruction);		
                	}
                }

                constantFold(currInstruction, opcode);

                deadInstrs.push_back(currInstruction);
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

void localOptBasicBlock(BLOCK_TYPE basicBlock, int passes=2) {

    for (int i = 0; i < passes; i++) {
        localDeadCodeRemoval(basicBlock);
        localConstantPropagation(basicBlock);
        // localConstantPropagation(basicBlock);
    }

}