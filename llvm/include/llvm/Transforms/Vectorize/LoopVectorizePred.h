//===---- LoopVectorizePred.h ---------------------------------------*- C++
//-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the LLVM pass which needs to be run before loop vectorizer. This pass
// checks if there is a backward dependence between instructions ,if so, then it
// tries to reorders instructions, so as to convert non-vectorizable loop into
// vectorizable form and generates target-independent LLVM-IR. The vectorizer
// uses the LoopAccessAnalysis  and MemorySSA  analysis to check for
// dependences, inorder to reorder the instructions.

//===----------------------------------------------------------------

#ifndef LLVM_TRANSFORMS_VECTORIZE_LOOPVECTORIZEPRED_H
#define LLVM_TRANSFORMS_VECTORIZE_LOOPVECTORIZEPRED_H

#include "llvm/ADT/MapVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include <functional>

namespace llvm {

/// The LoopVectorizePred Pass.
struct LoopVectorizePredPass : public PassInfoMixin<LoopVectorizePredPass> {
  bool DisableUnrolling = false;
  /// If true, consider all loops for vectorization.
  /// If false, only loops that explicitly request vectorization are
  /// considered.
  bool AlwaysVectorize = true;

  ScalarEvolution *SE;
  LoopInfo *LI;
  TargetTransformInfo *TTI;
  DominatorTree *DT;

  TargetLibraryInfo *TLI;
  AliasAnalysis *AA;
  AssumptionCache *AC;
  std::function<const LoopAccessInfo &(Loop &)> *GetLAA;
  MemorySSA *MSSA;

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

  // Shim for old PM.

  bool runImpl(Function &F, ScalarEvolution &SE_, LoopInfo &LI_,
               TargetTransformInfo &TTI_, DominatorTree &DT_,
               TargetLibraryInfo *TLI_, AliasAnalysis &AA_,
               AssumptionCache &AC_,
               std::function<const LoopAccessInfo &(Loop &)> &GetLAA_,
               MemorySSA &MSSA_);

  bool processLoop(Loop *L);
};
} // namespace llvm

#endif // LLVM_TRANSFORMS_VECTORIZE_LOOPVECTORIZEPRED_H
