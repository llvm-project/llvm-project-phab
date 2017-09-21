//===-- MemorySSAAliasSets.h - Alias sets using MemorySSA -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This a a helper class that creates alias sets using MemorySSA, each
// set being marked with properties that enablei/disable promotion.
// It is used for promotion in the Loop Invariant Code Motion Pass.
// It also stores insertion points into MemorySSA for a particular loop.
// These need to be reset if the sets are reused.
//===----------------------------------------------------------------------===//
#ifndef LLVM_ANALYSIS_MEMORYSSA_ALIASSETS_H
#define LLVM_ANALYSIS_MEMORYSSA_ALIASSETS_H

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

using namespace llvm;

namespace llvm {
class MemorySSAAliasSets {
public:
  MemorySSA *MSSA;
  MemorySSAUpdater *MSSAUpdater;

private:
  Loop *L;
  LoopInfo *LI;
  AliasAnalysis *AA;
  DominatorTree *DT;
  SmallVector<MemoryAccess *, 4> MSSAInsertPts;

  class SingleMustAliasVec {
    MemoryLocation Loc;
    SmallSetVector<Value *, 8> Vec;

  public:
    // CanNeverPromote trumps everything else
    // Set is either may-alias, has a volatile store or aliases an instruction
    // witout a MemoryLocation.
    bool CanNeverPromote = false;
    bool LoopInvariantAccess = false;
    bool HasAGoodStore = false;
    SingleMustAliasVec(MemoryLocation loc) : Loc(loc), CanNeverPromote(false) {
      Vec.insert(const_cast<Value *>(Loc.Ptr));
    }
    void addMemoryLocation(Optional<MemoryLocation> MemLoc) {
      if (MemLoc == None)
        return;
      Vec.insert(const_cast<Value *>(MemLoc.getValue().Ptr));
    }
    MemoryLocation getMemoryLocation() { return Loc; }
    SmallSetVector<Value *, 8> *getVecRef() { return &Vec; }
    bool isGoodForPromotion() {
      return !CanNeverPromote && LoopInvariantAccess && HasAGoodStore;
    }
    void setMemoryLocation(Value *LI, Value *Replacement) {
      Loc = MemoryLocation(Replacement, Loc.Size, Loc.AATags);
      Vec.remove(LI);
      Vec.insert(Replacement);
    }
    void dump() {
      dbgs() << "Set cannot be promoted: " << CanNeverPromote << "\n";
      dbgs() << "Has only loop invariant accesses: " << LoopInvariantAccess
             << "\n";
      dbgs() << "Contains a promotable store: " << HasAGoodStore << "\n";
      dbgs() << "Contents: \n";
      for (Value *V : Vec)
        dbgs() << " Value: " << *V << "\n";
    }
  };

  SmallVector<SingleMustAliasVec *, 4> AllSets;
  DenseMap<const MemoryAccess *, SingleMustAliasVec *> StoredDefs;
  unsigned MemorySSATraversalPenalty = 0;
  unsigned CAP_NO_ALIAS_SETS = 30;
  unsigned CAP_MSSA_TRAVERSALS = 30;

  void dump() {
    for (SingleMustAliasVec *Set : AllSets)
      Set->dump();
  }

  bool isLoopInvariant(const Value *V) {
    if(!L)
      return true;
    return L->isLoopInvariant(V);
  }
  bool loopContains(Instruction *I) {
    if(!L)
      return true;
    return L->contains(I);
  }
  bool loopContains(BasicBlock *B) {
    if(!L)
      return true;
    return L->contains(B);
  }

  AliasResult alias(Optional<MemoryLocation> MemLocI, Instruction *InstrI,
                    MemoryLocation &MemLocJ) {
    if (MemLocI == None) {
      ModRefInfo ModRef = AA->getModRefInfo(InstrI, MemLocJ);
      if (bool(ModRef & MRI_Mod)) {
        return MustAlias;
      }
      if (bool(ModRef & MRI_Ref)) {
        return MayAlias;
      }
      return NoAlias;
    } else {
      AliasResult Result = AA->alias(MemLocI.getValue(), MemLocJ);
      return Result;
    }
  }

  void addToAliasSets(const MemoryUse *Use) {
    // Follow-up to the BIG NOTE:
    // Because we allowed some imprecision, it is possible that the
    // definingAccess is not the clobberingAccess. We *really* want the
    // correct aliasing definition here, otherwise we're creating a set with
    // things that don't alias.
    // This is currently visible in "LoopSimplify/ashr-crash.ll"(Puts @a,@c,@d
    // in the same set because its uses all point to the newly promoted Def)
    // We *have to* use getClobberingAccess here for correctness!
    // To avoid incurring the performance penalty of complete info:
    // - we don't check against existing sets
    // - we get first defining access, then clobbering access. If the two are
    // different, then we increment a local cost - the cost of optimizing the
    // info for this use past the "memssa-check-limit".
    // - we stop creating sets after CAP_MSSA_TRAVERSALS calls to getClobbering
    // that did work (result != getDefining).
    // - we do *not* have partial info to reuse, so nothing is promoted when
    // cost is exceeded.
    MemoryDef *DefiningAcc = dyn_cast_or_null<MemoryDef>(
        const_cast<MemoryUse *>(Use)->getDefiningAccess());
    MemoryDef *ClobberingAcc = dyn_cast_or_null<MemoryDef>(
        MSSA->getWalker()->getClobberingMemoryAccess(
            const_cast<MemoryUse *>(Use)));

    if (DefiningAcc != ClobberingAcc)
      MemorySSATraversalPenalty++;
    if (!ClobberingAcc || !ClobberingAcc->getMemoryInst())
      return;

    SingleMustAliasVec *OneAliasSet = StoredDefs[ClobberingAcc];
    if (!OneAliasSet)
      return;

    Instruction *InstrI = Use->getMemoryInst();
    Optional<MemoryLocation> MemLocI = MemoryLocation::getOrNone(InstrI);
    MemoryLocation MemLocJ = OneAliasSet->getMemoryLocation();
    // FIXME: This is KNOWN by MSSA, i.e. if may or must. TODO: expose this.
    AliasResult Result = alias(MemLocI, InstrI, MemLocJ);
    if (Result == MayAlias) {
      OneAliasSet->CanNeverPromote = true;
    }
    OneAliasSet->addMemoryLocation(MemLocI);
  }

  void addToAliasSets(const MemoryDef *Def) {
    Instruction *InstrI = Def->getMemoryInst();
    Optional<MemoryLocation> MemLocI = MemoryLocation::getOrNone(InstrI);

    SingleMustAliasVec *FoundMustAlias = nullptr;
    bool FoundMayAlias = false;

    StoreInst *SIn = dyn_cast_or_null<StoreInst>(InstrI);
    bool LoopInvariantAccess =
        ((MemLocI != None) && isLoopInvariant(MemLocI.getValue().Ptr));
    bool CanNeverPromote = (SIn && SIn->isVolatile()) || MemLocI == None;
    bool HasAGoodStore = SIn && !SIn->isVolatile();

    for (auto OneAliasSet : AllSets) {
      MemoryLocation MemLocJ = OneAliasSet->getMemoryLocation();
      AliasResult Result = alias(MemLocI, InstrI, MemLocJ);
      if (Result == MustAlias) {
        OneAliasSet->addMemoryLocation(MemLocI);
        if (FoundMustAlias != nullptr || CanNeverPromote) {
          OneAliasSet->CanNeverPromote = true;
          FoundMayAlias = true;
          if (FoundMustAlias) {
            FoundMustAlias->CanNeverPromote = true;
          }
        }
        OneAliasSet->LoopInvariantAccess |= LoopInvariantAccess;
        OneAliasSet->HasAGoodStore |= HasAGoodStore;
        FoundMustAlias = OneAliasSet;
        StoredDefs[Def] = FoundMustAlias;
      }
      if (Result == MayAlias) {
        FoundMayAlias = true;
        OneAliasSet->CanNeverPromote = true;
        if (FoundMustAlias) {
          FoundMustAlias->CanNeverPromote = true;
        }
      }
    }

    // Still need to add this to a new set
    if (!FoundMustAlias && MemLocI != None) {
      SingleMustAliasVec *newEntry = new SingleMustAliasVec(MemLocI.getValue());
      newEntry->CanNeverPromote = CanNeverPromote || FoundMayAlias;
      newEntry->LoopInvariantAccess = LoopInvariantAccess;
      newEntry->HasAGoodStore = HasAGoodStore;
      StoredDefs[Def] = newEntry;
      AllSets.push_back(newEntry);
    }
  }

public:
  MemorySSAAliasSets(MemorySSA *mssa, MemorySSAUpdater *mssaupdater, Loop *l,
                     LoopInfo *li, AliasAnalysis *aa, DominatorTree *dt,
                     SmallVectorImpl<BasicBlock *> &ExitBlocks)
      : MSSA(mssa), MSSAUpdater(mssaupdater), L(l), LI(li), AA(aa), DT(dt),
        AllSets() {
    resetMSSAInsertPts(ExitBlocks);
  }

  void resetMSSAInsertPts(SmallVectorImpl<BasicBlock *> &ExitBlocks) {
    // TODO: Better way to fill this in.
    MSSAInsertPts.reserve(ExitBlocks.size());
    for (unsigned I = 0; I < ExitBlocks.size(); ++I) {
      MSSAInsertPts.push_back(nullptr);
    }
  }

  // Creates all alias sets in L
  void createSets() { addToSets(DT->getNode(L->getHeader())); }

  // Create all alias sets given a basic block (for testing)
  void createSets(BasicBlock *B) { addToSets(DT->getNode(B)); }

  void addToSets(DomTreeNode *N) {
    // We want to visit parents before children. We will enque all the parents
    // before their children in the worklist and process the worklist in order.
    SmallVector<DomTreeNode *, 16> Worklist = collectChildrenInLoop(N, L);

    for (DomTreeNode *DTN : Worklist) {
      BasicBlock *BB = DTN->getBlock();
      auto *Accesses = MSSA->getBlockAccesses(BB);
      if (Accesses)
        for (auto &Acc : make_range(Accesses->begin(), Accesses->end())) {
          if (AllSets.size() > CAP_NO_ALIAS_SETS ||
              MemorySSATraversalPenalty > CAP_MSSA_TRAVERSALS) {
            AllSets.clear();
            return;
          }
          // Process uses
          const MemoryUse *Use = dyn_cast_or_null<MemoryUse>(&Acc);
          if (Use)
            addToAliasSets(Use);
          // Process defs
          const MemoryDef *Def = dyn_cast_or_null<MemoryDef>(&Acc);
          if (Def)
            addToAliasSets(Def);
        }
    }
    // dbgs()<<"No sets: "<<AllSets.size()<<" and mssa penalty:
    // "<<MemorySSATraversalPenalty<<"\n";  dump();
  }

  SmallSetVector<Value *, 8> *getSetIfPromotableOrNull(unsigned I) {
    if (AllSets[I]->isGoodForPromotion()) {
      SmallSetVector<Value *, 8> *MustAliasPtrs = AllSets[I]->getVecRef();
      assert(MustAliasPtrs->size() > 0 && "Alias set cannot be empty!!!\n");
      return MustAliasPtrs;
    }
    return nullptr;
  }

  unsigned size() { return AllSets.size(); }

  void replace(Value *LoadI, Value *Replacement) {
    for (auto OneAliasSet : AllSets) {
      MemoryLocation MemLocJ = OneAliasSet->getMemoryLocation();
      if (MemLocJ.Ptr == LoadI) {
        OneAliasSet->setMemoryLocation(LoadI, Replacement);
        if (!OneAliasSet->CanNeverPromote && OneAliasSet->HasAGoodStore &&
            !OneAliasSet->LoopInvariantAccess &&
            isLoopInvariant(Replacement)) {
          OneAliasSet->LoopInvariantAccess = true;
        }
      }
    }
  }

  MemoryAccess *getInsertPoint(int I) { return MSSAInsertPts[I]; }
  void setInsertPoint(MemoryAccess *Acc, int I) { MSSAInsertPts[I] = Acc; }
};
} // end namespace llvm

#endif // LLVM_ANALYSIS_MEMORYSSA_ALIASSETS_H
