/*===-- object.c - tool for testing libLLVM and llvm-c API ----------------===*\
|*                                                                            *|
|*                     The LLVM Compiler Infrastructure                       *|
|*                                                                            *|
|* This file is distributed under the University of Illinois Open Source      *|
|* License. See LICENSE.TXT for details.                                      *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements the --add-named-metadata-operand and --set-metadata   *|
|* commands in llvm-c-test.                                                   *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "llvm-c-test.h"

int llvm_add_named_metadata_operand(void) {
  LLVMContextRef C = LLVMGetGlobalContext();
  LLVMModuleRef M = LLVMModuleCreateWithName("Mod");
  LLVMValueRef Values[] = {
    LLVMValueAsMetadata(LLVMConstInt(LLVMInt32Type(), 0, 0))
  };

  // This used to trigger an assertion
  LLVMAddNamedMetadataOperand(M, "name", LLVMCreateMDNode(C, Values, 1));

  LLVMDisposeModule(M);

  return 0;
}

int llvm_set_metadata(void) {
  LLVMBuilderRef B = LLVMCreateBuilder();
  LLVMValueRef Values[] = {
    LLVMValueAsMetadata(LLVMConstInt(LLVMInt32Type(), 0, 0))
  };

  // This used to trigger an assertion
  LLVMSetMetadata(
      LLVMBuildRetVoid(B),
      LLVMGetMDKindID("kind", 4),
      LLVMCreateMDNode(LLVMGetGlobalContext(), Values, 1));

  LLVMDisposeBuilder(B);

  return 0;
}
