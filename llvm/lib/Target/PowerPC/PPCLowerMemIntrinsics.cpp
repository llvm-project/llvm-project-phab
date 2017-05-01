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
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/LowerMemIntrinsics.h"

#define DEBUG_TYPE "ppc-lower-mem-intrinsics"

// This pass will loop over all MemCpyInstrs and expand some of them into loops.
// For known compile time sizes, calls where the size belongs to
// [MemcpyLoopFloor, MemcpyLoopCeil] will be expanded. For unknown sizes we are
// currently expanding all call sites. The pass is off by default and can be
// enabled with 'ppc-expand-extra-memcpy=true'. A follow on patch will refine
// the expansions based on profiling data.

STATISTIC(MemCpyLoopExpansions, "Number of memcpy calls expanded into a loop.");

using namespace llvm;

static cl::opt<bool> EnableMemcpyExpansionPass(
    "ppc-expand-extra-memcpy",
    cl::desc("Enable the extra memcpy expansion pass"), cl::init(false),
    cl::Hidden);

// Options used to tune the size range where memcpy expansions occur.
static cl::opt<unsigned> MemcpyLoopFloor(
    "ppc-memcpy-loop-floor", cl::Hidden, cl::init(129),
    cl::desc(
        "The lower size bound of memcpy calls to get expanded into a loop"));

static cl::opt<unsigned> MemcpyLoopCeil(
    "ppc-memcpy-loop-ceil", cl::Hidden, cl::init(256),
    cl::desc("The upper size bound of memcpy calls to get expanded in a loop"));

namespace {
class PPCLowerMemIntrinsics : public ModulePass {
public:
  static char ID;

  PPCLowerMemIntrinsics() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetTransformInfoWrapperPass>();
  }

  bool runOnModule(Module &M) override;
  /// Loops over all uses of llvm.memcpy and expands the call if warranted.
  //  \p MemcpyDecl is the function declaration of llvm.memcpy.
  bool expandMemcopies(Function &MemcpyDecl);

  StringRef getPassName() const override {
    return "PPC Lower Memory Intrinsics";
  }
};
} // end anonymous namespace

char PPCLowerMemIntrinsics::ID = 0;

INITIALIZE_PASS(PPCLowerMemIntrinsics, DEBUG_TYPE,
                "Lower Memory intrinsics into loops", false, false)

// Checks whether the cpu arch is one where we want to expand
// memcpy calls. We expand for little-endian PPC cpus.
static bool CPUCheck(const std::string &CpuStr) {
  return StringSwitch<bool>(CpuStr)
      .Case("pwr8", true)
      .Case("pwr9", true)
      .Case("ppc64le", true)
      .Default(false);
}

// Determines if we want to expand a specific memcpy call.
static bool shouldExpandMemCpy(MemCpyInst *MC) {
  // If compiling for -O0, -Oz or -Os we don't want to expand.
  Function *ParentFunc = MC->getParent()->getParent();
  if (ParentFunc->optForSize() ||
      ParentFunc->hasFnAttribute(Attribute::OptimizeNone))
    return false;

  // See if the cpu arch is one we want to expand for. If there is no
  // target-cpu attibute assume we don't want to  expand.
  Attribute CPUAttr = ParentFunc->getFnAttribute("target-cpu");
  if (CPUAttr.hasAttribute(Attribute::None) ||
      !CPUCheck(CPUAttr.getValueAsString())) {
    return false;
  }

  // Expand known sizes within the range [MemcpyLoopFloor, MemcpyLoopCeil].
  ConstantInt *CISize = dyn_cast<ConstantInt>(MC->getLength());
  if (CISize) {
    return CISize->getZExtValue() >= MemcpyLoopFloor &&
           CISize->getZExtValue() <= MemcpyLoopCeil;
  }

  // Otherwise expand unkown sizes ...
  return true;
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
  } else {
    createMemCpyLoopUnknownSize(MI, MI->getRawSource(), MI->getRawDest(),
                                MI->getLength(), MI->getAlignment(),
                                MI->getAlignment(), MI->isVolatile(),
                                MI->isVolatile(), TTI);
  }
}

bool PPCLowerMemIntrinsics::expandMemcopies(Function &F) {
  bool AnyExpanded = false;
  assert(Intrinsic::memcpy == F.getIntrinsicID());
  // loop over all memcpy calls
  for (auto I : F.users()) {
    MemCpyInst *MC = dyn_cast<MemCpyInst>(I);
    assert(MC && "Must be a MemcpyInst!");
    if (shouldExpandMemCpy(MC)) {
      const TargetTransformInfo &TTI =
          getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
      ppcExpandMemCpyAsLoop(MC, TTI);
      MC->eraseFromParent();
      AnyExpanded = true;
      ++MemCpyLoopExpansions;
    }
  }
  return AnyExpanded;
}

bool PPCLowerMemIntrinsics::runOnModule(Module &M) {
  if (!EnableMemcpyExpansionPass)
    return false;

  if (skipModule(M))
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
