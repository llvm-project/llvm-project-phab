//===- AggressiveInstCombine.cpp ------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the aggressive expression pattern combiner classes.
// Currently, it handles expression patterns for:
//  * Truncate instruction
//
//===----------------------------------------------------------------------===//

#include "AggressiveInstCombineInternal.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
using namespace llvm;

#define DEBUG_TYPE "aggressive-instcombine"

namespace {
/// Contains expression pattern combiner logic.
/// This class provides both the logic to combine expression patterns and
/// combine them. It differs from InstCombiner class in that it runs only once,
/// which allows pattern optimization to have higher complexity than the O(1)
/// required by the instruction combiner.
class AggressiveInstCombiner : public FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid

  AggressiveInstCombiner() : FunctionPass(ID) {
    initializeAggressiveInstCombinerPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  /// Run all expression pattern optimizations on the given /p F function.
  ///
  /// \param F function to optimize.
  /// \returns true if the IR is changed.
  bool runOnFunction(Function &F) override;

};
} // namespace

void AggressiveInstCombiner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addPreserved<AAResultsWrapperPass>();
  AU.addPreserved<BasicAAWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
}

bool AggressiveInstCombiner::runOnFunction(Function &F) {
  auto &TLI = getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  auto &DL = F.getParent()->getDataLayout();

  bool MadeIRChange = false;

  // Handle TruncInst patterns
  TruncInstCombine TIC(TLI, DL);
  MadeIRChange |= TIC.run(F);

  // TODO: add more patterns to handle...

  return MadeIRChange;
}

char AggressiveInstCombiner::ID = 0;
INITIALIZE_PASS_BEGIN(AggressiveInstCombiner, "aggressive-instcombine",
                      "Combine pattern based expressions", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_END(AggressiveInstCombiner, "aggressive-instcombine",
                    "Combine pattern based expressions", false, false)

FunctionPass *llvm::createAggressiveInstCombinerPass() {
  return new AggressiveInstCombiner();
}
