//===- ScopedNoAliasAA.h - Scoped No-Alias Alias Analysis -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This is the interface for a metadata-based scoped no-alias analysis.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_SCOPEDNOALIASAA_H
#define LLVM_ANALYSIS_SCOPEDNOALIASAA_H

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace llvm {
class DominatorTree;

/// A simple AA result which uses scoped-noalias metadata to answer queries.
class ScopedNoAliasAAResult : public AAResultBase<ScopedNoAliasAAResult> {
  friend AAResultBase<ScopedNoAliasAAResult>;

public:
  ScopedNoAliasAAResult(DominatorTree *DT) : AAResultBase(), DT(DT) {}
  ScopedNoAliasAAResult(ScopedNoAliasAAResult &&Arg)
      : AAResultBase(std::move(Arg)) {}

  /// Handle invalidation events from the new pass manager.
  ///
  /// By definition, this result is stateless and so remains valid.
  bool invalidate(Function &, const PreservedAnalyses &) { return false; }

  AliasResult alias(const MemoryLocation &LocA, const MemoryLocation &LocB);
  ModRefInfo getModRefInfo(ImmutableCallSite CS, const MemoryLocation &Loc);
  ModRefInfo getModRefInfo(ImmutableCallSite CS1, ImmutableCallSite CS2);

  // FIXME: This interface can be removed once the legacy-pass-manager support
  // is removed.
  void setDT(DominatorTree *DT) {
    this->DT = DT;
  }

private:
  bool mayAliasInScopes(const MDNode *Scopes, const MDNode *NoAlias) const;
  void collectMDInDomain(const MDNode *List, const MDNode *Domain,
                         SmallPtrSetImpl<const MDNode *> &Nodes) const;

  bool findCompatibleNoAlias(const Value *P,
                             const MDNode *ANoAlias, const MDNode *BNoAlias,
                             const DataLayout &DL,
                             SmallPtrSetImpl<const Value *> &Visited,
                             SmallVectorImpl<Instruction *> &CompatibleSet,
                             int Depth = 0);
  bool noAliasByIntrinsic(const MDNode *ANoAlias,
                          const Value *APtr,
                          const MDNode *BNoAlias,
                          const Value *BPtr,
                          ImmutableCallSite CSA = ImmutableCallSite(),
                          ImmutableCallSite CSB = ImmutableCallSite());

  DominatorTree *DT;
};

/// Analysis pass providing a never-invalidated alias analysis result.
class ScopedNoAliasAA : public AnalysisInfoMixin<ScopedNoAliasAA> {
  friend AnalysisInfoMixin<ScopedNoAliasAA>;
  static char PassID;

public:
  typedef ScopedNoAliasAAResult Result;

  ScopedNoAliasAAResult run(Function &F, AnalysisManager<Function> &AM);
};

/// Legacy wrapper pass to provide the ScopedNoAliasAAResult object.
class ScopedNoAliasAAWrapperPass : public ImmutablePass {
  std::unique_ptr<ScopedNoAliasAAResult> Result;

public:
  static char ID;

  ScopedNoAliasAAWrapperPass();

  ScopedNoAliasAAResult &getResult() { setDT(); return *Result; }
  const ScopedNoAliasAAResult &getResult() const { return *Result; }

  bool doInitialization(Module &M) override;
  bool doFinalization(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  void setDT();
};

//===--------------------------------------------------------------------===//
//
// createScopedNoAliasAAWrapperPass - This pass implements metadata-based
// scoped noalias analysis.
//
ImmutablePass *createScopedNoAliasAAWrapperPass();
}

#endif
