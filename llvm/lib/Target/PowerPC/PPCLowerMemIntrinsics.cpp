//===-------- PPCLowerMemIntrinsics.cpp - Expand memory instinsics  -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// An IR to IR  pass that expands llvm.memcpy intrinsics into the equivalent
/// load-store loops.
///
//===----------------------------------------------------------------------===//

#include "PPC.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/LowerMemIntrinsics.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#define DEBUG_TYPE "ppc-memcpy-loop-lowering"

// This pass will loop over all MemCpyInstrs and expand some of them into loops.
// For known compile time sizes, calls where the size belongs to
// [MemcpyLoopFloor, MemcpyLoopCeil] will be expanded. For unknown sizes we are
// currently expanding all call sites. The pass is off by default and can be
// enabled with 'ppc-enable-memcpy-loops=true'.

STATISTIC(MemCpyCalls, "Total number of memcpy calls found.");
STATISTIC(MemCpyLoopNotExpanded, "Total number of memcpy calls not expanded.");
STATISTIC(MemCpyLoopExpansions,
		          "Total number of memcpy calls expanded into a loop.");
STATISTIC(MemCpyKnownSizeCalls,
		          "Total Number of known size memcpy calls found.");
STATISTIC(MemCpyUnknownSizeCalls,
		          "Total Number of unknown size memcpy calls found.");
STATISTIC(MemCpyVersioned, "Number of unknown size memcpy calls versioned.");
STATISTIC(MemCpyKnownSizeExpanded,
		          "Number of known size memcpy calls expanded into a loop.");
STATISTIC(MemCpyLTMemcpyLoopFloor, "Number of memcpy calls not expanded into a "
		                                   "loop because size lt MemcpyLoopFloor.");
STATISTIC(MemCpyGTMemcpyLoopCeil, "Number of memcpy calls not expanded into a "
		                                  "loop because size gt MemcpyLoopCeil.");
STATISTIC(
		    MemCpyPgoCold,
		        "Number of memcpy calls not expanded into a loop due to pgo cold path.");
STATISTIC(MemCpyMinSize, "Number of memcpy calls not expanded into a loop "
		                         "cause it's compiling for min size or opt-none.");
STATISTIC(MemCpyNoTargetCPU, "Number of memcpy calls not expanded into a loop "
		                             "because target cpu is not as expected.");

using namespace llvm;

static cl::opt<bool> EnableMemcpyExpansionPass(
    "ppc-enable-memcpy-loops",
    cl::desc("Enable the PPC pass that lowers memcpy calls into loops."),
    cl::init(true), cl::Hidden);

// Options used to tune the size range where memcpy expansions occur.
static cl::opt<unsigned> MemcpyLoopFloor(
    "ppc-memcpy-loop-floor", cl::Hidden, cl::init(129),
    cl::desc(
        "The lower size bound of memcpy calls to get expanded into a loop"));

static cl::opt<unsigned> MemcpyLoopCeil(
    "ppc-memcpy-loop-ceil", cl::Hidden, cl::init(512),
    cl::desc("The upper size bound of memcpy calls to get expanded in a loop"));

static cl::opt<unsigned> MemcpyLoopUnknownThreshold(
    "ppc-memcpy-loop-unknown-threshold", cl::Hidden, cl::init(128),
    cl::desc("The upper size bound of memcpy calls to get expanded in a loop for unknown sizes"));

static cl::opt<bool> MemcpyLoopDoKnown(
    "ppc-memcpy-known-loops",
    cl::desc("Enable memcpy loop expansion for known size loops."),
    cl::init(true), cl::Hidden);

static cl::opt<bool> MemcpyLoopDoUnknown(
    "ppc-memcpy-unknown-loops", 
    cl::desc("Enable memcpy loop expansion for unknown size loops."),
    cl::init(false), cl::Hidden);

static cl::opt<bool> MemcpyLoopDoUnknownNonPGO(
    "ppc-memcpy-non-pgo-unknown-loops",
    cl::desc("Enable memcpy loop expansion for unknown size loops even without PGO information."),
    cl::init(false), cl::Hidden);

namespace {
class PPCLowerMemIntrinsics : public ModulePass {
public:
  static char ID;

  PPCLowerMemIntrinsics() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetTransformInfoWrapperPass>();
    AU.addRequired<ProfileSummaryInfoWrapperPass>();
  }

  bool shouldExpandMemCpy(MemCpyInst *MC);
  bool runOnModule(Module &M) override;
  /// Loops over all uses of llvm.memcpy and expands the call if warranted.
  //  \p MemcpyDecl is the function declaration of llvm.memcpy.
  bool expandMemcopies(Function &MemcpyDecl);

  StringRef getPassName() const override {
    return "PPC Lower memcpy into loops";
  }
};
} // end anonymous namespace

// Checks whether the cpu arch is one where we want to expand
// memcpy calls.
static bool CPUCheck(const std::string &CpuStr) {
  return StringSwitch<bool>(CpuStr)
      .Case("pwr8", true)
      .Case("pwr9", true)
      .Case("ppc64le", true) // The default cpu for little-endian.
      .Default(false);
}

// Determines if we want to expand a specific memcpy call.
bool PPCLowerMemIntrinsics::shouldExpandMemCpy(MemCpyInst *MC) {
  // If compiling for -O0, -Oz or -Os we don't want to expand.
  Function *ParentFunc = MC->getParent()->getParent();
  if (ParentFunc->optForSize() ||
      ParentFunc->hasFnAttribute(Attribute::OptimizeNone)) {
    ++MemCpyMinSize;
    return false;
  }

  // See if the cpu arch is one we want to expand for. If there is no
  // target-cpu attibute assume we don't want to  expand.
  Attribute CPUAttr = ParentFunc->getFnAttribute("target-cpu");
  if (CPUAttr.hasAttribute(Attribute::None) ||
      !CPUCheck(CPUAttr.getValueAsString())) {
    ++MemCpyNoTargetCPU;
    return false;
  }

  // Check if it is a memcpy call with known size
  ConstantInt *CISize = dyn_cast<ConstantInt>(MC->getLength());
  if (CISize) {
    if (!MemcpyLoopDoKnown) {
      ++MemCpyMinSize;
      return false;
    }
    ++MemCpyKnownSizeCalls;
  }
  else {
    if (!MemcpyLoopDoUnknown) {
      ++MemCpyMinSize;
      return false;
    }
    ++MemCpyUnknownSizeCalls;
  }

  // Do not expand cold call sites based on profiling information
  ProfileSummaryInfo *PSI =
      getAnalysis<ProfileSummaryInfoWrapperPass>().getPSI();
  bool hasPGOInfo = false;
  if (PSI) {
    DominatorTree DT(*ParentFunc);
    LoopInfo LI(DT);
    BranchProbabilityInfo BPI(*ParentFunc, LI);
    BlockFrequencyInfo BFI(*ParentFunc, BPI, LI);

    Optional<uint64_t> Count = PSI->getProfileCount(MC, &BFI);
    if (Count.hasValue()) {
      hasPGOInfo = true;
      if (PSI->isColdCallSite(CallSite(MC), &BFI)) {
        ++MemCpyPgoCold;
        return false;
      }
    }
  }

  // Expand known sizes within the range [MemcpyLoopFloor, MemcpyLoopCeil].
  if (CISize) {
    if (CISize->getZExtValue() > MemcpyLoopCeil) {
      ++MemCpyGTMemcpyLoopCeil;
      return false;
    } else if (CISize->getZExtValue() < MemcpyLoopFloor) {
      ++MemCpyLTMemcpyLoopFloor;
      return false;
    }
    return true;
  }

  // For unknown size, only version if there is PGO info
  return (hasPGOInfo || MemcpyLoopDoUnknownNonPGO);
}

// returns condition to be used to determine unknown size memCpy expansion
static Value *getExpandUnknownSizeMemCpyCond(MemCpyInst *MI) {

  IRBuilder<> Builder(MI);
  Value *Op1 = MI->getLength();
  Value *Op2 = ConstantInt::get(Op1->getType(), MemcpyLoopUnknownThreshold);
  Value *Cond = Builder.CreateICmpULE(Op1, Op2);
  return Cond;
}

// Wrapper function that determines which expansion to call depending on if the
// size is a compile time constant or not. Will go away when the old memcpy
// expansion implementation does.
static void ppcExpandMemCpyAsLoop(MemCpyInst *MI,
                                  const TargetTransformInfo &TTI) {
  if (ConstantInt *ConstLen = dyn_cast<ConstantInt>(MI->getLength())) {
    createMemCpyLoopKnownSize(MI, MI->getRawSource(), MI->getRawDest(),
                              ConstLen, MI->getAlignment(), MI->getAlignment(),
                              MI->isVolatile(), MI->isVolatile(), TTI);
    ++MemCpyKnownSizeExpanded;
  } else {
    // create if-then-else block and insert before memCpy instruction
    TerminatorInst *ThenTerm, *ElseTerm;
    SplitBlockAndInsertIfThenElse(getExpandUnknownSizeMemCpyCond(MI),
                                  MI, &ThenTerm, &ElseTerm, nullptr);
    // Generate memCpy expansion loop in then-block
    createMemCpyLoopUnknownSize(ThenTerm, MI->getRawSource(), MI->getRawDest(),
                                MI->getLength(), MI->getAlignment(),
                                MI->getAlignment(), MI->isVolatile(),
                                MI->isVolatile(), TTI);

    // create a copy of MI and instert to else-block
    IRBuilder<> Builder(MI);
    Builder.SetInsertPoint(ElseTerm);
    Builder.Insert(MI->clone());
    ++MemCpyVersioned;
  }
}

bool PPCLowerMemIntrinsics::expandMemcopies(Function &F) {
  bool AnyExpanded = false;
  assert(Intrinsic::memcpy == F.getIntrinsicID() &&
         "expandMemcopies called on wrong function declaration.");
  // loop over all memcpy calls
  for (auto I : F.users()) {
    MemCpyInst *MC = dyn_cast<MemCpyInst>(I);
    assert(MC && "Must be a MemcpyInst!");
    ++MemCpyCalls;
    if (shouldExpandMemCpy(MC)) {
      Function *ParentFunc = MC->getParent()->getParent();
      const TargetTransformInfo &TTI =
          getAnalysis<TargetTransformInfoWrapperPass>().getTTI(*ParentFunc);
      ppcExpandMemCpyAsLoop(MC, TTI);
      MC->eraseFromParent();
      AnyExpanded = true;
      ++MemCpyLoopExpansions;
    } else {
      ++MemCpyLoopNotExpanded;
    }
  }
  return AnyExpanded;
}

bool PPCLowerMemIntrinsics::runOnModule(Module &M) {
  if (!EnableMemcpyExpansionPass || skipModule(M))
    return false;

  bool Modified = false;
  for (Function &F : M) {
    // Looking for the declaration of llvm.memcpy so we can skip
    // any definition.
    if (!F.isDeclaration())
      continue;

    switch (F.getIntrinsicID()) {
    default:
      break;
    case Intrinsic::memcpy:
      Modified = expandMemcopies(F);
    }
  }

  return Modified;
}

ModulePass *llvm::createPPCLowerMemIntrinsicsPass() {
  return new PPCLowerMemIntrinsics();
}

char PPCLowerMemIntrinsics::ID = 0;
INITIALIZE_PASS_BEGIN(PPCLowerMemIntrinsics, "PPCLowerMemIntrinsics",
		                                      "Lower mem intrinsics into loops", false, false)
INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
INITIALIZE_PASS_END(PPCLowerMemIntrinsics, "PPCLowerMemIntrinsics",
				                      "Lower mem intrinsics into loops", false, false)
