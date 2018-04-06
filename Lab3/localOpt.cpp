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
			localDeadCodeRemoval(currBlock);
            localDeadCodeRemoval(currBlock);
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

void localOptBasicBlock(BLOCK_TYPE basicBlock) {

	// Get the instruction of the basic block
	VALUE_TYPE currInstruction = LLVMGetFirstInstruction(basicBlock);
    int j = 0;

	while (currInstruction != NULL) {
        j += 1;
		printf("%s %d :", "INSTRUCTION", j);
		printf("%s   num_ops:", LLVMPrintValueToString(currInstruction));

		// Get the number of operands
		int numOps = LLVMGetNumOperands(currInstruction);
		printf("%d\n", numOps);

		for (int i = 1; i < numOps; ++i) {
			// VALUE_TYPE op = LLVMGetOperand(currInstruction, i);
			LLVMUseRef u = LLVMGetOperandUse(currInstruction, i);

			while (u != NULL) {
				VALUE_TYPE uv = LLVMGetUsedValue(u);
				VALUE_TYPE usv = LLVMGetUser(u);
				printf("USE: %s\n", LLVMPrintValueToString(uv));
				printf("USER: %s\n", LLVMPrintValueToString(usv));
				u = LLVMGetNextUse(u);
			}
            printf("\n");

			// printf("%s\n", LLVMPrintValueToString(op));
		}

		currInstruction = LLVMGetNextInstruction(currInstruction);
		printf("%s\n", "");
	}

	printf("%s\n", "");
}