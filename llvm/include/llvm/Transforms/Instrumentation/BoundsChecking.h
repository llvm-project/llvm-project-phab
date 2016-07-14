//===- BoundsChecking.h - Instrumentation for run-time bounds checking --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass that instruments the code to perform run-time
// bounds checking on loads, stores, and other memory intrinsics.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_BOUNDSCHECKING_H
#define LLVM_TRANSFORMS_BOUNDSCHECKING_H

#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Instrumentation.h"

namespace llvm {

typedef IRBuilder<TargetFolder> BuilderTy;

class BoundsChecking : public PassInfoMixin<BoundsChecking> {
  const TargetLibraryInfo *TLI;
  ObjectSizeOffsetEvaluator *ObjSizeEval;
  BuilderTy *Builder;
  Instruction *Inst;
  BasicBlock *TrapBB;

public:
  PreservedAnalyses run(Function &F, AnalysisManager<Function> &FAM);

private:
  BasicBlock *getTrapBB();
  void emitBranchToTrap(Value *Cmp = nullptr);
  bool instrument(Value *Ptr, Value *InstVal, const DataLayout &DL);

  friend class BoundsCheckingLegacy;
  bool check(Function &F, const TargetLibraryInfo *TLI);
};

} // End llvm namespace
#endif
