//===-------------------- SubstituteLoadWithSubShr.cpp --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass recognizes cases where a load can be replaced by subtract and
// shift right instructions.
//
// In case of loading integer value from an array of constants which is defined
// as follows:
//
//   int array[SIZE] = {0x0, 0x1, 0x3, 0x7, 0xF ..., 2^(SIZE-1) - 1}
//
// array[idx] can be replaced with (0xFFFFFFFF >> (sub 32, idx))
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/Local.h"
#include <iostream>

using namespace llvm;

#define DEBUG_TYPE "substitute-load-with-sub-shr"

namespace {

class SubstituteLoadWithSubShr : public FunctionPass {

public:
  static char ID;
  SubstituteLoadWithSubShr() : FunctionPass(ID) {
    initializeSubstituteLoadWithSubShrPass(*PassRegistry::getPassRegistry());
  }

  StringRef getPassName() const override {
    return "Substitute Load With Sub Shr";
  }

  bool runOnFunction(Function &F) override;

private:
  bool
  ReplaceLoadWithSubShr(LoadInst *LI, SmallVector<Instruction *, 32> &DeadInsts,
                        SmallVector<GlobalVariable *, 32> &PossibleDeadArrays);
};
} // end anonymous namespace.

char SubstituteLoadWithSubShr::ID = 0;
INITIALIZE_PASS_BEGIN(SubstituteLoadWithSubShr, DEBUG_TYPE,
                      "SubstituteLoadWithSubShr", false, false)
INITIALIZE_PASS_END(SubstituteLoadWithSubShr, DEBUG_TYPE,
                    "SubstituteLoadWithSubShr", false, false)

FunctionPass *llvm::createSubstituteLoadWithSubShrPass() {
  return new SubstituteLoadWithSubShr();
}

bool SubstituteLoadWithSubShr::ReplaceLoadWithSubShr(
    LoadInst *LI, SmallVector<Instruction *, 32> &DeadInsts,
    SmallVector<GlobalVariable *, 32> &PossibleDeadArrays) {

  if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(LI->getOperand(0))) {
    if (GlobalVariable *GV = dyn_cast<GlobalVariable>(GEP->getOperand(0))) {
      if (GV->isConstant() && GV->hasDefinitiveInitializer()) {

        Constant *Init = GV->getInitializer();
        Type *Ty = Init->getType();
        if ((!isa<ConstantArray>(Init) && !isa<ConstantDataArray>(Init)) ||
            !Ty->getArrayElementType()->isIntegerTy() ||
            Ty->getArrayNumElements() >
                Ty->getArrayElementType()->getScalarSizeInBits())
          return nullptr;

        // Check if the array's constant elements are suitable to our case.
        uint64_t ArrayElementCount = Init->getType()->getArrayNumElements();
        for (uint64_t i = 0; i < ArrayElementCount; i++) {
          ConstantInt *Elem =
              dyn_cast<ConstantInt>(Init->getAggregateElement(i));
          if (Elem->getZExtValue() != (((uint64_t)1 << i) - 1))
            return nullptr;
        }

        IRBuilder<> Builder(LI->getContext());
        Builder.SetInsertPoint(LI);

        // Do the transformation (assuming 32-bit elements):
        // -> elemPtr = getelementptr @array, 0, idx
        //    elem = load elemPtr
        // <- hiBit = 32 - idx
        //    elem = 0xFFFFFFFF >> hiBit
        Type *ElemTy = Ty->getArrayElementType();
        Value *LoadIdx = Builder.CreateZExtOrTrunc(GEP->getOperand(2), ElemTy);

        APInt ElemSize =
            APInt(ElemTy->getScalarSizeInBits(), ElemTy->getScalarSizeInBits());
        Constant *ElemSizeConst = Constant::getIntegerValue(ElemTy, ElemSize);
        Value *Sub = Builder.CreateSub(ElemSizeConst, LoadIdx);

        Constant *AllOnes = Constant::getAllOnesValue(ElemTy);
        Value *LShr = Builder.CreateLShr(AllOnes, Sub);

        LI->replaceAllUsesWith(LShr);
        DeadInsts.push_back(LI);

        // Insert the array to mark it as potentially dead.
        if (find(PossibleDeadArrays, GV) == PossibleDeadArrays.end())
          PossibleDeadArrays.push_back(GV);

        return true;
      }
    }
  }
  return false;
}

bool SubstituteLoadWithSubShr::runOnFunction(Function &F) {
  SmallVector<Instruction *, 32> DeadInsts;
  SmallVector<GlobalVariable *, 32> PossibleDeadArrays;
  bool Changed = false;

  // Perform the transformation if possible while keep tracking of dead
  // instructions and potential dead arrays.
  for (auto &I : instructions(F)) {
    if (LoadInst *LI = dyn_cast<LoadInst>(&I))
      Changed |= ReplaceLoadWithSubShr(LI, DeadInsts, PossibleDeadArrays);
  }

  if (Changed) {
    for (auto &I : DeadInsts) {
      RecursivelyDeleteTriviallyDeadInstructions(I);
    }

    for (auto &GV : PossibleDeadArrays) {
      if (GV->use_empty() && GV->isDiscardableIfUnused())
        GV->eraseFromParent();
    }
  }

  return Changed;
}
