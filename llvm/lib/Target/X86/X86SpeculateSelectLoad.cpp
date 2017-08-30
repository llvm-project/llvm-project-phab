//===-- X86SpeculateSelectLoad- Speculatively load operands of a select --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
// For a select instruction where the operands are address calculations
// of two independent loads, the pass tries to speculate the loads and
// feed them into the select instruction, this allows early parallel execution
// of the loads and possibly memory folding into the CMOV instructions later on.
// The pass currently only handles cases where the loads are elements of the
// same struct.
//===---------------------------------------------------------------------===//

#include "X86.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include <deque>

using namespace llvm;

#define DEBUG_TYPE "x86speculateload"

namespace llvm {
void initializeX86SpeculateSelectLoadPassPass(PassRegistry &);
}

namespace {

class X86SpeculateSelectLoadPass : public FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid.

  X86SpeculateSelectLoadPass() : FunctionPass(ID) {
    initializeX86SpeculateSelectLoadPassPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &Fn) override;
  StringRef getPassName() const override {
    return "X86 Speculatively load before select instruction";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetTransformInfoWrapperPass>();
  }

private:
  bool OptimizeSelectInst(SelectInst *SI);
  const DataLayout *DL;
  const TargetTransformInfo *TTI;
  SmallVector<Instruction *, 2> InstrForRemoval;
};
}

FunctionPass *llvm::createX86SpeculateSelectLoadPass() {
  return new X86SpeculateSelectLoadPass();
}

char X86SpeculateSelectLoadPass::ID = 0;

INITIALIZE_PASS_BEGIN(X86SpeculateSelectLoadPass, "x86-speculateload",
                      "X86 Speculatively load before select instruction", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(X86SpeculateSelectLoadPass, "x86-speculateload",
                    "X86 Speculatively load before select instruction", false,
                    false)

bool X86SpeculateSelectLoadPass::OptimizeSelectInst(SelectInst *SI) {
  GetElementPtrInst *GEPIT = dyn_cast<GetElementPtrInst>(SI->getTrueValue());
  GetElementPtrInst *GEPIF = dyn_cast<GetElementPtrInst>(SI->getFalseValue());
  if (GEPIT == nullptr || GEPIF == nullptr)
    return false;

  // The pass currently only handles cases where the loads are elements of the
  // same struct.
  if (!GEPIT->getSourceElementType()->isStructTy() ||
      !GEPIF->getSourceElementType()->isStructTy() ||
      GEPIT->getPointerOperand() != GEPIF->getPointerOperand() ||
      !GEPIT->hasAllConstantIndices() || !GEPIF->hasAllConstantIndices())
    return false;

  if (!SI->hasOneUse())
    return false;

  // Bail out if there is a good chance we'll be loading from two different
  // cache lines instead of one.
  // This is a shallow check of the struct's fields layout.
  // TODO: deeper check for GEPs with more than one index.
  if (StructType *STy = dyn_cast<StructType>(GEPIT->getSourceElementType())) {
    // Get the indices of the elements in the struct

    ConstantInt *Idx1 = dyn_cast<ConstantInt>(GEPIT->getOperand(2));
    ConstantInt *Idx2 = dyn_cast<ConstantInt>(GEPIF->getOperand(2));
    const StructLayout *STL = DL->getStructLayout(STy);
    if (Idx1 && Idx2 && STL) {
      signed offset1 = STL->getElementOffset(Idx1->getZExtValue());
      signed offset2 = STL->getElementOffset(Idx2->getZExtValue());
      unsigned dist = abs(offset1 - offset2);
      assert((TTI->getCacheLineSize() > 0) &&
             "CacheLineSize information is missing for X86");
      if (dist > TTI->getCacheLineSize())
        return false;
    }
  }

  for (User *U : SI->users()) {
    if (LoadInst *LI = dyn_cast<LoadInst>(U)) {
      if (!LI->isSimple())
        return false;
      IRBuilder<> Builder(SI);
      LoadInst *LT = Builder.CreateAlignedLoad(GEPIT, LI->getAlignment());
      LoadInst *LF = Builder.CreateAlignedLoad(GEPIF, LI->getAlignment());
      SelectInst *NewSI =
          cast<SelectInst>(Builder.CreateSelect(SI->getCondition(), LT, LF));
      LI->replaceAllUsesWith(NewSI);
      InstrForRemoval.push_back(LI);
      InstrForRemoval.push_back(SI);

      return true;
    }
  }
  return false;
}

bool X86SpeculateSelectLoadPass::runOnFunction(Function &F) {
  if (skipFunction(F) || F.optForSize())
    return false;

  DL = &F.getParent()->getDataLayout();
  TTI = &getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);

  bool Changed = false;

  for (auto &BB : F)
    for (auto &I : BB)
      if (SelectInst *SI = dyn_cast<SelectInst>(&I))
        Changed |= OptimizeSelectInst(SI);

  for (auto Instr : InstrForRemoval)
    Instr->eraseFromParent();

  InstrForRemoval.clear();

  return Changed;
}

