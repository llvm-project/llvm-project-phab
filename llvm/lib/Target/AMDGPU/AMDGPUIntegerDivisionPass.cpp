//===-- IntegerDivisionPass.cpp - Expand div/mod instructions -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
//===----------------------------------------------------------------------===//

/*Modified Integer Division Pass*/

#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Utils/IntegerDivision.h"
#include "AMDGPU.h"
#include "AMDGPU64bitDivision.h"

using namespace llvm;

#define DEBUG_TYPE "AMDGPU integer-division"

namespace {

class AMDGPUIntegerDivision : public FunctionPass,
                        public InstVisitor<AMDGPUIntegerDivision, bool> {

  const TargetMachine *TM;
  const TargetLowering *TLI;

  int checkingVariable;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 

  bool shouldExpandDivRem(const BinaryOperator &I);

public:
  static char ID;
  explicit AMDGPUIntegerDivision(const TargetMachine *TM = nullptr)
    : FunctionPass(ID), TM(TM), TLI(nullptr) {
  }

  bool doInitialization(Module &M) override;
  bool runOnFunction(Function &F) override;

  const char *getPassName() const override {
    return "Integer Division Pass";
  }

  bool visitInstruction(Instruction &I) {
    return false;
  }

  bool visitSDiv(BinaryOperator &I);
  bool visitUDiv(BinaryOperator &I);
  bool visitSRem(BinaryOperator &I);
  bool visitURem(BinaryOperator &I);
};

} // End anonymous namespace

char AMDGPUIntegerDivision::ID = 0;
//char &llvm::IntegerDivisionID = AMDGPUIntegerDivision::ID;
INITIALIZE_TM_PASS(AMDGPUIntegerDivision, DEBUG_TYPE,"Expand integer division", false, false);

char &llvm::AMDGPUIntegerDivisionID = AMDGPUIntegerDivision::ID;
/*INITIALIZE_PASS_BEGIN(AMDGPUIntegerDivision, DEBUG_TYPE,
                      "Add AMDGPU function attributes", false, false)
INITIALIZE_PASS_END(AMDGPUIntegerDivision, DEBUG_TYPE,
                    "Add AMDGPU function attributes", false, false)*/

bool AMDGPUIntegerDivision::doInitialization(Module &M) {
  return false;
}

bool AMDGPUIntegerDivision::runOnFunction(Function &F) {
//  llvm_unreachable("does this happen");
  if (TM)
    TLI = TM->getSubtargetImpl(F)->getTargetLowering();

  else 
    errs()<<"\n Target Machine Not Initialized\n";

  for (Function::iterator BBI = F.begin(), BBE = F.end(); BBI != BBE; ++BBI) {
    BasicBlock *BB = &*BBI;
    for (BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; ++II) {
      Instruction *I = &*II;
      if (visit(*I)) {
        BBI = F.begin();
        break;
      }
    }
  }

  return false;
}

bool AMDGPUIntegerDivision::shouldExpandDivRem(const BinaryOperator &I) {
  assert(TLI);
  bool shouldExpandInIr = TLI && TLI->shouldExpandDivRemInIR(I);
  // TODO:Uthkarsh later modify to handle signed 64 bit too.  
  bool isUdiv64 = I.getOpcode() == Instruction::UDiv &&  I.getType()->getIntegerBitWidth() == 64;
  return shouldExpandInIr && isUdiv64;
}
/*TODO:Uthkarsh 
  Change the function calls to your own menthods instead of the in-built integer division which
  introduces more control flow. 
*/
bool AMDGPUIntegerDivision::visitSDiv(BinaryOperator &I) {
  if (shouldExpandDivRem(I)) {
    expandDivision(&I);
    return true;
  }
  return false;
}

bool AMDGPUIntegerDivision::visitUDiv(BinaryOperator &I) {
  // Should call the underlying IR expansion only for 64 bit divisions
  if (shouldExpandDivRem(I)) {
    AMDExpandUDivision(&I);
    return true;
  }
  return false;
}

bool AMDGPUIntegerDivision::visitSRem(BinaryOperator &I) {
  if (shouldExpandDivRem(I)) {
    expandRemainder(&I);
    return true;
  }
  return false;
}

bool AMDGPUIntegerDivision::visitURem(BinaryOperator &I) {
  if (shouldExpandDivRem(I)) {
    expandRemainder(&I);
    return true;
  }
  return false;
}

FunctionPass *llvm::createAMDGPUIntegerDivisionPass(const TargetMachine *TM) {
  return new AMDGPUIntegerDivision(TM);
}

//static RegisterPass<AMDGPUIntegerDivision> X("AMD GPU Integer Division", "IR level 64 bit integer division",false,false);