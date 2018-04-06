#include "localOpt.hpp"

void localOpt(LLVMModuleRef mod);
void localDeadCodeRemoval(BLOCK_TYPE block);
void localOptBasicBlock(BLOCK_TYPE basicBlock);

void localOpt(LLVMModuleRef mod) {
	// Get first function of the module ref

	VALUE_TYPE currFunction = LLVMGetFirstFunction(mod);

	while (currFunction != NULL) {
		// Get all the basic blocks of this function and local optimise
		BLOCK_TYPE currBlock = LLVMGetFirstBasicBlock(currFunction);

		while (currBlock != NULL) {
            localOptBasicBlock(currBlock);
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

void localOptBasicBlock(BLOCK_TYPE basicBlock, int passes=2) {

    for (int i = 0; i < passes; i++) {
        localDeadCodeRemoval(basicBlock);
    }

}