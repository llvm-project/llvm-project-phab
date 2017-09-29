//===-------- PPCColdCC.cpp - Expand memory instinsics  -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
///
//===----------------------------------------------------------------------===//

#include "PPC.h"
#include "PPCTargetMachine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

#define DEBUG_TYPE "coldcc"

STATISTIC(NumInternalFunc, "Number of internal functions");
STATISTIC(NumColdCC, "Number of functions marked coldcc");

static cl::opt<bool> EnableColdCCStressTest(
    "ppc-enable-coldcc-stress-test",
    cl::desc("Enable stress test of PPC coldcc."),
    cl::init(false), cl::Hidden);


static cl::opt<bool> EnableColdCC(
    "ppc-enable-coldcc",
    cl::desc("Enable the PPC pass to use coldcc on internal functions."),
    cl::init(false), cl::Hidden);

namespace {
class PPCColdCC : public ModulePass {
public:
  static char ID;
  PPCColdCC() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<BlockFrequencyInfoWrapperPass>();
  }

  bool runOnModule(Module &M) override;

  bool hasColdCallSite(Function &F);
  bool isColdCallSite(CallSite CS, BlockFrequencyInfo *CallerBFI);
  bool markCallSitesColdCC(Function &F);
  StringRef getPassName() const override {
    return "PPC add coldcc";
  }
  const PPCSubtarget *ST;
};
} // namespace

bool PPCColdCC::hasColdCallSite(Function &F) {
  unsigned NumCallSites, NumColdCallSites;
  NumCallSites = 0; NumColdCallSites = 0;
  for (User *U : F.users()) {
    if (isa<BlockAddress>(U))
      continue;
    CallSite CS(cast<Instruction>(U));
    NumCallSites++;
    Function *CallerFunc = CS.getInstruction()->getParent()->getParent();
    BlockFrequencyInfo *CallerBFI =
        &getAnalysis<BlockFrequencyInfoWrapperPass>(*CallerFunc).getBFI();
    if (isColdCallSite(CS, CallerBFI)) {
      NumColdCallSites++;
    }
  }

  if (NumCallSites == NumColdCallSites)
    return true;
  else
    return false;
}

bool PPCColdCC::isColdCallSite(CallSite CS,
                                    BlockFrequencyInfo *CallerBFI) {

  const BranchProbability ColdProb(10, 100);
  auto CallSiteBB = CS.getInstruction()->getParent();
  auto CallSiteFreq = CallerBFI->getBlockFreq(CallSiteBB);
  auto CallerEntryFreq =
      CallerBFI->getBlockFreq(&(CS.getCaller()->getEntryBlock()));

  return CallSiteFreq < CallerEntryFreq * ColdProb;
}

bool PPCColdCC::markCallSitesColdCC(Function &F) {
  for (User *U : F.users()) {
    if (isa<BlockAddress>(U))
      continue;
    CallSite CS(cast<Instruction>(U));
    CS.setCallingConv(CallingConv::Cold);
  }

  return true;
}

bool PPCColdCC::runOnModule(Module &M) {
  bool Changed = false;
  auto *TPC = getAnalysisIfAvailable<TargetPassConfig>();
  if (!TPC)
    return false;

  auto &TM = TPC->getTM<PPCTargetMachine>();
  if (EnableColdCC) {
    for (Function &F : M) {
      ST = TM.getSubtargetImpl(F);
      if (!ST->isSVR4ABI())
        return false;
      if (F.hasLocalLinkage() && !F.isVarArg() && !F.hasAddressTaken()) {
        NumInternalFunc++;
        if (hasColdCallSite(F) || EnableColdCCStressTest) {
          F.setCallingConv(CallingConv::Cold);
          Changed = markCallSitesColdCC(F);
          NumColdCC++;
        }
      }
    }
  }
  return Changed;
}

ModulePass *llvm::createPPCColdCCPass() { return new PPCColdCC(); }

char PPCColdCC::ID = 0;
INITIALIZE_PASS_BEGIN(PPCColdCC, "PPCColdCC",
                      "Add coldcc", false, false)

INITIALIZE_PASS_DEPENDENCY(BlockFrequencyInfoWrapperPass)

INITIALIZE_PASS_END(PPCColdCC, "PPCColdCC",
                    "Add coldcc", false, false)
