#include "localOpt.hpp"

void localOpt(LLVMModuleRef mod);
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

void localOptBasicBlock(BLOCK_TYPE basicBlock) {

	// Get the instruction of the basic block
	VALUE_TYPE currInstruction = LLVMGetFirstInstruction(basicBlock);

	while (currInstruction != NULL) {
		printf("%s\n", "NEW INSTRUCTION: ");
		printf("%s\n", LLVMPrintValueToString(currInstruction));

		// Get the number of operands
		int numOps = LLVMGetNumOperands(currInstruction);
		printf("%d\n", numOps);

		for (int i = 0; i < numOps; ++i) {
			// VALUE_TYPE op = LLVMGetOperand(currInstruction, i);
			printf("%s\n", "USE:");
			LLVMUseRef u = LLVMGetOperandUse(currInstruction, i);

			while (u != NULL) {
				printf("%s\n", "NEXT USE:");
				VALUE_TYPE uv = LLVMGetUsedValue(u);
				VALUE_TYPE usv = LLVMGetUser(u);
				printf("USE: %s\n", LLVMPrintValueToString(uv));
				printf("USER: %s\n", LLVMPrintValueToString(usv));
				u = LLVMGetNextUse(u);
			}

			// printf("%s\n", LLVMPrintValueToString(op));
		}

		currInstruction = LLVMGetNextInstruction(currInstruction);
		printf("%s\n", "");
	}

	printf("%s\n", "");
}