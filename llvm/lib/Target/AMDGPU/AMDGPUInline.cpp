//===- AMDGPUInline.cpp - Code to perform simple function inlining --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/Inliner.h"

using namespace llvm;

#define DEBUG_TYPE "inline"

static cl::opt<int>
HintThreshold("amdgpu-inlinehint-threshold", cl::Hidden, cl::init(12000),
              cl::desc("Threshold for inlining functions with inline hint"));

static cl::opt<int>
ArgAllocaCost("amdgpu-inline-arg-alloca-cost", cl::Hidden, cl::init(20000),
              cl::desc("Cost of alloca argument"));

static cl::opt<unsigned>
ArgAllocaCutoff("amdgpu-inline-arg-alloca-cutoff", cl::Hidden, cl::init(256),
                cl::desc("Maximum alloca size to use for inline cost"));

static cl::opt<size_t>
MaxBB("amdgpu-inline-max-bb", cl::Hidden, cl::init(300),
      cl::desc("Maximum BB number allowed in a function after inlining"));

namespace {

class AMDGPUInliner : public LegacyInlinerBase {

public:
  AMDGPUInliner() : LegacyInlinerBase(ID) {
    initializeAMDGPUInlinerPass(*PassRegistry::getPassRegistry());
    Params = getInlineParams(2000);
    Params.HintThreshold = HintThreshold;
  }

  static char ID; // Pass identification, replacement for typeid

  unsigned getInlineThreshold(CallSite CS) const;

  InlineCost getInlineCost(CallSite CS) override;

  bool runOnSCC(CallGraphSCC &SCC) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  TargetTransformInfoWrapperPass *TTIWP;

  InlineParams Params;
};

} // end anonymous namespace

char AMDGPUInliner::ID = 0;
INITIALIZE_PASS_BEGIN(AMDGPUInliner, "amdgpu-inline",
                "AMDGPU Function Integration/Inlining", false, false)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_END(AMDGPUInliner, "amdgpu-inline",
                "AMDGPU Function Integration/Inlining", false, false)

Pass *llvm::createAMDGPUFunctionInliningPass() { return new AMDGPUInliner(); }

bool AMDGPUInliner::runOnSCC(CallGraphSCC &SCC) {
  TTIWP = &getAnalysis<TargetTransformInfoWrapperPass>();
  return LegacyInlinerBase::runOnSCC(SCC);
}

void AMDGPUInliner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetTransformInfoWrapperPass>();
  LegacyInlinerBase::getAnalysisUsage(AU);
}

unsigned AMDGPUInliner::getInlineThreshold(CallSite CS) const {
  int Thres = Params.DefaultThreshold;

  Function *Caller = CS.getCaller();
  // Listen to the inlinehint attribute when it would increase the threshold
  // and the caller does not need to minimize its size.
  Function *Callee = CS.getCalledFunction();
  bool InlineHint = Callee && !Callee->isDeclaration() &&
    Callee->hasFnAttribute(Attribute::InlineHint);
  if (InlineHint && HintThreshold > Thres
      && !Caller->hasFnAttribute(Attribute::MinSize))
    Thres = HintThreshold;

  const DataLayout &DL = Caller->getParent()->getDataLayout();
  if (Callee) {
    // If we have a pointer to private array passed into a function
    // it will not be optimized out, leaving scratch usage.
    // Increase the inline threshold to allow inliniting in this case.
    uint64_t AllocaSize = 0;
    for (Value *PtrArg : CS.args()) {
      if (!PtrArg->getType()->isPointerTy())
        continue;
      PtrArg = GetUnderlyingObject(PtrArg, DL);
      if (const AllocaInst *AI = dyn_cast<AllocaInst>(PtrArg)) {
        Type *Ty = AI->getAllocatedType();
        if (!Ty->isSized())
          continue;
        AllocaSize += DL.getTypeAllocSize(Ty);
        // If the amount of stack memory is excessive we will not be able
        // to get rid of the scratch anyway, bail out.
        if (AllocaSize > ArgAllocaCutoff) {
          AllocaSize = 0;
          break;
        }
      }
    }
    if (AllocaSize)
      Thres += ArgAllocaCost;
  }

  return (unsigned)Thres;
}

// Check if call is just a wrapper around another call.
// In this case we only have call and ret instructions.
static bool isWrapperOnlyCall(CallSite CS) {
  Function *Callee = CS.getCalledFunction();
  if (!Callee || Callee->size() != 1)
    return false;
  const BasicBlock &BB = Callee->getEntryBlock();
  if (const Instruction *I = BB.getFirstNonPHI()) {
    if (!isa<CallInst>(I)) {
      return false;
    }
    if (isa<ReturnInst>(*std::next(I->getIterator()))) {
      DEBUG(dbgs() << "    Wrapper only call detected: "
                   << Callee->getName() << '\n');
      return true;
    }
  }
  return false;
}

InlineCost AMDGPUInliner::getInlineCost(CallSite CS) {
  Function *Callee = CS.getCalledFunction();
  Function *Caller = CS.getCaller();
  TargetTransformInfo &TTI = TTIWP->getTTI(*Callee);

  if (!Callee || Callee->isDeclaration() || CS.isNoInline() ||
      !TTI.areInlineCompatible(Caller, Callee))
    return llvm::InlineCost::getNever();

  if (CS.hasFnAttr(Attribute::AlwaysInline)) {
    if (isInlineViable(*Callee))
      return llvm::InlineCost::getAlways();
    return llvm::InlineCost::getNever();
  }

  if (isWrapperOnlyCall(CS))
    return llvm::InlineCost::getAlways();

  // Single BB does not increase total BB amount, thus subtract 1
  size_t Size = Caller->size() + Callee->size() - 1;
  if (MaxBB && Size > MaxBB)
    return InlineCost::getNever();

  InlineParams LocalParams = Params;
  LocalParams.DefaultThreshold = (int)getInlineThreshold(CS);
  OptimizationRemarkEmitter ORE(Caller);
  std::function<AssumptionCache &(Function &)> GetAssumptionCache =
      [this](Function &F) -> AssumptionCache & {
    return ACT->getAssumptionCache(F);
  };

  return llvm::getInlineCost(CS, Callee, LocalParams, TTI, GetAssumptionCache,
                             None, PSI, &ORE);
}
