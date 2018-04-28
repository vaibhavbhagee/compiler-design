#ifndef LLVM_HEADERS_H
#define LLVM_HEADERS_H

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Initialization.h>
#include <llvm-c/Transforms/Scalar.h>

#define VALUE_TYPE LLVMValueRef
#define FUNCTION_TYPE LLVMValueRef
#define BLOCK_TYPE LLVMBasicBlockRef
#define DATATYPE_TYPE LLVMTypeRef
#define PASSMANAGER_TYPE LLVMPassManagerRef

// using namespace llvm;

#endif