//===-- FunctionDataSharing.cpp - Mark pointers as shared -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "NVPTX.h"
#include "NVPTXUtilities.h"
#include "NVPTXTargetMachine.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace llvm {
void initializeNVPTXFunctionDataSharingPass(PassRegistry &);
}

namespace {
class NVPTXFunctionDataSharing : public FunctionPass {
  bool runOnFunction(Function &F) override;
  bool runOnKernelFunction(Function &F);
  bool runOnDeviceFunction(Function &F);

public:
  static char ID; // Pass identification, replacement for typeid
  NVPTXFunctionDataSharing(const NVPTXTargetMachine *TM = nullptr)
      : FunctionPass(ID), TM(TM) {}
  StringRef getPassName() const override {
    return "Function level data sharing pass.";
  }

private:
  const NVPTXTargetMachine *TM;
};
} // namespace

char NVPTXFunctionDataSharing::ID = 1;

INITIALIZE_PASS(NVPTXFunctionDataSharing, "nvptx-function-data-sharing",
                "Function Data Sharing (NVPTX)", false, false)

static void markPointerAsShared(Value *Ptr) {
  if (Ptr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_SHARED)
    return;

  // Deciding where to emit the addrspacecast pair.
  // Insert right after Ptr if Ptr is an instruction.
  BasicBlock::iterator InsertPt =
      std::next(cast<Instruction>(Ptr)->getIterator());
  assert(InsertPt != InsertPt->getParent()->end() &&
         "We don't call this function with Ptr being a terminator.");

  auto *PtrInShared = new AddrSpaceCastInst(
      Ptr, PointerType::get(Ptr->getType()->getPointerElementType(),
                            ADDRESS_SPACE_SHARED),
      Ptr->getName(), &*InsertPt);
  // Old version
  auto *PtrInGeneric = new AddrSpaceCastInst(PtrInShared, Ptr->getType(),
                                             Ptr->getName(), &*InsertPt);
  // Replace with PtrInGeneric all uses of Ptr except PtrInShared.
  Ptr->replaceAllUsesWith(PtrInGeneric);
  PtrInShared->setOperand(0, Ptr);
}

// =============================================================================
// Main function for this pass.
// =============================================================================
bool NVPTXFunctionDataSharing::runOnKernelFunction(Function &F) {
  if (TM && TM->getDrvInterface() == NVPTX::CUDA) {
    for (auto &B : F) {
      for (auto &I : B) {
        auto *AI = dyn_cast<AllocaInst>(&I);
        if (!AI)
          continue;
        if (AI->getType()->isPointerTy() && ptrIsStored(AI))
          markPointerAsShared(AI);
      }
    }
  }

  return true;
}

// Device functions only need to copy byval args into local memory.
bool NVPTXFunctionDataSharing::runOnDeviceFunction(Function &F) {
  return true;
}

bool NVPTXFunctionDataSharing::runOnFunction(Function &F) {
  return isKernelFunction(F) ? runOnKernelFunction(F) : runOnDeviceFunction(F);
}

FunctionPass *
llvm::createNVPTXFunctionDataSharingPass(const NVPTXTargetMachine *TM) {
  return new NVPTXFunctionDataSharing(TM);
}
