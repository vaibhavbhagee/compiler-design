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

}